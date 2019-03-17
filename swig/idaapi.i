%{
#include <loader.hpp>
#include <diskio.hpp>
%}

%{
#include <Python.h>

#ifdef HAVE_SSIZE_T
#define _SSIZE_T_DEFINED 1
#endif

//<code(py_idaapi)>
//</code(py_idaapi)>
%}

%constant ea_t BADADDR = ea_t(-1);
%constant sel_t BADSEL = sel_t(-1);
%constant size_t SIZE_MAX = size_t(-1);
/* %constant nodeidx_t BADNODE = nodeidx_t(-1); */

%include "typemaps.i"

%include "cstring.i"
%include "carrays.i"
%include "cpointer.i"

%pythoncode %{
#<pycode(py_idaapi)>
#</pycode(py_idaapi)>
%}


%inline %{
//<inline(py_idaapi)>
//</inline(py_idaapi)>
%}

//-------------------------------------------------------------------------
%inline %{
//<inline(py_idaapi_loader_input)>
//</inline(py_idaapi_loader_input)>
%}
