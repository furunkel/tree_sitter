#include "ruby.h"

extern const void *tree_sitter_template(void);
extern void require_core(void);

void Init_template()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cTemplate = rb_define_class_under(mTreeSitter, "Template", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cTemplate_s = rb_singleton_class(rb_cTemplate);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_template);
  rb_ivar_set(rb_cTemplate, language_id, rb_language_func);
}
