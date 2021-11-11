#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"

typedef struct ast_node_type {
  TSNode ts_node;
} AstNode;

typedef struct point_type {
  TSPoint ts_point;
} Point;

void init_node();

VALUE rb_new_node(TSNode ts_node);
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