#include "ruby.h"

extern const void *tree_sitter_typescript(void);
extern void require_core(void);

void Init_typescript()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cTypescript = rb_define_class_under(mTreeSitter, "Typescript", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cTypescript_s = rb_singleton_class(rb_cTypescript);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_typescript);
  rb_ivar_set(rb_cTypescript, language_id, rb_language_func);
}
