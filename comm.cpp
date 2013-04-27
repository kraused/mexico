
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

#include "comm.hpp"
#include "log.hpp"
#include "memory.hpp"
#include "utils.hpp"


mexico::Comm::Comm(Instance* ptr, MPI_Comm i_comm)
: Pointers(ptr)
{
    MPI_Comm_dup (i_comm, &comm);
    MPI_Comm_rank(comm, &myrank);
    MPI_Comm_size(comm, &nprocs);

    MEXICO_WRITE(Log::DEBUG, "nprocs = %d, myrank = %d", nprocs, myrank);

    alltoallv_send_displs = memory->alloc_int(nprocs);
    alltoallv_recv_displs = memory->alloc_int(nprocs);
}

void mexico::Comm::alltoall(void* sendbuf, int sendcnt, MPI_Datatype sendtype, 
                             void* recvbuf, int recvcnt, MPI_Datatype recvtype)
{
    MPI_Alltoall(sendbuf, sendcnt, sendtype,
                 recvbuf, recvcnt, recvtype, comm);
}

void mexico::Comm::alltoallv(void* sendbuf, int* sendcnts, MPI_Datatype sendtype,
                              void* recvbuf, int* recvcnts, MPI_Datatype recvtype)
{
    incl_scan(sendcnts, sendcnts+nprocs, alltoallv_send_displs);
    incl_scan(recvcnts, recvcnts+nprocs, alltoallv_recv_displs);

    MPI_Alltoallv(sendbuf, sendcnts, alltoallv_send_displs, sendtype,
                  recvbuf, recvcnts, alltoallv_recv_displs, recvtype, comm);
}

void mexico::Comm::allreduce(void* sendbuf, void* recvbuf, int cnt, MPI_Datatype type, MPI_Op op)
{
    MPI_Allreduce(sendbuf, recvbuf, cnt, type, op, comm);
}

void mexico::Comm::allgather(void* sendbuf, int sendcnt, MPI_Datatype sendtype,
                              void* recvbuf, int recvcnt, MPI_Datatype recvtype)
{
    MPI_Allgather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
}

void mexico::Comm::win_create(void* buf, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Win* win)
{
    MPI_Win_create(buf, size, disp_unit, info, comm, win);
}

int mexico::Comm::translate_to_MPI_COMM_WORLD(int rank)
{
    MPI_Group grp, grp_world;
    int rank_world;

    MPI_Comm_group(comm, &grp);
    MPI_Comm_group(MPI_COMM_WORLD, &grp_world);

    MPI_Group_translate_ranks(grp, 1, &rank, grp_world, &rank_world);

    return rank_world;
}

MPI_Request mexico::Comm::isend(void* buf, int count, MPI_Datatype datatype, int dest, int tag)
{
    MPI_Request request;
    MPI_Isend(buf, count, datatype, dest, tag, comm, &request);

    return request;
}

MPI_Request mexico::Comm::irecv(void* buf, int count, MPI_Datatype datatype, int source, int tag)
{
    MPI_Request request;
    MPI_Irecv(buf, count, datatype, source, tag, comm, &request);

    return request;
}

void mexico::Comm::probe(int source, int tag, MPI_Status* status)
{
    MPI_Probe(source, tag, comm, status);
}

void mexico::Comm::recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Status* status)
{
    MPI_Recv(buf, count, datatype, source, tag, comm, status);
}

