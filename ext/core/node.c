#include "node.h"

// #undef NDEBUG
// #include <assert.h>

VALUE rb_cNode;
VALUE rb_cPoint;

static ID id_eq;
static ID id_add;
static ID id_del;

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


VALUE rb_new_node(VALUE rb_tree, TSNode ts_node)
{
  AstNode *node = RB_ZALLOC(AstNode);
  node->ts_node = ts_node;
  node->rb_tree = rb_tree;
  return TypedData_Wrap_Struct(rb_cNode, &node_type, node);
}

/*
 * Public: Render the node and its children as a string.
 *
 * Returns a {String}.
 */
VALUE rb_node_to_s(VALUE self)
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
VALUE rb_node_type(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  return rb_str_new_cstr(ts_node_type(node->ts_node));
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
VALUE rb_node_start_point(VALUE self)
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
VALUE rb_node_end_point(VALUE self)
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
VALUE rb_node_is_named(VALUE self)
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
VALUE rb_node_child_count(VALUE self)
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
VALUE rb_node_named_child_count(VALUE self)
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
VALUE rb_node_first_child(VALUE self)
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

VALUE rb_node_parents(VALUE self)
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

VALUE rb_node_parent(VALUE self)
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
VALUE rb_node_first_named_child(VALUE self)
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
VALUE rb_node_last_child(VALUE self)
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
VALUE rb_node_last_named_child(VALUE self)
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

/*
 * Public: Return the child at the specified index.
 *
 * Returns a {Node} or nil.
 */
VALUE rb_node_child_at(VALUE self, VALUE child_index)
{
  Check_Type(child_index, T_FIXNUM);
  uint32_t i = NUM2UINT(child_index);

  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  uint32_t child_count = ts_node_child_count(node->ts_node);

  if (i > child_count) {
    return Qnil;
  } else {
    TSNode child = ts_node_child(node->ts_node, i);
    return rb_new_node(node->rb_tree, child);
  }
}

/*
 * Public: Return the named child at the specified index.
 *
 * Returns a {Node} or nil.
 */
VALUE rb_node_named_child_at(VALUE self, VALUE child_index)
{
  Check_Type(child_index, T_FIXNUM);
  uint32_t i = NUM2UINT(child_index);

  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  uint32_t child_count = ts_node_named_child_count(node->ts_node);

  if (i > child_count) {
    return Qnil;
  } else {
    TSNode child = ts_node_named_child(node->ts_node, i);
    return rb_new_node(node->rb_tree, child);
  }
}

/*
 * Public: Return named children.
 *
 * Returns a {Array<Node>} or nil.
 */
VALUE rb_node_named_children(VALUE self)
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
VALUE rb_node_children(VALUE self)
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

VALUE rb_node_each_child(VALUE self)
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

VALUE rb_node_each_child_with_field_name(VALUE self)
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

VALUE rb_node_each_named_child(VALUE self)
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
VALUE rb_point_row(VALUE self)
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
VALUE rb_point_column(VALUE self)
{
  Point *point;
  TypedData_Get_Struct(self, Point, &point_type, point);
  return UINT2NUM(point->ts_point.column);
}

VALUE rb_node_text_(TSNode ts_node, VALUE rb_input) {
  uint32_t start_byte = ts_node_start_byte(ts_node);
  uint32_t end_byte = ts_node_end_byte(ts_node);
  const char *input = RSTRING_PTR(rb_input);

  size_t input_len = RSTRING_LEN(rb_input);

  if(start_byte == end_byte) {
    return Qnil;
  }

  if(start_byte >= input_len || end_byte > input_len) {
    rb_raise(rb_eRuntimeError, "text range exceeds input length (%d-%d > %zu)", start_byte, end_byte, input_len);
    return Qnil;
  }
  return rb_str_new(input + start_byte, end_byte - start_byte);
}

VALUE rb_node_text(VALUE self)
{
  AstNode *node;
  TypedData_Get_Struct(self, AstNode, &node_type, node);

  Tree *tree;
  TypedData_Get_Struct(node->rb_tree, Tree, &tree_type, tree);

  VALUE rb_input = tree->rb_input;
  if(NIL_P(rb_input)) {
    rb_raise(rb_eTreeSitterError, "no input attached");
    return Qnil;
  }
  return rb_node_text_(node->ts_node, rb_input);
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

typedef struct {
  TSNode node;
  const char *input;
} TableEntry;

#define INDEX_LIST_ENTRY_CAPA 4
#ifndef MAX
#define MAX(a,b) (((a)<(b))?(b):(a))
#endif

typedef struct {
  uint32_t len;
  uint32_t next;
  uint32_t data[INDEX_LIST_ENTRY_CAPA];
} IndexListEntry;

typedef struct {
  uint32_t capa;
  uint32_t len;
  IndexListEntry *entries;
} IndexList;

// from st.c
#define FNV1_32A_INIT 0x811c9dc5


static st_index_t
table_entry_hash(st_data_t arg) {
  TableEntry *table_entry = (TableEntry *) arg;

  uint32_t start_byte = ts_node_start_byte(table_entry->node);
  uint32_t end_byte = ts_node_end_byte(table_entry->node);
  const char *str = table_entry->input + start_byte;
  size_t len = end_byte - start_byte;

  //return st_hash(str, len, FNV1_32A_INIT);
  return rb_memhash(str, len); // ^ len;
}

static int
table_entry_cmp(st_data_t x, st_data_t y) {
  TableEntry *table_entry_x = (TableEntry *) x;
  TableEntry *table_entry_y = (TableEntry *) y;

  uint32_t start_byte_x = ts_node_start_byte(table_entry_x->node);
  uint32_t end_byte_x = ts_node_end_byte(table_entry_x->node);

  uint32_t start_byte_y = ts_node_start_byte(table_entry_y->node);
  uint32_t end_byte_y = ts_node_end_byte(table_entry_y->node);

  uint32_t len_x = end_byte_x - start_byte_x;
  uint32_t len_y = end_byte_y - start_byte_y;

  if(len_x != len_y) {
    return 1;
  } else {
    const char *str_x = table_entry_x->input + start_byte_x;
    const char *str_y = table_entry_y->input + start_byte_y;
    return memcmp(str_x, str_y, len_x);
  }
}

static const struct st_hash_type type_table_entry_hash = {
    table_entry_cmp,
    table_entry_hash,
};

static IndexListEntry *
index_list_request_entry(IndexList *index_list) {
  if(!(index_list->len < index_list->capa)) {
    uint32_t new_capa = 2 * index_list->capa;
    RB_REALLOC_N(index_list->entries, IndexListEntry, new_capa);
    index_list->capa = new_capa;
  }
  IndexListEntry *entry = &index_list->entries[index_list->len];
  index_list->len++;
  return entry;
}


static IndexListEntry *
index_list_append(IndexList *index_list, IndexListEntry *entry, uint32_t value) {
  if(entry == NULL) {
      IndexListEntry *new_entry = index_list_request_entry(index_list);
      new_entry->next = UINT32_MAX;
      new_entry->len = 1;
      new_entry->data[0] = value;
      return new_entry;
  } else {
    if(!(entry->len < INDEX_LIST_ENTRY_CAPA)) {
      IndexListEntry *new_entry = index_list_request_entry(index_list);
      size_t entry_index = entry - index_list->entries;
      new_entry->next = entry_index;
      new_entry->len = 1;
      new_entry->data[0] = value;
      return new_entry;
    } else {
      entry->data[entry->len] = value;
      entry->len++;
      return entry;
    }
  }

}

static void
check_node_array(VALUE rb_ary, Tree **tree_out, const char **input_out) {
  size_t len = RARRAY_LEN(rb_ary);
  if(len > 0) {
    AstNode *first_node;
    TypedData_Get_Struct(RARRAY_AREF(rb_ary, 0), AstNode, &node_type, first_node);
    VALUE rb_first_tree = first_node->rb_tree;
    Tree *first_tree = (Tree *) DATA_PTR(rb_first_tree);

    for(size_t i = 1; i < len; i++) {
      AstNode *node;
      TypedData_Get_Struct(RARRAY_AREF(rb_ary, 0), AstNode, &node_type, node);
      VALUE rb_tree = node->rb_tree;
      Tree *tree = (Tree *) DATA_PTR(rb_tree);
      if(tree != first_tree) {
        rb_raise(rb_eArgError, "nodes from different trees");
        return;
      }
    }

    *tree_out = first_tree;
    VALUE rb_input = first_tree->rb_input;
    *input_out = RSTRING_PTR(rb_input);
  }
}

typedef struct {
  IndexList *index_list;
  uint32_t value;
} UpdateArg;

int update_callback(st_data_t *key, st_data_t *value, st_data_t arg, int existing) {
  UpdateArg *update_arg = (UpdateArg *) arg;
  IndexList *index_list = update_arg->index_list;
  uint32_t insert_value = update_arg->value;

  IndexListEntry *entry = existing ? (IndexListEntry *) (*value) : NULL;
  IndexListEntry *new_entry = index_list_append(index_list, entry, insert_value);
  *value = (st_data_t) (new_entry);
  return ST_CONTINUE;
}


static void
node_diff(VALUE rb_old, VALUE rb_new, IndexList *index_list, TableEntry *table_entries_old, st_table *overlap,
          st_table *_overlap, st_table *old_index_map, const char *input_old, const char *input_new,
          uint32_t start_old, uint32_t len_old, uint32_t start_new, uint32_t len_new, VALUE rb_out_ary) {

  if(len_old == 0 && len_new == 0) return;

  uint32_t sub_start_old = start_old;
  uint32_t sub_start_new = start_new;
  uint32_t sub_length = 0;

  for(size_t iold = start_old; iold < start_old + len_old; iold++) {
    VALUE rb_node = RARRAY_AREF(rb_old, iold);
    AstNode *node = (AstNode *) DATA_PTR(rb_node);

    TableEntry *key = &table_entries_old[iold];
    key->node = node->ts_node;
    key->input = input_old;

    UpdateArg arg = {
      .index_list = index_list,
      .value = iold,
    };

    st_update(old_index_map, (st_data_t) key, update_callback, (st_data_t) &arg);
  }

  if(true || len_old > 0) {
    for(size_t inew = start_new; inew < start_new + len_new; inew++) {
      VALUE rb_node = RARRAY_AREF(rb_new, inew);
      AstNode *node = (AstNode *) DATA_PTR(rb_node);

      assert(_overlap->num_entries == 0);

      TableEntry key = {
        .node = node->ts_node,
        .input = input_new
      };

      IndexListEntry *list_entry;

      if(st_lookup(old_index_map, (st_data_t)&key, (st_data_t *) &list_entry)) {
        IndexListEntry *iter = list_entry;
        while(true) {
          for(size_t i = 0; i < iter->len; i++) {
            uint32_t iold = iter->data[i];

            st_data_t prev_sub_len = 0;

            if(iold > start_old) {
              st_lookup(overlap, (st_data_t) (iold - 1), (st_data_t *) &prev_sub_len);
            }

            uint32_t new_sub_len = prev_sub_len + 1;
            if(new_sub_len > sub_length) {
                sub_length = new_sub_len;
                sub_start_old = iold - sub_length + 1;
                sub_start_new = inew - sub_length + 1;
            }
            assert(sub_length <= len_old);
            assert(sub_length <= len_new);
            assert(sub_start_old >= start_old);
            assert(sub_start_new >= start_new);
            st_insert(_overlap, (st_data_t) iold, new_sub_len);
          }

          if(iter->next == UINT32_MAX) {
            break;
          } else {
            iter = &index_list->entries[iter->next];
          }
        }
      }

      {
        st_table *tmp = overlap;
        overlap = _overlap;
        _overlap = tmp;
        st_clear(_overlap);
      }
    }
  }

  if(sub_length == 0) {
    if(len_old > 0) {
      rb_ary_push(rb_out_ary, rb_assoc_new(RB_ID2SYM(id_del), rb_ary_subseq(rb_old, start_old, len_old)));
    }

    if(len_new > 0) {
      rb_ary_push(rb_out_ary, rb_assoc_new(RB_ID2SYM(id_add), rb_ary_subseq(rb_new, start_new, len_new)));
    }
  } else {
    st_clear(_overlap);
    st_clear(overlap);
    st_clear(old_index_map);

    assert(sub_length <= len_old);
    assert(sub_length <= len_new);
    assert(sub_start_old >= start_old);
    assert(sub_start_new >= start_new);

    node_diff(rb_old, rb_new, index_list, table_entries_old, overlap, _overlap, old_index_map, input_old, input_new,
              start_old, sub_start_old - start_old,
              start_new, sub_start_new - start_new,
              rb_out_ary);


    {
      rb_ary_push(rb_out_ary, rb_assoc_new(RB_ID2SYM(id_eq), rb_ary_subseq(rb_new, sub_start_new, sub_length)));
    }

    st_clear(_overlap);
    st_clear(overlap);
    st_clear(old_index_map);

    node_diff(rb_old, rb_new, index_list, table_entries_old, overlap, _overlap, old_index_map, input_old, input_new,
              sub_start_old + sub_length, (start_old + len_old) - (sub_start_old + sub_length),
              sub_start_new + sub_length, (start_new + len_new) - (sub_start_new + sub_length),
              rb_out_ary);

  }
}

static VALUE
rb_node_diff_s(VALUE self, VALUE rb_old, VALUE rb_new) {
  size_t len_old = RARRAY_LEN(rb_old);
  size_t len_new = RARRAY_LEN(rb_new);

  const char *input_old = NULL;
  const char *input_new = NULL;

  Tree *tree_old = NULL;
  Tree *tree_new = NULL;

  check_node_array(rb_old, &tree_old, &input_old);
  check_node_array(rb_new, &tree_new, &input_new);

  // TableEntry *table_entries = RB_ALLOC_N(TableEntry, len_old + len_new);
  TableEntry *table_entries_old = RB_ALLOC_N(TableEntry, len_old);
  // TableEntry *table_entries_new = table_entries + len_old;

  IndexList index_list;
  index_list.capa = len_old;
  index_list.len = 0;
  index_list.entries = RB_ALLOC_N(IndexListEntry, index_list.capa);

  VALUE rb_out_ary = rb_ary_new_capa(MAX(len_old, len_new));

  st_table *overlap = st_init_numtable_with_size(64);
  st_table *_overlap = st_init_numtable_with_size(64);
  st_table *old_index_map = st_init_table_with_size(&type_table_entry_hash, 128);

  node_diff(rb_old, rb_new, &index_list, table_entries_old, overlap, _overlap, old_index_map,
            input_old, input_new, 0, len_old, 0, len_new, rb_out_ary);

  st_free_table(overlap);
  st_free_table(_overlap);
  st_free_table(old_index_map);
  xfree(table_entries_old);
  xfree(index_list.entries);

  return rb_out_ary;
}

void init_node()
{
  id_add = rb_intern("+");
  id_del = rb_intern("-");
  id_eq = rb_intern("=");

  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");

  rb_cNode = rb_define_class_under(rb_mTreeSitter, "Node", rb_cObject);
  rb_define_method(rb_cNode, "to_s", rb_node_to_s, 0);
  rb_define_method(rb_cNode, "type", rb_node_type, 0);
  rb_define_method(rb_cNode, "named?", rb_node_is_named, 0);
  rb_define_method(rb_cNode, "child_count", rb_node_child_count, 0);
  rb_define_method(rb_cNode, "named_child_count", rb_node_named_child_count, 0);
  rb_define_method(rb_cNode, "first_child", rb_node_first_child, 0);
  rb_define_method(rb_cNode, "parent", rb_node_parent, 0);
  rb_define_method(rb_cNode, "parents", rb_node_parents, 0);
  rb_define_method(rb_cNode, "first_named_child", rb_node_first_named_child, 0);
  rb_define_method(rb_cNode, "last_child", rb_node_last_named_child, 0);
  rb_define_method(rb_cNode, "last_named_child", rb_node_last_named_child, 0);
  rb_define_method(rb_cNode, "child_at", rb_node_child_at, 1);
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
  rb_define_method(rb_cNode, "==", rb_node_eq, 1);
  rb_define_method(rb_cNode, "hash", rb_node_hash, 0);
  rb_define_method(rb_cNode, "eql?", rb_node_eq, 1);

  rb_define_singleton_method(rb_cNode, "diff", rb_node_diff_s, 2);

  rb_cPoint = rb_define_class_under(rb_cNode, "Point", rb_cObject);
  rb_define_method(rb_cPoint, "row", rb_point_row, 0);
  rb_define_method(rb_cPoint, "column", rb_point_column, 0);
}
