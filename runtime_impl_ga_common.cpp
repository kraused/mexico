
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
#ifdef HAVE_MPI_H
#include <mpi.h>
#endif
#ifdef MEXICO_HAVE_MACDECLS_H
#include <macdecls.h>
#endif
#include <numeric>

#include "runtime_impl_ga_common.hpp"
#include "job.hpp"
#include "runtime.hpp"
#include "memory.hpp"
#include "comm.hpp"
#include "assert.hpp"
#include "utils.hpp"
#include "log.hpp"


#ifdef MEXICO_HAVE_GA
mexico::RuntimeImpl_GA_Common::RuntimeImpl_GA_Common(Instance* ptr, const std::string& hints)
: RuntimeImpl(ptr)
{
    int i_type, o_type, i_ndims, o_ndims, *list, i, *map, nblocks, *worker;

    /// Read the hints
    MEXICO_READ_HINT(hints, "use_irreg_distr", use_irreg_distr);

    if(instance->pe_is_worker)
    {
        MPI_Type_extent(job->i_type, &job_i_extent);
        MPI_Type_extent(job->o_type, &job_o_extent);
    }
    else
    {
        job_i_extent = 0;
        job_o_extent = 0;
    }

    i_buf = 0;
    o_buf = 0;

    /// ----------------------------------------------------------------------
    /// Compute i_ndims and o_ndims 
    
    i_ndims = (instance->pe_is_worker) ? job->i_N : 0;
    o_ndims = (instance->pe_is_worker) ? job->o_N : 0;

    comm->allreduce(MPI_IN_PLACE, &o_ndims, 1, MPI_INT, MPI_SUM);
    comm->allreduce(MPI_IN_PLACE, &i_ndims, 1, MPI_INT, MPI_SUM);

    MEXICO_WRITE(Log::MEDIUM, "[i|o]_ndims = [ %d, %d ]", i_ndims, o_ndims);
    /// ----------------------------------------------------------------------
    
    /// ----------------------------------------------------------------------
    /// Compute i_types and o_types

    i_type = (instance->pe_is_worker) ? convert_mpi_type_to_ga_type(job->i_type) : 0;
    o_type = (instance->pe_is_worker) ? convert_mpi_type_to_ga_type(job->o_type) : 0;

    /* FIXME Assumptions: 
             a) MT_XYZ > 0
             b) Types are consistent among workers. This is not a strong assumption
     */
    comm->allreduce(MPI_IN_PLACE, &i_type, 1, MPI_INT, MPI_MAX);
    comm->allreduce(MPI_IN_PLACE, &o_type, 1, MPI_INT, MPI_MAX);

    MEXICO_WRITE(Log::DEBUG, "[i|o]_type = [ %d, %d ]", i_type, o_type);
    /// ----------------------------------------------------------------------

    /// ----------------------------------------------------------------------
    /// Compute i_start and o_start

    i_start = memory->alloc_int(comm->nprocs);
    o_start = memory->alloc_int(comm->nprocs);

    i_start[comm->myrank] = (instance->pe_is_worker) ? job->i_N : 0;
    o_start[comm->myrank] = (instance->pe_is_worker) ? job->o_N : 0;

    comm->allgather(MPI_IN_PLACE, 1, MPI_INT, i_start, 1, MPI_INT);
    comm->allgather(MPI_IN_PLACE, 1, MPI_INT, o_start, 1, MPI_INT);

    excl_scan_in_place(i_start, i_start+comm->nprocs);
    excl_scan_in_place(o_start, o_start+comm->nprocs);
    /// ----------------------------------------------------------------------
    
    /// ----------------------------------------------------------------------
    /// Create p_handle
    list = memory->alloc_int(comm->nprocs);

    for(i = 0; i < comm->nprocs; ++i)
        list[i] = comm->translate_to_MPI_COMM_WORLD(i);
    p_handle = GA_Pgroup_create(list, comm->nprocs);

    memory->free_int(&list);
    /// ----------------------------------------------------------------------

    i_ga = GA_Create_handle();
    GA_Set_data(i_ga, 1, &i_ndims, i_type);
    GA_Set_pgroup(i_ga, p_handle);
    
    if(use_irreg_distr)
    {
        nblocks = instance->num_worker;
        map     = memory->alloc_int(instance->num_worker);
        for(i = 0; i < instance->num_worker; ++i)
            map[i] = i_start[instance->worker[i]];
        
        GA_Set_irreg_distr(i_ga, map, &nblocks);

        worker  = memory->alloc_int(instance->num_worker);
        for(i = 0; i < instance->num_worker; ++i)
            worker[i] = comm->translate_to_MPI_COMM_WORLD(instance->worker[i]);

        GA_Set_restricted (i_ga, worker, instance->num_worker);
        
        memory->free_int(&map);
        memory->free_int(&worker);
    }
    else
    {
        if(instance->pe_is_worker)
            i_buf = memory->alloc_char(job->i_N*job_i_extent);
    }

    char i_ga_name[] = "i_ga";
    GA_Set_array_name(i_ga, i_ga_name);

    GA_Allocate(i_ga);
    GA_Print_distribution(i_ga);


    o_ga = GA_Create_handle();
    GA_Set_data(o_ga, 1, &o_ndims, o_type);
    GA_Set_pgroup(o_ga, p_handle);

    if(use_irreg_distr)
    {
        nblocks = instance->num_worker;
        map     = memory->alloc_int(instance->num_worker);
        for(i = 0; i < instance->num_worker; ++i)
            map[i] = o_start[instance->worker[i]];

        GA_Set_irreg_distr(o_ga, map, &nblocks);

        worker  = memory->alloc_int(instance->num_worker);
        for(i = 0; i < instance->num_worker; ++i)
            worker[i] = comm->translate_to_MPI_COMM_WORLD(instance->worker[i]);

        GA_Set_restricted (o_ga, worker, instance->num_worker);

        memory->free_int(&map);
        memory->free_int(&worker);
    }
    else
    {
        if(instance->pe_is_worker)
            o_buf = memory->alloc_char(job->o_N*job_o_extent);
    }

    char o_ga_name[] = "o_ga";
    GA_Set_array_name(o_ga, o_ga_name);

    GA_Allocate(o_ga);
    GA_Print_distribution(o_ga);
}

mexico::RuntimeImpl_GA_Common::~RuntimeImpl_GA_Common()
{
    GA_Print_stats();
    
    GA_Destroy(i_ga);
    GA_Destroy(o_ga);

    if(!use_irreg_distr and instance->pe_is_worker)
    {
        memory->free_char((char** )&i_buf);
        memory->free_char((char** )&o_buf);
    }

    memory->free_int(&i_start);
    memory->free_int(&o_start);
}

int mexico::RuntimeImpl_GA_Common::convert_mpi_type_to_ga_type(MPI_Datatype type)
{
    if(MPI_CHAR == type)
        return MT_CHAR;
    if(MPI_INT == type)
        return MT_INT;
    if(MPI_FLOAT == type)
        return MT_REAL;
    if(MPI_DOUBLE == type)
        return MT_DBL;

    MEXICO_FATAL("Cannot translate type");
    return -1;
}

#endif

