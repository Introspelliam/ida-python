%{
#include <struct.hpp>
%}
// Kernel-only symbols
%ignore save_structs;
%ignore get_struc_name(tid_t);
%ignore get_member_name(tid_t);
%ignore get_member_by_id(tid_t, struc_t **); // allow version w/ qstring* only

//-------------------------------------------------------------------------
// For 'get_member_by_id()'
%typemap(in,numinputs=0) qstring *out_mname (qstring temp) {
  $1 = &temp;
}
%typemap(argout) qstring *out_mname {
  if (result)
  {
    %append_output(PyString_FromStringAndSize($1->begin(), $1->length()));
  }
  else
  {
    Py_XDECREF(resultobj);
    Py_INCREF(Py_None);
    resultobj = Py_None;
  }
}
%typemap(freearg) qstring* out_mname
{
  // Nothing. We certainly don't want 'temp' to be deleted.
}

//-------------------------------------------------------------------------
// For 'get_member_by_fullname()' and 'get_member_by_id()'
%typemap(in,numinputs=0) struc_t **sptr_place (struc_t *temp) {
  $1 = &temp;
}
%typemap(argout) struc_t **sptr_place {
  if ( result )
  {
    %append_output(SWIG_NewPointerObj(SWIG_as_voidptr(*($1)), SWIGTYPE_p_struc_t, 0 |  0 ));
  }
  else
  {
    Py_XDECREF(resultobj);
    Py_INCREF(Py_None);
    resultobj = Py_None;
  }
}

%nonnul_argument_prototype(
        asize_t get_member_size(const member_t *nonnul_mptr),
        const member_t *nonnul_mptr);

//-------------------------------------------------------------------------
%include "struct.hpp"
// Add a get_member() member function to struc_t.
// This helps to access the members array in the class.
%extend struc_t {
  member_t *get_member(int index) { return &(self->members[index]); }
}

%inline %{
//<inline(py_struct)>
//</inline(py_struct)>
%}
