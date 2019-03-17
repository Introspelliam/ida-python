
%ignore user2str;
%ignore back_char;
%ignore qstr2user;
%ignore user2qstr;
%ignore str2user;
%rename (str2user) py_str2user;
%ignore convert_encoding;
%ignore is_valid_utf8;
%ignore qustrlen;
%ignore get_utf8_char;
%ignore put_utf8_char;
%ignore prev_utf8_char;
%ignore idb_utf8;
%ignore scr_utf8;
%ignore utf8_scr;
%ignore change_codepage;
%ignore utf16_utf8;
%ignore utf8_utf16;
%ignore acp_utf8;
%ignore utf8_wchar16;
%ignore utf8_wchar32;
%ignore skip_utf8;
%ignore qustrncpy;
%ignore expand_argv;
%ignore free_argv;
%ignore qwait;
%ignore qwait_for_handles;
%ignore qwait_timed;
%ignore ida_true_type;
%ignore ida_false_type;
%ignore bitcount;
%ignore round_up_power2;
%ignore round_down_power2;

//<typemaps(pro)>
//</typemaps(pro)>

%include "pro.h"

// we must include those manually here
%import "ida.hpp"
%import "xref.hpp"
%import "typeinf.hpp"
%import "enum.hpp"
%import "netnode.hpp"
//

void qvector<int>::grow(const int &x=0);
%ignore qvector<int>::grow;
void qvector<unsigned int>::grow(const unsigned int &x=0);
%ignore qvector<unsigned int>::grow;
void qvector<long long>::grow(const long long &x=0);
%ignore qvector<long long>::grow;
void qvector<unsigned long long>::grow(const unsigned long long &x=0);
%ignore qvector<unsigned long long>::grow;

//---------------------------------------------------------------------
%template(intvec_t)       qvector<int>;
%template(uintvec_t)      qvector<unsigned int>;
%template(longlongvec_t)  qvector<long long>;
%template(ulonglongvec_t) qvector<unsigned long long>;
%template(boolvec_t)      qvector<bool>;

%pythoncode %{
%}


%uncomparable_elements_qvector(simpleline_t, strvec_t);
%template(sizevec_t)  qvector<size_t>;
typedef uvalvec_t eavec_t;// vector of addresses

SWIG_DECLARE_PY_CLINKED_OBJECT(qstrvec_t)

%inline %{
//<inline(py_pro)>
//</inline(py_pro)>
%}

%include "carrays.i"
%include "cpointer.i"
%array_class(uchar, uchar_array);
%array_class(tid_t, tid_array);
%array_class(ea_t, ea_array);
%array_class(sel_t, sel_array);
%array_class(uval_t, uval_array);
%pointer_class(int, int_pointer);
%pointer_class(ea_t, ea_pointer);
%pointer_class(sval_t, sval_pointer);
%pointer_class(sel_t, sel_pointer);

%pythoncode %{
#<pycode(py_pro)>
#</pycode(py_pro)>
%}
