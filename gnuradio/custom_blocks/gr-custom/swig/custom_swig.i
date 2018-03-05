/* -*- c++ -*- */

#define CUSTOM_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "custom_swig_doc.i"

%{
#include "custom/subaru_tpms_decode_cpp_char.h"
%}

%include "custom/subaru_tpms_decode_cpp_char.h"
GR_SWIG_BLOCK_MAGIC2(custom, subaru_tpms_decode_cpp_char);
