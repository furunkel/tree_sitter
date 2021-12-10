#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"

typedef struct {
  TSNode ts_node;
  VALUE rb_tree;
} AstNode;

typedef struct {
  TSPoint ts_point;
} Point;

void init_node();

VALUE rb_node_byte_range_(TSNode node);
VALUE rb_node_text_(TSNode ts_node, VALUE rb_input);
VALUE rb_new_node(VALUE rb_tree, TSNode ts_node);