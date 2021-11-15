#pragma once

#ifndef __MSXML_LIBRARY_DEFINED__
#define __MSXML_LIBRARY_DEFINED__
#endif

#include "ruby.h"
#include "tree_sitter/api.h"
#include "ruby/encoding.h"

#include "tree.h"
#include "node.h"

#define CSTR2SYM(s) (ID2SYM(rb_intern((s))))

extern VALUE rb_eTreeSitterError;

void Init_treesitter();