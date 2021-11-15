#include "ruby.h"

extern const void *tree_sitter_python(void);
extern void require_core(void);

void Init_python()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cPython = rb_define_class_under(mTreeSitter, "Python", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cPython_s = rb_singleton_class(rb_cPython);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_python);
  rb_ivar_set(rb_cPython, language_id, rb_language_func);
}
