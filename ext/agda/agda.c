#include "ruby.h"

extern const void *tree_sitter_agda(void);
extern void require_core(void);

void Init_agda()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cAgda = rb_define_class_under(mTreeSitter, "Agda", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cAgda_s = rb_singleton_class(rb_cAgda);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_agda);
  rb_ivar_set(rb_cAgda, language_id, rb_language_func);
}
