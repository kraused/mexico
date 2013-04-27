
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

#ifndef MEXICO_RUNTIME_IMPL_MPI_ALLTOALL_HPP_INCLUDED
#define MEXICO_RUNTIME_IMPL_MPI_ALLTOALL_HPP_INCLUDED 1

#include <string>

#include "pointers.hpp"
#include "runtime_impl_mpi_common.hpp"


namespace mexico
{

/// RuntimeImpl_MPI_Alltoall: Runtime implementation based on MPI_Alltoall()'s
class RuntimeImpl_MPI_Alltoall : public RuntimeImpl_MPI_Common
{

public:
    RuntimeImpl_MPI_Alltoall(Instance* ptr, const std::string& hints);

    /// Destructor
    ~RuntimeImpl_MPI_Alltoall();

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

    int* num_vals_to_send;
    int* num_vals_to_recv;

    /// Count number of values to receive and send
    long total_num_msgs_to_recv() const;
    long total_num_msgs_to_send() const;

    /// Communication buffer
    char* comm_send_buf;
    char* comm_recv_buf;

    /// Buffer for the communication of offsets
    int* offsets_send_buf;
    int* offsets_recv_buf;

    /// Displacement vector (temporarily used)
    int* displs;

    /// Send/recv requests. These are only used if exch_with_pt2pt
    /// is true.
    MPI_Request* send_req;
    MPI_Request* recv_req;

    /// Whether or not to pack offsets and data together in
    /// the pre_comm and post_comm routine. This only works if job->i_type 
    /// equals i_type
    bool pack;
    /// Use point-to-point non-blocking communication in the
    /// exchange() routine or collective communication.
    bool exch_with_pt2pt;

    /// alltoallv() or point-to-point communication
    void exchange(void*, int*, MPI_Datatype, void*, int*, MPI_Datatype);

};

}

#endif

