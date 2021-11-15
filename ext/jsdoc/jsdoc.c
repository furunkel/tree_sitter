#include "ruby.h"

extern const void *tree_sitter_jsdoc(void);
extern void require_core(void);

void Init_jsdoc()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJsdoc = rb_define_class_under(mTreeSitter, "Jsdoc", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cJsdoc_s = rb_singleton_class(rb_cJsdoc);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_jsdoc);
  rb_ivar_set(rb_cJsdoc, language_id, rb_language_func);
}
