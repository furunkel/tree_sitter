#include "tree.h"
#include "tree_sitter/api.h"

static VALUE rb_cTree;
static VALUE rb_cTreeCursor;
static VALUE rb_cLanguage;

static ID id_type;
static ID id_types;
static ID id_byte_range;
static ID id_children;
static ID id_attach;
static ID id_field;
static ID id_text;
static ID id_whitespace;
ID id___language__;

#ifndef MAX
#define MAX(a,b) (((a)<(b))?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) (((a)>(b))?(b):(a))
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
language_free(void* obj)
{
  Language* language = (Language*)obj;
  st_free_table(language->ts_symbol_table);
  st_free_table(language->ts_field_table);
  xfree(language->ts_symbol2id);
  xfree(language->ts_field2id);
  xfree(obj);
}

#define TS_NODE_ARRAY_EMBED_LEN 64

typedef struct {
  TSNode *data;
  TSNode embed_data[TS_NODE_ARRAY_EMBED_LEN];
  uint32_t len;
  uint32_t capa;
} TSNodeArray;

static void
ts_node_array_init(TSNodeArray *array) {
  array->data = array->embed_data;
  array->len = 0;
  array->capa = TS_NODE_ARRAY_EMBED_LEN;
}

static void
ts_node_array_destroy(TSNodeArray *array) {
  if(array->data != array->embed_data) {
    xfree(array->data);
  }
}

static void
ts_node_array_clear(TSNodeArray *array) {
  array->len = 0;
}

static void
ts_node_array_push(TSNodeArray *array, TSNode node) {
  if(array->len == array->capa) {
    uint32_t new_capa = array->capa * 2;
    if(array->data == array->embed_data) {
      array->data = RB_ALLOC_N(TSNode, new_capa);
      MEMCPY(array->data, array->embed_data, TSNode, array->len);
    } else {
      RB_REALLOC_N(array->data, TSNode, new_capa);
    }
  } else {
    array->data[array->len++] = node;
  }
}


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

  language->ts_symbol2id = RB_ALLOC_N(ID, symbol_count);
  language->ts_field2id = RB_ALLOC_N(ID, field_count);

  for(uint32_t i = 0; i < symbol_count; i++) {
    const char *symbol_name = ts_language_symbol_name(ts_language, (TSSymbol) i);
    ID symbol_id = rb_intern(symbol_name);
    st_insert(language->ts_symbol_table, (st_data_t) symbol_id, i);
    language->ts_symbol2id[i] = symbol_id;
  }

  for(uint32_t i = 0; i < field_count; i++) {
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
rb_tree_alloc(VALUE self)
{
  Tree* tree = RB_ZALLOC(Tree);
  return TypedData_Wrap_Struct(self, &tree_type, tree);
}

static VALUE
rb_tree_cursor_alloc(VALUE self)
{
  TreeCursor* tree_cursor = RB_ZALLOC(TreeCursor);
  return TypedData_Wrap_Struct(self, &tree_cursor_type, tree_cursor);
}

static VALUE
rb_tree_language(VALUE self)
{
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  VALUE rb_klass = rb_class_of(self);
  VALUE rb_language = rb_ivar_get(rb_klass, id___language__);
  return rb_language;
}

Language *
rb_tree_language_(VALUE self) {
  VALUE rb_language = rb_tree_language(self);
  Language* language;
  TypedData_Get_Struct(rb_language, Language, &language_type, language);
  return language;
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
node_to_hash(TSNode node, Tree* tree, const char* field_name)
{
  VALUE rb_hash = rb_hash_new();

  const char* type = ts_node_type(node);
  VALUE rb_type;
  if (type != NULL) {
    rb_type = rb_str_new2(type);
  } else {
    rb_type = Qnil;
  }

  uint32_t child_count = ts_node_child_count(node);
  uint32_t named_child_count = ts_node_named_child_count(node);
  VALUE rb_children = Qnil;
  if (named_child_count > 0) {
    rb_children = rb_ary_new_capa(child_count);

    for (uint32_t i = 0; i < child_count; i++) {
      TSNode child_node = ts_node_child(node, i);
      if (!ts_node_is_named(child_node))
        continue;

      const char* child_field_name = ts_node_field_name_for_child(node, i);
      VALUE rb_child_hash = node_to_hash(child_node, tree, child_field_name);
      rb_ary_push(rb_children, rb_child_hash);
    }
  }

  rb_hash_aset(rb_hash, RB_ID2SYM(id_type), rb_type);
  rb_hash_aset(rb_hash, RB_ID2SYM(id_children), rb_children);

  if (named_child_count == 0 && tree->rb_input != Qnil) {
    VALUE rb_text = rb_node_text_(node, tree->rb_input);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_text), rb_text);
  } else {
    VALUE rb_byte_range = rb_node_byte_range_(node);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_byte_range), rb_byte_range);
  }

  if (field_name) {
    VALUE rb_field_name = rb_str_new_cstr(field_name);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_field), rb_field_name);
  }

  return rb_hash;
}

static VALUE
rb_tree_to_h(VALUE self)
{
  Tree* tree;

  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  TSNode root_node = ts_tree_root_node(tree->ts_tree);
  return node_to_hash(root_node, tree, NULL);
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

static bool
rb_tree_merge__(const TSNode *nodes, size_t len, TSNode *out_nodes, size_t *out_len) {
  TSNode *parents = ALLOCA_N(TSNode, len);
  bool change = false;
  TSNode **parent_ptrs = ALLOCA_N(TSNode *, len);

  for(size_t i = 0; i < len; i++) {
    // if(ts_node_is_named(nodes[i])) {
      parents[i] = ts_node_parent(nodes[i]);
    // } else {
    //   parents[i].id = 0;
    // }
    parent_ptrs[i] = &parents[i];
  }

  qsort(parent_ptrs, len, sizeof(TSNode *), node_id_cmp);

  for(size_t i = 0; i < len; i++) {
    TSNode *parent_ptr = parent_ptrs[i];
    size_t parent_index = parent_ptr - parents;

    /* unnamed child node - copy child */
    // if(parent_ptr->id == 0) {
    //   out_nodes[(*out_len)] = nodes[parent_index];
    //   (*out_len)++;
    // } else 
    {
      uint32_t j;
      for(j = 1; i + j < len && parent_ptrs[i + j]->id == parent_ptr->id; j++) {}

      //FIXME: use named child count?
      // uint32_t child_count = ts_node_named_child_count(*parent_ptr);
      uint32_t child_count = ts_node_child_count(*parent_ptr);

      if(child_count > 1 && j >= child_count) {
        out_nodes[(*out_len)] = *parent_ptr;
        (*out_len)++;
        i += j - 1;
        change = true;
      } else {
        out_nodes[(*out_len)] = nodes[parent_index];
        (*out_len)++;
      }
    }
  }

  return change;
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

static VALUE
rb_tree_merge_(int argc, VALUE *argv, VALUE self) {

  if(argc == 1) {
    return argv[0];
  } else if(argc == 0) {
    return Qnil;
  }

  // should fit on the stack?
  TSNode *nodes = ALLOCA_N(TSNode, argc);
  VALUE rb_tree = Qnil;
  Tree *tree = NULL;

  for(int i = 0; i < argc; i++) {
    VALUE rb_node = argv[i];
    AstNode *node;

    TypedData_Get_Struct(rb_node, AstNode, &node_type, node);
    rb_tree = node->rb_tree;
    Tree *tree_ = (Tree *) DATA_PTR(rb_tree);
    if(tree != NULL && tree != tree_) {
      rb_raise(rb_eTreeSitterError, "nodes belong to different trees");
      return Qnil;
    }
    tree = tree_;
    nodes[i] = node->ts_node;
  }

  size_t len = argc;
  TSNode *out_nodes = ALLOCA_N(TSNode, argc);

  int i = 0;
redo:
  size_t out_len = 0;
  bool change = rb_tree_merge__(nodes, len, out_nodes, &out_len);

  if(change && i++ < 20) {
    TSNode *tmp = nodes;
    nodes = out_nodes;
    out_nodes = tmp;
    len = out_len;
    goto redo;
  }

  VALUE rb_ary = rb_ary_new_capa(out_len);
  for(size_t i = 0; i < out_len; i++) {
    rb_ary_push(rb_ary, rb_new_node(rb_tree, out_nodes[i]));
  }
  return rb_ary;
}

static VALUE
rb_tree_merge(int argc, VALUE *argv, VALUE self) {
  if(argc == 1 && RB_TYPE_P(argv[0], T_ARRAY)) {
    return rb_tree_merge_(RARRAY_LEN(argv[0]), RARRAY_PTR(argv[0]), self);
  } else {
    return rb_tree_merge_(argc, argv, self);
  }
}

static TSNode
find_node_by_byte(TSNode node, uint32_t goal_byte, TSNodeArray *node_array) {
  int64_t ret;
  TSNode found_node;
  TSTreeCursor tree_cursor = ts_tree_cursor_new(node);

  if(node_array != NULL) {
    ts_node_array_push(node_array, node);
  }

  while((ret = ts_tree_cursor_goto_first_child_for_byte(&tree_cursor, goal_byte)) != -1) {
    if(node_array != NULL) {
      ts_node_array_push(node_array, ts_tree_cursor_current_node(&tree_cursor));
    }
  }
  found_node = ts_tree_cursor_current_node(&tree_cursor);
  ts_tree_cursor_delete(&tree_cursor);
  return found_node;
}

static VALUE
rb_tree_find_by_byte(VALUE self, VALUE rb_goal_byte, VALUE rb_parents) {
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  VALUE *rb_goal_bytes;
  size_t goal_bytes_len;
  bool include_parents = RTEST(rb_parents);

  if(RB_TYPE_P(rb_goal_byte, T_ARRAY)) {
    rb_goal_bytes = RARRAY_PTR(rb_goal_byte);
    goal_bytes_len = RARRAY_LEN(rb_goal_byte);
  } else {
    Check_Type(rb_goal_byte, T_FIXNUM);
    rb_goal_bytes = &rb_goal_byte;
    goal_bytes_len = 1;
  }

  TSNode prev_node;
  TSNode root_node = ts_tree_root_node(tree->ts_tree);

  // TSNode *found_nodes = ALLOCA_N(TSNode, goal_bytes_len);

  VALUE rb_retval = Qnil;
  if(goal_bytes_len > 1) {
    rb_retval = rb_ary_new_capa(goal_bytes_len);
  }

  VALUE rb_prev_node_or_path;
  TSNodeArray node_array;

  if(include_parents) {
    ts_node_array_init(&node_array);
  }

  for(size_t i = 0; i < goal_bytes_len; i++) {
    uint32_t goal_byte = (uint32_t) FIX2UINT(rb_goal_bytes[i]);

    if(i > 0) {
      uint32_t prev_start_byte = ts_node_start_byte(prev_node);
      uint32_t prev_end_byte = ts_node_end_byte(prev_node);

      // we only return leaf nodes, so this must be the same node or path
      if(goal_byte >= prev_start_byte && goal_byte < prev_end_byte) {
        // for now let's not output duplicates
        // rb_ary_push(rb_retval, rb_prev_node_or_path);
        continue;
      }
    }

    ts_node_array_clear(&node_array);
    TSNode node = find_node_by_byte(root_node, goal_byte, include_parents ? &node_array : NULL);
    VALUE rb_node_or_path;

    if(include_parents) {
      VALUE rb_path = rb_ary_new_capa(node_array.len);
      for(size_t i = 0; i < node_array.len; i++) {
        rb_ary_push(rb_path, rb_new_node(self, node_array.data[i]));
      }
      rb_node_or_path = rb_path;
    } else {
      VALUE rb_node = rb_new_node(self, node);
      rb_ary_push(rb_retval, rb_node);
      rb_node_or_path = rb_node;
    }

    if(goal_bytes_len == 1) {
      rb_retval = rb_node_or_path;
    } else {
      rb_ary_push(rb_retval, rb_node_or_path);
      rb_prev_node_or_path = rb_node_or_path;
      prev_node = node;
    }
  }

  if(include_parents) {
    ts_node_array_destroy(&node_array);
  }

  return rb_retval;
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
  return ID2SYM(language->ts_field2id[field_id]);
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

void
init_tree()
{

  id_type = rb_intern("type");
  id_types = rb_intern("types");
  id_byte_range = rb_intern("byte_range");
  id_children = rb_intern("children");
  id_attach = rb_intern("attach");
  id_field = rb_intern("field");
  id_text = rb_intern("text");
  id_whitespace = rb_intern("whitespace");
  id___language__ = rb_intern("@__language__");

  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");

  rb_cTree = rb_define_class_under(rb_mTreeSitter, "Tree", rb_cObject);
  rb_define_alloc_func(rb_cTree, rb_tree_alloc);
  rb_define_method(rb_cTree, "initialize", rb_tree_initialize, -1);
  rb_define_method(rb_cTree, "attach", rb_tree_attach, 1);
  rb_define_method(rb_cTree, "detach", rb_tree_detach, 0);
  rb_define_method(rb_cTree, "root_node", rb_tree_root_node, 0);
  rb_define_method(rb_cTree, "language", rb_tree_language, 0);

  rb_define_method(rb_cTree, "__find_by_byte__", rb_tree_find_by_byte, 2);
  rb_define_private_method(rb_cTree, "__to_h__", rb_tree_to_h, 0);
  rb_define_singleton_method(rb_cTree, "merge", rb_tree_merge, -1);
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
}
