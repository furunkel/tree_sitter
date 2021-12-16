#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_ql(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_ql()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cQl = rb_define_class_under(mTreeSitter, "Ql", cTree);

  VALUE rb_cQl_s = rb_singleton_class(rb_cQl);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_ql());

  rb_ivar_set(rb_cQl, id___language__, rb_language);
}
