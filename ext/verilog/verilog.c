#include "ruby.h"

extern const void *tree_sitter_verilog(void);
extern void require_core(void);

void Init_verilog()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cVerilog = rb_define_class_under(mTreeSitter, "Verilog", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cVerilog_s = rb_singleton_class(rb_cVerilog);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_verilog);
  rb_ivar_set(rb_cVerilog, language_id, rb_language_func);
}
