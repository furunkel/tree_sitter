#include "node.h"
#include "tree_sitter/api.h"

// #undef NDEBUG
// #include <assert.h>

VALUE rb_cNode;
VALUE rb_cPoint;

static ID id_type;
static ID id_byte_range;
static ID id_children;
static ID id_field;
static ID id_text;

static void node_free(void *n)
{
  xfree(n);
}

static void node_mark(void *n) {
  AstNode *node = (AstNode *) n;
  rb_gc_mark(node->rb_tree);
}

void point_free(void *p)
{
  xfree(p);
}

extern const rb_data_type_t tree_type;

const rb_data_type_t node_type = {
    .wrap_struct_name = "Node",
    .function = {
        .dmark = node_mark,
        .dfree = node_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static const rb_data_type_t point_type = {
    .wrap_struct_name = "Point",
    .function = {
        .dmark = NULL,
        .dfree = point_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static Tree *
node_get_tree(AstNode *node) {
  return (Tree *) DATA_PTR(node->rb_tree);
}

VALUE
rb_new_node(VALUE rb_tree, TSNode ts_node)
{
  AstNode *node = RB_ZALLOC(AstNode);
  node->ts_node = ts_node;
  node->rb_tree = rb_tree;
  return TypedData_Wrap_Struct(rb_cNode, &node_type, node);
}

VALUE
rb_new_node_with_field(VALUE rb_tree, TSNode ts_node, TSFieldId field_id) {
  AstNode *node = RB_ZALLOC(AstNode);
  node->ts_node = ts_node;
  node->rb_tree = rb_tree;
  node->cached_field = field_id;
  return TypedData_Wrap_Struct(rb_cNode, &node_type, node);
}

/*
 * Public: Render the node and its children as a string.
 *
 * Returns a {String}.
 */
static VALUE
rb_node_to_s(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  return rb_str_new_cstr(ts_node_string(node->ts_node));
}

/*
 * Public: The node type.
 *
 * Returns a {String}.
 */
static VALUE
rb_node_type(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Language *language = rb_tree_language_(node->rb_tree);
  VALUE rb_type = RB_ID2SYM(language_symbol2id(language, ts_node_symbol(node->ts_node)));
  return rb_type;

  // return rb_str_new_cstr(ts_node_type(node->ts_node));
}

static VALUE
rb_node_type_p(int argc, VALUE *argv, VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  Language *language = rb_tree_language_(node->rb_tree);

  for(int i = 0; i < argc; i++) {
    if(RB_TYPE_P(argv[i], T_SYMBOL)) {
      ID id = SYM2ID(argv[i]);
      ID node_id = language_symbol2id(language, ts_node_symbol(node->ts_node));
      if(id == node_id) return Qtrue;
    }
  }

  return Qfalse;
}

static VALUE
rb_node_tree(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  return node->rb_tree;
}

static VALUE
rb_new_point(TSPoint ts_point) {
  Point *point = RB_ALLOC(Point);
  point->ts_point = ts_point;
  return TypedData_Wrap_Struct(rb_cPoint, &point_type, point);
}

/*
 * Public: Get the starting position for a node.
 *
 * Returns a {Point}.
 */
static VALUE
rb_node_start_point(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  TSPoint start = ts_node_start_point(node->ts_node);
  return rb_new_point(start);
}

/*
 * Public: Get the ending position for a node.
 *
 * Returns a {Point}.
 */
static VALUE
rb_node_end_point(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  TSPoint end = ts_node_end_point(node->ts_node);
  return rb_new_point(end);
}

/*
 * Public: Does the node have a name?
 *
 * Returns a {Boolean}.
 */
static VALUE
rb_node_is_named(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  return ts_node_is_named(node->ts_node) ? Qtrue : Qfalse;
}

/*
 * Public: The number of named and unnamed children.
 *
 * Returns an {Integer}.
 */
static VALUE
rb_node_child_count(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  return UINT2NUM(ts_node_child_count(node->ts_node));
}

/*
 * Public: The number of named children.
 *
 * Returns an {Integer}.
 */
static VALUE
rb_node_named_child_count(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  return UINT2NUM(ts_node_named_child_count(node->ts_node));
}

/*
 * Public: Return the first child.
 *
 * Returns a {Node} or nil.
 */
static VALUE
rb_node_first_child(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  TSNode child = ts_node_child(node->ts_node, 0);

  if (ts_node_is_null(child)) {
    return Qnil;
  } else {
    return rb_new_node(node->rb_tree, child);
  }
}

static VALUE
rb_node_parents(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  VALUE rb_ary = rb_ary_new_capa(3);
  const unsigned max_parents = 1024;
  TSNode parent = node->ts_node;
  for(unsigned i = 0; i < max_parents; i++) {
    parent = ts_node_parent(parent);
    if(ts_node_is_null(parent)) break;
    rb_ary_push(rb_ary, rb_new_node(node->rb_tree, parent));
  } 

  return rb_ary;
}

static VALUE
rb_node_parent(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  TSNode parent = ts_node_parent(node->ts_node);

  if (ts_node_is_null(parent)) {
    return Qnil;
  } else {
    return rb_new_node(node->rb_tree, parent);
  }
}

/*
 * Public: Return the first named child.
 *
 * Returns a {Node} or nil.
 */
static VALUE
rb_node_first_named_child(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  TSNode child = ts_node_named_child(node->ts_node, 0);

  if (ts_node_is_null(child)) {
    return Qnil;
  } else {
    return rb_new_node(node->rb_tree, child);
  }
}

/*
 * Public: Return the last child.
 *
 * Returns a {Node} or nil.
 */
static VALUE
rb_node_last_child(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  uint32_t child_count = ts_node_child_count(node->ts_node);
  if (child_count > 0) {
    TSNode child = ts_node_child(node->ts_node, child_count - 1);
    return rb_new_node(node->rb_tree, child);
  } else {
    return Qnil;
  }
}

/*
 * Public: Return the last named child.
 *
 * Returns a {Node} or nil.
 */
static VALUE
rb_node_last_named_child(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  uint32_t child_count = ts_node_named_child_count(node->ts_node);
  if (child_count > 0) {
    TSNode child = ts_node_named_child(node->ts_node, child_count - 1);
    return rb_new_node(node->rb_tree, child);
  } else {
    return Qnil;
  }
}

static bool
rb_node_child_at_(TSNode node, VALUE child_index, TSNode *child) {
  int64_t i = (int64_t) FIX2INT(child_index);
  int64_t child_count = (int64_t) ts_node_child_count(node);

  if(i < 0) {
    i += child_count;
    if(i < 0) {
      return false;
    }
  }
  if (i > child_count) {
    return false;
  } else {
    *child = ts_node_child(node, i);
    return !ts_node_is_null(*child);
  }
}

/*
 * Public: Return the child at the specified index.
 *
 * Returns a {Node} or nil.
 */
static VALUE
rb_node_child_at(VALUE self, VALUE child_index)
{
  Check_Type(child_index, T_FIXNUM);

  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  TSNode child;
  if(rb_node_child_at_(node->ts_node, child_index, &child)) {
    return rb_new_node(node->rb_tree, child);
  } else {
    return Qnil;
  }
}


static VALUE
rb_node_dig(int argc, VALUE *argv, VALUE self) {
  if(argc == 0) {
    return self;
  }

  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Language *language = rb_tree_language_(node->rb_tree);

  TSNode n = node->ts_node;

  for(int i = 0; i < argc; i++) {
    VALUE rb_index_or_field = argv[i];
    switch(rb_type(rb_index_or_field)) {
      case RUBY_T_SYMBOL: {
        st_data_t table_val;
        ID id = RB_SYM2ID(rb_index_or_field);
        if(st_lookup(language->ts_field_table, (st_data_t) id, &table_val)) {
          TSNode child = ts_node_child_by_field_id(n, (TSFieldId) table_val);
          if(ts_node_is_null(child)) {
            return Qnil;
          } else {
            n = child;
          }
        } else {
          return Qnil;  
        }
        break;
      }
      case RUBY_T_FIXNUM:
        TSNode child;
        if(rb_node_child_at_(n, rb_index_or_field, &child)) {
          n = child;
        } else {
          return Qnil;
        }
        break;
      default: {
        rb_raise(rb_eArgError, "expected integer or symbol");
        return Qnil;
      }
    }
  }
  return rb_new_node(node->rb_tree, n);
}

static VALUE
rb_node_has_ancestor_path(int argc, VALUE *argv, VALUE self) {

  if(argc == 0) {
    return Qtrue;
  }

  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Language *language = rb_tree_language_(node->rb_tree);

  VALUE rb_retval = Qfalse;
  Tree *tree = node_get_tree(node);

  TSTreeCursor tree_cursor = ts_tree_cursor_new(ts_tree_root_node(tree->ts_tree));
  // ts_tree_cursor_goto_first_child_for_byte(&tree_cursor,);

  for(int i = 0; i < argc; i++) {
    VALUE sym = argv[argc - i - 1];
    ID id = SYM2ID(sym);
    st_data_t table_val;
    bool found = false;

    if(st_lookup(language->ts_field_table, (st_data_t) id, &table_val)) {
      if(ts_tree_cursor_current_field_id(&tree_cursor) == (TSFieldId) table_val) {
        found = true;
      } else {
        goto done;
      }
    }

    if(!ts_tree_cursor_goto_parent(&tree_cursor)) {
      goto done;
    }

    if(!found && st_lookup(language->ts_symbol_table, (st_data_t) id, &table_val)) {
      TSNode parent = ts_tree_cursor_current_node(&tree_cursor);
      if(ts_node_symbol(parent) != (TSSymbol) table_val) {
        goto done;
      } 
    } else {
      goto done;
    }
}

done:
  ts_tree_cursor_delete(&tree_cursor);
  return rb_retval; 
}

static VALUE
rb_node_field(VALUE self) {
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Language *language = rb_tree_language_(node->rb_tree);

  if(node->cached_field != 0) {
    ID id = language_field2id(language, node->cached_field);
    return RB_ID2SYM(id);
  }

  TSNode parent_node = ts_node_parent(node->ts_node);
  if(ts_node_is_null(parent_node)) {
    return Qnil;
  }

  VALUE rb_retval = Qnil;

  TSTreeCursor cursor = ts_tree_cursor_new(parent_node);
  if(!ts_tree_cursor_goto_first_child(&cursor)) goto done;

  while(ts_tree_cursor_goto_next_sibling(&cursor)) {
    TSNode child_node = ts_tree_cursor_current_node(&cursor);
    if(node->ts_node.id == child_node.id) {
      TSFieldId field_id = ts_tree_cursor_current_field_id(&cursor);
      if(field_id != 0) {
        rb_retval = RB_ID2SYM(language_field2id(language, field_id));
        goto done;
      }
    }
  }

done:
  ts_tree_cursor_delete(&cursor);
  return rb_retval;  
}

static VALUE
rb_node_field_p(int argc, VALUE *argv, VALUE self) {
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Language *language = rb_tree_language_(node->rb_tree);

  if(node->cached_field != 0) {
    ID node_field_id = language_field2id(language, node->cached_field);
    for(int i = 0; i < argc; i++) {
      VALUE rb_field = argv[i];
      if(!RB_TYPE_P(rb_field, T_SYMBOL)) continue;

      ID field_id = SYM2ID(rb_field);
      if(node_field_id == field_id) return Qtrue;
    }
    return Qfalse;
  } else {
    TSNode parent_node = ts_node_parent(node->ts_node);
    if(ts_node_is_null(parent_node)) {
      return Qfalse;
    }

    for(int i = 0; i < argc; i++) {
      VALUE rb_field = argv[i];
      if(!RB_TYPE_P(rb_field, T_SYMBOL)) continue;

      ID field_id = SYM2ID(rb_field);
      st_data_t ts_field_id;
      if(st_lookup(language->ts_field_table, (st_data_t) field_id, &ts_field_id)) {
        TSNode child = ts_node_child_by_field_id(parent_node, ts_field_id);
        if(child.id == node->ts_node.id) return Qtrue;
      }
    }
    return Qfalse;
  }
}

static VALUE
rb_node_child_by_field(VALUE self, VALUE rb_field) {
  Check_Type(rb_field, T_SYMBOL);

  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Language *language = rb_tree_language_(node->rb_tree);

  ID id = SYM2ID(rb_field);
  st_data_t field_id;
  if(st_lookup(language->ts_field_table, (st_data_t) id, &field_id)) {
    TSNode child = ts_node_child_by_field_id(node->ts_node, field_id);
    if(ts_node_is_null(child)) {
      return Qnil;
    } else {
      return rb_new_node(node->rb_tree, child);
    }
  } else {
    return Qnil;
  }
}

/*
 * Public: Return the named child at the specified index.
 *
 * Returns a {Node} or nil.
 */
static VALUE
rb_node_named_child_at(VALUE self, VALUE child_index)
{
  Check_Type(child_index, T_FIXNUM);
  int64_t i = FIX2INT(child_index);

  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  int64_t child_count = ts_node_named_child_count(node->ts_node);

  if(i < 0) {
    i += child_count;
    if(i < 0) {
      return Qnil;
    }
  }

  if (i > child_count) {
    return Qnil;
  } else {
    TSNode child = ts_node_named_child(node->ts_node, i);
    if(ts_node_is_null(child)) {
      return Qnil;
    }
    return rb_new_node(node->rb_tree, child);
  }
}

/*
 * Public: Return named children.
 *
 * Returns a {Array<Node>} or nil.
 */
static VALUE
rb_node_named_children(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  uint32_t child_count = ts_node_named_child_count(node->ts_node);
  VALUE rb_ary = rb_ary_new_capa(child_count);

  for(uint32_t i = 0; i < child_count; i++) {
    rb_ary_push(rb_ary, rb_new_node(node->rb_tree, ts_node_named_child(node->ts_node, i)));
  }
  return rb_ary;
}

/*
 * Public: Return all children.
 *
 * Returns a {Array<Node>} or nil.
 */
static VALUE
rb_node_children(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  uint32_t child_count = ts_node_child_count(node->ts_node);
  VALUE rb_ary = rb_ary_new_capa(child_count);

  for(uint32_t i = 0; i < child_count; i++) {
    rb_ary_push(rb_ary, rb_new_node(node->rb_tree, ts_node_child(node->ts_node, i)));
  }
  return rb_ary;
}

static VALUE
node_enum_length(VALUE rb_node, VALUE args, VALUE eobj)
{
  AstNode *node;
  TypedData_Get_Struct(rb_node, AstNode, &node_type, node);
  uint32_t child_count = ts_node_child_count(node->ts_node);
  return UINT2NUM(child_count);
}

static VALUE
rb_node_each_child(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  RETURN_SIZED_ENUMERATOR(self, 0, 0, node_enum_length);

  uint32_t child_count = ts_node_child_count(node->ts_node);
  for(uint32_t i = 0; i < child_count; i++) {
    VALUE rb_child_node = rb_new_node(node->rb_tree, ts_node_child(node->ts_node, i));
    rb_yield(rb_child_node);
  }
  return self;
}

static VALUE
rb_node_each_child_with_field_name(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  RETURN_SIZED_ENUMERATOR(self, 0, 0, node_enum_length);

  uint32_t child_count = ts_node_child_count(node->ts_node);
  for(uint32_t i = 0; i < child_count; i++) {
    const char *field_name = ts_node_field_name_for_child(node->ts_node, i);
    VALUE rb_field_name = Qnil;

    if(field_name) {
      rb_field_name = rb_str_new_cstr(field_name);
    }

    VALUE rb_child_node = rb_new_node(node->rb_tree, ts_node_child(node->ts_node, i));
    rb_yield_values(2, rb_child_node, rb_field_name);
  }
  return self;
}


static VALUE
node_enum_named_length(VALUE rb_node, VALUE args, VALUE eobj)
{
  AstNode *node;
  TypedData_Get_Struct(rb_node, AstNode, &node_type, node);
  uint32_t child_count = ts_node_named_child_count(node->ts_node);
  return UINT2NUM(child_count);
}

static VALUE
rb_node_each_named_child(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  RETURN_SIZED_ENUMERATOR(self, 0, 0, node_enum_named_length);

  uint32_t child_count = ts_node_named_child_count(node->ts_node);
  for(uint32_t i = 0; i < child_count; i++) {
    VALUE rb_child_node = rb_new_node(node->rb_tree, ts_node_named_child(node->ts_node, i));
    rb_yield(rb_child_node);
  }
  return self;
}

/*
 * Public: Return the row for a point.
 *
 * Returns an {Integer}.
 */
static VALUE
rb_point_row(VALUE self)
{
  Point *point;
  TypedData_Get_Struct(self, Point, &point_type, point);
  return UINT2NUM(point->ts_point.row);
}

/*
 * Public: Return the column for a point.
 *
 * Returns an {Integer}.
 */
static VALUE
rb_point_column(VALUE self)
{
  Point *point;
  TypedData_Get_Struct(self, Point, &point_type, point);
  return UINT2NUM(point->ts_point.column);
}

static void
rb_tree_check_attached(Tree *tree) {
  if(NIL_P(tree->rb_input)) {
    rb_raise(rb_eTreeSitterError, "no input attached");
  }
}

static void
rb_node_check_input_range(uint32_t start_byte, uint32_t end_byte, size_t input_len) {
  if(start_byte >= input_len || end_byte > input_len) {
    rb_raise(rb_eRuntimeError, "text range exceeds input length (%d-%d > %zu)", start_byte, end_byte, input_len);
  }
}

VALUE rb_node_text_(TSNode ts_node, VALUE rb_input) {
  uint32_t start_byte = ts_node_start_byte(ts_node);
  uint32_t end_byte = ts_node_end_byte(ts_node);

  if(start_byte == end_byte) {
    return rb_str_new("", 0);
  }

  const char *input = RSTRING_PTR(rb_input);
  size_t input_len = RSTRING_LEN(rb_input);

  rb_node_check_input_range(start_byte, end_byte, input_len);
  return rb_str_new(input + start_byte, end_byte - start_byte);
}

static VALUE
rb_node_text(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Tree *tree;
  TypedData_Get_Struct(node->rb_tree, Tree, &tree_type, tree);

  rb_tree_check_attached(tree);
  VALUE rb_text = rb_node_text_(node->ts_node, tree->rb_input);

  return rb_text;
}

static VALUE
rb_node_text_p(int argc, VALUE *argv, VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Tree *tree;
  TypedData_Get_Struct(node->rb_tree, Tree, &tree_type, tree);

  rb_tree_check_attached(tree);
  VALUE rb_input = tree->rb_input;

  TSNode ts_node = node->ts_node;
  uint32_t start_byte = ts_node_start_byte(ts_node);
  uint32_t end_byte = ts_node_end_byte(ts_node);

  for(int i = 0; i < argc; i++) {
    VALUE rb_text = argv[i];
    if(rb_type(rb_text) == T_STRING) {
      size_t text_len = RSTRING_LEN(rb_text);

      if(end_byte - start_byte != text_len) continue;
      if(start_byte == end_byte && text_len == 0) return Qtrue;

      const char *input = RSTRING_PTR(rb_input);
      size_t input_len = RSTRING_LEN(rb_input);

      rb_node_check_input_range(start_byte, end_byte, input_len);
      if(!rb_memcmp(input + start_byte, RSTRING_PTR(rb_text), end_byte - start_byte)) {
        return Qtrue;
      }
    }
  }

  return Qfalse;
}

VALUE
rb_node_byte_range_(TSNode node) {
  uint32_t start_byte = ts_node_start_byte(node);
  uint32_t end_byte = ts_node_end_byte(node);
  return rb_range_new(INT2FIX(start_byte), INT2FIX(end_byte - 1), FALSE);
}

static VALUE
rb_node_byte_range(VALUE self) {
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  return rb_node_byte_range_(node->ts_node);
}

static VALUE
rb_node_start_byte(VALUE self) {
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  return INT2FIX(ts_node_start_byte(node->ts_node));
}

static VALUE
rb_node_end_byte(VALUE self) {
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  return INT2FIX(ts_node_end_byte(node->ts_node));
}


static VALUE
rb_node_descendant_of_type(VALUE self, VALUE rb_ancestor_type) {
  AstNode *node;
  st_data_t symbol;

  TypedData_Get_Struct(self, AstNode, &node_type, node);
  Language *language = rb_tree_language_(node->rb_tree);

  if(st_lookup(language->ts_symbol_table, (st_data_t) SYM2ID(rb_ancestor_type), &symbol)) {
    TSNode n = ts_node_parent(node->ts_node);
    while(!ts_node_is_null(n)) {
      if(ts_node_symbol(n) == (TSSymbol) symbol) {
        return Qtrue;
      }
      n = ts_node_parent(n);    
    }
    return Qfalse;
  } else {
    rb_raise(rb_eTreeSitterError, "invalid type");
    return Qnil;
  }
}

static VALUE
rb_node_eq(VALUE self, VALUE rb_other) {
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  if(!RB_TYPE_P(rb_other, RUBY_T_DATA)) {
    return Qfalse;
  }

  if(!rb_typeddata_is_kind_of(rb_other, &node_type)) {
    return Qfalse;
  }

  AstNode *other;
  TypedData_Get_Struct(rb_other, AstNode, &node_type, other);

  return (node->ts_node.id == other->ts_node.id) ? Qtrue: Qfalse;
}

static VALUE
rb_node_hash(VALUE self) {
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  return RB_ST2FIX((st_index_t) node->ts_node.id);
}

static VALUE
node_to_hash(TSTreeCursor *cursor, TSNode node, Tree* tree)
{
  VALUE rb_hash = rb_hash_new();

  const char* type = ts_node_type(node);
  VALUE rb_type;
  if (type != NULL) {
    rb_type = rb_str_new2(type);
  } else {
    rb_type = Qnil;
  }

  rb_hash_aset(rb_hash, RB_ID2SYM(id_type), rb_type);

  uint32_t named_child_count = ts_node_named_child_count(node);
  VALUE rb_children = Qnil;

  if (named_child_count == 0 && tree->rb_input != Qnil) {
    VALUE rb_text = rb_node_text_(node, tree->rb_input);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_text), rb_text);
  } else {
    VALUE rb_byte_range = rb_node_byte_range_(node);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_byte_range), rb_byte_range);
  }

  const char *field_name = ts_tree_cursor_current_field_name(cursor);
  if (field_name) {
    VALUE rb_field_name = rb_str_new_cstr(field_name);
    rb_hash_aset(rb_hash, RB_ID2SYM(id_field), rb_field_name);
  }

  if (named_child_count > 0) {
    rb_children = rb_ary_new_capa(named_child_count);
    if(ts_tree_cursor_goto_first_child(cursor)) {
      do {
        TSNode child_node = ts_tree_cursor_current_node(cursor);
        TSTreeCursor child_cursor = ts_tree_cursor_copy(cursor);
        if(ts_node_is_named(child_node)) {
          VALUE rb_child_hash = node_to_hash(&child_cursor, child_node, tree);
          rb_ary_push(rb_children, rb_child_hash);
        }
        ts_tree_cursor_delete(&child_cursor);
      } while(ts_tree_cursor_goto_next_sibling(cursor));
    }
  }

  rb_hash_aset(rb_hash, RB_ID2SYM(id_children), rb_children);

  return rb_hash;
}

static VALUE
rb_node_to_h(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);
  TSTreeCursor cursor = ts_tree_cursor_new(node->ts_node);

  Tree *tree = node_get_tree(node);
  VALUE rb_hash = node_to_hash(&cursor, node->ts_node, tree);

  ts_tree_cursor_delete(&cursor);
  return rb_hash;
}


void init_node()
{
  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");

  id_type = rb_intern("type");
  id_byte_range = rb_intern("byte_range");
  id_children = rb_intern("children");
  id_field = rb_intern("field");
  id_text = rb_intern("text");

  rb_cNode = rb_define_class_under(rb_mTreeSitter, "Node", rb_cObject);
  rb_define_method(rb_cNode, "to_s", rb_node_to_s, 0);
  rb_define_method(rb_cNode, "type", rb_node_type, 0);
  rb_define_method(rb_cNode, "tree", rb_node_tree, 0);
  rb_define_method(rb_cNode, "named?", rb_node_is_named, 0);
  rb_define_method(rb_cNode, "child_count", rb_node_child_count, 0);
  rb_define_method(rb_cNode, "named_child_count", rb_node_named_child_count, 0);
  rb_define_method(rb_cNode, "first_child", rb_node_first_child, 0);
  rb_define_method(rb_cNode, "parent", rb_node_parent, 0);
  rb_define_method(rb_cNode, "parents", rb_node_parents, 0);
  rb_define_method(rb_cNode, "first_named_child", rb_node_first_named_child, 0);
  rb_define_method(rb_cNode, "last_child", rb_node_last_child, 0);
  rb_define_method(rb_cNode, "last_named_child", rb_node_last_named_child, 0);
  rb_define_method(rb_cNode, "child_at", rb_node_child_at, 1);
  rb_define_method(rb_cNode, "dig", rb_node_dig, -1);
  rb_define_method(rb_cNode, "child_by_field", rb_node_child_by_field, 1);
  rb_define_method(rb_cNode, "field?", rb_node_field_p, -1);
  rb_define_method(rb_cNode, "field", rb_node_field, 0);
  rb_define_method(rb_cNode, "has_ancestor_path?", rb_node_has_ancestor_path, -1);
  rb_define_method(rb_cNode, "named_child_at", rb_node_named_child_at, 1);
  rb_define_method(rb_cNode, "start_position", rb_node_start_point, 0);
  rb_define_method(rb_cNode, "end_position", rb_node_end_point, 0);
  rb_define_method(rb_cNode, "children", rb_node_children, 0);
  rb_define_method(rb_cNode, "each_child", rb_node_each_child, 0);
  rb_define_method(rb_cNode, "each_child_with_field_name", rb_node_each_child_with_field_name, 0);
  rb_define_method(rb_cNode, "named_children", rb_node_named_children, 0);
  rb_define_method(rb_cNode, "each_named_child", rb_node_each_named_child, 0);
  rb_define_method(rb_cNode, "text", rb_node_text, 0);
  rb_define_method(rb_cNode, "byte_range", rb_node_byte_range, 0);
  rb_define_method(rb_cNode, "start_byte", rb_node_start_byte, 0);
  rb_define_method(rb_cNode, "end_byte", rb_node_end_byte, 0);
  rb_define_method(rb_cNode, "==", rb_node_eq, 1);
  rb_define_method(rb_cNode, "hash", rb_node_hash, 0);
  rb_define_method(rb_cNode, "eql?", rb_node_eq, 1);
  rb_define_method(rb_cNode, "descendant_of_type?", rb_node_descendant_of_type, 1);
  rb_define_private_method(rb_cNode, "__to_h__", rb_node_to_h, 0);
  rb_define_method(rb_cNode, "text?", rb_node_text_p, -1);
  rb_define_method(rb_cNode, "type?", rb_node_type_p, -1);

  rb_cPoint = rb_define_class_under(rb_cNode, "Point", rb_cObject);
  rb_define_method(rb_cPoint, "row", rb_point_row, 0);
  rb_define_method(rb_cPoint, "column", rb_point_column, 0);
}
