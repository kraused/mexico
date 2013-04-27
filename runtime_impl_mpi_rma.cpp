
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

#include "runtime_impl_mpi_rma.hpp"
#include "job.hpp"
#include "runtime.hpp"
#include "memory.hpp"
#include "comm.hpp"
#include "assert.hpp"
#include "utils.hpp"
#include "log.hpp"


mexico::RuntimeImpl_MPI_RMA::RuntimeImpl_MPI_RMA(Instance* ptr, const std::string& hints)
: RuntimeImpl_MPI_Common(ptr, hints)
{
    int i_ndims, o_ndims;

    /// Read the hints
    MEXICO_READ_HINT(hints, "coalesce", coalesce);

    if(instance->pe_is_worker)
    {
        MPI_Type_extent(job->i_type, &job_i_extent);
        MPI_Type_extent(job->o_type, &job_o_extent);

        i_buf = memory->mpi_alloc_mem(job->i_N*job_i_extent);
        o_buf = memory->mpi_alloc_mem(job->o_N*job_o_extent);
    }
    else
    {
        job_i_extent = 0;
        job_o_extent = 0;

        i_buf = 0;
        o_buf = 0;
    }

    /// ----------------------------------------------------------------------
    /// Compute i_ndims and o_ndims 
    
    i_ndims = (instance->pe_is_worker) ? job->i_N*job_i_extent : 0;
    o_ndims = (instance->pe_is_worker) ? job->o_N*job_o_extent : 0;

    MEXICO_WRITE(Log::MEDIUM, "[i|o]_ndims = [ %d, %d ]", i_ndims, o_ndims);
    /// ----------------------------------------------------------------------
    
    /// ----------------------------------------------------------------------
    /// Create the window

    /* TODO Can use the no_lock info here */

    comm->win_create(i_buf, i_ndims, 1, MPI_INFO_NULL, &i_win);
    comm->win_create(o_buf, o_ndims, 1, MPI_INFO_NULL, &o_win);
    /// ----------------------------------------------------------------------
}

mexico::RuntimeImpl_MPI_RMA::~RuntimeImpl_MPI_RMA()
{
    MPI_Win_free(&i_win);
    MPI_Win_free(&o_win);

    if(instance->pe_is_worker)
    {
        memory->mpi_free_mem(&i_buf);
        memory->mpi_free_mem(&o_buf);
    }
}

void mexico::RuntimeImpl_MPI_RMA::pre_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                       int* i_worker, int* i_offsets,
                                       void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                       int* o_worker, int* o_offsets)
{
    int i, j, w, w0, lo, lo0, i0, nv;
    MPI_Aint i_extent;

    /// Declared in RuntimeImpl_MPI_Common
    put_min_cnt = INT_MAX;
    put_max_cnt = -1;
    put_avg_cnt = 0;
    put_num     = 0;

    /// ----------------------------------------------------------------------
    /// Exchange the data
    MPI_Type_extent(i_type, &i_extent);

    MPI_Win_fence(0, i_win);

    if(not coalesce)
    {
        for(j = 0; j < i_max_worker_per_val; ++j)
            for(i = 0; i < i_num_vals; ++i)
            {
                if(-1 == (w = i_worker[i + i_num_vals*j]))
                    continue;

                put(&((char* )i_buf)[i*i_cnt*i_extent], i_cnt, i_type, w, i_cnt*i_offsets[i + i_num_vals*j]*i_extent, i_win);
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

                lo = i_cnt*i_offsets[i + i_num_vals*j];

                if(0 == nv)
                {
                    /// Our bucket is empty
                    lo0 = lo;
                    i0  = i;
                    w0  = w;
                    nv  = 1;
                    continue;
                }

                /// We need to copy to/from a contiguous region, hence it
                /// is not sufficient to check if lo == lo0 + i_cnt*nv. We
                /// also need to make sure that i == i0 + nv. We could also
                /// reorder the data after the get but this would require an
                /// additional index management
                if(w == w0 and lo == lo0 + i_cnt*nv and i == i0 + nv)
                {
                    /// Add it to the bucket
                    ++nv;
                    continue;
                }

                /// The item does not match the bucket so we send the
                /// current bucket
                put(&((char* )i_buf)[i0*i_cnt*i_extent], i_cnt*nv, i_type, w0, lo0*i_extent, i_win);

                /// Our bucket is empty
                lo0 = lo;
                i0  = i;
                w0  = w;
                nv  = 1;
            }

            /// Make sure the bucket is empty on start of the next iteration
            if(nv > 0)
                put(&((char* )i_buf)[i0*i_cnt*i_extent], i_cnt*nv, i_type, w0, lo0*i_extent, i_win);
        }
    }

    MPI_Win_fence(0, i_win);
    /// ----------------------------------------------------------------------

    /// Compute the average
    put_avg_cnt = put_avg_cnt/put_num;

    MEXICO_WRITE(Log::DEBUG, "put cnt stats: num = %d, min = %d, max = %d, avg = %.3f", put_num, put_min_cnt, put_max_cnt, put_avg_cnt);
}

void mexico::RuntimeImpl_MPI_RMA::post_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                        int* i_worker, int* i_offsets,
                                        void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                        int* o_worker, int* o_offsets)
{
    int i, j, w, w0, lo, lo0, i0, nv;
    MPI_Aint o_extent;
    
    MPI_Type_extent(o_type, &o_extent);

    /// Declared in RuntimeImpl_MPI_Common
    get_min_cnt = INT_MAX;
    get_max_cnt = -1;
    get_avg_cnt = 0;
    get_num     = 0;

    /// ----------------------------------------------------------------------
    /// Exchange the data
    MPI_Win_fence(0, o_win);

    if(not coalesce)
    {
        for(j = 0; j < o_max_worker_per_val; ++j)
            for(i = 0; i < o_num_vals; ++i)
            {
                if(-1 == (w = o_worker[i + o_num_vals*j]))
                    continue;

                get(&((char* )o_buf)[o_cnt*o_extent*(i + o_num_vals*j)], o_cnt, o_type, w, o_cnt*o_offsets[i + o_num_vals*j]*o_extent, o_win);
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

                lo = o_cnt*o_offsets[i + o_num_vals*j];

                if(0 == nv)
                {
                    /// Our bucket is empty
                    lo0 = lo;
                    i0  = i;
                    w0  = w;
                    nv  = 1;
                    continue;
                }

                /// We need to copy to/from a contiguous region, hence it
                /// is not sufficient to check if lo == lo0 + o_cnt*nv. We
                /// also need to make sure that i == i0 + nv. We could also
                /// reorder the data after the get but this would require an
                /// additional index management
                if(w == w0 and lo == lo0 + o_cnt*nv and i == i0 + nv)
                {
                    /// Add it to the bucket
                    ++nv;
                    continue;
                }

                /// The item does not match the bucket so we send the
                /// current bucket
                get(&((char* )o_buf)[o_cnt*o_extent*(i0 + o_num_vals*j)], o_cnt*nv, o_type, w0, lo0*o_extent, o_win);

                /// Our bucket is empty
                lo0 = lo;
                i0  = i;
                w0  = w;
                nv  = 1;
            }

            /// Make sure the bucket is empty on start of the next iteration
            if(nv > 0)
                get(&((char* )o_buf)[o_cnt*o_extent*(i0 + o_num_vals*j)], o_cnt*nv, o_type, w0, lo0*o_extent, o_win);
        }
    }

    MPI_Win_fence(0, o_win);
    /// ----------------------------------------------------------------------

    /// Compute the average
    get_avg_cnt = get_avg_cnt/get_num;

    MEXICO_WRITE(Log::DEBUG, "get cnt stats: num = %d, min = %d, max = %d, avg = %.3f", get_num, get_min_cnt, get_max_cnt, get_avg_cnt);
}

