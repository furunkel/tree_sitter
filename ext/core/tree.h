#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"

typedef struct {
  TSTreeCursor ts_tree_cursor;
  VALUE rb_tree;
} TreeCursor;

typedef struct {
  TSLanguage *ts_language;

  st_table *ts_symbol_table;
  ID *ts_symbol2id;
  size_t field_count;
  size_t symbol_count;

  st_table *ts_field_table;
  ID *ts_field2id;
} Language;

typedef struct {
  TSTree *ts_tree;
  VALUE rb_input;
  Language *language;
} Tree;

void init_tree();
VALUE rb_new_language(TSLanguage *ts_language);

extern const rb_data_type_t language_type;
extern const rb_data_type_t tree_type;

static inline Language *
rb_tree_language_(VALUE self) {
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  return tree->language;
}