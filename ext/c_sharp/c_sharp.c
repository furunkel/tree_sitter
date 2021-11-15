#include "ruby.h"

extern const void *tree_sitter_c_sharp(void);
extern void require_core(void);

void Init_c_sharp()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cCSharp = rb_define_class_under(mTreeSitter, "CSharp", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cCSharp_s = rb_singleton_class(rb_cCSharp);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_c_sharp);
  rb_ivar_set(rb_cCSharp, language_id, rb_language_func);
}
