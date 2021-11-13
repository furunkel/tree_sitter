#include "tree.h"
#include "tree_sitter/api.h"

static VALUE rb_cTree;
static ID id_type;
static ID id_byte_range;
static ID id_children;
static ID id_attach;
static ID id_field;
static ID id_text;

static void tree_free(void *obj) {
  Tree *tree = (Tree *) obj;
  ts_tree_delete(tree->ts_tree);
  xfree(obj);
}

static void tree_mark(void *obj) {
  Tree *tree = (Tree *) obj;
  rb_gc_mark(tree->rb_input);
}

const rb_data_type_t tree_type = {
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
VALUE rb_tree_initialize(int argc, VALUE *argv, VALUE self)
{
  VALUE rb_input;
  VALUE rb_options;

  rb_scan_args(argc, argv, "1:", &rb_input, &rb_options);
  Check_Type(rb_input, T_STRING);

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
  const TSLanguage *language = language_func();
  ts_parser_set_language(parser, language);
  TSTree *ts_tree = ts_parser_parse_string(parser, NULL, RSTRING_PTR(rb_input), RSTRING_LEN(rb_input));
  ts_parser_delete(parser);

  tree->ts_tree = ts_tree;

  VALUE rb_attach = Qfalse;
  if(!NIL_P(rb_options)) {
    rb_attach = rb_hash_aref(rb_options, RB_ID2SYM(id_attach));
  }

  if(RTEST(rb_attach)) {
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
VALUE rb_tree_root_node(VALUE self)
{
  Tree *tree;

  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  TSNode ts_node = ts_tree_root_node(tree->ts_tree);
  return rb_new_node(self, ts_node);
}

static VALUE
rb_node_byte_range_(TSNode node) {
  uint32_t start_byte = ts_node_start_byte(node);
  uint32_t end_byte = ts_node_end_byte(node);
  return rb_range_new(INT2FIX(start_byte), INT2FIX(end_byte - 1), FALSE);
}

static VALUE node_to_hash(TSNode node, Tree *tree, const char *field_name) {
  VALUE rb_hash = rb_hash_new();

  const char *type = ts_node_type(node);
  VALUE rb_type = rb_str_new2(type);
  uint32_t child_count = ts_node_child_count(node);
  VALUE rb_children = rb_ary_new_capa(child_count);

  for(uint32_t i = 0; i < child_count; i++) {
    TSNode child_node = ts_node_child(node, i);
    if(!ts_node_is_named(child_node)) continue;

    const char *child_field_name = ts_node_field_name_for_child(node, i);
    VALUE rb_child_hash = node_to_hash(child_node, tree, child_field_name);
    rb_ary_push(rb_children, rb_child_hash);
  }

  rb_hash_aset(rb_hash, RB_ID2SYM(id_type), rb_type);
  rb_hash_aset(rb_hash, RB_ID2SYM(id_children), rb_children);

  if(tree->rb_input != Qnil) {
    VALUE rb_text = rb_node_text_(node, tree->rb_input);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_text), rb_text);
  } else {
    VALUE rb_byte_range = rb_node_byte_range_(node);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_byte_range), rb_byte_range);
  }

  if(field_name) {
    VALUE rb_field_name = rb_str_new_cstr(field_name);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_field), rb_field_name);
  }

  return rb_hash;
}

VALUE rb_tree_to_h(VALUE self)
{
  Tree *tree;

  TypedData_Get_Struct(self, Tree, &tree_type, tree);
  TSNode root_node = ts_tree_root_node(tree->ts_tree);
  return node_to_hash(root_node, tree, NULL);
}

static void node_lex(TSNode node, Tree *tree, VALUE rb_ary) {
  uint32_t child_count = ts_node_child_count(node);
  if(child_count == 0) {
    VALUE rb_text = rb_node_text_(node, tree->rb_input);
    rb_ary_push(rb_ary, rb_text);
  } else {
    for(uint32_t i = 0; i < child_count; i++) {
      TSNode child_node = ts_node_child(node, i);
      node_lex(child_node, tree, rb_ary);
    }
  }
}


VALUE rb_tree_lex(VALUE self)
{
  Tree *tree;

  TypedData_Get_Struct(self, Tree, &tree_type, tree);

  if(tree->rb_input == Qnil) {
    return Qnil;
  }

  TSNode root_node = ts_tree_root_node(tree->ts_tree);

  VALUE rb_ary = rb_ary_new();
  node_lex(root_node, tree, rb_ary);

  return rb_ary;
}

void init_tree()
{

  id_type = rb_intern("type");
  id_byte_range = rb_intern("byte_range");
  id_children = rb_intern("children");
  id_attach = rb_intern("attach");
  id_field = rb_intern("field");
  id_text = rb_intern("text");

  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");

  rb_eTreeError = rb_define_class_under(rb_mTreeSitter, "TreeError", rb_eStandardError);

  rb_cTree = rb_define_class_under(rb_mTreeSitter, "Tree", rb_cObject);
  rb_define_alloc_func(rb_cTree, rb_tree_alloc);
  rb_define_method(rb_cTree, "initialize", rb_tree_initialize, -1);
  rb_define_method(rb_cTree, "root_node", rb_tree_root_node, 0);
  rb_define_method(rb_cTree, "to_h", rb_tree_to_h, 0);
  rb_define_method(rb_cTree, "lex", rb_tree_lex, 0);

  VALUE rb_cTree_ = rb_singleton_class(rb_cTree);
  rb_define_alias(rb_cTree_, "parse", "new");
}
