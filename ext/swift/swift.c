#include "ruby.h"

extern const void *tree_sitter_swift(void);
extern void require_core(void);

void Init_swift()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cSwift = rb_define_class_under(mTreeSitter, "Swift", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cSwift_s = rb_singleton_class(rb_cSwift);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_swift);
  rb_ivar_set(rb_cSwift, language_id, rb_language_func);
}
