#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"
#include "node.h"

#define SUBTREE_COUNTER_ENTRY_MAX_CHILDREN 16
#define SUBTREE_COUNTER_INIT_CAPA 1024 * 10

typedef struct {
  uint64_t id;
  uint16_t field;
} SubtreeCounterEntryChild;

typedef struct {
  uint16_t type;
  uint16_t text_len;
  uint32_t count;
  uint16_t child_count;
  uint16_t depth;
  uint16_t child_fields[SUBTREE_COUNTER_ENTRY_MAX_CHILDREN];
  uint64_t child_ids[SUBTREE_COUNTER_ENTRY_MAX_CHILDREN];
  SubtreeCounterEntryChild *children;
  char *text;
} SubtreeCounterEntry;

typedef struct {
  SubtreeCounterEntry *entry;
  uint64_t id;
} SubtreeCounterEntryWithId;

typedef struct {
  st_table *id_map;
//   SubtreeCounterEntry *nodes;
  SubtreeCounterEntry **entries_ptrs;
  size_t entries_len;
  size_t entries_capa;
  VALUE rb_language;
  uint16_t *types;
  ssize_t types_len;
} SubtreeCounter;

typedef struct {
  VALUE rb_subtree_counter;
  SubtreeCounterEntry *entry;
} SubtreeCounterEntryRb;

void init_misc();