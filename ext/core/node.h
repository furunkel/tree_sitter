#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "common.h"

typedef struct {
  uint16_t cached_field;
  uint16_t index;
  VALUE rb_tree;
  TSNode ts_node;
} AstNode;

typedef struct {
  TSPoint ts_point;
} Point;

#include "tree.h"

void init_node();

VALUE rb_node_byte_range_(TSNode node);
VALUE rb_node_text_(TSNode ts_node, VALUE rb_input);
VALUE rb_new_node(VALUE rb_tree, TSNode ts_node);
VALUE rb_new_node_with_field(VALUE rb_tree, TSNode ts_node, TSFieldId field_id);
TSPoint rb_point_point_(VALUE rb_point);
Tree *node_get_tree(AstNode *node);