
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

#ifndef MEXICO_RUNTIME_IMPL_MPI_COMMON_HPP_INCLUDED
#define MEXICO_RUNTIME_IMPL_MPI_COMMON_HPP_INCLUDED 1

#include <string>

#include "pointers.hpp"
#include "runtime_impl.hpp"


namespace mexico
{

/// RuntimeImpl_MPI_Common: Base class for all MPI based runtime 
///                         implementations
class RuntimeImpl_MPI_Common : public RuntimeImpl
{

public:
    RuntimeImpl_MPI_Common(Instance* ptr, const std::string& hints);

    /// Destructor
    ~RuntimeImpl_MPI_Common();

protected:
    /// create_struct_int_type creates an MPI datatype for a
    /// struct which looks like
    /// struct __attribute__((packed)) { int i, <type> t[cnt] }
    /// where <type> is the intrinsic datatype corresponding to
    /// type. The packed attribute means that no padding should be
    /// added
    /// The returned type is already committed
    MPI_Datatype create_struct_int_type(int cnt, MPI_Datatype type);

    /// Number of calls to MPI_Put and the minimal, maximal and
    /// average count
    int   put_min_cnt, 
          put_max_cnt, 
          put_num;
    float put_avg_cnt;
    /// Number of calls to MPI_Get and the minimal, maximal and
    /// average count
    int   get_min_cnt,
          get_max_cnt,
          get_num;
    float get_avg_cnt;

    /// Simplified interface to MPI_Put
    inline void put(void* addr, int cnt, MPI_Datatype type, int rank, MPI_Aint disp, MPI_Win win)
    {
        MPI_Put(addr, cnt, type, rank, disp, cnt, type, win);

        put_min_cnt = std::min(put_min_cnt, cnt);
        put_max_cnt = std::max(put_max_cnt, cnt);
        put_avg_cnt = put_avg_cnt + cnt;

        put_num += 1;
    }

    /// Simplified interface to MPI_Get
    inline void get(void* addr, int cnt, MPI_Datatype type, int rank, MPI_Aint disp, MPI_Win win)
    {
        MPI_Get(addr, cnt, type, rank, disp, cnt, type, win);

        get_min_cnt = std::min(get_min_cnt, cnt);
        get_max_cnt = std::max(get_max_cnt, cnt);
        get_avg_cnt = get_avg_cnt + cnt;

        get_num += 1;    
    }

};

}

#endif

