#include "ruby.h"

extern const void *tree_sitter_css(void);
extern void require_core(void);

void Init_css()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cCss = rb_define_class_under(mTreeSitter, "Css", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cCss_s = rb_singleton_class(rb_cCss);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_css);
  rb_ivar_set(rb_cCss, language_id, rb_language_func);
}
