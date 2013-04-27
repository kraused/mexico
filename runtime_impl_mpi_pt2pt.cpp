
/// vi: tabstop=4:expandtab

/* Copyright 2010 University of Lugano. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of the University of Lugano.
 */

#include "mexico_config.hpp"

#include <stdlib.h>
#ifdef MEXICO_HAVE_MPI_H
#include <mpi.h>
#endif
#include <numeric>

#include "runtime_impl_mpi_pt2pt.hpp"
#include "job.hpp"
#include "runtime.hpp"
#include "memory.hpp"
#include "comm.hpp"
#include "assert.hpp"
#include "utils.hpp"
#include "log.hpp"


mexico::RuntimeImpl_MPI_Pt2Pt::RuntimeImpl_MPI_Pt2Pt(Instance* ptr, const std::string& hints)
: RuntimeImpl_MPI_Common(ptr, hints)
{
    int w;

    if(instance->pe_is_worker)
    {
        MPI_Type_extent(job->i_type, &job_i_extent);
        MPI_Type_extent(job->o_type, &job_o_extent);

        i_buf = memory->alloc_char(job->i_N*job_i_extent);
        o_buf = memory->alloc_char(job->o_N*job_o_extent);
    }
    else
    {
        job_i_extent = 0;
        job_o_extent = 0;

        i_buf = 0;
        o_buf = 0;
    }

    num_msgs_to_send = memory->alloc_int(comm->nprocs);
    num_msgs_to_recv = memory->alloc_int(comm->nprocs);

    displs = memory->alloc_int(comm->nprocs);

    /// Allocated on demand
    comm_recv_buf = 0;
    comm_send_buf = 0;
    offsets_send_buf = 0;
    
    /// Splitted send buffer used in post_comm. Here we need to
    /// be able to reallocate the individual buffers and hence
    /// cannot use a large 1d array
    split_send_buf = (char** )memory->alloc_ptr(comm->nprocs);
    for(w = 0; w < comm->nprocs; ++w)
        split_send_buf[w] = 0;

    send_req = (MPI_Request* )memory->alloc_char(comm->nprocs*sizeof(MPI_Request));
    recv_req = (MPI_Request* )memory->alloc_char(comm->nprocs*sizeof(MPI_Request));
}

mexico::RuntimeImpl_MPI_Pt2Pt::~RuntimeImpl_MPI_Pt2Pt()
{
    int w;

    memory->free_char((char** )&send_req);
    memory->free_char((char** )&recv_req);

    for(w = 0; w < comm->nprocs; ++w)
        memory->free_char(&split_send_buf[w]);
    
    memory->free_int (&offsets_send_buf);
    memory->free_char(&comm_send_buf);
    memory->free_char(&comm_recv_buf);

    memory->free_int(&displs);
    memory->free_int(&num_msgs_to_send);
    memory->free_int(&num_msgs_to_recv);

    if(instance->pe_is_worker)
    {
        memory->free_char((char** )&i_buf);
        memory->free_char((char** )&o_buf);
    }
}

long mexico::RuntimeImpl_MPI_Pt2Pt::total_num_msgs_to_recv() const
{       
    long N;
        
    N = std::accumulate(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, 0);
    MEXICO_ASSERT(N >= 0);
            
    return N;
}               
                    
long mexico::RuntimeImpl_MPI_Pt2Pt::total_num_msgs_to_send() const
{               
    long N; 

    N = std::accumulate(num_msgs_to_send, num_msgs_to_send+comm->nprocs, 0);
    MEXICO_ASSERT(N >= 0);
        
    return N;
}       

void mexico::RuntimeImpl_MPI_Pt2Pt::pre_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                             int* i_worker, int* i_offsets,
                                             void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                             int* o_worker, int* o_offsets)
{
    int i, j, w, count;
    MPI_Aint i_extent;
    long N, stride;
    MPI_Datatype packed;
    MPI_Status status;

    MPI_Type_extent(i_type, &i_extent);
    packed = create_struct_int_type(i_cnt, i_type);
    stride = sizeof(int) + i_cnt*i_extent;

    /// ----------------------------------------------------------------------
    /// Count the number of messages to be send
    std::fill(num_msgs_to_send, num_msgs_to_send+comm->nprocs, 0);
    
    for(j = 0; j < i_max_worker_per_val; ++j)
        for(i = 0; i < i_num_vals; ++i)
        {
            if(-1 == (w = i_worker[i + i_num_vals*j]))
                continue; 
                
#ifndef NDEBUG
            if(w < 0 || w >= comm->nprocs)
                MEXICO_FATAL("Invalid worker w = %d", w);
#endif          

            num_msgs_to_send[w] += 1;
        }   
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Pack data
    memory->realloc_char(&comm_send_buf, total_num_msgs_to_send()*stride);
        
    incl_scan(num_msgs_to_send, num_msgs_to_send+comm->nprocs, displs);
    
    for(j = 0; j < i_max_worker_per_val; ++j)
        for(i = 0; i < i_num_vals; ++i)
        {
            if(-1 == (w = i_worker[i + i_num_vals*j]))
                continue; 
                    
            *((int* )&comm_send_buf[displs[w]*stride]) = i_offsets[i + i_num_vals*j];
            std::copy(&((char* )i_buf)[i_cnt*i_extent*i],
                      &((char* )i_buf)[i_cnt*i_extent*i]+i_cnt*i_extent,
                      &comm_send_buf[displs[w]*stride + sizeof(int)]);
                          
            displs[w] += 1;
        }
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Send data
    incl_scan(num_msgs_to_send, num_msgs_to_send+comm->nprocs, displs);

    for(w = 0; w < comm->nprocs; ++w)
        if(num_msgs_to_send[w] > 0)
            send_req[w] = comm->isend(comm_send_buf + displs[w]*stride, num_msgs_to_send[w], packed, w, 0);
        else
            send_req[w] = MPI_REQUEST_NULL;
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Reorder data
    if(instance->pe_is_worker)
    {
        N = 0;
        while(N < job->i_N)
        {
            comm->probe(MPI_ANY_SOURCE, 0, &status);

            /// Reallocate the buffer 
            MPI_Get_count(&status, packed, &count);
            MEXICO_ASSERT(count >= 0);
            memory->realloc_char(&comm_recv_buf, count*stride);
           
            comm->recv(comm_recv_buf, count, packed, status.MPI_SOURCE, status.MPI_TAG);

            /// Scatter the data
            for(i = 0; i < count; ++i)
            {
                j = *((int* )&comm_recv_buf[i*stride]);
                
                /// Caution: Need to use the i_buf member variable here!
                std::copy(&comm_recv_buf[i*stride + sizeof(int)],
                          &comm_recv_buf[i*stride + sizeof(int)]+i_cnt*job_i_extent,
                          &((char* )this->i_buf)[j*i_cnt*job_i_extent]);
            }
            
            N += count*i_cnt;
        }
    }

    MPI_Waitall(comm->nprocs, send_req, MPI_STATUSES_IGNORE);
    /// ----------------------------------------------------------------------

    MPI_Type_free(&packed);
}

void mexico::RuntimeImpl_MPI_Pt2Pt::post_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                              int* i_worker, int* i_offsets,
                                              void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                              int* o_worker, int* o_offsets)
{
    int i, j, w, count;
    long N;
    MPI_Status status;
    MPI_Aint o_extent;

    MPI_Type_extent(o_type, &o_extent);

    /// ----------------------------------------------------------------------
    /// Count the number of messages to be send and received
    std::fill(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, 0);
    
    for(j = 0; j < o_max_worker_per_val; ++j)
        for(i = 0; i < o_num_vals; ++i)
        {
            if(-1 == (w = o_worker[i + o_num_vals*j]))
                continue; 
                
#ifndef NDEBUG
            if(w < 0 || w >= comm->nprocs)
                MEXICO_FATAL("Invalid worker w = %d", w);
#endif          

            num_msgs_to_recv[w] += 1;
        }   
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Pack and send offsets
    memory->realloc_int(&offsets_send_buf, total_num_msgs_to_recv());

    incl_scan(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, displs);

    for(j = 0; j < o_max_worker_per_val; ++j)
        for(i = 0; i < o_num_vals; ++i)
        {
            if(-1 == (w = o_worker[i + o_num_vals*j]))
                continue;

            offsets_send_buf[displs[w]++] = o_offsets[i + o_num_vals*j];
        }
    
    incl_scan(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, displs);

    for(w = 0; w < comm->nprocs; ++w)
        if(num_msgs_to_send[w] > 0)
            send_req[w] = comm->isend(offsets_send_buf + displs[w], num_msgs_to_recv[w], MPI_INT, w, 1);
        else
            send_req[w] = MPI_REQUEST_NULL;
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Receive offsets
    std::fill(num_msgs_to_send, num_msgs_to_send+comm->nprocs, 0);

    if(instance->pe_is_worker)
    {
        N = 0;
        while(N < job->o_N)
        {
            comm->probe(MPI_ANY_SOURCE, 1, &status);

            /// Reallocate the buffer 
            MPI_Get_count(&status, MPI_INT, &count);
            MEXICO_ASSERT(count >= 0);
            memory->realloc_int((int** )&comm_recv_buf, count);

            comm->recv(comm_recv_buf, count, MPI_INT, status.MPI_SOURCE, status.MPI_TAG);

            /// Gather the data for the processing element w
            w = status.MPI_SOURCE;

            num_msgs_to_send[w] = count;
            memory->realloc_char(&split_send_buf[w], num_msgs_to_send[w]*o_cnt*job_o_extent);

            for(i = 0; i < count; ++i)
            {
                j = ((int* )comm_recv_buf)[i];

                /// Caution: Need to use the o_buf member variable here!
                std::copy(&((char* )this->o_buf)[j*o_cnt*job_o_extent],
                          &((char* )this->o_buf)[j*o_cnt*job_o_extent]+o_cnt*job_o_extent,
                          &((char* )split_send_buf[w])[i*o_cnt*job_o_extent]); 
            }

            N += count*o_cnt;
        }
    }

    MPI_Waitall(comm->nprocs, send_req, MPI_STATUSES_IGNORE);
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Send the data back
    memory->realloc_char(&comm_recv_buf, total_num_msgs_to_recv()*o_cnt*o_extent);

    incl_scan(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, displs);
    for(w = 0; w < comm->nprocs; ++w)
        if(num_msgs_to_recv[w] > 0)
            recv_req[w] = comm->irecv((char* )comm_recv_buf + displs[w]*o_cnt*o_extent, num_msgs_to_recv[w]*o_cnt, o_type, w, 2);
        else
            recv_req[w] = MPI_REQUEST_NULL;

    for(w = 0; w < comm->nprocs; ++w)
        if(num_msgs_to_send[w] > 0)
            send_req[w] = comm->isend(split_send_buf[w], num_msgs_to_send[w]*o_cnt, job->o_type, w, 2);
        else
            send_req[w] = MPI_REQUEST_NULL;

    MPI_Waitall(comm->nprocs, recv_req, MPI_STATUSES_IGNORE);
    MPI_Waitall(comm->nprocs, send_req, MPI_STATUSES_IGNORE);
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Reorder the data
    incl_scan(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, displs);

    for(j = 0; j < o_max_worker_per_val; ++j)
        for(i = 0; i < o_num_vals; ++i)
        {
            if(-1 == (w = o_worker[i + o_num_vals*j]))
                continue;

            std::copy(&((char* )comm_recv_buf)[displs[w]*o_cnt*o_extent],
                      &((char* )comm_recv_buf)[displs[w]*o_cnt*o_extent]+o_cnt*o_extent,
                      &((char* )o_buf)[o_cnt*o_extent*(i + o_num_vals*j)]);

            displs[w] += 1;
        }
    /// ----------------------------------------------------------------------
}

