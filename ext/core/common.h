#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "ruby/encoding.h"

#ifndef MAX
#define MAX(a,b) (((a)<(b))?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) (((a)>(b))?(b):(a))
#endif

#ifndef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

#define CSTR2SYM(s) (ID2SYM(rb_intern((s))))
extern VALUE rb_eTreeSitterError;