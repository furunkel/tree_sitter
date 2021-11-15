#include "ruby.h"

extern const void *tree_sitter_regex(void);
extern void require_core(void);

void Init_regex()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cRegex = rb_define_class_under(mTreeSitter, "Regex", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cRegex_s = rb_singleton_class(rb_cRegex);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_regex);
  rb_ivar_set(rb_cRegex, language_id, rb_language_func);
}
