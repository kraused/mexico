
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

#ifndef MEXICO_JOB_HPP_INCLUDED
#define MEXICO_JOB_HPP_INCLUDED 1

#include "mexico_config.hpp"

#ifdef MEXICO_HAVE_MPI_H
#include <mpi.h>
#endif


namespace mexico
{

/// Job: Base class for all user-supplied jobs
class Job
{

public:
    Job();

    int i_N;                        ///< Number of input values to the
                                    ///  exec function
    MPI_Datatype i_type;            ///< Type of the input data
    int o_N;                        ///< Number of output values of the
                                    ///  exec function
    MPI_Datatype o_type;            ///< Type of the output function

    int no_comm;                    ///< If no_comm is set to 1, the
                                    ///  user guarantees that in the exec()
                                    ///  function no communication is
                                    ///  performed
    bool no_comm_overwriteable;     ///< Allow overwriting the no_comm
                                    ///  hint by the description file.
                                    ///  The default is: yes

    /// Execution function. This function must be
    /// implemented by the user. The function is passed
    /// the input and output buffer as arguments
    virtual void exec(void* i_buf, void* o_buf) = 0;
};

}

#endif

