#include "tree.h"
#include "tree_sitter/api.h"
#include <wctype.h>
static VALUE rb_cTree;
static VALUE rb_cTreeCursor;
static VALUE rb_cLanguage;
static VALUE rb_cTreePath;

static ID id_types;
static ID id_whitespace;
static ID id_attach;

ID id_error;
ID id___language__;

#ifndef MAX
#define MAX(a,b) (((a)<(b))?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) (((a)>(b))?(b):(a))
#endif

#ifndef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

static void
tree_free(void* obj)
{
  Tree* tree = (Tree*)obj;
  ts_tree_delete(tree->ts_tree);
  xfree(obj);
}

static void
tree_mark(void* obj)
{
  Tree* tree = (Tree*)obj;
  rb_gc_mark(tree->rb_input);
}

const rb_data_type_t tree_type = {
    .wrap_struct_name = "Tree",
    .function = {
        .dmark = tree_mark,
        .dfree = tree_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void
tree_cursor_free(void* obj)
{
  TreeCursor* tree = (TreeCursor*)obj;
  ts_tree_cursor_delete(&tree->ts_tree_cursor);
  xfree(obj);
}

static void
tree_cursor_mark(void* obj)
{
  TreeCursor* tree_cursor = (TreeCursor*)obj;
  rb_gc_mark(tree_cursor->rb_tree);
}

extern const rb_data_type_t node_type;

const rb_data_type_t tree_cursor_type = {
    .wrap_struct_name = "Tree::Cursor",
    .function = {
        .dmark = tree_cursor_mark,
        .dfree = tree_cursor_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void
tree_path_free(void* obj)
{
  TreePath* tree_path = (TreePath*)obj;
  xfree(tree_path->nodes);
  xfree(obj);
}

static void
tree_path_mark(void* obj)
{
  TreePath* tree_path = (TreePath *)obj;
  rb_gc_mark(tree_path->rb_tree);
}

const rb_data_type_t tree_path_type = {
    .wrap_struct_name = "Tree::Path",
    .function = {
        .dmark = tree_path_mark,
        .dfree = tree_path_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};


static void
query_free(void* obj)
{
  Query* query = (Query*)obj;
  ts_query_delete(query->ts_query);
  xfree(obj);
}

static void
query_mark(void* obj)
{
  // Query* query = (Query *)obj;
}

const rb_data_type_t query_type = {
    .wrap_struct_name = "Language::Query",
    .function = {
        .dmark = query_mark,
        .dfree = query_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void
language_free(void* obj)
{
  Language* language = (Language*)obj;
  st_free_table(language->ts_symbol_table);
  st_free_table(language->ts_field_table);
  xfree(language->ts_symbol2id);
  xfree(language->ts_field2id);
  xfree(obj);
}

// #define TS_NODE_ARRAY_EMBED_LEN 64

// typedef struct {
//   TSNode *data;
//   TSNode embed_data[TS_NODE_ARRAY_EMBED_LEN];
//   uint32_t len;
//   uint32_t capa;
// } TSNodeArray;

// static void
// ts_node_array_init(TSNodeArray *array) {
//   array->data = array->embed_data;
//   array->len = 0;
//   array->capa = TS_NODE_ARRAY_EMBED_LEN;
// }

// static void
// ts_node_array_destroy(TSNodeArray *array) {
//   if(array->data != array->embed_data) {
//     xfree(array->data);
//   }
// }

// static void
// ts_node_array_clear(TSNodeArray *array) {
//   array->len = 0;
// }

// static void
// ts_node_array_push(TSNodeArray *array, TSNode node) {
//   if(array->len == array->capa) {
//     uint32_t new_capa = array->capa * 2;
//     if(array->data == array->embed_data) {
//       array->data = RB_ALLOC_N(TSNode, new_capa);
//       MEMCPY(array->data, array->embed_data, TSNode, array->len);
//     } else {
//       RB_REALLOC_N(array->data, TSNode, new_capa);
//     }
//   } else {
//     array->data[array->len++] = node;
//   }
// }


/*static void
language_mark(void* obj)
{
  // Language* language = (Language*)obj;
  // rb_gc_mark(language->rb_input);
}*/

const rb_data_type_t language_type = {
    .wrap_struct_name = "Language",
    .function = {
        .dmark = NULL /*language_mark*/,
        .dfree = language_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

VALUE
rb_new_language(TSLanguage *ts_language)
{
  Language *language = RB_ZALLOC(Language);
  language->ts_language = ts_language;

  uint32_t symbol_count = ts_language_symbol_count(ts_language);
  uint32_t field_count = ts_language_field_count(ts_language);

  language->ts_symbol_table = st_init_numtable();
  language->ts_field_table = st_init_numtable();

  language->ts_symbol2id = RB_ZALLOC_N(ID, symbol_count);
  language->ts_field2id = RB_ZALLOC_N(ID, field_count + 1);
  language->symbol_count = symbol_count;
  language->field_count = field_count + 1;

  for(uint32_t i = 0; i < symbol_count; i++) {
    const char *symbol_name = ts_language_symbol_name(ts_language, (TSSymbol) i);
    if(symbol_name != NULL) {
      ID symbol_id = rb_intern(symbol_name);
      st_insert(language->ts_symbol_table, (st_data_t) symbol_id, i);
      language->ts_symbol2id[i] = symbol_id;
    }
  }

  /* NOTE: for soem reason it's <= field_count */
  for(uint32_t i = 0; i <= field_count; i++) {
    const char *field_name = ts_language_field_name_for_id(ts_language, (TSFieldId) i);
    if(field_name != NULL) {
      ID field_id = rb_intern(field_name);
      st_insert(language->ts_field_table, (st_data_t) field_id, i);
      language->ts_field2id[i] = field_id;
    }  
  }

  return TypedData_Wrap_Struct(rb_cLanguage, &language_type, language);
}

static VALUE
rb_language_fields(VALUE self) {
  Language* language;
  TypedData_Get_Struct(self, Language, &language_type, language);

  VALUE rb_fields = rb_ary_new_capa(language->field_count);
  for(size_t i = 0; i < language->field_count; i++) {
    ID id = language->ts_field2id[i];
    if(id) {
      rb_ary_push(rb_fields, RB_ID2SYM(id));
    }
  }

  return rb_fields;
}

static VALUE
rb_language_symbols(VALUE self) {
  Language* language;
  TypedData_Get_Struct(self, Language, &language_type, language);

  VALUE rb_symbols = rb_ary_new_capa(language->symbol_count);
  for(size_t i = 0; i < language->symbol_count; i++) {
    rb_ary_push(rb_symbols, RB_ID2SYM(language->ts_symbol2id[i]));
  }
  return rb_symbols;
}

bool language_id2field(Language *language, ID id, TSFieldId *field_id) {
  st_data_t ts_field_id;
  if(st_lookup(language->ts_field_table, (st_data_t) id, &ts_field_id)) {
    *field_id = (TSFieldId) ts_field_id;
    return true;
  }
  return false;
}

bool language_id2symbol(Language *language, ID id, TSSymbol *symbol) {
  st_data_t ts_symbol;
  if(st_lookup(language->ts_symbol_table, (st_data_t) id, &ts_symbol)) {
    *symbol = (TSSymbol) ts_symbol;
    return true;
  }
  return false;
}

static VALUE
rb_query_new(VALUE self, VALUE rb_source) {
  Check_Type(rb_source, T_STRING);

  char *source = RSTRING_PTR(rb_source);
  size_t length = RSTRING_LEN(rb_source);

  uint32_t error_offset;
  TSQueryError error_type;

  VALUE rb_language = rb_ivar_get(self, id___language__);
  Language* language;
  TypedData_Get_Struct(rb_language, Language, &language_type, language);

  TSQuery *ts_query = ts_query_new(language->ts_language, source, length, &error_offset, &error_type);
  if(!ts_query) {
        // Adapted from https://github.com/tree-sitter/py-tree-sitter/blob/master/tree_sitter/binding.c
        // The MIT License (MIT)
        // Original version Copyright (c) 2019 Max Brunsfeld, GitHub
        char *word_start = &source[error_offset];
        char *word_end = word_start;
        while (word_end < &source[length] &&
               (iswalnum(*word_end) || *word_end == '-' || *word_end == '_' || *word_end == '?' ||
                *word_end == '.'))
            word_end++;
        char c = *word_end;
        *word_end = 0;
        switch (error_type) {
        case TSQueryErrorNodeType:
            rb_raise(rb_eArgError, "Invalid node type %s", &source[error_offset]);
            break;
        case TSQueryErrorField:
            rb_raise(rb_eArgError, "Invalid field name %s", &source[error_offset]);
            break;
        case TSQueryErrorCapture:
            rb_raise(rb_eArgError, "Invalid capture name %s", &source[error_offset]);
            break;
        default:
            rb_raise(rb_eArgError, "Invalid syntax at offset %u", error_offset);
            break;
        }
        *word_end = c;
        return Qnil;
  }

  Query *query = RB_ZALLOC(Query);

  query->language = language;
  query->ts_query = ts_query;

  return TypedData_Wrap_Struct(self, &query_type, query);
}

typedef struct {
  TSQueryCursor *cursor;
  AstNode *node;
} QueryRunArgs;

static VALUE
rb_query_run_yield(VALUE args_) {
  QueryRunArgs *args = (QueryRunArgs *) args_;
  uint32_t capture_index;
  TSQueryMatch match;
  while (ts_query_cursor_next_capture(args->cursor, &match, &capture_index)) {
    VALUE rb_captures = rb_ary_new_capa(match.capture_count);
    for(uint32_t i = 0; i < match.capture_count; i++) {
      //FIXME: use match.captures[i].index to set the index of the capture.
      rb_ary_push(rb_captures, rb_new_node(args->node->rb_tree, match.captures[i].node));
    }
    rb_yield_values(2, rb_captures, UINT2NUM(match.pattern_index));
  }
  return Qnil;
}

static VALUE
rb_query_run_ensure(VALUE args_) {
  QueryRunArgs *args = (QueryRunArgs *) args_;
  ts_query_cursor_delete(args->cursor);
  return Qnil;
}

static VALUE
rb_query_run(VALUE self, VALUE rb_node, VALUE rb_start_byte, VALUE rb_end_byte, VALUE rb_start_point, VALUE rb_end_point) {

  Query* query;
  TypedData_Get_Struct(self, Query, &query_type, query);

  TSPoint start_point = {.row = 0, .column = 0};
  TSPoint end_point = {.row = UINT32_MAX, .column = UINT32_MAX};
  unsigned start_byte = 0, end_byte = UINT32_MAX;

  if(RTEST(rb_start_byte)) {
    start_byte = FIX2UINT(rb_start_byte);
  }

  if(RTEST(rb_end_byte)) {
    end_byte = FIX2UINT(rb_end_byte);
  }

  if(RTEST(rb_start_point)) {
    start_point = rb_point_point_(rb_start_point);
  }

  if(RTEST(rb_end_point)) {
    end_point = rb_point_point_(rb_end_point);
  }

  AstNode* node;
  TypedData_Get_Struct(rb_node, AstNode, &node_type, node);

  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_set_byte_range(cursor, start_byte, end_byte);
  ts_query_cursor_set_point_range(cursor, start_point, end_point);
  ts_query_cursor_exec(cursor, query->ts_query, node->ts_node);

  QueryRunArgs run_args = {
    .cursor = cursor,
    .node = node,
  };

  rb_ensure(rb_query_run_yield, (VALUE) &run_args, rb_query_run_ensure, (VALUE) &run_args);
  return self;
}

static VALUE
rb_tree_alloc(VALUE self)
{
  Tree* tree = RB_ZALLOC(Tree);

  VALUE rb_language = rb_ivar_get(self, id___language__);
  Language* language;
  TypedData_Get_Struct(rb_language, Language, &language_type, language);
  tree->language = language;

  return TypedData_Wrap_Struct(self, &tree_type, tree);
}

static VALUE
rb_tree_cursor_alloc(VALUE self)
{
  TreeCursor* tree_cursor = RB_ZALLOC(TreeCursor);
  return TypedData_Wrap_Struct(self, &tree_cursor_type, tree_cursor);
}

static VALUE
rb_new_tree_path(VALUE rb_tree, TreePathNode *nodes, size_t len)
{
  TreePath* tree_path = RB_ZALLOC(TreePath);
  tree_path->rb_tree = rb_tree;
  tree_path->nodes = nodes;
  tree_path->len = len;
  return TypedData_Wrap_Struct(rb_cTreePath, &tree_path_type, tree_path);
}

static VALUE
rb_tree_language(VALUE self)
{
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  (void) tree;

  VALUE rb_klass = rb_class_of(self);
  VALUE rb_language = rb_ivar_get(rb_klass, id___language__);
  return rb_language;
}

static VALUE
rb_tree_language_s(VALUE self)
{
  VALUE rb_language = rb_ivar_get(self, id___language__);
  return rb_language;
}

/*
 * Public: Creates a new tree
 *
 */
static VALUE
rb_tree_initialize(int argc, VALUE* argv, VALUE self)
{
  VALUE rb_input;
  VALUE rb_options;

  rb_scan_args(argc, argv, "1:", &rb_input, &rb_options);
  Check_Type(rb_input, T_STRING);

  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  Language* language = rb_tree_language_(self);
  TSLanguage *ts_language = language->ts_language;
  TSParser* parser = ts_parser_new();

  ts_parser_set_language(parser, ts_language);
  TSTree* ts_tree = ts_parser_parse_string(
    parser, NULL, RSTRING_PTR(rb_input), RSTRING_LEN(rb_input));
  ts_parser_delete(parser);

  tree->ts_tree = ts_tree;

  VALUE rb_attach = Qtrue;
  if (!NIL_P(rb_options)) {
    rb_attach = rb_hash_lookup2(rb_options, RB_ID2SYM(id_attach), Qtrue);
  }

  if (RTEST(rb_attach)) {
    tree->rb_input = rb_input;
  } else {
    tree->rb_input = Qnil;
  }

  return self;
}

/*
 * Public: Returns the tree root node.
 *
 * Returns a {Node}.
 */
static VALUE
rb_tree_root_node(VALUE self)
{
  Tree* tree;

  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  TSNode ts_node = ts_tree_root_node(tree->ts_tree);
  return rb_new_node(self, ts_node);
}

static VALUE
rb_tree_copy(VALUE self)
{
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  Tree* clone = RB_ZALLOC(Tree);
  clone->ts_tree = ts_tree_copy(tree->ts_tree);
  clone->rb_input = tree->rb_input;
  return TypedData_Wrap_Struct(rb_cTree, &tree_type, clone);
}

  //   VALUE rb_text = rb_node_text_(node, rb_input);
  //   if(!NIL_P(rb_text)) {
  //     if(!types) {
  //       rb_ary_push(rb_ary, rb_text);
  //     } else {
  //       VALUE rb_type = ID2SYM(rb_intern(type));
  //       VALUE rb_pair = rb_assoc_new(rb_text, rb_type);
  //       rb_ary_push(rb_ary, rb_pair);
  //     }
  //   }
  // } else {
  //   for(uint32_t i = 0; i < child_count; i++) {
  //     TSNode child_node = ts_node_child(node, i);
  //     collect_leaf_nodes(child_node, rb_input, rb_ary, types, comments);
  //   }
  // }

// static int
// node_start_byte_cmp(const void* a, const void* b)
// {
//   TSNode* node_a = (TSNode*)a;
//   TSNode* node_b = (TSNode*)b;

//   uint32_t start_byte_a = ts_node_start_byte(*node_a);
//   uint32_t start_byte_b = ts_node_start_byte(*node_b);

//   return start_byte_a - start_byte_b;
// }

static VALUE
rb_tree_attach(VALUE self, VALUE rb_input)
{
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  Check_Type(rb_input, T_STRING);

  tree->rb_input = rb_input;
  return self;
}

static VALUE
rb_tree_detach(VALUE self)
{
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  VALUE rb_input = tree->rb_input;
  tree->rb_input = Qnil;

  return rb_input;
}

static VALUE
rb_tree_cursor_initialize(VALUE self, VALUE rb_node)
{
  TreeCursor* tree_cursor;
  TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);

  AstNode* node;
  TypedData_Get_Struct(rb_node, AstNode, &node_type, node);

  tree_cursor->ts_tree_cursor = ts_tree_cursor_new(node->ts_node);
  tree_cursor->rb_tree = node->rb_tree;
  return self;
}

#define TREE_CURSOR_NAV_METHOD(name)                                           \
  static VALUE rb_tree_cursor_##name(VALUE self)                               \
  {                                                                            \
    TreeCursor* tree_cursor;                                                   \
    TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);    \
    bool ret = ts_tree_cursor_##name(&tree_cursor->ts_tree_cursor);            \
    return ret ? self : Qnil;                                                  \
  }

TREE_CURSOR_NAV_METHOD(goto_parent)
TREE_CURSOR_NAV_METHOD(goto_first_child)
TREE_CURSOR_NAV_METHOD(goto_next_sibling)

static int
node_id_cmp(const void* a, const void* b)
{
  TSNode** node_a = (TSNode**)a;
  TSNode** node_b = (TSNode**)b;

  int diff = (uintptr_t)(**node_a).id - (uintptr_t)(**node_b).id;
  return diff;
}

static VALUE
rb_tree_find_common_parent_(int argc, VALUE *argv, VALUE self) {
  // should fit on the stack?
  VALUE rb_tree;
  Tree *tree = NULL;
  AstNode *node = NULL;
  uint32_t min_byte = UINT32_MAX;
  uint32_t max_byte = 0;

  for(int i = 0; i < argc; i++) {
    VALUE rb_node = argv[i];
    AstNode *node_;

    TypedData_Get_Struct(rb_node, AstNode, &node_type, node_);
    rb_tree = node_->rb_tree;
    Tree *tree_ = (Tree *) DATA_PTR(rb_tree);
    if(tree != NULL && tree != tree_) {
      rb_raise(rb_eTreeSitterError, "nodes belong to different trees");
      return Qnil;
    }
    min_byte = MIN(min_byte, ts_node_start_byte(node_->ts_node));
    max_byte = MAX(max_byte, ts_node_end_byte(node_->ts_node));

    tree = tree_;
    node = node_;
  }

  if(min_byte > max_byte) {
    return Qnil;
  } else {
    TSNode n = node->ts_node;
    while(!(ts_node_is_null(n) || (ts_node_start_byte(n) <= min_byte && ts_node_end_byte(n) >= max_byte))) {
      n = ts_node_parent(n);
    }
    return rb_new_node(rb_tree, n);
  }
}

static VALUE
rb_tree_find_common_parent(int argc, VALUE *argv, VALUE self) {
  if(argc == 1 && RB_TYPE_P(argv[0], T_ARRAY)) {
    return rb_tree_find_common_parent_(RARRAY_LEN(argv[0]), RARRAY_PTR(argv[0]), self);
  } else {
    return rb_tree_find_common_parent_(argc, argv, self);
  }
}

// static bool
// rb_tree_merge__(const TSNode *nodes, size_t len, TSNode *out_nodes, size_t *out_len) {
//   TSNode *parents = ALLOCA_N(TSNode, len);
//   bool change = false;
//   TSNode **parent_ptrs = ALLOCA_N(TSNode *, len);

//   for(size_t i = 0; i < len; i++) {
//     // if(ts_node_is_named(nodes[i])) {
//       parents[i] = ts_node_parent(nodes[i]);
//     // } else {
//     //   parents[i].id = 0;
//     // }
//     parent_ptrs[i] = &parents[i];
//   }

//   qsort(parent_ptrs, len, sizeof(TSNode *), node_id_cmp);

//   for(size_t i = 0; i < len; i++) {
//     TSNode *parent_ptr = parent_ptrs[i];
//     size_t parent_index = parent_ptr - parents;

//     /* unnamed child node - copy child */
//     // if(parent_ptr->id == 0) {
//     //   out_nodes[(*out_len)] = nodes[parent_index];
//     //   (*out_len)++;
//     // } else 
//     {
//       uint32_t j;
//       for(j = 1; i + j < len && parent_ptrs[i + j]->id == parent_ptr->id; j++) {}

//       //FIXME: use named child count?
//       // uint32_t child_count = ts_node_named_child_count(*parent_ptr);
//       uint32_t child_count = ts_node_child_count(*parent_ptr);

//       if(child_count > 1 && j >= child_count) {
//         out_nodes[(*out_len)] = *parent_ptr;
//         (*out_len)++;
//         i += j - 1;
//         change = true;
//       } else {
//         out_nodes[(*out_len)] = nodes[parent_index];
//         (*out_len)++;
//       }
//     }
//   }

//   return change;
// }

// static VALUE
// rb_tree_merge_(int argc, VALUE *argv, VALUE self) {

//   if(argc == 1) {
//     return argv[0];
//   } else if(argc == 0) {
//     return Qnil;
//   }

//   // should fit on the stack?
//   TreePath *paths = ALLOCA_N(TreePath, argc);
//   VALUE rb_tree = Qnil;
//   Tree *tree = NULL;

//   for(int i = 0; i < argc; i++) {
//     VALUE rb_path = argv[i];
//     TreePath *path;
//     TypedData_Get_Struct(rb_path, TreePath, &tree_path_type, path);
//     rb_tree = path->rb_tree;
//     Tree *tree_ = (Tree *) DATA_PTR(rb_tree);
//     if(tree != NULL && tree != tree_) {
//       rb_raise(rb_eTreeSitterError, "nodes belong to different trees");
//       return Qnil;
//     }
//     tree = tree_;
//     paths[i] = path;
//   }

//   size_t len = argc;
//   TreePath *out_paths = ALLOCA_N(TreePath, argc);

//   int i = 0;
// redo:
//   size_t out_len = 0;
//   bool change = rb_tree_merge__(paths, len, out_paths, &out_len);

//   if(change && i++ < 20) {
//     TreePath *tmp = pathes;
//     pathes = out_paths;
//     out_paths = tmp;
//     len = out_len;
//     goto redo;
//   }

//   VALUE rb_ary = rb_ary_new_capa(out_len);
//   for(size_t i = 0; i < out_len; i++) {
//     rb_ary_push(rb_ary, rb_new_node(rb_tree, out_nodes[i]));
//   }
//   return rb_ary;
// }

// static VALUE
// rb_tree_merge(int argc, VALUE *argv, VALUE self) {
//   if(argc == 1 && RB_TYPE_P(argv[0], T_ARRAY)) {
//     return rb_tree_merge_(RARRAY_LEN(argv[0]), RARRAY_PTR(argv[0]), self);
//   } else {
//     return rb_tree_merge_(argc, argv, self);
//   }
// }

static void
find_path_by_byte(VALUE rb_tree, Language *language, TSNode node, uint32_t min_byte, uint32_t max_byte, TreePathNode **nodes_out, size_t *nodes_len_out) {
  TSTreeCursor tree_cursor = ts_tree_cursor_new(node);
  TSNode current_node = node;
  TSFieldId current_field_id = 0;

  size_t nodes_capa = 16;
  size_t nodes_len = 0;
  TreePathNode *nodes = RB_ZALLOC_N(TreePathNode, nodes_capa);

  while(true) {
    uint32_t mid_byte = (uint32_t) (((uint64_t)max_byte + (uint64_t) min_byte) / 2);
    int64_t ret = ts_tree_cursor_goto_first_child_for_byte(&tree_cursor, mid_byte);

    // if ret == -1 we are at the goal node, so we want the node, not its type
    uint32_t start_byte = ts_node_start_byte(current_node);
    uint32_t end_byte = ts_node_end_byte(current_node);

    // fprintf(stderr, "min/max byte: %d/%d/%d\n", min_byte, max_byte, mid_byte);
    // fprintf(stderr, "start/end byte: %d/%d\n", start_byte, end_byte);

    if(start_byte <= min_byte && end_byte > max_byte) {
      if(nodes_len == nodes_capa) {
        size_t new_capa = nodes_capa * 2;
        RB_REALLOC_N(nodes, TreePathNode, new_capa);
        nodes_capa = new_capa;
      } else {
        TreePathNode path_node = {
          .ts_node = current_node,
          .field_id = current_field_id
        };
        nodes[nodes_len++] = path_node;
      }
    }

    if(ret == -1) {
      break;
    } 

    current_field_id = ts_tree_cursor_current_field_id(&tree_cursor);
    current_node = ts_tree_cursor_current_node(&tree_cursor);
  }
  ts_tree_cursor_delete(&tree_cursor);

  *nodes_out = nodes;
  *nodes_len_out = nodes_len;
}

static VALUE
rb_tree_path_to(VALUE self, VALUE rb_goal_byte) {
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  TSNode root_node = ts_tree_root_node(tree->ts_tree);

  uint32_t min_byte = UINT32_MAX;
  uint32_t max_byte = 0;

  if(RB_TYPE_P(rb_goal_byte, T_ARRAY)) {
    for(long int i = 0; i < RARRAY_LEN(rb_goal_byte); i++) {
      VALUE rb_byte = RARRAY_AREF(rb_goal_byte, i);
      uint32_t byte = (uint32_t) FIX2UINT(rb_byte);
      min_byte = MIN(min_byte, byte);
      max_byte = MAX(max_byte, byte);
    }
  } else {
    min_byte = (uint32_t) FIX2UINT(rb_goal_byte);
    max_byte = min_byte;
  }

  Language *language = rb_tree_language_(self);

  size_t nodes_len;
  TreePathNode *nodes;
  find_path_by_byte(self, language, root_node, min_byte, max_byte, &nodes, &nodes_len);

  VALUE rb_path = rb_new_tree_path(self, nodes, nodes_len);
  return rb_path;
}


static TSNode
find_node_by_byte(VALUE rb_tree, Language *language, TSNode node, uint32_t min_byte, uint32_t max_byte) {
  TSTreeCursor tree_cursor = ts_tree_cursor_new(node);
  TSNode found_node = {0, };

  uint32_t mid_byte = max_byte / 2 + min_byte / 2;

  fprintf(stderr, "MIN/MAX %d/%d => %d\n", min_byte, max_byte, mid_byte);

  while(true) {
    int64_t ret = ts_tree_cursor_goto_first_child_for_byte(&tree_cursor, mid_byte);
    TSNode node = ts_tree_cursor_current_node(&tree_cursor);

    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);

    fprintf(stderr, "START/END %s %d/%d\n", ts_node_type(node), start_byte, end_byte);

    if(start_byte <= min_byte && end_byte > max_byte) {
      found_node = node;
    } else {
      break;
    }

    if(ret == -1) {
      break;
    } 
  }

  ts_tree_cursor_delete(&tree_cursor);
  return found_node;
}

static VALUE
rb_tree_find_by_byte(VALUE self, VALUE rb_goal_byte) {
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  uint32_t min_byte = UINT32_MAX;
  uint32_t max_byte = 0;

  if(RB_TYPE_P(rb_goal_byte, T_ARRAY)) {
    for(long int i = 0; i < RARRAY_LEN(rb_goal_byte); i++) {
      VALUE rb_byte = RARRAY_AREF(rb_goal_byte, i);
      uint32_t byte = (uint32_t) FIX2UINT(rb_byte);
      min_byte = MIN(min_byte, byte);
      max_byte = MAX(max_byte, byte);
    }
  } else {
    Check_Type(rb_goal_byte, T_FIXNUM);
    min_byte = (uint32_t) FIX2UINT(rb_goal_byte);
    max_byte = min_byte;
  }

  TSNode root_node = ts_tree_root_node(tree->ts_tree);
  Language *language = rb_tree_language_(self);

  TSNode node = find_node_by_byte(self, language, root_node, min_byte, max_byte);
  if(ts_node_is_null(node)) {
    return Qnil;
  } else {
    VALUE rb_node = rb_new_node(self, node);
    return rb_node;
  }
}

static VALUE
rb_tree_cursor_goto_first_child_for_byte(VALUE self, VALUE rb_goal_byte)
{
  TreeCursor* tree_cursor;
  TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);

  uint32_t goal_byte = (uint32_t) FIX2UINT(rb_goal_byte);
  int64_t ret = ts_tree_cursor_goto_first_child_for_byte(
    &tree_cursor->ts_tree_cursor, goal_byte);
  return ret == -1 ? Qnil : LL2NUM(ret);
}

static VALUE
rb_tree_cursor_current_node(VALUE self)
{
  TreeCursor* tree_cursor;
  TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);

  TSNode current_node =
    ts_tree_cursor_current_node(&tree_cursor->ts_tree_cursor);
  return rb_new_node(tree_cursor->rb_tree, current_node);
}

static VALUE
rb_tree_cursor_current_field_name(VALUE self)
{
  TreeCursor* tree_cursor;
  TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);

  return rb_str_new_cstr(
    ts_tree_cursor_current_field_name(&tree_cursor->ts_tree_cursor));
}

static VALUE
rb_tree_cursor_current_field(VALUE self)
{
  TreeCursor* tree_cursor;
  TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);

  Language *language = rb_tree_language_(tree_cursor->rb_tree);
  TSFieldId field_id = ts_tree_cursor_current_field_id(&tree_cursor->ts_tree_cursor);
  return ID2SYM(language_field2id(language, field_id));
}

static VALUE
rb_tree_cursor_copy(VALUE self)
{
  TreeCursor* tree_cursor;
  TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);

  TreeCursor *clone = RB_ZALLOC(TreeCursor);
  clone->rb_tree = tree_cursor->rb_tree;
  clone->ts_tree_cursor = ts_tree_cursor_copy(&tree_cursor->ts_tree_cursor);
  return TypedData_Wrap_Struct(rb_cTreeCursor, &tree_cursor_type, clone);
}

static VALUE
rb_tree_path_get_node(TreePath *tree_path, long index) {
  TreePathNode path_node = tree_path->nodes[index];
  /*TODO: add option to cache Ruby nodes */
  return rb_new_node_with_field(tree_path->rb_tree, path_node.ts_node, path_node.field_id);
}

static VALUE
raise_invalid_index_error(long index, long min, long max) {
    rb_raise(rb_eIndexError, "index %ld outside bounds: %ld...%ld", index, min, max);
}

static VALUE
rb_tree_path_aref_(VALUE self, VALUE rb_index, bool raise)
{
  TreePath* tree_path;
  TypedData_Get_Struct(self, TreePath, &tree_path_type, tree_path);

  long index = FIX2LONG(rb_index);
  if(index < 0) {
    index = tree_path->len + index;
  }
  if(index < 0 || index >= tree_path->len) {
    if(raise) {
      raise_invalid_index_error(index, 0, tree_path->len);
    }
    return Qnil;
  }
  return rb_tree_path_get_node(tree_path, index);
}

static VALUE
rb_tree_path_aref(VALUE self, VALUE rb_index)
{
  return rb_tree_path_aref_(self, rb_index, false);
}

static VALUE
rb_tree_path_at(VALUE self, VALUE rb_index) {
  return rb_tree_path_aref_(self, rb_index, false);
}

static VALUE
rb_tree_path_fetch(VALUE self, VALUE rb_index) {
  return rb_tree_path_aref_(self, rb_index, true);
}

#define INDEX_METHOD_SETUP_BEFORE \
  long before; \
  if(RB_NIL_P(rb_before)) { \
    before = tree_path->len; \
  } else { \
    before = FIX2LONG(rb_before); \
    if(before < 0 || before >= tree_path->len) { \
      raise_invalid_index_error(before, 0, tree_path->len); \
    } \
  } \
  if(before == 0) return -1;

#define INDEX_METHOD_SETUP_ARGS \
  int elem_type = TYPE(rb_elems); \
  long argc; \
  VALUE *argv; \
  if(elem_type == T_ARRAY) { \
    argc = RARRAY_LEN(rb_elems); \
    argv = RARRAY_PTR(rb_elems);\
  } else if(elem_type == T_SYMBOL) {\
    argc = 1;\
    argv = &rb_elems;\
  } else { \
    rb_raise(rb_eArgError, "must pass symbol or array of symbols");\
    return -1; \
  }


static long
tree_path_rindex_by_type(TreePath *tree_path, VALUE rb_elems, VALUE rb_before) {
  INDEX_METHOD_SETUP_BEFORE
  Language *language = rb_tree_language_(tree_path->rb_tree);
  INDEX_METHOD_SETUP_ARGS

  for(long i = before - 1; i >= 0; i--) {
    TreePathNode path_node = tree_path->nodes[i];
    for(long j = 0; j < argc; j++) {
      ID id = SYM2ID(argv[j]);
      if(language_symbol2id(language, ts_node_symbol(path_node.ts_node)) == id) {
        return i;
      }
    }
  }
  return -1;
}

static long
tree_path_rindex_by_field(TreePath *tree_path, VALUE rb_elems, VALUE rb_before) {
  INDEX_METHOD_SETUP_BEFORE
  Language *language = rb_tree_language_(tree_path->rb_tree);
  INDEX_METHOD_SETUP_ARGS

  for(long i = before - 1; i >= 0; i--) {
    TreePathNode path_node = tree_path->nodes[i];
    for(long j = 0; j < argc; j++) {
      ID id = SYM2ID(argv[j]);
      if(language_field2id(language, path_node.field_id) == id) {
        return i;
      }
    }
  }
  return -1;
}


static VALUE
rb_tree_path_find_by_type(VALUE self, VALUE rb_elems, VALUE rb_before, VALUE rb_return_index) {
  TreePath* tree_path;
  TypedData_Get_Struct(self, TreePath, &tree_path_type, tree_path);

  long index = tree_path_rindex_by_type(tree_path, rb_elems, rb_before);
  if(index < 0) {
    return Qnil;
  } else {
    VALUE rb_node = rb_tree_path_get_node(tree_path, index);
    rb_p(rb_node);
    if(RB_TEST(rb_return_index)) {
      return rb_assoc_new(rb_node, LONG2FIX(index));
    } else {
      return rb_node;
    }
  }
}


static VALUE
rb_tree_path_rindex_by_field(VALUE self, VALUE rb_elems, VALUE rb_before)
{
  TreePath* tree_path;
  TypedData_Get_Struct(self, TreePath, &tree_path_type, tree_path);

  long index = tree_path_rindex_by_field(tree_path, rb_elems, rb_before);
  if(index < 0) {
    return Qnil;
  } else {
    return LONG2FIX(index);
  }
}

static VALUE
rb_tree_path_rindex_by_type(VALUE self, VALUE rb_elems, VALUE rb_before)
{
  TreePath* tree_path;
  TypedData_Get_Struct(self, TreePath, &tree_path_type, tree_path);

  long index = tree_path_rindex_by_type(tree_path, rb_elems, rb_before);
  if(index < 0) {
    return Qnil;
  } else {
    return LONG2FIX(index);
  }
}


static VALUE
rb_tree_path_size(VALUE self)
{
  TreePath* tree_path;
  TypedData_Get_Struct(self, TreePath, &tree_path_type, tree_path);

  return LONG2FIX((long) tree_path->len);
}

static VALUE
rb_tree_path_first(VALUE self)
{
  TreePath* tree_path;
  TypedData_Get_Struct(self, TreePath, &tree_path_type, tree_path);

  return rb_tree_path_get_node(tree_path, 0);
}

static VALUE
rb_tree_path_last(VALUE self)
{
  TreePath* tree_path;
  TypedData_Get_Struct(self, TreePath, &tree_path_type, tree_path);

  if(tree_path->len == 0) {
    return Qnil;
  }

  return rb_tree_path_get_node(tree_path, tree_path->len - 1);
}

static VALUE
tree_path_enum_length(VALUE rb_tree_path, VALUE args, VALUE eobj)
{
  TreePath* tree_path;
  TypedData_Get_Struct(rb_tree_path, TreePath, &tree_path_type, tree_path);
  return UINT2NUM(tree_path->len);
}


static VALUE
rb_tree_path_each(VALUE self)
{
  TreePath* tree_path;
  TypedData_Get_Struct(self, TreePath, &tree_path_type, tree_path);
  RETURN_SIZED_ENUMERATOR(self, 0, 0, tree_path_enum_length);

  for(uint32_t i = 0; i < tree_path->len; i++) {
    VALUE rb_node = rb_tree_path_get_node(tree_path, i);
    rb_yield(rb_node);
  }
  return self;
}

void
init_tree()
{
  id_types = rb_intern("types");
  id_attach = rb_intern("attach");
  id_whitespace = rb_intern("whitespace");
  id___language__ = rb_intern("@__language__");
  id_error = rb_intern("error");

  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");
  rb_cTree = rb_define_class_under(rb_mTreeSitter, "Tree", rb_cObject);
  rb_define_alloc_func(rb_cTree, rb_tree_alloc);
  rb_define_method(rb_cTree, "initialize", rb_tree_initialize, -1);
  rb_define_method(rb_cTree, "attach", rb_tree_attach, 1);
  rb_define_method(rb_cTree, "detach", rb_tree_detach, 0);
  rb_define_method(rb_cTree, "root_node", rb_tree_root_node, 0);
  rb_define_method(rb_cTree, "language", rb_tree_language, 0);
  rb_define_singleton_method(rb_cTree, "language", rb_tree_language_s, 0);

  rb_define_method(rb_cTree, "__path_to__", rb_tree_path_to, 1);
  rb_define_method(rb_cTree, "__find_by_byte__", rb_tree_find_by_byte, 1);
  // rb_define_singleton_method(rb_cTree, "merge", rb_tree_merge, -1);
  rb_define_singleton_method(rb_cTree, "find_common_parent", rb_tree_find_common_parent, -1);

  rb_define_method(rb_cTree, "clone", rb_tree_copy, 0);
  rb_define_method(rb_cTree, "copy", rb_tree_copy, 0);

  VALUE rb_cTree_s = rb_singleton_class(rb_cTree);
  rb_define_alias(rb_cTree_s, "parse", "new");

  rb_cTreeCursor = rb_define_class_under(rb_cTree, "Cursor", rb_cObject);
  rb_define_alloc_func(rb_cTreeCursor, rb_tree_cursor_alloc);
  rb_define_method(rb_cTreeCursor, "initialize", rb_tree_cursor_initialize, 1);
  rb_define_method(
    rb_cTreeCursor, "goto_parent", rb_tree_cursor_goto_parent, 0);
  rb_define_method(
    rb_cTreeCursor, "goto_first_child", rb_tree_cursor_goto_first_child, 0);
  rb_define_method(
    rb_cTreeCursor, "goto_next_sibling", rb_tree_cursor_goto_next_sibling, 0);
  rb_define_method(rb_cTreeCursor,
                   "goto_first_child_for_byte",
                   rb_tree_cursor_goto_first_child_for_byte,
                   1);
  rb_define_method(
    rb_cTreeCursor, "current_field_name", rb_tree_cursor_current_field_name, 0);
  rb_define_method(
    rb_cTreeCursor, "current_field", rb_tree_cursor_current_field, 0);
  rb_define_method(
    rb_cTreeCursor, "current_node", rb_tree_cursor_current_node, 0);

  rb_define_method(
    rb_cTreeCursor, "clone", rb_tree_cursor_copy, 0);
  rb_define_method(
    rb_cTreeCursor, "copy", rb_tree_cursor_copy, 0);

  rb_cLanguage = rb_define_class_under(rb_mTreeSitter, "Language", rb_cObject);
  rb_undef_alloc_func(rb_cLanguage);

  rb_define_method(
    rb_cLanguage, "fields", rb_language_fields, 0);

  rb_define_method(
    rb_cLanguage, "symbols", rb_language_symbols, 0);

  rb_cTreePath = rb_define_class_under(rb_cTree, "Path", rb_cObject);
  rb_include_module(rb_cTreePath, rb_mEnumerable);
  rb_define_method(rb_cTreePath, "[]", rb_tree_path_aref, 1);
  rb_define_method(rb_cTreePath, "at", rb_tree_path_at, 1);
  rb_define_method(rb_cTreePath, "fetch", rb_tree_path_fetch, 1);
  rb_define_method(rb_cTreePath, "size", rb_tree_path_size, 0);
  rb_define_method(rb_cTreePath, "each", rb_tree_path_each, 0);
  rb_define_method(rb_cTreePath, "__rindex_by_field__", rb_tree_path_rindex_by_field, 2);
  rb_define_method(rb_cTreePath, "__rindex_by_type__", rb_tree_path_rindex_by_type, 2);
  rb_define_method(rb_cTreePath, "__find_by_type__", rb_tree_path_find_by_type, 3);
  rb_define_method(rb_cTreePath, "last", rb_tree_path_last, 0);
  rb_define_method(rb_cTreePath, "first", rb_tree_path_first, 0);

  VALUE rb_cQuery = rb_define_class_under(rb_cTree, "Query", rb_cObject);
  rb_define_singleton_method(rb_cQuery, "new", rb_query_new, 1);
  rb_undef_alloc_func(rb_cQuery);
  rb_define_method(rb_cQuery, "__run__", rb_query_run, 5);
}
