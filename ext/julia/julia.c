#include "ruby.h"

extern const void *tree_sitter_julia(void);
extern void require_core(void);

void Init_julia()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJulia = rb_define_class_under(mTreeSitter, "Julia", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cJulia_s = rb_singleton_class(rb_cJulia);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_julia);
  rb_ivar_set(rb_cJulia, language_id, rb_language_func);
}
