
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

#ifndef MEXICO_RUNTIME_IMPL_MPI_PT2PT_HPP_INCLUDED
#define MEXICO_RUNTIME_IMPL_MPI_PT2PT_HPP_INCLUDED 1

#include <string>

#include "pointers.hpp"
#include "runtime_impl_mpi_common.hpp"


namespace mexico
{

/// RuntimeImpl_MPI_Pt2Pt: Runtime implementation based on point-to-point communication
class RuntimeImpl_MPI_Pt2Pt : public RuntimeImpl_MPI_Common
{

public:
    RuntimeImpl_MPI_Pt2Pt(Instance* ptr, const std::string& hints);

    /// Destructor
    ~RuntimeImpl_MPI_Pt2Pt();

    /// See Runtime::pre_comm()
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
                  int* o_offsets);

    /// See Runtime::post_comm()
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
                   int* o_offsets);


private:    
    /// Number of messages to receive and send
    int* num_msgs_to_send;
    int* num_msgs_to_recv;

    /// Count number of values to send and receive
    long total_num_msgs_to_send() const;
    long total_num_msgs_to_recv() const;
    
    /// Communication buffer
    char* comm_send_buf;
    char* comm_recv_buf;

    /// Send buffer used in post_comm()
    char** split_send_buf;

    /// Buffer for sending the o_offset values in the
    /// post_comm routine
    int* offsets_send_buf;

    /// Send and receive requests
    MPI_Request* send_req;
    MPI_Request* recv_req;

    /// Displacement vector (temporarily used)
    int* displs;

};

}

#endif

