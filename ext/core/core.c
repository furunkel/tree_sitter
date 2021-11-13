#include "core.h"

__attribute__((visibility("default"))) void Init_core()
{
  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");

  init_tree();
  init_node();
}
