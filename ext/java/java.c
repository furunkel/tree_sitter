#include "ruby.h"

extern const void *tree_sitter_java(void);
extern void require_core(void);

static VALUE
rb_java_initialize(int argc, VALUE *argv, VALUE self) {
  ID language_id = rb_intern("@__language__");
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_java);
  rb_ivar_set(self, language_id, rb_language_func);
	rb_call_super_kw(argc, argv, RB_PASS_CALLED_KEYWORDS);
	return Qnil;
}

void Init_java()
{
  rb_require("tree_sitter/core");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJava = rb_define_class_under(mTreeSitter, "Java", cTree);
  rb_define_method(rb_cJava, "initialize", rb_java_initialize, -1);
}
