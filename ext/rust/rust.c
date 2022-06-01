#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_rust(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_rust()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));
  VALUE cQuery = rb_const_get(cTree, rb_intern("Query"));

  VALUE rb_cRust = rb_define_class_under(mTreeSitter, "Rust", cTree);
  VALUE rb_cRust_Query = rb_define_class_under(rb_cRust, "Query", cQuery);

  VALUE rb_cRust_s = rb_singleton_class(rb_cRust);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_rust());

  rb_ivar_set(rb_cRust, id___language__, rb_language);
  rb_ivar_set(rb_cRust_Query, id___language__, rb_language);
}
