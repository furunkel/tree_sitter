#include "ruby.h"

extern const void *tree_sitter_jsdoc(void);
extern void require_core(void);

static VALUE
rb_jsdoc_initialize(int argc, VALUE *argv, VALUE self) {
  ID language_id = rb_intern("@__language__");
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_jsdoc);
  rb_ivar_set(self, language_id, rb_language_func);
	rb_call_super_kw(argc, argv, RB_PASS_CALLED_KEYWORDS);
	return Qnil;
}

void Init_jsdoc()
{
  rb_require("tree_sitter/core");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJsdoc = rb_define_class_under(mTreeSitter, "Jsdoc", cTree);
  rb_define_method(rb_cJsdoc, "initialize", rb_jsdoc_initialize, -1);
}
