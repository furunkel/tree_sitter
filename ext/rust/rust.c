#include "ruby.h"

extern const void *tree_sitter_rust(void);
extern void require_core(void);

void Init_rust()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cRust = rb_define_class_under(mTreeSitter, "Rust", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cRust_s = rb_singleton_class(rb_cRust);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_rust);
  rb_ivar_set(rb_cRust, language_id, rb_language_func);
}
