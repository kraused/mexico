
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

#ifndef MEXICO_PARSER_HPP_INCLUDED
#define MEXICO_PARSER_HPP_INCLUDED 1

#include <string>

#include "pointers.hpp"


namespace mexico
{

/// Forwarding
class Ast;

/// Parser: Unified parser for all the input namelists
class Parser : public Pointers
{

public:
    Parser(Instance* ptr);

    /// parse file (with content file_content)
    void read(FILE* file);
    void read(const char* file_content);

    /// Find an integer variable by the name of the namelist
    /// and the variable name. The function returns the 
    /// value.
    int find_by_name_int(const char* namelist_name,
                         const char* var_name);

    /// Find a float variable by the name of the namelist
    /// and the variable name. The function returns the 
    /// value.
    float find_by_name_float(const char* namelist_name,
                             const char* var_name);

    /// Find a string variable by the name of the namelist
    /// and the variable name. The function returns the 
    /// value.
    std::string find_by_name_str(const char* namelist_name,
                                 const char* var_name);

    /// Find a bool variable by the name of the namelist
    /// and the variable name. The function returns the
    /// value.
    bool find_by_name_bool(const char* namelist_name,
                           const char* var_name);

    /// Print a namelist content to the lg
    void print_namelist(const char* namelist_name);

private:
    Ast* ast; ///< The AST

};

}

#endif

