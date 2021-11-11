#include "core.h"

__attribute__((visibility("default"))) void Init_core()
{
  VALUE tree_sitter = rb_define_module("TreeSitter");

  init_tree();
  init_node();
}
