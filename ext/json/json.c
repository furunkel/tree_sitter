#include "ruby.h"

extern const void *tree_sitter_json(void);
extern void require_core(void);

void Init_json()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJson = rb_define_class_under(mTreeSitter, "Json", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cJson_s = rb_singleton_class(rb_cJson);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_json);
  rb_ivar_set(rb_cJson, language_id, rb_language_func);
}
