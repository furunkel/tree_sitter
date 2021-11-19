#include "tree.h"
#include "tree_sitter/api.h"

static VALUE rb_cTree;
static VALUE rb_cTreeCursor;
static ID id_type;
static ID id_types;
static ID id_byte_range;
static ID id_children;
static ID id_attach;
static ID id_field;
static ID id_text;
static ID id_whitespace;

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

typedef const TSLanguage* (*TSLanguageFunc)();

static const TSLanguage*
get_language_from_class(VALUE klass)
{
  ID language_id = rb_intern("@__language__");
  VALUE rb_language = rb_ivar_get(klass, language_id);
  if (NIL_P(rb_language)) {
    rb_raise(rb_eRuntimeError,
             "language missing, did you try to instantiate Tree directly?");
    return Qnil;
  }
  TSLanguageFunc language_func = (TSLanguageFunc)NUM2ULL(rb_language);
  return language_func();
}

/*
 * Public: Creates a new tree
 *
 */
VALUE
rb_tree_initialize(int argc, VALUE* argv, VALUE self)
{
  VALUE rb_input;
  VALUE rb_options;

  rb_scan_args(argc, argv, "1:", &rb_input, &rb_options);
  Check_Type(rb_input, T_STRING);

  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  VALUE rb_klass = rb_class_of(self);
  const TSLanguage* language = get_language_from_class(rb_klass);
  TSParser* parser = ts_parser_new();

  ts_parser_set_language(parser, language);
  TSTree* ts_tree = ts_parser_parse_string(
    parser, NULL, RSTRING_PTR(rb_input), RSTRING_LEN(rb_input));
  ts_parser_delete(parser);

  tree->ts_tree = ts_tree;

  VALUE rb_attach = Qfalse;
  if (!NIL_P(rb_options)) {
    rb_attach = rb_hash_aref(rb_options, RB_ID2SYM(id_attach));
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
VALUE
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

VALUE
rb_tree_to_h(VALUE self)
{
  Tree* tree;

  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  TSNode root_node = ts_tree_root_node(tree->ts_tree);
  return node_to_hash(root_node, tree, NULL);
}

static void
collect_leaf_nodes(TSNode node,
                   TSNode* leaf_ary,
                   size_t* index,
                   size_t* leaf_ary_size)
{
  uint32_t child_count = ts_node_child_count(node);
  if (child_count == 0) {
    if (*index >= *leaf_ary_size) {
      size_t new_leaf_ary_size = *leaf_ary_size * 2;
      leaf_ary = ruby_xrealloc2(leaf_ary,
                                sizeof(TSNode) * *leaf_ary_size,
                                sizeof(TSNode) * new_leaf_ary_size);

      // should raise an exception
      if (leaf_ary == NULL)
        return;
      *leaf_ary_size = new_leaf_ary_size;
    }

    leaf_ary[*index] = node;
    (*index)++;

  } else {
    for (uint32_t i = 0; i < child_count; i++) {
      TSNode child_node = ts_node_child(node, i);
      collect_leaf_nodes(child_node, leaf_ary, index, leaf_ary_size);
    }
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
}

static int
node_start_byte_cmp(const void* a, const void* b)
{
  TSNode* node_a = (TSNode*)a;
  TSNode* node_b = (TSNode*)b;

  uint32_t start_byte_a = ts_node_start_byte(*node_a);
  uint32_t start_byte_b = ts_node_start_byte(*node_b);

  return start_byte_a - start_byte_b;
}

static void
add_whitespace_token(uint32_t prev_end_byte,
                     uint32_t cur_start_byte,
                     const char* input,
                     size_t input_len,
                     VALUE rb_ary,
                     bool types)
{

  if (prev_end_byte < cur_start_byte && prev_end_byte > 0 &&
      cur_start_byte <= input_len) {
    bool all_whitespace = true;
    for (size_t j = prev_end_byte; j < cur_start_byte; j++) {
      if (!isspace(input[j])) {
        all_whitespace = false;
        break;
      }
    }
    if (all_whitespace) {
      VALUE rb_text = rb_usascii_str_new(input + prev_end_byte,
                                         cur_start_byte - prev_end_byte);
      if (!types) {
        rb_ary_push(rb_ary, rb_text);
      } else {
        VALUE rb_type = ID2SYM(id_whitespace);
        VALUE rb_pair = rb_assoc_new(rb_text, rb_type);
        rb_ary_push(rb_ary, rb_pair);
      }
    }
  }
}

VALUE
rb_tree_fringe_s(VALUE self,
                 VALUE rb_input,
                 VALUE rb_types,
                 VALUE rb_comments,
                 VALUE rb_whitespace)
{
  Check_Type(rb_input, T_STRING);

  const TSLanguage* language = get_language_from_class(self);
  TSParser* parser = ts_parser_new();
  ts_parser_set_language(parser, language);
  TSTree* ts_tree = ts_parser_parse_string(
    parser, NULL, RSTRING_PTR(rb_input), RSTRING_LEN(rb_input));
  TSNode root_node = ts_tree_root_node(ts_tree);

  VALUE rb_ary = rb_ary_new();

  bool types = RB_TEST(rb_types);
  bool comments = RB_TEST(rb_comments);
  bool whitespace = RB_TEST(rb_whitespace);

  size_t leaf_ary_size = 512;
  size_t index = 0;
  TSNode* leaf_ary = RB_ALLOC_N(TSNode, leaf_ary_size);

  char* input = RSTRING_PTR(rb_input);
  size_t input_len = RSTRING_LEN(rb_input);

  collect_leaf_nodes(root_node, leaf_ary, &index, &leaf_ary_size);

  qsort(leaf_ary, index, sizeof(TSNode), node_start_byte_cmp);

  fprintf(stderr, "%zu %zu\n", index, leaf_ary_size);

  for (size_t i = 0; i < index; i++) {
    TSNode node = leaf_ary[i];

    const char* type = NULL;
    if (types || !comments) {
      type = ts_node_type(node);

      // FIXME: do all languages use "comment" as type?
      if (!comments && !strcmp(type, "comment")) {
        continue;
      }
    }

    if (whitespace) {
      if (i > 0) {
        TSNode prev_node = leaf_ary[i - 1];
        uint32_t prev_end_byte = ts_node_end_byte(prev_node);
        uint32_t cur_start_byte = ts_node_start_byte(node);

        add_whitespace_token(
          prev_end_byte, cur_start_byte, input, input_len, rb_ary, types);
      }
    }

    VALUE rb_text = rb_node_text_(node, rb_input);
    if (!NIL_P(rb_text)) {
      if (!types) {
        rb_ary_push(rb_ary, rb_text);
      } else {
        VALUE rb_type = ID2SYM(rb_intern(type));
        VALUE rb_pair = rb_assoc_new(rb_text, rb_type);
        rb_ary_push(rb_ary, rb_pair);
      }
    }
  }

  if (whitespace && index > 0) {
    TSNode last_node = leaf_ary[index - 1];
    uint32_t last_end_byte = ts_node_end_byte(last_node);
    add_whitespace_token(last_end_byte, input_len, input, input_len, rb_ary, types);
  }

  xfree(leaf_ary);
  ts_parser_delete(parser);
  ts_tree_delete(ts_tree);

  return rb_ary;
}

// VALUE rb_tree_lex(int argc, VALUE *argv, VALUE self)
// {
//   Tree *tree;
//   TypedData_Get_Struct(self, Tree, &tree_type, tree);

//   if(tree->rb_input == Qnil) {
//     return Qnil;
//   }

//   VALUE rb_options;
//   rb_scan_args(argc, argv, ":", &rb_options);

//   VALUE rb_types = Qfalse;
//   if(!NIL_P(rb_options)) {
//     rb_types = rb_hash_aref(rb_options, ID2SYM(id_types));
//   }

//   TSNode root_node = ts_tree_root_node(tree->ts_tree);

//   VALUE rb_ary = rb_ary_new();
//   collect_leaf_nodes(root_node, tree->rb_input, rb_ary, RTEST(rb_types), true
//   /*TODO*/);

//   return rb_ary;
// }

VALUE
rb_tree_attach(VALUE self, VALUE rb_input)
{
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  Check_Type(rb_input, T_STRING);

  tree->rb_input = rb_input;
  return self;
}

VALUE
rb_tree_detach(VALUE self)
{
  Tree* tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  VALUE rb_input = tree->rb_input;
  tree->rb_input = Qnil;

  return rb_input;
}

VALUE
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
    return ret ? Qtrue : Qfalse;                                               \
  }

TREE_CURSOR_NAV_METHOD(goto_parent)
TREE_CURSOR_NAV_METHOD(goto_first_child)
TREE_CURSOR_NAV_METHOD(goto_next_sibling)

static VALUE
rb_tree_cursor_goto_first_child_for_byte(VALUE self, VALUE rb_goal_byte)
{
  TreeCursor* tree_cursor;
  TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);

  uint32_t goal_byte = (uint32_t)FIX2UINT(rb_goal_byte);
  uint64_t ret = ts_tree_cursor_goto_first_child_for_byte(
    &tree_cursor->ts_tree_cursor, goal_byte);
  return LL2NUM(ret);
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
rb_tree_cursor_current_field_id(VALUE self)
{
  TreeCursor* tree_cursor;
  TypedData_Get_Struct(self, TreeCursor, &tree_cursor_type, tree_cursor);

  return INT2NUM(ts_tree_cursor_current_field_id(&tree_cursor->ts_tree_cursor));
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

  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");

  rb_cTree = rb_define_class_under(rb_mTreeSitter, "Tree", rb_cObject);
  rb_define_alloc_func(rb_cTree, rb_tree_alloc);
  rb_define_method(rb_cTree, "initialize", rb_tree_initialize, -1);
  rb_define_method(rb_cTree, "attach", rb_tree_attach, 1);
  rb_define_method(rb_cTree, "detach", rb_tree_detach, 0);
  rb_define_method(rb_cTree, "root_node", rb_tree_root_node, 0);
  // rb_define_method(rb_cTree, "lex", rb_tree_lex, -1);

  rb_define_private_method(rb_cTree, "__to_h__", rb_tree_to_h, 0);

  VALUE rb_cTree_s = rb_singleton_class(rb_cTree);
  rb_define_private_method(rb_cTree_s, "__fringe__", rb_tree_fringe_s, 4);

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
    rb_cTreeCursor, "current_field_id", rb_tree_cursor_current_field_id, 0);
  rb_define_method(
    rb_cTreeCursor, "current_node", rb_tree_cursor_current_node, 0);

  rb_define_method(
    rb_cTreeCursor, "clone", rb_tree_cursor_copy, 0);
  rb_define_method(
    rb_cTreeCursor, "copy", rb_tree_cursor_copy, 0);
}
