
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

#ifndef MEXICO_RUNTIME_IMPL_GA_COMMON_HPP_INCLUDED
#define MEXICO_RUNTIME_IMPL_GA_COMMON_HPP_INCLUDED 1

#include "mexico_config.hpp"

#include <string>
#ifdef MEXICO_HAVE_GA_H
#include <ga.h>
#endif
#include "pointers.hpp"
#include "runtime_impl.hpp"


#ifdef MEXICO_HAVE_GA
namespace mexico
{

/// RuntimeImpl_GA_Common: Captures the common functionality of all runtime
///                        implementations based on the Global Arrays library
///
/// WARNING: Global Arrays provides no way to check whether the library
///          was initialized (there is no function like MPI_Is_initialized()
///          as of version 5.0). Therefore we REQUIRE users to have GA
///          initialized if they want to use this runtime implementation.
///
/// Note that RuntimeImpl_GA_Common already derives from RuntimeImpl. Hence,
/// base classes should not directly inherit from this class.
class RuntimeImpl_GA_Common : public RuntimeImpl
{

public:
    RuntimeImpl_GA_Common(Instance* ptr, const std::string& hints);

    /// Destructor
    ~RuntimeImpl_GA_Common();

protected:
    /// The global arrays for the input and output
    int i_ga, o_ga;
    /// Start of the parts of the worker
    int *i_start, *o_start;
    /// Processor group handle
    int p_handle;
    /// Whether or not to use GA_Set_irreg_distr()
    bool use_irreg_distr;

    /// Convert from an MPI_Datatype to a GA type
    int convert_mpi_type_to_ga_type(MPI_Datatype type);

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

    /// Simplified interface for NGA_Put
    inline void put(int ga, int lo, int cnt, void* buf)
    {
        int hi;

        hi = lo + cnt - 1;
        NGA_Put(ga, &lo, &hi, buf, NULL);

        put_min_cnt = std::min(put_min_cnt, cnt);
        put_max_cnt = std::max(put_max_cnt, cnt);
        put_avg_cnt = put_avg_cnt + cnt;

        put_num += 1;    
    }

    /// Simplified interface for NGA_Get
    inline void get(int ga, int lo, int cnt, void* buf)
    {
        int hi;

        hi = lo + cnt - 1;
        NGA_Get(ga, &lo, &hi, buf, NULL);

        get_min_cnt = std::min(get_min_cnt, cnt);
        get_max_cnt = std::max(get_max_cnt, cnt);
        get_avg_cnt = get_avg_cnt + cnt;

        get_num += 1;
    }

    /// Simplified interface for NGA_Access
    inline void access(int ga, int lo, int cnt, void* ptr)
    {
        int hi;

        hi = lo + cnt - 1;
        NGA_Access(ga, &lo, &hi, ptr, NULL);
    }

    /// Simplified interface for NGA_Release
    inline void release(int ga, int lo, int cnt)
    {
        int hi;

        hi = lo + cnt - 1;
        NGA_Release(ga, &lo, &hi);
    }

    /// Simplified interface for NGA_Release_update
    inline void release_update(int ga, int lo, int cnt)
    {
        int hi;

        hi = lo + cnt - 1;
        NGA_Release_update(ga, &lo, &hi);
    }

};

}
#endif

#endif

