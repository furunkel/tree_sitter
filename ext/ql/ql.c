#include "ruby.h"

extern const void *tree_sitter_ql(void);
extern void require_core(void);

void Init_ql()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cQl = rb_define_class_under(mTreeSitter, "Ql", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cQl_s = rb_singleton_class(rb_cQl);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_ql);
  rb_ivar_set(rb_cQl, language_id, rb_language_func);
}
