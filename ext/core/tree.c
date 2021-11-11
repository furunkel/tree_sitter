#include "tree.h"
#include "tree_sitter/api.h"

static VALUE rb_cTree;

static void tree_free(void *obj) {
  Tree *tree = (Tree *) obj;
  ts_tree_delete(tree->ts_tree);
  xfree(obj);
}

static const rb_data_type_t tree_type = {
    .wrap_struct_name = "tree",
    .function = {
        .dmark = NULL,
        .dfree = tree_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

/*
 * Internal: Allocate a new tree
 *
 */
static VALUE rb_tree_alloc(VALUE self)
{
  Tree *tree = RB_ZALLOC(Tree);
  return TypedData_Wrap_Struct(self, &tree_type, tree);
}

typedef const TSLanguage * (*TSLanguageFunc)();

/*
 * Public: Creates a new tree
 *
 */
VALUE rb_tree_initialize(VALUE self, VALUE rb_input_string)
{
  Check_Type(rb_input_string, T_STRING);

  Tree *tree;
  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_language = rb_ivar_get(self, language_id);
  if(NIL_P(rb_language)) {
    rb_raise(rb_eRuntimeError, "language missing, did you try to instantiate Tree directly?");
    return Qnil;
  }
  TSLanguageFunc language_func = (TSLanguageFunc) NUM2ULL(rb_language);

  TSParser *parser = ts_parser_new();
  TSLanguage *language = language_func();
  ts_parser_set_language(parser, language);
  TSTree *ts_tree = ts_parser_parse_string(parser, NULL, RSTRING_PTR(rb_input_string), RSTRING_LEN(rb_input_string));
  ts_parser_delete(parser);

  tree->ts_tree = ts_tree;
  return self;
}

/*
 * Public: Returns the tree root node.
 *
 * Returns a {Node}.
 */
VALUE rb_tree_root_node(VALUE self)
{
  Tree *tree;

  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  TSNode ts_node = ts_tree_root_node(tree->ts_tree);

  return rb_new_node(ts_node);
}

static VALUE node_to_hash(TSNode node) {

  VALUE rb_hash = rb_hash_new();

  const char *type = ts_node_type(node);
  VALUE rb_key = rb_str_new2(type);

  uint32_t child_count = ts_node_named_child_count(node);
  VALUE rb_children = rb_ary_new2(child_count);
  for(uint32_t i = 0; i < child_count; i++) {
    TSNode child_node = ts_node_named_child(node, i);
    VALUE rb_child_hash = node_to_hash(child_node);
    rb_ary_push(rb_children, rb_child_hash);
  }

  rb_hash_aset(rb_hash, rb_key, rb_children);
  return rb_hash;
}

VALUE rb_tree_to_h(VALUE self)
{
  Tree *tree;

  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  TSNode root_node = ts_tree_root_node(tree->ts_tree);
  return node_to_hash(root_node);
}

void init_tree()
{
  VALUE tree_sitter = rb_define_module("TreeSitter");

  rb_eTreeError = rb_define_class_under(tree_sitter, "TreeError", rb_eStandardError);

  rb_cTree = rb_define_class_under(tree_sitter, "Tree", rb_cObject);
  rb_define_alloc_func(rb_cTree, rb_tree_alloc);
  rb_define_method(rb_cTree, "initialize", rb_tree_initialize, 1);
  rb_define_method(rb_cTree, "root_node", rb_tree_root_node, 0);
  rb_define_method(rb_cTree, "to_h", rb_tree_to_h, 0);
}
