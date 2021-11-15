#include "ruby.h"

extern const void *tree_sitter_cpp(void);
extern void require_core(void);

void Init_cpp()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cCpp = rb_define_class_under(mTreeSitter, "Cpp", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cCpp_s = rb_singleton_class(rb_cCpp);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_cpp);
  rb_ivar_set(rb_cCpp, language_id, rb_language_func);
}
