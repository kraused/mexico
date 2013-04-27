
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

#include "parser.hpp"
#include "ast.hpp"
#include "log.hpp"


// ----------------------------------------------------------------------------------------------------
// Tree-walking algorithms for finding values in the AST
// ----------------------------------------------------------------------------------------------------
template<typename Tp>
void extract(mexico::Ast* ast, mexico::Log* log, Tp* p)
{
    MEXICO_FATAL("invalid type Tp.");
}

template<>
void extract<int>(mexico::Ast* ast, mexico::Log* log, int* p)
{
    if('I' == ast->nodetype)
        *p = ast->i;
    else
        goto ftn_fail;

ftn_exit:
    return;
ftn_fail:
    MEXICO_FATAL("ast->nodetype == %c, expected I", ast->nodetype);
    goto ftn_exit;
}

template<>
void extract<float>(mexico::Ast* ast, mexico::Log* log, float* p)
{
    if('F' == ast->nodetype)
        *p = ast->f;
    else
    if('I' == ast->nodetype)
        *p = ast->i;
    else
        goto ftn_fail;

ftn_exit:
    return;
ftn_fail:
    MEXICO_FATAL("ast->nodetype == %c, expected F or I", ast->nodetype);
    goto ftn_exit;
}

template<>
void extract<std::string>(mexico::Ast* ast, mexico::Log* log, std::string* p)
{
    if('S' == ast->nodetype)
        *p = ast->s;
    else
        goto ftn_fail;

ftn_exit:
    return;
ftn_fail:
    MEXICO_FATAL("ast->nodetype == %c, expected S", ast->nodetype);
    goto ftn_exit;
}

template<>
void extract<bool>(mexico::Ast* ast, mexico::Log* log, bool* p)
{
    if('B' == ast->nodetype)
        *p = ast->b;
    else
        goto ftn_fail;

ftn_exit:
    return;
ftn_fail:
    MEXICO_FATAL("ast->nodetype == %c, expected B", ast->nodetype);
    goto ftn_exit;
}

template<typename Tp>
static int find(mexico::Ast* ast, mexico::Log* log, const char* namelist_name, const char* var_name, Tp* p)
{
    int retval = 0;

    if(!ast)
        MEXICO_FATAL("!ast");

    switch(ast->nodetype)
    {
        case 'L':               //< namelist list
        case ',':               //< inside a list
            retval = find(ast->l, log, namelist_name, var_name, p);
            if(!retval)         /// Go down the subtree only if we haven't found the value already
                retval = find(ast->r, log, namelist_name, var_name, p);
            break;
        case '&':               //< a namelist
#ifndef NEBUG
            if('S' != ast->l->nodetype)
                MEXICO_FATAL("ast->l->nodetype == %c, expected S", ast->l->nodetype);
#endif
            if(std::string(namelist_name) == ast->l->s)
                retval = find(ast->r, log, namelist_name, var_name, p);
            break;
        case '=':
#ifndef NEBUG
            if('S' != ast->l->nodetype)
                MEXICO_FATAL("ast->l->nodetype == %c, expected S", ast->l->nodetype);
#endif
            if(std::string(var_name) == ast->l->s)
            {   
                extract(ast->r, log, p);
                retval = 1;
            }
            break;
            /// Should never see these in this function
        case 'I':
        case 'F':
        case 'S':
        case 'B':
        default:
            MEXICO_FATAL("bad node %c", ast->nodetype);
            goto ftn_exit;
    }

ftn_exit:
    return retval;
}

/// Print a single assignment
static void print_sa(const char* name, mexico::Ast* ast, mexico::Log* log)
{
    /* TODO The trailing comma is not so nice because the last assigment in
            the namelist should not have a trailing comma
     */
    switch(ast->nodetype)
    {
        case 'I':
            MEXICO_WRITE(mexico::Log::ALWAYS, "%s\t=\t%d,", name, ast->i);
            break;
        case 'F':
            MEXICO_WRITE(mexico::Log::ALWAYS, "%s\t=\t%f,", name, ast->f);
            break;
        case 'S':
            MEXICO_WRITE(mexico::Log::ALWAYS, "%s\t=\t'%s',", name, ast->s.c_str());
            break;
        case 'B':
            MEXICO_WRITE(mexico::Log::ALWAYS, "%s\t=\t%d,", name, ast->b);
            break;
        default:
            MEXICO_FATAL("bad node %c", ast->nodetype);
    }        
}

static int print(mexico::Ast* ast, mexico::Log* log, const char* namelist_name)
{
    int retval = 0;

    if(!ast)
        MEXICO_FATAL("!ast");

    switch(ast->nodetype)
    {
        case 'L':               //< namelist list
        case ',':               //< inside a list
            retval = print(ast->l, log, namelist_name);
            if(!retval)         /// Go down the subtree only if we haven't found the namelist
                retval = print(ast->r, log, namelist_name);
            break;
        case '&':               //< a namelist
#ifndef NEBUG
            if('S' != ast->l->nodetype)
                MEXICO_FATAL("ast->l->nodetype == %c, expected S", ast->l->nodetype);
#endif
            if(std::string(namelist_name) == ast->l->s)
                retval = print(ast->r, log, namelist_name);
            break;
        case '=':
#ifndef NEBUG
            if('S' != ast->l->nodetype)
                MEXICO_FATAL("ast->l->nodetype == %c, expected S", ast->l->nodetype);
#endif
            print_sa(ast->l->s.c_str(), ast->r, log);
            retval = 1;
            break;
            /// Should never see these in this function
        case 'I':
        case 'F':
        case 'S':
        case 'B':
        default:
            MEXICO_FATAL("bad node %c", ast->nodetype);
            goto ftn_exit;
    }

ftn_exit:
    return retval;
}

// ----------------------------------------------------------------------------------------------------

mexico::Parser::Parser(Instance* ptr)
: Pointers(ptr), ast(0)
{
}

extern void mexico_Parser_read(mexico::Ast**, FILE*);
extern void mexico_Parser_read(mexico::Ast**, const char*);

void mexico::Parser::read(FILE* file)
{
    mexico_Parser_read(&ast, file);
}

void mexico::Parser::read(const char* file_content)
{
    mexico_Parser_read(&ast, file_content);
}

int mexico::Parser::find_by_name_int(const char* namelist_name, const char* var_name)
{
    int i;
    find(ast, log, namelist_name, var_name, &i);

    return i;
}

float mexico::Parser::find_by_name_float(const char* namelist_name, const char* var_name)
{
    float f;
    find(ast, log, namelist_name, var_name, &f);

    return f;
}

std::string mexico::Parser::find_by_name_str(const char* namelist_name, const char* var_name)
{
    std::string s;
    find(ast, log, namelist_name, var_name, &s);    

    return s;
}

bool mexico::Parser::find_by_name_bool(const char* namelist_name, const char* var_name)
{
    bool b;
    find(ast, log, namelist_name, var_name, &b);

    return b;
}

void mexico::Parser::print_namelist(const char* namelist_name)
{
    MEXICO_WRITE(Log::ALWAYS, "&%s", namelist_name);
    print(ast, log, namelist_name);
    MEXICO_WRITE(Log::ALWAYS, "/");    
}

