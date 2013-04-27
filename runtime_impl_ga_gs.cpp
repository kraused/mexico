
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

#include "runtime_impl_ga_gs.hpp"
#include "job.hpp"
#include "runtime.hpp"
#include "memory.hpp"
#include "comm.hpp"
#include "assert.hpp"
#include "utils.hpp"
#include "log.hpp"


#ifdef MEXICO_HAVE_GA
mexico::RuntimeImpl_GA_gs::RuntimeImpl_GA_gs(Instance* ptr, const std::string& hints)
: RuntimeImpl_GA_Common(ptr, hints)
{
    spots = 0;
    subsarray = 0;
    vals = 0;
}

mexico::RuntimeImpl_GA_gs::~RuntimeImpl_GA_gs()
{
    memory->free_char((char** )&vals);
    memory->free_int(&spots);
    memory->free_ptr((void*** )&subsarray);
}

void mexico::RuntimeImpl_GA_gs::pre_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                          int* i_worker, int* i_offsets,
                                          void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                          int* o_worker, int* o_offsets)
{
    int i, j, k, w, lo, num_vals_to_send, ii;
    MPI_Aint i_extent;

    /// ----------------------------------------------------------------------
    /// Exchange the data
    MPI_Type_extent(i_type, &i_extent);

    num_vals_to_send = 0;
    for(j = 0; j < i_max_worker_per_val; ++j)
        for(i = 0; i < i_num_vals; ++i)
        {
            if(-1 == (w = i_worker[i + i_num_vals*j]))
                continue;
#ifndef NDEBUG
            if(w < 0 || w >= comm->nprocs)
                MEXICO_FATAL("Invalid worker w = %d", w);
#endif

            num_vals_to_send += i_cnt;
        }

    memory->realloc_char((char** )&vals, num_vals_to_send*i_extent);
    memory->realloc_int(&spots, num_vals_to_send);
    memory->realloc_ptr((void*** )&subsarray, num_vals_to_send);

    ii = 0;
    for(j = 0; j < i_max_worker_per_val; ++j)
        for(i = 0; i < i_num_vals; ++i)
        {
            if(-1 == (w = i_worker[i + i_num_vals*j]))
                continue;
            
            lo = i_start[w] + i_cnt*i_offsets[i + i_num_vals*j];
            
            std::copy(&((char* )i_buf)[i_cnt*i_extent*i],
                      &((char* )i_buf)[i_cnt*i_extent*i]+i_cnt*i_extent,
                      &((char* )vals)[ii*i_extent]);

            for(k = 0; k < i_cnt; ++k, ++ii)
                subsarray[ii] = &(spots[ii] = lo + k);
        }

    NGA_Scatter(i_ga, vals, subsarray, num_vals_to_send); 

    GA_Pgroup_sync(p_handle);
    MEXICO_WRITE(Log::DEBUG, "Finished exchange of data");
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// "Localize" the data if the distribution is not irregular, otherwise we
    /// can just access the local portion.
    /// Caution: Need to use the i_buf and o_buf member variables here
    if(use_irreg_distr)
    {
        if(instance->pe_is_worker)
        {
            access(i_ga, i_start[comm->myrank], job->i_N, &this->i_buf);
            access(o_ga, o_start[comm->myrank], job->o_N, &this->o_buf);
        }
    }
    {
        if(instance->pe_is_worker)
            get(i_ga, i_start[comm->myrank], job->i_N, this->i_buf);

        GA_Pgroup_sync(p_handle);
        MEXICO_WRITE(Log::DEBUG, "Finished copy to local buffer");
    }
    /// ----------------------------------------------------------------------
}

void mexico::RuntimeImpl_GA_gs::post_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                           int* i_worker, int* i_offsets,
                                           void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                           int* o_worker, int* o_offsets)
{
    int i, j, k, w, lo, num_vals_to_recv, ii;
    MPI_Aint o_extent;
    
    MPI_Type_extent(o_type, &o_extent);

    /// ----------------------------------------------------------------------
    /// "Globalize" the output data if the data distribution is not irregular.
    /// Caution: Need to use the i_buf and o_buf member variable here!
    if(use_irreg_distr)
    {
        if(instance->pe_is_worker)
        {
            release       (i_ga, i_start[comm->myrank], job->i_N);
            release_update(o_ga, o_start[comm->myrank], job->o_N);
        }

        /// We still need the sync here to make sure that all worker
        /// have called release/release_update before we access o_ga
        GA_Pgroup_sync(p_handle);
    }
    else
    {
        if(instance->pe_is_worker)
            put(o_ga, o_start[comm->myrank], job->o_N, this->o_buf);

        GA_Pgroup_sync(p_handle);
        MEXICO_WRITE(Log::DEBUG, "Finished copy to global array");
    }
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Exchange the data

    num_vals_to_recv = 0;
    for(j = 0; j < o_max_worker_per_val; ++j)
        for(i = 0; i < o_num_vals; ++i)
        {
            if(-1 == (w = o_worker[i + o_num_vals*j]))
                continue;
#ifndef NDEBUG
            if(w < 0 || w >= comm->nprocs)
                MEXICO_FATAL("Invalid worker w = %d", w);
#endif

            num_vals_to_recv += o_cnt;
        }

    memory->realloc_char((char** )&vals, num_vals_to_recv*o_extent);
    memory->realloc_int(&spots, num_vals_to_recv);
    memory->realloc_ptr((void*** )&subsarray, num_vals_to_recv);

    ii = 0;
    for(j = 0; j < o_max_worker_per_val; ++j)
        for(i = 0; i < o_num_vals; ++i)
        {
            if(-1 == (w = o_worker[i + o_num_vals*j]))
                continue;

            lo = o_start[w] + o_cnt*o_offsets[i + o_num_vals*j];
            for(k = 0; k < o_cnt; ++k, ++ii)
                subsarray[ii] = &(spots[ii] = lo + k);
        }

    NGA_Gather(o_ga, o_buf, subsarray, num_vals_to_recv); 

    GA_Pgroup_sync(p_handle);
    MEXICO_WRITE(Log::DEBUG, "Finished exchange of data");
    /// ----------------------------------------------------------------------
}

#endif

