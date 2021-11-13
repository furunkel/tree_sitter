#include "ruby.h"

extern const void *tree_sitter_regex(void);
extern void require_core(void);

static VALUE
rb_regex_initialize(int argc, VALUE *argv, VALUE self) {
  ID language_id = rb_intern("@__language__");
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_regex);
  rb_ivar_set(self, language_id, rb_language_func);
	rb_call_super_kw(argc, argv, RB_PASS_CALLED_KEYWORDS);
	return Qnil;
}

void Init_regex()
{
  rb_require("tree_sitter/core");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cRegex = rb_define_class_under(mTreeSitter, "Regex", cTree);
  rb_define_method(rb_cRegex, "initialize", rb_regex_initialize, -1);
}
