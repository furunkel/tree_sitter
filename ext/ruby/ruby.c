#include "ruby.h"

extern const void *tree_sitter_ruby(void);
extern void require_core(void);

void Init_ruby()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cRuby = rb_define_class_under(mTreeSitter, "Ruby", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cRuby_s = rb_singleton_class(rb_cRuby);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_ruby);
  rb_ivar_set(rb_cRuby, language_id, rb_language_func);
}
