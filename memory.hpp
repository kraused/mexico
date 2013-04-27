
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

#ifndef MEXICO_MEMORY_HPP_INCLUDED
#define MEXICO_MEMORY_HPP_INCLUDED 1

#include "mexico_config.hpp"

#include "pointers.hpp"


namespace mexico
{

/// Memory: The memory management module
class Memory : public Pointers
{

public:
    Memory(Instance* ptr);

    /// Allocation routines
    char*   alloc_char(long N);
    int*    alloc_int(long N);
    float*  alloc_float(long N);
    double* alloc_double(long N);
    long*   alloc_long(long N);
    void**  alloc_ptr(long N);

    /// Reallocation
    void realloc_char(char** p, long N);
    void realloc_int(int** p, long N);
    void realloc_float(float** p, long N);
    void realloc_double(double** p, long N);
    void realloc_long(long** p, long N);
    void realloc_ptr(void*** p, long N);

    /// Free routines
    void free_char(char** p);
    void free_int(int** p);
    void free_float(float** p);
    void free_double(double** p);
    void free_long(long** p);
    void free_ptr(void*** p);

#ifdef MEXICO_HAVE_MPI
    /// Allocate memory using MPI_Alloc_mem
    /// and free it
    void* mpi_alloc_mem(long N);
    void  mpi_free_mem(void** p);
#endif

#ifdef MEXICO_HAVE_SHMEM
    /// Allocate and free memory from the symmetric heap. It is
    /// important to note that
    /// a) These routines are collective
    /// b) The same arguments must be passed on all processing elements
    void* shmalloc(long N);
    void  shfree(void** p);
#endif

};

}

#endif

