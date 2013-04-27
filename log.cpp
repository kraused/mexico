
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

#include <stdlib.h>

#include "log.hpp"


mexico::Log::Log(Instance* ptr)
: Pointers(ptr)
{
    start_stamp = MPI_Wtime();
}

void mexico::Log::report(FILE *fh, const char* file, int lineno, const char *prefix, const char *fmt, va_list params)
{
    char msg[4096];
    vsnprintf(msg, sizeof(msg), fmt, params);

    fprintf(fh, " %-9s %-9.3f %s(%3d): %s\n", prefix, MPI_Wtime()-start_stamp, file, lineno, msg);
    fflush(fh);
}

void mexico::Log::write(const char* file, int lineno, int level, const char* fmt, ...)
{
    va_list params;

    if(level > debug)
        return;

    va_start(params, fmt);
    report(stdout, file, lineno, "INFO", fmt, params);
    va_end  (params);
}

void mexico::Log::warn(const char* file, int lineno, const char* fmt, ...)
{
    va_list params;

    va_start(params, fmt);
    report(stderr, file, lineno, "WARN", fmt, params);
    va_end(params);
}
    
void mexico::Log::fatal(const char* file, int lineno, const char* fmt, ...)
{
    va_list params;

    va_start(params, fmt);
    report(stderr, file, lineno, "ERR" , fmt, params);
    va_end(params);

    exit(128);
}

