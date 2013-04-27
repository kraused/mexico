
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

#ifndef MEXICO_INSTANCE_HPP_INCLUDED
#define MEXICO_INSTANCE_HPP_INCLUDED 1

#include "mexico_config.hpp"

#ifdef MEXICO_HAVE_MPI_H
#include <mpi.h>
#endif
#include <stdio.h>


namespace mexico
{

/// Forward declaration
class Log;
class Parser;
class Memory;
class Comm;
class Runtime;
class Job;


/// Instance: User interface of the library
/// Users of the library are supposed to create an instance
/// of Instance for each job they want to run. Instance variables
/// should be long-living since the library is not optimized
/// for fast setup times.
class Instance
{

public:
    /// Create a new instance. The user passes a communicator
    /// to the instance which is internally duplicated. The
    /// second and third argument are the number of worker pes
    /// in the communicator and the ranks. The last argument
    /// is a pointer to a job instance. It is only relevant
    /// on worker pes. The last argument is a stream which is
    /// parsed for parameters, hints, etc. See parser.hpp for 
    /// the description of the file format.
    /// The function is collective on comm.
    Instance(MPI_Comm comm,
             int num_worker,
             int* worker,
             Job* job,
             FILE* file);

    /// Create a new instance. This function performs the same
    /// actions as the previous constructor but the last argument
    /// is the content of the description file. This allows calling
    /// applications to pass the description without need to
    /// create a real file.
    Instance(MPI_Comm comm,
             int num_worker,
             int* worker,
             Job* job,
             const char* file_content);

    /// Destructor
    ~Instance();

    /// Execute the job. This call is collective on the
    /// communicator
    /// As in MPI, a message is formed by a buffer, a size and a datatype.
    /// The user must specify the input and output "message". Additionally,
    /// he must specify how the values are supposed to be scattered onto
    /// the worker. 
    /// i_num_vals equals the number of messages, i.e., i_cnt*i_num_vals values
    /// of type i_type are send
    /// The value i_max_worker_per_val gives an upper bound for the number
    /// of worker a single message must be send to. The arrays i_worker, i_offsets
    /// are matrices of size i_num_vals x i_max_worker_per_val stored in column-major
    /// ordering which store the target worker and the offsets
    /// in the local buffer of this worker. Negative entries in i_worker 
    /// (and the corresponding entry in i_offsets) are ignored. Similarly,
    /// o_worker and o_offsets specify from which worker to get the output
    /// data and which local offset the data has in the remote buffer. The
    /// output values are stored in o_buf.
    /// The types i_type, o_type and the i_cnt, o_cnt must be the same on all 
    /// processor. This simplifies the implementation significantly.
    /// Offsets are given in units of i_cnt*extent(i_type).
    void exec(void* i_buf,
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


    /// Member functions are public for convenient use in the
    /// library but must not be touched by external code!
    Log* log;           ///< The logging module
    Parser* parser;     ///< The parser for the description
                        ///  file
    Memory* memory;     ///< Memory management module
    Comm* comm;         ///< Communication module
    Runtime* runtime;   ///< The runtime
    Job* job;           ///< The job to be executed, this
                        ///  is user input and not touched
                        ///  by the library except for read
                        ///  access to the public variables
                        ///  and for the call to exec()

    int num_worker;     ///< Number of worker
    int* worker;        ///< List of workers
    int pe_is_worker;   ///< True if the processing element is a
                        ///  worker, otherwise false.

};

}

#endif

