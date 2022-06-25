#include "misc.h"

#undef NDEBUG
#include <assert.h>

static VALUE rb_cSubtreeCounter;
static VALUE rb_cSubtreeCounterEntry;

extern const rb_data_type_t node_type;
extern const rb_data_type_t language_type;

static void
subtree_counter_entry_free(SubtreeCounterEntry *entry) {
    xfree(entry->text);
    xfree(entry->children);
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
  xfree(subtree_counter->types);
  xfree(obj);
}

static void
subtree_counter_mark(void* obj)
{
  SubtreeCounter* subtree_counter = (SubtreeCounter*)obj;
  rb_gc_mark(subtree_counter->rb_language);
}

static const rb_data_type_t subtree_counter_type = {
    .wrap_struct_name = "SubtreeCounter",
    .function = {
        .dmark = subtree_counter_mark,
        .dfree = subtree_counter_free,
        .dsize = NULL,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void
subtree_counter_entry_rb_free(void* obj)
{
  // we don't free, the counter will free this
  // we keep a reference to the entire counter, so the entry is guaranteed to be there
  // SubtreeCounterEntryRb* subtree_counter_entry = (SubtreeCounterEntryRb *)obj;
  // subtree_counter_entry_free(subtree_counter_entry->entry);
  xfree(obj);
}

static void
subtree_counter_entry_rb_mark(void* obj)
{
  SubtreeCounterEntryRb* subtree_counter_entry = (SubtreeCounterEntryRb *)obj;
  rb_gc_mark(subtree_counter_entry->rb_subtree_counter);
}

static const rb_data_type_t subtree_counter_entry_type = {
    .wrap_struct_name = "SubtreeCounter::Entry",
    .function = {
        .dmark = subtree_counter_entry_rb_mark,
        .dfree = subtree_counter_entry_rb_free,
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
    uint16_t field_id;
    uint64_t node_id;
    if(i < SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
      field_id = entry->child_fields[i];
      node_id = entry->child_ids[i];
    } else {
      field_id = entry->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].field;
      node_id = entry->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].id;
    }
    h = st_hash_uint32(h, node_id);
    if(field_id) {
      h = st_hash_uint32(h, field_id);
    }
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
    uint16_t field_id_x, field_id_y;
    uint64_t node_id_x, node_id_y;
    if(i < SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
      field_id_x = entry_x->child_fields[i];
      node_id_x = entry_x->child_ids[i];
      field_id_y = entry_y->child_fields[i];
      node_id_y = entry_y->child_ids[i];
    } else {
      field_id_x = entry_x->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].field;
      node_id_x = entry_x->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].id;
      field_id_y = entry_y->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].field;
      node_id_y = entry_y->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].id;
    }

    if(node_id_x != node_id_y) return false;
    if(field_id_x != field_id_y) return false;
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

static int
symbol_cmp(const void* a, const void* b)
{
  uint16_t sym_a = *(uint16_t *)a;
  uint16_t sym_b = *(uint16_t *)b;

  return sym_a - sym_b;
}

static bool
subtree_counter_use_type(SubtreeCounter *subtree_counter, uint16_t symbol) {
  return bsearch(&symbol, subtree_counter->types, subtree_counter->types_len, sizeof(uint16_t), symbol_cmp) != NULL;
}

static VALUE
rb_subtree_counter_initialize(VALUE self, VALUE rb_language, VALUE rb_types) {
  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(self, SubtreeCounter, &subtree_counter_type, subtree_counter);

  Language* language;
  TypedData_Get_Struct(rb_language, Language, &language_type, language);

  size_t types_len = RARRAY_LEN(rb_types);
  subtree_counter->types= RB_ALLOC_N(uint16_t, types_len);

  for(size_t i = 0; i < types_len; i++) {
    VALUE rb_symbol = RARRAY_AREF(rb_types, i);
    TSSymbol symbol;
    if(language_id2symbol(language, RB_SYM2ID(rb_symbol), &symbol)) {
      subtree_counter->types[i] = symbol;
    } else {
      rb_raise(rb_eArgError, "invalid symbol %"PRIsVALUE"", rb_symbol);
    }
  }

  qsort(subtree_counter->types, types_len, sizeof(uint16_t), symbol_cmp);

  subtree_counter->types_len = types_len;

  subtree_counter->rb_language = rb_language;
  return self;
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

static uint64_t
add_entry_ptr(SubtreeCounter *subtree_counter, SubtreeCounterEntry *entry) {
  if(subtree_counter->entries_len == subtree_counter->entries_capa) {
    size_t new_capa = subtree_counter->entries_capa * 2;
    RB_REALLOC_N(subtree_counter->entries_ptrs, SubtreeCounterEntry *, new_capa);
    subtree_counter->entries_capa = new_capa;
  }

  uint64_t node_id = subtree_counter->entries_len;
  subtree_counter->entries_len++;
  subtree_counter->entries_ptrs[node_id] = entry;
  return node_id;
}

static void
copy_text(SubtreeCounterEntry *entry, TSNode ts_node, Tree *tree) {
  uint32_t start_byte = ts_node_start_byte(ts_node);
  uint32_t end_byte = ts_node_end_byte(ts_node);

  uint32_t text_len = end_byte - start_byte;
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
  uint64_t key_entry_id;
} UpdateArg;

static int
update_callback(st_data_t *key, st_data_t *value, st_data_t arg, int existing) {
  UpdateArg *update_arg = (UpdateArg *) arg;
  SubtreeCounter *subtree_counter = update_arg->subtree_counter;
  SubtreeCounterEntry *key_entry = update_arg->key_entry;

  if(!existing) {
    // the key is allocated in nodes
    // advance the len, because we actually need this key

    uint64_t entry_id = add_entry_ptr(subtree_counter, key_entry);
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

static uint64_t
add_subtrees_(SubtreeCounter *subtree_counter, TSTreeCursor *cursor, TSNode node, Tree* tree, uint16_t *out_depth)
{
  uint32_t child_count = ts_node_child_count(node);
  SubtreeCounterEntry *key_entry = RB_ALLOC(SubtreeCounterEntry);
  key_entry->child_count = 0;
  key_entry->children = NULL;
  uint16_t node_type = ts_node_symbol(node);
  uint16_t max_child_depth = 0;

  if(child_count > 0) {
    // fprintf(stderr, "NC: %u\n", named_child_count);
    // handle all children first

    if(child_count > SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
      key_entry->children = RB_ALLOC_N(SubtreeCounterEntryChild, child_count - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN);
      // fprintf(stderr, "WARN: allocated %d extra children\n", child_count - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN);
    }

    if(ts_tree_cursor_goto_first_child(cursor)) {
      do {
        TSNode child_node = ts_tree_cursor_current_node(cursor);
        TSTreeCursor child_cursor = ts_tree_cursor_copy(cursor);
        uint16_t child_type = ts_node_symbol(child_node);

        if(subtree_counter_use_type(subtree_counter, child_type)) {
          uint16_t child_depth = 0;
          uint64_t child_id = add_subtrees_(subtree_counter, &child_cursor, child_node, tree, &child_depth);
          uint16_t field_id = ts_tree_cursor_current_field_id(cursor);


          assert(key_entry->child_count < child_count);

          if(key_entry->child_count < SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
            key_entry->child_ids[key_entry->child_count] = child_id;
            key_entry->child_fields[key_entry->child_count] = field_id;
          } else {
            key_entry->children[key_entry->child_count - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].id = child_id;
            key_entry->children[key_entry->child_count - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].field = field_id;
          }
          key_entry->child_count++;

          max_child_depth = MAX(max_child_depth, child_depth);
        } else {
          // fprintf(stderr, "filtered out type %s\n", ts_node_type(child_node));
        }
        ts_tree_cursor_delete(&child_cursor);
      } while(ts_tree_cursor_goto_next_sibling(cursor));
    }
  }

  // uint64_t key_entry_id = add_node(subtree_counter, &key_entry);
  // uint64_t key_entry_id = subtree_counter->entries_len;
  key_entry->count = 1;
  key_entry->depth = max_child_depth + 1;
  key_entry->type = node_type;
  *out_depth = max_child_depth + 1;

  if(child_count == 0) {
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

static uint64_t
add_subtrees(SubtreeCounter *subtree_counter, AstNode *root) {

  TSTreeCursor cursor = ts_tree_cursor_new(root->ts_node);
  Tree *tree = node_get_tree(root);

  if(!subtree_counter_use_type(subtree_counter, ts_node_symbol(root->ts_node))) {
    rb_raise(rb_eArgError, "the root node's type must be in types");
    return -1;
  }

  uint16_t root_depth;
  uint64_t root_id = add_subtrees_(subtree_counter, &cursor, root->ts_node, tree, &root_depth);

  ts_tree_cursor_delete(&cursor);

  return root_id;
}

static VALUE
rb_subtree_counter_add(VALUE self, VALUE rb_node) {
  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(self, SubtreeCounter, &subtree_counter_type, subtree_counter);

  AstNode *node;
  TypedData_Get_Struct(rb_node, AstNode, &node_type, node);

  Tree *tree = node_get_tree(node);
  Language *node_language = tree->language;

  Language* language;
  TypedData_Get_Struct(subtree_counter->rb_language, Language, &language_type, language);

  if(language != node_language) {
    rb_raise(rb_eArgError, "node has different language than counter");
    return Qnil;
  }

  uint64_t root_id = add_subtrees(subtree_counter, node);
  return RB_ULL2NUM(root_id);
}

static VALUE
rb_subtree_counter_size(VALUE self) {
  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(self, SubtreeCounter, &subtree_counter_type, subtree_counter);

  return RB_SIZE2NUM(subtree_counter->entries_len);
}

static VALUE
rb_subtree_counter_entry_new(VALUE rb_subtree_counter, SubtreeCounterEntry *entry) {
  SubtreeCounterEntryRb* subtree_counter_entry = RB_ZALLOC(SubtreeCounterEntryRb);
  subtree_counter_entry->rb_subtree_counter = rb_subtree_counter;
  subtree_counter_entry->entry = entry;

  return TypedData_Wrap_Struct(rb_cSubtreeCounterEntry, &subtree_counter_entry_type, subtree_counter_entry);
};

static VALUE
rb_subtree_counter_aref(VALUE self, VALUE rb_index) {
  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(self, SubtreeCounter, &subtree_counter_type, subtree_counter);
  ssize_t index = RB_NUM2SSIZE(rb_index);

  if(index < 0) {
    index += subtree_counter->entries_len;
  }
  if( (size_t) index >= subtree_counter->entries_len) {
    rb_raise(rb_eIndexError, "index %ld outside bounds: %d...%ld", index, 0, subtree_counter->entries_len);
  }

  return rb_subtree_counter_entry_new(self, subtree_counter->entries_ptrs[index]);
}

static VALUE
subtree_counter_enum_length(VALUE rb_subtree_counter, VALUE args, VALUE eobj)
{
  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(rb_subtree_counter, SubtreeCounter, &subtree_counter_type, subtree_counter);
  return RB_SIZE2NUM(subtree_counter->entries_len);
}

static VALUE
rb_subtree_counter_each(VALUE self)
{
  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(self, SubtreeCounter, &subtree_counter_type, subtree_counter);
  RETURN_SIZED_ENUMERATOR(self, 0, 0, subtree_counter_enum_length);

  for(size_t i = 0; i < subtree_counter->entries_len; i++) {
    VALUE rb_entry = rb_subtree_counter_entry_new(self, subtree_counter->entries_ptrs[i]);
    rb_yield(rb_entry);
  }
  return self;
}

static VALUE
rb_subtree_counter_entry_type(VALUE self) {
  SubtreeCounterEntryRb *subtree_counter_entry;
  TypedData_Get_Struct(self, SubtreeCounterEntryRb, &subtree_counter_entry_type, subtree_counter_entry);

  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(subtree_counter_entry->rb_subtree_counter, SubtreeCounter, &subtree_counter_type, subtree_counter);

  Language* language;
  TypedData_Get_Struct(subtree_counter->rb_language, Language, &language_type, language);

  return RB_ID2SYM(language_symbol2id(language, (TSSymbol) subtree_counter_entry->entry->type));
}

static VALUE
rb_subtree_counter_entry_text(VALUE self) {
  SubtreeCounterEntryRb *subtree_counter_entry;
  TypedData_Get_Struct(self, SubtreeCounterEntryRb, &subtree_counter_entry_type, subtree_counter_entry);

  if(subtree_counter_entry->entry->text_len == 0) {
    return Qnil;
  }

  return rb_str_new(subtree_counter_entry->entry->text, subtree_counter_entry->entry->text_len);
}

static VALUE
rb_subtree_counter_entry_count(VALUE self) {
  SubtreeCounterEntryRb *subtree_counter_entry;
  TypedData_Get_Struct(self, SubtreeCounterEntryRb, &subtree_counter_entry_type, subtree_counter_entry);

  return UINT2NUM(subtree_counter_entry->entry->count);
}

static VALUE
rb_subtree_counter_entry_child_fields(VALUE self) {
  SubtreeCounterEntryRb *subtree_counter_entry;
  TypedData_Get_Struct(self, SubtreeCounterEntryRb, &subtree_counter_entry_type, subtree_counter_entry);
  SubtreeCounterEntry *entry = subtree_counter_entry->entry;

  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(subtree_counter_entry->rb_subtree_counter, SubtreeCounter, &subtree_counter_type, subtree_counter);

  Language* language;
  TypedData_Get_Struct(subtree_counter->rb_language, Language, &language_type, language);

  VALUE rb_ary = rb_ary_new_capa(subtree_counter_entry->entry->child_count);
  for(uint16_t i = 0; i < subtree_counter_entry->entry->child_count; i++) {

    TSFieldId field_id;
    if(i < SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
      field_id = (TSFieldId) entry->child_fields[i];
    } else {
      field_id = (TSFieldId) entry->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].field;
    }
    VALUE rb_field = Qnil;
    if(field_id) {
      rb_field = RB_ID2SYM(language_field2id(language, field_id));
    } 
    rb_ary_push(rb_ary, rb_field);
  }

  return rb_ary;
}

static VALUE
rb_subtree_counter_entry_child_ids(VALUE self) {
  SubtreeCounterEntryRb *subtree_counter_entry;
  TypedData_Get_Struct(self, SubtreeCounterEntryRb, &subtree_counter_entry_type, subtree_counter_entry);

  SubtreeCounterEntry *entry = subtree_counter_entry->entry;

  VALUE rb_ary = rb_ary_new_capa(subtree_counter_entry->entry->child_count);
  for(uint16_t i = 0; i < subtree_counter_entry->entry->child_count; i++) {
    uint64_t node_id;
    if(i < SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
      node_id = entry->child_ids[i];
    } else {
      node_id = entry->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].id;
    }
    rb_ary_push(rb_ary, RB_ULL2NUM(node_id));
  }

  return rb_ary;
}


// Adapted from from libjson-c
// Original implementation:
/*
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

static int json_escape_str(VALUE rb_buf, const char *str, size_t len, bool no_slash_escape)
{
  static const char *json_hex_chars = "0123456789abcdefABCDEF";
	size_t pos = 0, start_offset = 0;
	unsigned char c;
	while (len)
	{
		--len;
		c = str[pos];
		switch (c)
		{
		case '\b':
		case '\n':
		case '\r':
		case '\t':
		case '\f':
		case '"':
		case '\\':
		case '/':
			if (no_slash_escape && c == '/')
			{
				pos++;
				break;
			}

			if (pos > start_offset)
        rb_str_cat(rb_buf, str + start_offset, pos - start_offset);
			if (c == '\b')
        rb_str_cat(rb_buf, "\\b", 2);
			else if (c == '\n')
				rb_str_cat(rb_buf, "\\n", 2);
			else if (c == '\r')
				rb_str_cat(rb_buf, "\\r", 2);
			else if (c == '\t')
				rb_str_cat(rb_buf, "\\t", 2);
			else if (c == '\f')
				rb_str_cat(rb_buf, "\\f", 2);
			else if (c == '"')
				rb_str_cat(rb_buf, "\\\"", 2);
			else if (c == '\\')
				rb_str_cat(rb_buf, "\\\\", 2);
			else if (c == '/')
				rb_str_cat(rb_buf, "\\/", 2);

			start_offset = ++pos;
			break;
		default:
			if (c < ' ')
			{
				char sbuf[7];
				if (pos > start_offset)
					rb_str_cat(rb_buf, str + start_offset,
					                   pos - start_offset);
				snprintf(sbuf, sizeof(sbuf), "\\u00%c%c", json_hex_chars[c >> 4],
				         json_hex_chars[c & 0xf]);
				rb_str_cat(rb_buf, sbuf, (int)sizeof(sbuf) - 1);
				start_offset = ++pos;
			}
			else
				pos++;
		}
	}
	if (pos > start_offset)
		rb_str_cat(rb_buf, str + start_offset, pos - start_offset);
	return 0;
}

#define STR_CAT_STATIC(rb_buf, str) rb_str_cat(rb_buf, (str), strlen(str))

static void _rb_str_cat_u64(VALUE rb_str, uint64_t v) {
	char buf[32];
	int written = snprintf(buf, sizeof(buf), "%"PRIu64, v);
  assert(written <= (int) sizeof(buf));
  rb_str_cat(rb_str, buf, written);
}

static void _rb_str_cat_u32(VALUE rb_str, uint32_t v) {
	char buf[16];
	int written = snprintf(buf, sizeof(buf), "%"PRIu32, v);
  assert(written <= (int) sizeof(buf));
  rb_str_cat(rb_str, buf, written);
}

static void _rb_str_cat_u16(VALUE rb_str, uint16_t v) {
	char buf[8];
	int written = snprintf(buf, sizeof(buf), "%"PRIu16, v);
  assert(written <= (int) sizeof(buf));
  rb_str_cat(rb_str, buf, written);
}

void
subtree_counter_entry_to_json(SubtreeCounterEntry *entry, Language *language, VALUE rb_buf, bool type_ids, bool field_ids, bool newline) {
  STR_CAT_STATIC(rb_buf, "{");

  {
    STR_CAT_STATIC(rb_buf, "\"type\":");
    if(type_ids) {
      _rb_str_cat_u16(rb_buf, entry->type);
    } else {
      STR_CAT_STATIC(rb_buf, "\"");
      rb_str_cat_cstr(rb_buf, rb_id2name(language_symbol2id(language, entry->type)));
      STR_CAT_STATIC(rb_buf, "\"");
    }
  }

  if(entry->text_len > 0) {
    STR_CAT_STATIC(rb_buf, ",\"text\":");
    STR_CAT_STATIC(rb_buf, "\"");
    json_escape_str(rb_buf, entry->text, entry->text_len, false);
    STR_CAT_STATIC(rb_buf, "\"");
  }

  {
    STR_CAT_STATIC(rb_buf, ",\"count\":");
    _rb_str_cat_u32(rb_buf, entry->count);
  }

  if(entry->child_count > 0) {
    {
      STR_CAT_STATIC(rb_buf, ",\"child_ids\":[");
      for(ssize_t i = 0; i < entry->child_count; i++) {
        uint64_t node_id;
        if(i < SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
          node_id = entry->child_ids[i];
        } else {
          node_id = entry->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].id;
        }
        _rb_str_cat_u64(rb_buf, node_id);
        if(i < entry->child_count - 1) {
          STR_CAT_STATIC(rb_buf, ",");
        }
      }
      STR_CAT_STATIC(rb_buf, "]");
    }

    {
      STR_CAT_STATIC(rb_buf, ",\"child_fields\":[");
      for(ssize_t i = 0; i < entry->child_count; i++) {
        uint16_t field_id;
        if(i < SUBTREE_COUNTER_ENTRY_MAX_CHILDREN) {
          field_id = entry->child_fields[i];
        } else {
          field_id = entry->children[i - SUBTREE_COUNTER_ENTRY_MAX_CHILDREN].field;
        }

        if(field_id != 0) {
          if(field_ids) {
            _rb_str_cat_u16(rb_buf, field_id);
          } else {
            STR_CAT_STATIC(rb_buf, "\"");
            rb_str_cat_cstr(rb_buf, rb_id2name(language_field2id(language, field_id)));
            STR_CAT_STATIC(rb_buf, "\"");
          }
        } else {
          STR_CAT_STATIC(rb_buf, "null");
        }
        if(i < entry->child_count - 1) {
          STR_CAT_STATIC(rb_buf, ",");
        }
      }
      STR_CAT_STATIC(rb_buf, "]");
    }
  }

  {
    STR_CAT_STATIC(rb_buf, ",\"depth\":");
    _rb_str_cat_u16(rb_buf, entry->depth);
  }
  
  if(newline) {
    STR_CAT_STATIC(rb_buf, "}\n");
  } else {
    STR_CAT_STATIC(rb_buf, "}");
  }
}

static VALUE
rb_subtree_counter_to_json(VALUE self, VALUE rb_type_ids, VALUE rb_field_ids, VALUE rb_jsonl) {
  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(self, SubtreeCounter, &subtree_counter_type, subtree_counter);

  Language* language;
  TypedData_Get_Struct(subtree_counter->rb_language, Language, &language_type, language);

  VALUE rb_buf = rb_str_buf_new(128 * subtree_counter->entries_len);

  bool jsonl = RTEST(rb_jsonl);

  if(!jsonl) {
    STR_CAT_STATIC(rb_buf, "[");
  }

  for(size_t i = 0; i < subtree_counter->entries_len; i++) {
    subtree_counter_entry_to_json(subtree_counter->entries_ptrs[i], language, rb_buf, RTEST(rb_type_ids), RTEST(rb_field_ids), jsonl);

    if(!jsonl && i < subtree_counter->entries_len - 1) {
      STR_CAT_STATIC(rb_buf, ",");
    }
  }

  if(!jsonl) {
    STR_CAT_STATIC(rb_buf, "]");
  }

  return rb_buf;
}

static VALUE
rb_subtree_counter_entry_to_json(VALUE self, VALUE rb_type_ids, VALUE rb_field_ids) {
  SubtreeCounterEntryRb *subtree_counter_entry;
  TypedData_Get_Struct(self, SubtreeCounterEntryRb, &subtree_counter_entry_type, subtree_counter_entry);
  SubtreeCounterEntry *entry = subtree_counter_entry->entry;

  SubtreeCounter *subtree_counter;
  TypedData_Get_Struct(subtree_counter_entry->rb_subtree_counter, SubtreeCounter, &subtree_counter_type, subtree_counter);

  Language* language;
  TypedData_Get_Struct(subtree_counter->rb_language, Language, &language_type, language);

  VALUE rb_buf = rb_str_buf_new(128);
  subtree_counter_entry_to_json(entry, language, rb_buf, RTEST(rb_type_ids), RTEST(rb_field_ids), false);

  return rb_buf;
}


void
init_misc() {
  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");
  rb_cSubtreeCounter = rb_define_class_under(rb_mTreeSitter, "SubtreeCounter", rb_cObject);
  rb_cSubtreeCounterEntry = rb_define_class_under(rb_cSubtreeCounter, "Entry", rb_cObject);
  rb_define_alloc_func(rb_cSubtreeCounter, rb_subtree_counter_alloc);

  rb_define_method(rb_cSubtreeCounter, "initialize", rb_subtree_counter_initialize, 2);
  rb_define_method(rb_cSubtreeCounter, "add", rb_subtree_counter_add, 1);
  rb_define_method(rb_cSubtreeCounter, "size", rb_subtree_counter_size, 0);
  rb_define_method(rb_cSubtreeCounter, "[]", rb_subtree_counter_aref, 1);
  rb_define_method(rb_cSubtreeCounter, "each", rb_subtree_counter_each, 0);
  rb_define_method(rb_cSubtreeCounter, "__to_json__", rb_subtree_counter_to_json, 3);

  rb_define_method(rb_cSubtreeCounterEntry, "count", rb_subtree_counter_entry_count, 0);
  rb_define_method(rb_cSubtreeCounterEntry, "text", rb_subtree_counter_entry_text, 0);
  rb_define_method(rb_cSubtreeCounterEntry, "type", rb_subtree_counter_entry_type, 0);
  rb_define_method(rb_cSubtreeCounterEntry, "child_ids", rb_subtree_counter_entry_child_ids, 0);
  rb_define_method(rb_cSubtreeCounterEntry, "child_fields", rb_subtree_counter_entry_child_fields, 0);
  rb_define_method(rb_cSubtreeCounterEntry, "__to_json__", rb_subtree_counter_entry_to_json, 2);
}