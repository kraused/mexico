
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

#ifndef MEXICO_LOG_HPP_INCLUDED
#define MEXICO_LOG_HPP_INCLUDED 1

#include "pointers.hpp"

namespace mexico
{

/// Log: The logging facility for the library
class Log : public Pointers
{

public:
    Log(Instance* ptr);

    enum
    {
        ALWAYS = 0, /// Printed always
        MEDIUM = 1, /// Printed if the debug is 0, 1
        DEBUG  = 2  /// Only printed in debug equal to 2
    };

    /// Write to stdout. Only if level <= debug
    /// The first two arguments are the source filename and the
    /// line number. These arguments allow for precise location
    /// of the call to write(). To simplify the call to write(), 
    /// the MEXICO_WRITE macro is provided which automatically fills
    /// in the first two arguments.
    void write(const char* file, int lineno, int level, const char* fmt, ...);
#undef  MEXICO_WRITE
#define MEXICO_WRITE(level, ...)  log->write(__FILE__, __LINE__, level, __VA_ARGS__)

    /// Write a warning. This indicates a problem
    /// but the library will continue (probably trying
    /// to workaround the problem). However, for
    /// optimal performance, the user should investigate
    /// the source of the warning
    /// The first two arguments are the source filename and the
    /// line number. These arguments allow for precise location
    /// of the call to warn(). To simplify the call to warn(), 
    /// the MEXICO_WARN macro is provided which automatically fills
    /// in the first two arguments.
    void warn(const char* file, int lineno, const char* fmt, ...);
#undef  MEXICO_WARN
#define MEXICO_WARN(...) log->warn(__FILE__, __LINE__, __VA_ARGS__)

    /// If the library detects a fatal error it will
    /// not try to return control to the user for error
    /// handling but it will kill the application.
    /// The first two arguments are the source filename and the
    /// line number. These arguments allow for precise location
    /// of the call to fatal(). To simplify the call to fatal(), 
    /// the MEXICO_FATAL macro is provided which automatically fills
    /// in the first two arguments.    
    void fatal(const char* file, int lineno, const char* fmt, ...);
#undef  MEXICO_FATAL
#define MEXICO_FATAL(...) log->fatal(__FILE__, __LINE__, __VA_ARGS__)

    int debug;          ///< The debugging level

private:
    double start_stamp; ///< Time stamp of the creation of the log
                        ///  instance

    void report(FILE*, const char*, int, const char*, const char*, va_list);

};

}

#endif

