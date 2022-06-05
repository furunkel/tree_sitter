#include "misc.h"

#undef NDEBUG
#include <assert.h>

extern const rb_data_type_t node_type;

static void
subtree_counter_entry_free(SubtreeCounterEntry *entry) {
    xfree(entry->text);
    xfree(entry);
}

static void
subtree_counter_free(void* obj)
{
  SubtreeCounter* subtree_counter = (SubtreeCounter*)obj;
  st_free_table(subtree_counter->id_map);
  for(size_t i = 0; i < subtree_counter->entries_len; i++) {
    subtree_counter_entry_free(subtree_counter->entries_ptrs[i]);
  }
  xfree(subtree_counter->entries_ptrs);
  xfree(obj);
}



static void
subtree_counter_mark(void* obj)
{
    //   SubtreeCounter* subtree_counter = (SubtreeCounter*)obj;
    // rb_gc_mark(subtree_counter)
}

static const rb_data_type_t subtree_counter_type = {
    .wrap_struct_name = "Tree",
    .function = {
        .dmark = subtree_counter_mark,
        .dfree = subtree_counter_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static st_index_t
subtree_counter_entry_hash(st_data_t arg) {
  SubtreeCounterEntry *entry = (SubtreeCounterEntry *) arg;

  st_index_t h = st_hash_start(entry->type);
  h = st_hash_uint32(h, entry->child_count);
  h = st_hash_uint32(h, entry->text_len);
  h = st_hash_uint(h, rb_memhash(entry->text, entry->text_len));
  for(uint16_t i = 0; i < entry->child_count; i++) {
    h = st_hash_uint32(h, entry->child_ids[i]);
    h = st_hash_uint32(h, entry->child_fields[i]);
  }
  //return st_hash(str, len, FNV1_32A_INIT);
  // return rb_memhash(node->text, node->text_len); // ^ len;
  h = st_hash_end(h);

  return h;
}

static int
subtree_counter_entry_cmp(st_data_t x, st_data_t y) {
  SubtreeCounterEntry *entry_x = (SubtreeCounterEntry *) x;
  SubtreeCounterEntry *entry_y = (SubtreeCounterEntry *) y;

  if(entry_x->type != entry_y->type) return 1;
  if(entry_x->text_len != entry_y->text_len) return 1;
  if(entry_x->child_count != entry_y->child_count) return 1;

  for(uint16_t i = 0; i < entry_x->child_count; i++) {
    if(entry_x->child_ids[i] != entry_y->child_ids[i]) return false;
    if(entry_x->child_fields[i] != entry_y->child_fields[i]) return false;
  }

  return memcmp(entry_x->text, entry_y->text, entry_x->text_len);
}

static const struct st_hash_type subtree_counter_key_hash = {
    subtree_counter_entry_cmp,
    subtree_counter_entry_hash,
};

static VALUE
rb_subtree_counter_alloc(VALUE self)
{
  SubtreeCounter* subtree_counter = RB_ZALLOC(SubtreeCounter);
  subtree_counter->entries_capa = SUBTREE_COUNTER_INIT_CAPA;
  subtree_counter->entries_ptrs = RB_ALLOC_N(SubtreeCounterEntry *, SUBTREE_COUNTER_INIT_CAPA);
  // subtree_counter->nodes = RB_ALLOC_N(SubtreeCounterEntry, SUBTREE_COUNTER_INIT_CAPA);

  subtree_counter->id_map = st_init_table(&subtree_counter_key_hash);

  return TypedData_Wrap_Struct(self, &subtree_counter_type, subtree_counter);
}

// static uint16_t
// add_node(SubtreeCounter *subtree_counter, SubtreeCounterEntry **out_node) {
//   if(subtree_counter->entries_len == subtree_counter->entries_capa) {
//     size_t new_capa = subtree_counter->entries_capa * 2;
//     RB_REALLOC_N(subtree_counter->nodes, SubtreeCounterEntry, new_capa);
//     subtree_counter->entries_capa = new_capa;
//   }

//   size_t id = subtree_counter->entries_len;
//   *out_node = &subtree_counter->nodes[id];
//   return (uint16_t) id;
// }

static uint32_t
add_entry_ptr(SubtreeCounter *subtree_counter, SubtreeCounterEntry *entry) {
  if(subtree_counter->entries_len == subtree_counter->entries_capa) {
    size_t new_capa = subtree_counter->entries_capa * 2;
    RB_REALLOC_N(subtree_counter->entries_ptrs, SubtreeCounterEntry *, new_capa);
    subtree_counter->entries_capa = new_capa;
  }

  uint32_t node_id = subtree_counter->entries_len;
  subtree_counter->entries_len++;
  subtree_counter->entries_ptrs[node_id] = entry;
  return node_id;
}

static void
copy_text(SubtreeCounterEntry *entry, TSNode ts_node, Tree *tree) {
  uint32_t start_byte = ts_node_start_byte(ts_node);
  uint32_t end_byte = ts_node_end_byte(ts_node);

  uint16_t text_len = end_byte - start_byte;
  entry->text_len = text_len;

  if(text_len == 0) {
    return;
  }

  const char *input = RSTRING_PTR(tree->rb_input);
  size_t input_len = RSTRING_LEN(tree->rb_input);

  assert(end_byte <= input_len);

  entry->text = ruby_xmalloc(text_len);
  memcpy(entry->text, input + start_byte, text_len);
}

typedef struct {
  SubtreeCounter *subtree_counter;
  SubtreeCounterEntry *key_entry;
  uint32_t key_entry_id;
} UpdateArg;

static int
update_callback(st_data_t *key, st_data_t *value, st_data_t arg, int existing) {
  UpdateArg *update_arg = (UpdateArg *) arg;
  SubtreeCounter *subtree_counter = update_arg->subtree_counter;
  SubtreeCounterEntry *key_entry = update_arg->key_entry;

  if(!existing) {
    // the key is allocated in nodes
    // advance the len, because we actually need this key

    uint32_t entry_id = add_entry_ptr(subtree_counter, key_entry);
    *value = (st_data_t) entry_id;
    // fprintf(stderr, "NO!!\n");
  } else {
    SubtreeCounterEntry *existing_entry = (SubtreeCounterEntry *) (*key);
    existing_entry->count++;

    // fprintf(stderr, "CONFLICT!!!! %d\n", existing_entry->count);
    // assert(existing_entry->count <= 1000);

    assert(existing_entry != key_entry);
    // we already have this node/key, throw this one away
    // since we do not update entries_len, the space for this node/key is overwritten/reused
    // in the next insert
    subtree_counter_entry_free(key_entry);
  }

  update_arg->key_entry_id = *value;

  return ST_CONTINUE;
}

static uint16_t
add_subtrees_(SubtreeCounter *subtree_counter, TSTreeCursor *cursor, TSNode node, Tree* tree)
{
  uint32_t named_child_count = ts_node_named_child_count(node);
  SubtreeCounterEntry *key_entry = RB_ALLOC(SubtreeCounterEntry);
  key_entry->child_count = 0;

  if(named_child_count > 0) {
    // fprintf(stderr, "NC: %u\n", named_child_count);
    // handle all children first
    if(ts_tree_cursor_goto_first_child(cursor)) {
      do {
        TSNode child_node = ts_tree_cursor_current_node(cursor);
        TSTreeCursor child_cursor = ts_tree_cursor_copy(cursor);

        if(ts_node_is_named(child_node)) {
          key_entry->child_ids[key_entry->child_count] = add_subtrees_(subtree_counter, &child_cursor, child_node, tree);
          key_entry->child_fields[key_entry->child_count] = ts_tree_cursor_current_field_id(cursor);
          key_entry->child_count++;
          if(key_entry->child_count >= SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
            fprintf(stderr, "ERROR: exceeded max child count\n");
            ts_tree_cursor_delete(&child_cursor);
            break;
          }
        }
        ts_tree_cursor_delete(&child_cursor);
      } while(ts_tree_cursor_goto_next_sibling(cursor));
    }
  }

  // uint32_t key_entry_id = add_node(subtree_counter, &key_entry);
  // uint32_t key_entry_id = subtree_counter->entries_len;
  key_entry->count = 0;
  key_entry->type = ts_node_symbol(node);

  if(named_child_count == 0) {
    //FIXME: create explicit list of which types to have text
    copy_text(key_entry, node, tree);
  } else {
    key_entry->text = NULL;
    key_entry->text_len = 0;
  }

  UpdateArg update_arg = {
    .key_entry = key_entry,
    .subtree_counter = subtree_counter
  };

  st_update(subtree_counter->id_map, (st_data_t) key_entry, update_callback, (st_data_t) &update_arg);

  return update_arg.key_entry_id;
}

static uint32_t
add_subtrees(SubtreeCounter *subtree_counter, AstNode *root) {

  TSTreeCursor cursor = ts_tree_cursor_new(root->ts_node);
  Tree *tree = node_get_tree(root);
  uint32_t root_id = add_subtrees_(subtree_counter, &cursor, root->ts_node, tree);

  ts_tree_cursor_delete(&cursor);

  return root_id;
}

static VALUE
rb_subtree_counter_add(VALUE self, VALUE rb_node) {
  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(self, SubtreeCounter, &subtree_counter_type, subtree_counter);

  AstNode *node;
  TypedData_Get_Struct(rb_node, AstNode, &node_type, node);

  uint32_t root_id = add_subtrees(subtree_counter, node);

  return RB_UINT2NUM(root_id);
}

void
init_misc() {
  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");
  VALUE rb_cSubTreeCounter = rb_define_class_under(rb_mTreeSitter, "SubtreeCounter", rb_cObject);
  rb_define_alloc_func(rb_cSubTreeCounter, rb_subtree_counter_alloc);

  rb_define_method(rb_cSubTreeCounter, "add", rb_subtree_counter_add, 1);
}