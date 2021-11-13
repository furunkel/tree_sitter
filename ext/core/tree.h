#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"

static VALUE rb_eTreeError;

typedef struct {
  TSTree *ts_tree;
  VALUE rb_input;
} Tree;

void init_tree();
VALUE rb_tree_new(VALUE self, VALUE rb_str, VALUE rb_options);
