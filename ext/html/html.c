#include "ruby.h"

extern const void *tree_sitter_html(void);
extern void require_core(void);

void Init_html()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cHtml = rb_define_class_under(mTreeSitter, "Html", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cHtml_s = rb_singleton_class(rb_cHtml);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_html);
  rb_ivar_set(rb_cHtml, language_id, rb_language_func);
}
