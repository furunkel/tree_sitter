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

typedef struct {
  TSLanguage *ts_language;
  st_table *field_table;
  ID *ids;
} Language;

void init_tree();
VALUE rb_new_language(TSLanguage *ts_language);
Language *rb_tree_language_(VALUE self);