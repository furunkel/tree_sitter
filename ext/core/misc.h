#pragma once

#include "ruby.h"
#include "tree_sitter/api.h"
#include "core.h"
#include "node.h"

#define SUBTREE_COUNTER_ENTRY_MAX_CHILDREN 256
#define SUBTREE_COUNTER_INIT_CAPA 1024 * 10

typedef struct {
  uint16_t type;
  uint16_t text_len;
  uint16_t count;
  uint16_t child_count;
  uint16_t child_fields[SUBTREE_COUNTER_ENTRY_MAX_CHILDREN];
  uint32_t child_ids[SUBTREE_COUNTER_ENTRY_MAX_CHILDREN];
  char *text;
} SubtreeCounterEntry;


typedef struct {
  st_table *id_map;
//   SubtreeCounterEntry *nodes;
  SubtreeCounterEntry **entries_ptrs;
  size_t entries_len;
  size_t entries_capa;
} SubtreeCounter;

void init_misc();