#include "ruby.h"

extern const void *tree_sitter_verilog(void);
extern void require_core(void);

static VALUE
rb_verilog_initialize(int argc, VALUE *argv, VALUE self) {
  ID language_id = rb_intern("@__language__");
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_verilog);
  rb_ivar_set(self, language_id, rb_language_func);
	rb_call_super_kw(argc, argv, RB_PASS_CALLED_KEYWORDS);
	return Qnil;
}

void Init_verilog()
{
  rb_require("tree_sitter/core");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cVerilog = rb_define_class_under(mTreeSitter, "Verilog", cTree);
  rb_define_method(rb_cVerilog, "initialize", rb_verilog_initialize, -1);
}
