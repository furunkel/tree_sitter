#include "core.h"

VALUE rb_eTreeSitterError;

__attribute__((visibility("default"))) void Init_core()
{
  VALUE rb_mTreeSitter = rb_define_module("TreeSitter");
  rb_eTreeSitterError = rb_define_class_under(rb_mTreeSitter, "Error", rb_eStandardError);
 
  init_tree();
  init_node();
  init_misc();
}
