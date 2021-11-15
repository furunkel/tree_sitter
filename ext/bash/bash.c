#include "ruby.h"

extern const void *tree_sitter_bash(void);
extern void require_core(void);

void Init_bash()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cBash = rb_define_class_under(mTreeSitter, "Bash", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cBash_s = rb_singleton_class(rb_cBash);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_bash);
  rb_ivar_set(rb_cBash, language_id, rb_language_func);
}
