
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

#include <algorithm>

#include "instance.hpp"
#include "log.hpp"
#include "runtime.hpp"
#include "job.hpp"
#include "parser.hpp"
#include "comm.hpp"
#include "memory.hpp"


mexico::Instance::Instance(MPI_Comm comm, int num_worker, int* worker, Job* job, FILE* file)
{
    log = new Log(this);        /// Should be the first to create
    MEXICO_WRITE(Log::ALWAYS, "creating new mexico::Instance");

    parser = new Parser(this);
    parser->read(file);

    /// We have a sort of chicken-egg problem here: Log needs to 
    /// read the debug value from the input but in the parser itself
    /// we want to use FATAL() to handle errors. To get around
    /// this problem we don't make use of COMM_WRITE(Log::MEDIUM, ...)
    /// and COMM_WRITE(Log::DEBUG, ...) [i.e., those calls which
    /// depend on log->debug] in the parser and set the log->debug
    /// value now.
    log->debug = parser->find_by_name_int("log", "debug");
    MEXICO_WRITE(Log::MEDIUM, "debug level = %d", log->debug);

    /// Memory management unit
    memory = new Memory(this);

    /// Create the communicator
    this->comm = new Comm(this, comm);

    /// Initialize the worker array
    this->num_worker = num_worker;
    this->worker = memory->alloc_int(num_worker);
    std::copy(worker, worker+num_worker, this->worker);
    pe_is_worker = ( worker+num_worker != std::find(worker, worker+num_worker, this->comm->myrank) );

    /// Set the job instance
    this->job = job;

    /// The runtime is created as the last member
    runtime = new Runtime(this);
}

mexico::Instance::Instance(MPI_Comm comm, int num_worker, int* worker, Job* job, const char* file_content)
{
    log = new Log(this);
    MEXICO_WRITE(Log::ALWAYS, "creating new mexico::Instance");

    parser = new Parser(this);
    parser->read(file_content);

    log->debug = parser->find_by_name_int("log", "debug");
    MEXICO_WRITE(Log::MEDIUM, "debug level = %d", log->debug);

    memory = new Memory(this);
    this->comm = new Comm(this, comm);

    this->num_worker = num_worker;
    this->worker = memory->alloc_int(num_worker);
    std::copy(worker, worker+num_worker, this->worker);
    pe_is_worker = ( worker+num_worker != std::find(worker, worker+num_worker, this->comm->myrank) );

    this->job = job;
    
    runtime = new Runtime(this);
}

mexico::Instance::~Instance()
{
    delete runtime;
    /* delete worker */
    delete comm;
    delete memory;
    delete parser;
    delete log;
}

void mexico::Instance::exec(void* i_buf, int i_cnt, MPI_Datatype i_type, int i_num_vals, int i_max_worker_per_val,
                             int* i_worker, int* i_offsets, 
                             void* o_buf, int o_cnt, MPI_Datatype o_type, int o_num_vals, int o_max_worker_per_val,
                             int* o_worker, int* o_offsets)
{
    MEXICO_WRITE(Log::DEBUG, "start of Instance::exec() call");

    MEXICO_WRITE(Log::DEBUG, "calling Runtime::pre_comm()");
    runtime->pre_comm(i_buf, i_cnt, i_type, i_num_vals, i_max_worker_per_val, i_worker, i_offsets,
                      o_buf, o_cnt, o_type, o_num_vals, o_max_worker_per_val, o_worker, o_offsets);
   
    MEXICO_WRITE(Log::DEBUG, "running Job::exec()"); 
    runtime->exec_job();

    MEXICO_WRITE(Log::DEBUG, "calling Runtime::post_comm()");
    runtime->post_comm(i_buf, i_cnt, i_type, i_num_vals, i_max_worker_per_val, i_worker, i_offsets,
                       o_buf, o_cnt, o_type, o_num_vals, o_max_worker_per_val, o_worker, o_offsets);

    MEXICO_WRITE(Log::DEBUG, "Instance::exec() finished");
}

