#include "ruby.h"

extern const void *tree_sitter_java(void);
extern void require_core(void);

void Init_java()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJava = rb_define_class_under(mTreeSitter, "Java", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cJava_s = rb_singleton_class(rb_cJava);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_java);
  rb_ivar_set(rb_cJava, language_id, rb_language_func);
}
