
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

#ifndef MEXICO_RUNTIME_IMPL_HPP_INCLUDED
#define MEXICO_RUNTIME_IMPL_HPP_INCLUDED 1

#include "pointers.hpp"


namespace mexico
{

/// RuntimeImpl: Base class for all runtime implementations
class RuntimeImpl : public Pointers
{

public:
    RuntimeImpl(Instance* ptr);

    virtual ~RuntimeImpl();

    /// See Runtime::pre_comm()
    virtual void pre_comm(void* i_buf,
                          int i_cnt,
                          MPI_Datatype i_type,
                          int i_num_vals,
                          int i_max_worker_per_val,
                          int* i_worker,
                          int* i_offsets,
                          void* o_buf,
                          int o_cnt,
                          MPI_Datatype o_type,
                          int o_num_vals,
                          int o_max_worker_per_val,
                          int* o_worker,
                          int* o_offsets) = 0;    
    
    /// See Runtime::post_comm()
    virtual void post_comm(void* i_buf,
                           int i_cnt,
                           MPI_Datatype i_type,
                           int i_num_vals,
                           int i_max_worker_per_val,
                           int* i_worker,
                           int* i_offsets,
                           void* o_buf,
                           int o_cnt,
                           MPI_Datatype o_type,
                           int o_num_vals,
                           int o_max_worker_per_val,
                           int* o_worker,
                           int* o_offsets) = 0;
    
    /// Execute the job using i_buf and o_buf as input/output
    virtual void exec_job();


    /// Input and output buffers
    void* i_buf;
    void* o_buf;

    /// Extents of the input and output types
    MPI_Aint job_i_extent;
    MPI_Aint job_o_extent;

};

}

/// To each runtime implementation, a "hint" string variable is
/// given which can be used to set certain bool values in the class
/// to steer the class. The MEXICO_READ_HINT variable simplifies
/// reading out hints.
#undef  MEXICO_READ_HINT
#define MEXICO_READ_HINT(hints, name, var)              \
    do                                                  \
    {                                                   \
        (var) = ((hints).npos != (hints).find(name));   \
        MEXICO_WRITE(Log::DEBUG, name " = %d", var);    \
    } while(0)

#endif

