
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
#ifdef MEXICO_HAVE_MPP_SHMEM_H
#include <mpp/shmem.h>
#endif
#include <numeric>

#include "runtime_impl_shmem.hpp"
#include "job.hpp"
#include "runtime.hpp"
#include "memory.hpp"
#include "comm.hpp"
#include "assert.hpp"
#include "utils.hpp"
#include "log.hpp"


#ifdef MEXICO_HAVE_SHMEM
mexico::RuntimeImpl_SHMEM::RuntimeImpl_SHMEM(Instance* ptr, const std::string& hints)
: RuntimeImpl(ptr)
{
    long i_size, o_size;

    /// Read the hints
    MEXICO_READ_HINT(hints, "coalesce", coalesce);

    if(instance->pe_is_worker)
    {
        MPI_Type_extent(job->i_type, &job_i_extent);
        MPI_Type_extent(job->o_type, &job_o_extent);

        i_size = job->i_N*job_i_extent;
        o_size = job->o_N*job_o_extent;
    }
    else
    {
        job_i_extent = 0;
        job_o_extent = 0;

        i_size = 0;
        o_size = 0;
    }

    /// We need to make sure that all processing elements call
    /// shmalloc with 
    comm->allreduce(MPI_IN_PLACE, &i_size, 1, MPI_LONG, MPI_MAX);
    comm->allreduce(MPI_IN_PLACE, &o_size, 1, MPI_LONG, MPI_MAX);

    /// Colletive calls. Note that the memory->shmalloc() function
    /// allocates on each processing element the maximum of all the 
    i_buf = memory->shmalloc(i_size);
    o_buf = memory->shmalloc(o_size);
}

mexico::RuntimeImpl_SHMEM::~RuntimeImpl_SHMEM()
{
    memory->shfree(&i_buf);
    memory->shfree(&o_buf);
}


void mexico::RuntimeImpl_SHMEM::pre_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                       int* i_worker, int* i_offsets,
                                       void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                       int* o_worker, int* o_offsets)
{
    int i, j, w, w0, lo, lo0, i0, nv;
    MPI_Aint i_extent;

    MPI_Type_extent(i_type, &i_extent);

    /// ----------------------------------------------------------------------
    /// Exchange the data
    shmem_barrier_all();

    if(not coalesce)
    {
        for(j = 0; j < i_max_worker_per_val; ++j)
            for(i = 0; i < i_num_vals; ++i)
            {
                if(-1 == (w = i_worker[i + i_num_vals*j]))
                    continue;

                shmem_putmem(((char* )this->i_buf)+i_cnt*i_offsets[i + i_num_vals*j]*i_extent, &((char* )i_buf)[i*i_cnt*i_extent], i_cnt*i_extent, w);
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

                if(w == w0 and lo == lo0 + i_cnt*nv and i == i0 + nv)
                {
                    /// Add it to the bucket
                    ++nv;
                    continue;
                }

                /// The item does not match the bucket so we send the
                /// current bucket
                shmem_putmem(((char* )this->i_buf) + lo0*i_extent, &((char* )i_buf)[i0*i_cnt*i_extent], i_cnt*nv*i_extent, w0);

                /// Our bucket is empty
                lo0 = lo;
                i0  = i;
                w0  = w;
                nv  = 1;
            }

            /// Make sure the bucket is empty on start of the next iteration
            shmem_putmem(((char* )this->i_buf) + lo0*i_extent, &((char* )i_buf)[i0*i_cnt*i_extent], i_cnt*nv*i_extent, w0);
        }
    }

    shmem_barrier_all();
    /// ----------------------------------------------------------------------
}

void mexico::RuntimeImpl_SHMEM::post_comm(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                                        int* i_worker, int* i_offsets,
                                        void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                                        int* o_worker, int* o_offsets)
{
    int i, j, w, w0, lo, lo0, i0, nv;
    MPI_Aint o_extent;
    
    MPI_Type_extent(o_type, &o_extent);

    /// ----------------------------------------------------------------------
    /// Exchange the data
    shmem_barrier_all();

    if(not coalesce)
    {
        for(j = 0; j < o_max_worker_per_val; ++j)
            for(i = 0; i < o_num_vals; ++i)
            {
                if(-1 == (w = o_worker[i + o_num_vals*j]))
                    continue;

                shmem_getmem(&((char* )o_buf)[o_cnt*o_extent*(i + o_num_vals*j)], ((char* )this->o_buf) + o_cnt*o_offsets[i + o_num_vals*j]*o_extent, o_cnt*o_extent, w);
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

                if(w == w0 and lo == lo0 + o_cnt*nv and i == i0 + nv)
                {
                    /// Add it to the bucket
                    ++nv;
                    continue;
                }

                /// The item does not match the bucket so we send the
                /// current bucket
                shmem_getmem(&((char* )o_buf)[o_cnt*o_extent*(i0 + o_num_vals*j)], ((char* )this->o_buf) + lo0*o_extent, o_cnt*nv*o_extent, w0);

                /// Our bucket is empty
                lo0 = lo;
                i0  = i;
                w0  = w;
                nv  = 1;
            }

            /// Make sure the bucket is empty on start of the next iteration
            shmem_getmem(&((char* )o_buf)[o_cnt*o_extent*(i0 + o_num_vals*j)], ((char* )this->o_buf) + lo0*o_extent, o_cnt*nv*o_extent, w0);
        }
    }

    shmem_barrier_all();
    /// ----------------------------------------------------------------------
}
#endif

