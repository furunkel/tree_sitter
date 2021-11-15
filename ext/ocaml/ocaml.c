#include "ruby.h"

extern const void *tree_sitter_ocaml(void);
extern void require_core(void);

void Init_ocaml()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cOcaml = rb_define_class_under(mTreeSitter, "Ocaml", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cOcaml_s = rb_singleton_class(rb_cOcaml);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_ocaml);
  rb_ivar_set(rb_cOcaml, language_id, rb_language_func);
}
