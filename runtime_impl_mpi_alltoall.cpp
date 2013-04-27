
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

#include "runtime_impl_mpi_alltoall.hpp"
#include "job.hpp"
#include "runtime.hpp"
#include "memory.hpp"
#include "comm.hpp"
#include "assert.hpp"
#include "utils.hpp"
#include "log.hpp"


mexico::RuntimeImpl_MPI_Alltoall::RuntimeImpl_MPI_Alltoall(Instance* ptr, const std::string& hints)
: RuntimeImpl_MPI_Common(ptr, hints)
{
    /// Read the hints
    MEXICO_READ_HINT(hints, "pack", pack);
    MEXICO_READ_HINT(hints, "exch_with_pt2pt", exch_with_pt2pt);

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

    num_vals_to_send = memory->alloc_int(comm->nprocs);
    num_vals_to_recv = memory->alloc_int(comm->nprocs);

    displs = memory->alloc_int(comm->nprocs);

    /// These arrays are reallocates as needed
    offsets_send_buf = 0;
    offsets_recv_buf = 0;

    comm_send_buf = 0;
    comm_recv_buf = 0;

    if(exch_with_pt2pt)
    {
        send_req = (MPI_Request* )memory->alloc_char(comm->nprocs*sizeof(MPI_Request));
        recv_req = (MPI_Request* )memory->alloc_char(comm->nprocs*sizeof(MPI_Request));
    }
}

mexico::RuntimeImpl_MPI_Alltoall::~RuntimeImpl_MPI_Alltoall()
{
    if(exch_with_pt2pt)
    {  
        memory->free_char((char** )&send_req);
        memory->free_char((char** )&recv_req);
    }        
    
    memory->free_int(&offsets_send_buf);
    memory->free_int(&offsets_recv_buf);

    memory->free_char(&comm_send_buf);
    memory->free_char(&comm_recv_buf);
            
    memory->free_int(&displs);

    memory->free_int(&num_msgs_to_send);
    memory->free_int(&num_msgs_to_recv);
            
    memory->free_int(&num_vals_to_send);
    memory->free_int(&num_vals_to_recv);

    if(instance->pe_is_worker)
    {
        memory->free_char((char** )&i_buf);
        memory->free_char((char** )&o_buf);
    }                     
}

long mexico::RuntimeImpl_MPI_Alltoall::total_num_msgs_to_recv() const
{
    long N;
   
    N = std::accumulate(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, 0);
    MEXICO_ASSERT(N >= 0);
    
    return N;
}

long mexico::RuntimeImpl_MPI_Alltoall::total_num_msgs_to_send() const
{
    long N;
    
    N = std::accumulate(num_msgs_to_send, num_msgs_to_send+comm->nprocs, 0);
    MEXICO_ASSERT(N >= 0);

    return N;
}

void mexico::RuntimeImpl_MPI_Alltoall::pre_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                                 int* i_worker, int* i_offsets,
                                                 void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                                 int* o_worker, int* o_offsets)
{
    int i, j, w;
    MPI_Aint i_extent;
    long N, stride;
    MPI_Datatype packed;

    /// ----------------------------------------------------------------------
    /// Count the number of messages to be send and received
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
    
    comm->alltoall(num_msgs_to_send, 1, MPI_INT, num_msgs_to_recv, 1, MPI_INT);

    MEXICO_WRITE(Log::DEBUG, "num_msgs_to_[send,recv] = [ %d, %d ]",
                    std::accumulate(num_msgs_to_send, num_msgs_to_send+comm->nprocs, 0),
                    std::accumulate(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, 0));

    if(!job and 0 != std::accumulate(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, 0))
        MEXICO_FATAL("Should not happen: Non-worker receives messages!");

    scal(num_msgs_to_send, num_msgs_to_send+comm->nprocs, i_cnt, num_vals_to_send);
    scal(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, i_cnt, num_vals_to_recv);
    /// ----------------------------------------------------------------------

    if(pack)
    {
        MPI_Type_extent(i_type, &i_extent);
        packed = create_struct_int_type(i_cnt, i_type);
        stride = sizeof(int) + i_cnt*i_extent;

        /// ----------------------------------------------------------------------
        /// Pack offsets and data
        memory->realloc_char(&comm_send_buf, total_num_msgs_to_send()*stride);
        memory->realloc_char(&comm_recv_buf, total_num_msgs_to_recv()*stride);

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

        exchange(comm_send_buf, num_msgs_to_send, packed,
                 comm_recv_buf, num_msgs_to_recv, packed);
        
        /// ----------------------------------------------------------------------
        /// Reorder the data
        incl_scan(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, displs);

        for(w = 0; w < comm->nprocs; ++w)
            if(num_msgs_to_recv[w] > 0)
                for(i = 0; i < num_msgs_to_recv[w]; ++i)
                {
                    j = *((int* )&comm_recv_buf[(displs[w] + i)*stride]);
                
                    /// Caution: Need to use the i_buf member variable here!
                    std::copy(&comm_recv_buf[(displs[w]+i)*stride + sizeof(int)],
                              &comm_recv_buf[(displs[w]+i)*stride + sizeof(int)]+i_cnt*job_i_extent,
                              &((char* )this->i_buf)[j*i_cnt*job_i_extent]);
                }
        /// ----------------------------------------------------------------------

        MPI_Type_free(&packed);
    }
    else
    {
        /// ----------------------------------------------------------------------
        /// Communicate the offsets
        memory->realloc_int(&offsets_send_buf, total_num_msgs_to_send());
        memory->realloc_int(&offsets_recv_buf, total_num_msgs_to_recv());

        incl_scan(num_msgs_to_send, num_msgs_to_send+comm->nprocs, displs);
    
        for(j = 0; j < i_max_worker_per_val; ++j)
            for(i = 0; i < i_num_vals; ++i)
            {
                if(-1 == (w = i_worker[i + i_num_vals*j]))
                    continue;
        
                offsets_send_buf[displs[w]++] = i_offsets[i + i_num_vals*j];
            }

        exchange(offsets_send_buf, num_msgs_to_send, MPI_INT,
                 offsets_recv_buf, num_msgs_to_recv, MPI_INT);
        /// ----------------------------------------------------------------------

        /// ----------------------------------------------------------------------
        /// Communicate the values

        MPI_Type_extent(i_type, &i_extent);
        /// reallocate the internal buffers. Note that the we use i_extent and job_i_extent (which
        /// equals the extent of job->i_type).
        memory->realloc_char((char** )&comm_send_buf, total_num_msgs_to_send()*i_cnt*    i_extent);
        memory->realloc_char((char** )&comm_recv_buf, total_num_msgs_to_recv()*i_cnt*job_i_extent);

        incl_scan(num_msgs_to_send, num_msgs_to_send+comm->nprocs, displs);

        for(j = 0; j < i_max_worker_per_val; ++j)
            for(i = 0; i < i_num_vals; ++i)
            {
                if(-1 == (w = i_worker[i + i_num_vals*j]))
                    continue;

                std::copy(&((char* )i_buf)[i_cnt*i_extent*i], 
                          &((char* )i_buf)[i_cnt*i_extent*i]+i_cnt*i_extent, 
                          &((char* )comm_send_buf)[(displs[w]++)*i_cnt*i_extent]);
            }

        if(instance->pe_is_worker)  /// No job pointer on non-worker processing elements
            exchange(comm_send_buf, num_vals_to_send,      i_type, 
                     comm_recv_buf, num_vals_to_recv, job->i_type);
        else
            exchange(comm_send_buf, num_vals_to_send, i_type,
                     comm_recv_buf, num_vals_to_recv, i_type /* Type doesn't matter */);
        /// ----------------------------------------------------------------------

        /// ----------------------------------------------------------------------
        /// Reorder the data
        N = total_num_msgs_to_recv();
        for(i = 0; i < N; ++i)
        {
             MEXICO_ASSERT(offsets_recv_buf[i] >= 0 and
                           offsets_recv_buf[i]*i_cnt < job->i_N);
                
                /// Caution: Need to use the i_buf member variable here!
                std::copy(&((char* )comm_recv_buf)[i*i_cnt*job_i_extent],
                          &((char* )comm_recv_buf)[i*i_cnt*job_i_extent]+i_cnt*job_i_extent,
                          &((char* )this->i_buf)[offsets_recv_buf[i]*i_cnt*job_i_extent]);
        }
        /// ----------------------------------------------------------------------
    }
}

void mexico::RuntimeImpl_MPI_Alltoall::post_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                                  int* i_worker, int* i_offsets,
                                                  void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                                  int* o_worker, int* o_offsets)
{
    int i, j, k, w;
    MPI_Aint o_extent;

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

    comm->alltoall(num_msgs_to_recv, 1, MPI_INT, num_msgs_to_send, 1, MPI_INT);

    MEXICO_WRITE(Log::DEBUG, "num_msgs_to_[send,recv] = [ %d, %d ]",
                    std::accumulate(num_msgs_to_send, num_msgs_to_send+comm->nprocs, 0),
                    std::accumulate(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, 0));

    scal(num_msgs_to_send, num_msgs_to_send+comm->nprocs, o_cnt, num_vals_to_send);
    scal(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, o_cnt, num_vals_to_recv);
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Communicate the offsets
    memory->realloc_int(&offsets_send_buf, total_num_msgs_to_recv());
    memory->realloc_int(&offsets_recv_buf, total_num_msgs_to_send());

    incl_scan(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, displs);

    for(j = 0; j < o_max_worker_per_val; ++j)
        for(i = 0; i < o_num_vals; ++i)
        {
            if(-1 == (w = o_worker[i + o_num_vals*j]))
                continue;

            offsets_send_buf[displs[w]++] = o_offsets[i + o_num_vals*j];
        }

    /// It's a bit weird: We are sending the offsets for the values that we
    /// want to receive
    exchange(offsets_send_buf, num_msgs_to_recv, MPI_INT,
             offsets_recv_buf, num_msgs_to_send, MPI_INT);
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Communicate the values

    MPI_Type_extent(o_type, &o_extent);
    /// reallocate the internal buffers. Note that the we use o_extent and job_o_extent (which
    /// equals the extent of job->o_type).
    memory->realloc_char((char** )&comm_send_buf, total_num_msgs_to_send()*o_cnt*job_o_extent);
    memory->realloc_char((char** )&comm_recv_buf, total_num_msgs_to_recv()*o_cnt*    o_extent);

    incl_scan(num_msgs_to_send, num_msgs_to_send+comm->nprocs, displs);

    if(instance->pe_is_worker)
    {
        k = 0;  ///< Counter

        for(i = 0; i < comm->nprocs; ++i)
            for(j = displs[i]; j < displs[i] + num_msgs_to_send[i]; ++j, ++k)
            {
                MEXICO_ASSERT(offsets_recv_buf[k] >= 0 and
                               offsets_recv_buf[k]*o_cnt < job->o_N);

                /// Caution: Need to use the o_buf member variable here!
                std::copy(&((char* )this->o_buf)[offsets_recv_buf[k]*o_cnt*job_o_extent],
                          &((char* )this->o_buf)[offsets_recv_buf[k]*o_cnt*job_o_extent]+o_cnt*job_o_extent,
                          &((char* )comm_send_buf)[j*o_cnt*job_o_extent]); 
        }

        MEXICO_ASSERT(k == total_num_msgs_to_send());
    }
    else
        MEXICO_ASSERT(0 == total_num_msgs_to_send());

    if(instance->pe_is_worker)  /// No job pointer on non-worker processing elements
        exchange(comm_send_buf, num_vals_to_send, job->o_type,
                 comm_recv_buf, num_vals_to_recv,      o_type);
    else
        exchange(comm_send_buf, num_vals_to_send, o_type /* Type doesn't matter */,
                 comm_recv_buf, num_vals_to_recv, o_type);
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

void mexico::RuntimeImpl_MPI_Alltoall::exchange(void* send_buf, int* num_msgs_to_send, MPI_Datatype send_type,
                                                void* recv_buf, int* num_msgs_to_recv, MPI_Datatype recv_type)
{   
    MPI_Aint send_extent, recv_extent; 
    int w;

    if(exch_with_pt2pt)
    {
        MPI_Type_extent(recv_type, &recv_extent);

        /// Prepost the receives
        incl_scan(num_msgs_to_recv, num_msgs_to_recv+comm->nprocs, displs);
        for(w = 0; w < comm->nprocs; ++w)
            if(num_msgs_to_recv[w] > 0)
                recv_req[w] = comm->irecv((char* )recv_buf + displs[w]*recv_extent, num_msgs_to_recv[w], recv_type, w, 0);
            else
                recv_req[w] = MPI_REQUEST_NULL;

        MPI_Type_extent(send_type, &send_extent);

        incl_scan(num_msgs_to_send, num_msgs_to_send+comm->nprocs, displs);
        for(w = 0; w < comm->nprocs; ++w)
            if(num_msgs_to_send[w] > 0)
                send_req[w] = comm->isend((char* )send_buf + displs[w]*send_extent, num_msgs_to_send[w], send_type, w, 0);
            else
                send_req[w] = MPI_REQUEST_NULL;

        MPI_Waitall(comm->nprocs, recv_req, MPI_STATUSES_IGNORE);
        MPI_Waitall(comm->nprocs, send_req, MPI_STATUSES_IGNORE);
    }
    else
    {
        comm->alltoallv(send_buf, num_msgs_to_send, send_type,
                        recv_buf, num_msgs_to_recv, recv_type);
    }
}

