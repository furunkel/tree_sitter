#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "ruby/encoding.h"

#define CSTR2SYM(s) (ID2SYM(rb_intern((s))))
extern VALUE rb_eTreeSitterError;