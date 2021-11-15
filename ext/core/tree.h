#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"

typedef struct {
  TSTree *ts_tree;
  VALUE rb_input;
} Tree;

void init_tree();
VALUE rb_tree_new(VALUE self, VALUE rb_str, VALUE rb_options);
