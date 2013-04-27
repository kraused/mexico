
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

#ifndef MEXICO_RUNTIME_HPP_INCLUDED
#define MEXICO_RUNTIME_HPP_INCLUDED 1

#include "mexico_config.hpp"

#ifdef MEXICO_HAVE_MPI_H
#include <mpi.h>
#endif
#include <string>

#include "pointers.hpp"
#include "runtime_impl.hpp"


namespace mexico
{

/// Runtime: The runtime performs the communication and calls the
///          job exec function.
class Runtime : public Pointers
{

public:
    Runtime(Instance* ptr);

    /// Destructor
    ~Runtime();

    /// Shuffle data to worker processes. i_buf, i_cnt
    /// and i_type are the MPI-type message to send. i_num_vals equals
    /// the number of messages to send. i_max_worker_per_val
    /// defines an upper limit for the number of workers an item in i_buf 
    /// must be send to. i_worker and i_offsets are matrices of size
    /// i_num_vals x i_max_worker_per_val (column-major ordering) storing the
    /// target workers and target offsets (locally on the worker) for each
    /// data item
    void pre_comm(void* i_buf,
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
                  int* o_offsets)    
    {
        impl->pre_comm(i_buf, i_cnt, i_type, i_num_vals, i_max_worker_per_val, i_worker, i_offsets,
                       o_buf, o_cnt, o_type, o_num_vals, o_max_worker_per_val, o_worker, o_offsets);
    }

    /// Retrieve the output data from workers. o_buf, o_cnt and
    /// o_type are the MPI-type message. o_worker, o_offsets (of length
    /// o_num_vals) define the origin worker and the remote offset of the
    /// data to grep.
    void post_comm(void* i_buf,
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
                   int* o_offsets)  
    {
        impl->post_comm(i_buf, i_cnt, i_type, i_num_vals, i_max_worker_per_val, i_worker, i_offsets,
                        o_buf, o_cnt, o_type, o_num_vals, o_max_worker_per_val, o_worker, o_offsets);
    }

    /// Execute the job
    void exec_job()
    {
        impl->exec_job();
    }

    
    /// Name of the implementation
    std::string implementation;

    /// The implementation. All accesses to Runtime will be forwarded
    /// to the implementation
    RuntimeImpl* impl;

};

}

#endif

