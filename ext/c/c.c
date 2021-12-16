#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_c(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_c()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cC = rb_define_class_under(mTreeSitter, "C", cTree);

  VALUE rb_cC_s = rb_singleton_class(rb_cC);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_c());

  rb_ivar_set(rb_cC, id___language__, rb_language);
}
