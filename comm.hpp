
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

#ifndef MEXICO_COMM_HPP_INCLUDED
#define MEXICO_COMM_HPP_INCLUDED 1

#include "mexico_config.hpp"

#ifndef MEXICO_HAVE_MPI
#error "MEXICO doesn't work without MPI!"
#endif

#ifdef MEXICO_HAVE_MPI_H
#include <mpi.h>
#endif

#include "pointers.hpp"


namespace mexico
{

/// Comm: Communication module
class Comm : public Pointers
{

public:
    /// Create a new instance. The last argument
    /// is the communicator of which the internal
    /// communicator is a duplicate
    Comm(Instance* ptr, MPI_Comm i_comm);

    /// Allotall communication
    void alltoall(void* sendbuf, int sendcnt, MPI_Datatype sendtype, 
                  void* recvbuf, int recvcnt, MPI_Datatype recvtype);

    /// Simplified alltoallv call. This function computes
    /// the displacements automatically
    void alltoallv(void* sendbuf, int* sendcnts, MPI_Datatype sendtype,
                   void* recvbuf, int* recvcnts, MPI_Datatype recvtype);

    /// Barrier
    inline void barrier()
    {
        MPI_Barrier(comm);
    }

    /// Allreduce operation
    void allreduce(void* sendbuf, void* recvbuf, int cnt, MPI_Datatype type, MPI_Op op);

    /// Allgather operation
    void allgather(void* sendbuf, int sendcnt, MPI_Datatype sendtype,
                   void* recvbuf, int recvcnt, MPI_Datatype recvtype);

    /// Create an RMA window
    void win_create(void* buf, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Win* win);

    /// Translate a rank to the rank in MPI_COMM_WORLD
    int translate_to_MPI_COMM_WORLD(int rank);

    /// Point-to-point communication: Wrapper around MPI_Isend. The function
    /// returns the request
    MPI_Request isend(void* buf, int count, MPI_Datatype datatype, int dest, int tag);

    /// Point-to-point communication: Wrapper around MPI_Irecv. The function
    /// returns the request
    MPI_Request irecv(void* buf, int count, MPI_Datatype datatype, int source, int tag);

    /// Point-to-point communication: Wrapper around MPI_Probe
    void probe(int source, int tag, MPI_Status* status);

    /// Point-to-point communication: 
    void recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Status* status = MPI_STATUS_IGNORE);

private:
    MPI_Comm comm;      ///< The communicator for
                        ///  the library
public:
    int myrank;         ///< Rankd of the processing element
    int nprocs;         ///< Number of processors

    int* alltoallv_send_displs;
    int* alltoallv_recv_displs;

};

}

#endif

