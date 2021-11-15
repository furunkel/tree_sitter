#include "ruby.h"

extern const void *tree_sitter_javascript(void);
extern void require_core(void);

void Init_javascript()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJavascript = rb_define_class_under(mTreeSitter, "Javascript", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cJavascript_s = rb_singleton_class(rb_cJavascript);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_javascript);
  rb_ivar_set(rb_cJavascript, language_id, rb_language_func);
}
