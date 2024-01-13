#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"

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

typedef struct {
  TSNode ts_node;
  TSFieldId field_id;
} TreePathNode;

typedef struct {
  TreePathNode *nodes;
  uint32_t len;
  VALUE rb_tree;
} TreePath;

typedef struct {
  Language *language;
  TSQuery *ts_query;
} Query;

#include "node.h"

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

extern ID id_error;

static inline ID
language_symbol2id(Language *language, TSSymbol symbol) {
  if(symbol == ((TSSymbol) -1)) {
    return id_error;
  } else {
    return language->ts_symbol2id[symbol];
  }
}

static inline ID
language_field2id(Language *language, TSFieldId field_id) {
  return language->ts_field2id[field_id];
}

bool language_id2field(Language *language, ID id, TSFieldId *field_id);
bool language_id2symbol(Language *language, ID id, TSSymbol *symbol);