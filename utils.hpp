
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

#ifndef MEXICO_UTILS_HPP_INCLUDED
#define MEXICO_UTILS_HPP_INCLUDED 1

namespace mexico
{

/// Scale a vector
template<typename Tp>
void scal(Tp* start, Tp* end, Tp alpha, Tp* out)
{
    while(start != end)
        *out++ = alpha*(*start++);
}

/// Exclusive scan operation
template<typename Tp>
void excl_scan(Tp* start, Tp* end, Tp* out) 
{
    Tp val;

    *out++ = val = *start++;
    while(start != end)
        *out++ = val = val + *start++;
}

/// Exclusive scan operation performed in place
template<typename Tp>
void excl_scan_in_place(Tp* start, Tp* end)
{
    Tp val0, val1, val2;

    val0 = 0;
    val1 = *start;

    *start++ = 0;
    while(start != end)
    {
        val2 = val0 + val1;
        val0 = val2;
        val1 = *start;

        *start++ = val2;
    }
}

/// Inclusive scan operation
template<typename Tp>
void incl_scan(Tp* start, Tp* end, Tp* out)
{
    Tp val;

    *out++ = val = 0;
    while(start+1 != end)
        *out++ = val = val + *start++;
}

}

#endif

