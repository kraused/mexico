
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
#include <limits.h>

#include "runtime_impl_ga.hpp"
#include "job.hpp"
#include "runtime.hpp"
#include "memory.hpp"
#include "comm.hpp"
#include "assert.hpp"
#include "utils.hpp"
#include "log.hpp"


#ifdef MEXICO_HAVE_GA
mexico::RuntimeImpl_GA::RuntimeImpl_GA(Instance* ptr, const std::string& hints)
: RuntimeImpl_GA_Common(ptr, hints)
{
    /// Read the hints
    MEXICO_READ_HINT(hints, "coalesce", coalesce);
}

mexico::RuntimeImpl_GA::~RuntimeImpl_GA()
{
}

void mexico::RuntimeImpl_GA::pre_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                       int* i_worker, int* i_offsets,
                                       void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                       int* o_worker, int* o_offsets)
{
    int i, j, w, lo, lo0, i0, nv;
    MPI_Aint i_extent;

    MPI_Type_extent(i_type, &i_extent);

    /// Declared in RuntimeImpl_GA_Common
    put_min_cnt = INT_MAX;
    put_max_cnt = -1;
    put_avg_cnt = 0;
    put_num     = 0;

    /// ----------------------------------------------------------------------
    /// Exchange the data
    MEXICO_WRITE(Log::DEBUG, "Starting exchange of data");

    if(not coalesce)
    {
        for(j = 0; j < i_max_worker_per_val; ++j)
            for(i = 0; i < i_num_vals; ++i)
            {
                if(-1 == (w = i_worker[i + i_num_vals*j]))
                    continue;

                put(i_ga, i_start[w] + i_cnt*i_offsets[i + i_num_vals*j], i_cnt, &((char* )i_buf)[i*i_cnt*i_extent]);
            }
    }
    else
    {
        for(j = 0; j < i_max_worker_per_val; ++j)
        {
            /// Start with an empty bucket
            nv = 0;

            for(i = 0; i < i_num_vals; ++i)
            {
                if(-1 == (w = i_worker[i + i_num_vals*j]))
                    continue;

                lo = i_start[w] + i_cnt*i_offsets[i + i_num_vals*j];

                if(0 == nv)
                {
                    /// Our bucket is empty
                    lo0 = lo;
                    i0  = i;
                    nv  = 1;
                    continue;
                }

                /// We need to copy to/from a contiguous region, hence it
                /// is not sufficient to check if lo == lo0 + i_cnt*nv. We
                /// also need to make sure that i == i0 + nv. We could also
                /// reorder the data after the get but this would require an
                /// additional index management
                if(lo == lo0 + i_cnt*nv and i == i0 + nv)
                {
                    /// Add it to the bucket
                    ++nv;
                    continue;
                }

                /// The item does not match the bucket so we send the
                /// current bucket
                put(i_ga, lo0, nv*i_cnt, &((char* )i_buf)[i0*i_cnt*i_extent]);

                /// Our bucket is empty
                lo0 = lo;
                i0  = i;
                nv  = 1;
            }

            /// Make sure the bucket is empty on start of the next iteration
            if(nv > 0)
                put(i_ga, lo0, nv*i_cnt, &((char* )i_buf)[i0*i_cnt*i_extent]);
        }
    }

    GA_Pgroup_sync(p_handle);
    MEXICO_WRITE(Log::DEBUG, "Finished exchange of data");
    /// ----------------------------------------------------------------------

    /// Compute the average
    put_avg_cnt = put_avg_cnt/put_num;
    
    MEXICO_WRITE(Log::DEBUG, "put cnt stats: num = %d, min = %d, max = %d, avg = %.3f", put_num, put_min_cnt, put_max_cnt, put_avg_cnt);

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

void mexico::RuntimeImpl_GA::post_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                        int* i_worker, int* i_offsets,
                                        void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                        int* o_worker, int* o_offsets)
{
    int i, j, w, lo, lo0, i0, nv;
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

    /// Declared in RuntimeImpl_GA_Common
    get_min_cnt = INT_MAX;
    get_max_cnt = -1;
    get_avg_cnt = 0;
    get_num     = 0;

    /// ----------------------------------------------------------------------
    /// Exchange the data
    GA_Init_fence();

    if(not coalesce)
    {
        for(j = 0; j < o_max_worker_per_val; ++j)
            for(i = 0; i < o_num_vals; ++i)
            {
                if(-1 == (w = o_worker[i + o_num_vals*j]))
                    continue;

                get(o_ga, o_start[w] + o_cnt*o_offsets[i + o_num_vals*j], o_cnt, &((char* )o_buf)[o_cnt*o_extent*(i + o_num_vals*j)]);
            }
    }
    else
    { 
        for(j = 0; j < o_max_worker_per_val; ++j)
        {
            /// Start with an empty bucket
            nv = 0;

            for(i = 0; i < o_num_vals; ++i)
            {
                if(-1 == (w = o_worker[i + o_num_vals*j]))
                    continue;

                lo = o_start[w] + o_cnt*o_offsets[i + o_num_vals*j];

                if(0 == nv)
                {
                    /// Our bucket is empty
                    lo0 = lo;
                    i0  = i;
                    nv  = 1;
                    continue;
                }

                /// We need to copy to/from a contiguous region, hence it
                /// is not sufficient to check if lo == lo0 + o_cnt*nv. We
                /// also need to make sure that i == i0 + nv. We could also
                /// reorder the data after the get but this would require an
                /// additional index management
                if(lo == lo0 + o_cnt*nv and i == i0 + nv)
                {
                    /// Add it to the bucket
                    ++nv;
                    continue;
                }

                /// The item does not match the bucket so we send the
                /// current bucket
                get(o_ga, lo0, nv*o_cnt, &((char* )o_buf)[o_cnt*o_extent*(i0 + o_num_vals*j)]);

                /// Our bucket is empty
                lo0 = lo;
                i0  = i;
                nv  = 1;
            }

            /// Make sure the bucket is empty on start of the next iteration
            if(nv > 0)
                get(o_ga, lo0, nv*o_cnt, &((char* )o_buf)[o_cnt*o_extent*(i0 + o_num_vals*j)]);
        }
    }

    GA_Fence();
    MEXICO_WRITE(Log::DEBUG, "Finished exchange of data");
    /// ----------------------------------------------------------------------
    
    /// Compute the average
    get_avg_cnt = get_avg_cnt/get_num;
    
    MEXICO_WRITE(Log::DEBUG, "get cnt stats: num = %d, min = %d, max = %d, avg = %.3f", get_num, get_min_cnt, get_max_cnt, get_avg_cnt);
}

#endif

