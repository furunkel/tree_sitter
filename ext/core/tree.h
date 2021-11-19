#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"

typedef struct {
  TSTree *ts_tree;
  VALUE rb_input;
} Tree;

typedef struct {
  TSTreeCursor ts_tree_cursor;
  VALUE rb_tree;
} TreeCursor;

void init_tree();