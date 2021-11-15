#include "ruby.h"

extern const void *tree_sitter_go(void);
extern void require_core(void);

void Init_go()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cGo = rb_define_class_under(mTreeSitter, "Go", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cGo_s = rb_singleton_class(rb_cGo);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_go);
  rb_ivar_set(rb_cGo, language_id, rb_language_func);
}
