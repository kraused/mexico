
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

#include "memory.hpp"
#include "log.hpp"


mexico::Memory::Memory(Instance* ptr)
: Pointers(ptr)
{
}

#define DEF_ALLOC(TYPE)                                                             \
    TYPE* mexico::Memory::alloc_ ## TYPE(long N)                                   \
    {                                                                               \
        MEXICO_WRITE(Log::DEBUG, "Allocating %.3f MB", 1e-6*N*sizeof(TYPE));       \
        return (TYPE* )malloc(N*sizeof(TYPE));                                      \
    }

DEF_ALLOC(char)
DEF_ALLOC(int)
DEF_ALLOC(float)
DEF_ALLOC(double)
DEF_ALLOC(long)

void** mexico::Memory::alloc_ptr(long N)
{
    MEXICO_WRITE(Log::DEBUG, "Allocating %.3f MB", 1e-6*N*sizeof(void*));
    return (void** )malloc(N*sizeof(void*));
}


#define DEF_REALLOC(TYPE)                                                           \
    void mexico::Memory::realloc_ ## TYPE(TYPE** p, long N)                        \
    {                                                                               \
        MEXICO_WRITE(Log::DEBUG, "Reallocating to %.3f MB", 1e-6*N*sizeof(TYPE));  \
        *p = (TYPE* )realloc(*p, N*sizeof(TYPE));                                   \
    }

DEF_REALLOC(char)
DEF_REALLOC(int)
DEF_REALLOC(float)
DEF_REALLOC(double)
DEF_REALLOC(long)

void mexico::Memory::realloc_ptr(void*** p, long N)
{
    MEXICO_WRITE(Log::DEBUG, "Reallocating to %.3f MB", 1e-6*N*sizeof(void**));
    *p = (void** )realloc(*p, N*sizeof(void*));
}


#define DEF_FREE(TYPE)                                                              \
    void mexico::Memory::free_ ## TYPE(TYPE** p)                                   \
    {                                                                               \
        free(*p);                                                                   \
        *p = NULL;                                                                  \
    }

DEF_FREE(char)
DEF_FREE(int)
DEF_FREE(float)
DEF_FREE(double)
DEF_FREE(long)

void mexico::Memory::free_ptr(void*** p)
{
    free(*p);
    *p = NULL;
}

#ifdef MEXICO_HAVE_MPI
void* mexico::Memory::mpi_alloc_mem(long N)
{
    void* p;
    MPI_Alloc_mem(N, MPI_INFO_NULL, &p);

    return p;
}

void mexico::Memory::mpi_free_mem(void** p)
{
    MPI_Free_mem(*p);
    *p = NULL;
}
#endif

#ifdef MEXICO_HAVE_SHMEM
void* mexico::Memory::shmalloc(long N)
{
    return ::shmalloc(N);
}

void mexico::Memory::shfree(void** p)
{
    ::shfree(*p);
    *p = NULL;
}
#endif

