#include "ruby.h"

extern const void *tree_sitter_tsq(void);
extern void require_core(void);

void Init_tsq()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cTsq = rb_define_class_under(mTreeSitter, "Tsq", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cTsq_s = rb_singleton_class(rb_cTsq);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_tsq);
  rb_ivar_set(rb_cTsq, language_id, rb_language_func);
}
