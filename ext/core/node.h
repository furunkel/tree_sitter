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
VALUE rb_node_to_s(VALUE self);
VALUE rb_node_type(VALUE self);
VALUE rb_node_start_point(VALUE self);
VALUE rb_node_end_point(VALUE self);
VALUE rb_node_is_named(VALUE self);
VALUE rb_node_child_count(VALUE self);
VALUE rb_node_named_child_count(VALUE self);
VALUE rb_node_first_child(VALUE self);
VALUE rb_node_first_named_child(VALUE self);
VALUE rb_node_last_child(VALUE self);
VALUE rb_node_last_named_child(VALUE self);
VALUE rb_node_child(VALUE self, VALUE child_index);
VALUE rb_node_named_child(VALUE self, VALUE child_index);