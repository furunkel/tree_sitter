#include "ruby.h"

extern const void *tree_sitter_haskell(void);
extern void require_core(void);

void Init_haskell()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cHaskell = rb_define_class_under(mTreeSitter, "Haskell", cTree);

  ID language_id = rb_intern("@__language__");
  VALUE rb_cHaskell_s = rb_singleton_class(rb_cHaskell);
  VALUE rb_language_func = ULL2NUM((uintptr_t) tree_sitter_haskell);
  rb_ivar_set(rb_cHaskell, language_id, rb_language_func);
}
