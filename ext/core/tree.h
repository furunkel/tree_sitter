#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"

static VALUE rb_eTreeError;

typedef struct {
  TSTree *ts_tree;
} Tree;

void init_tree();
static VALUE rb_tree_alloc(VALUE self);
VALUE rb_tree_new(VALUE self, VALUE rb_str, VALUE rb_options);
VALUE rb_tree_set_language(VALUE self, VALUE lang);
VALUE rb_tree_set_input_string(VALUE self, VALUE str);
VALUE rb_tree_parse(VALUE self);
