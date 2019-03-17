%{
#include <segregs.hpp>
%}

%import "range.i"

// Ignore kernel-only symbols
%ignore delete_v660_segreg_t;
%ignore v660_segreg_t;

#define R_es 29
#define R_cs 30
#define R_ss 31
#define R_ds 32
#define R_fs 33
#define R_gs 34

%include "segregs.hpp"
