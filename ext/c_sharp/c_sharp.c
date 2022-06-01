#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_c_sharp(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_c_sharp()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));
  VALUE cQuery = rb_const_get(cTree, rb_intern("Query"));

  VALUE rb_cCSharp = rb_define_class_under(mTreeSitter, "CSharp", cTree);
  VALUE rb_cCSharp_Query = rb_define_class_under(rb_cCSharp, "Query", cQuery);

  VALUE rb_cCSharp_s = rb_singleton_class(rb_cCSharp);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_c_sharp());

  rb_ivar_set(rb_cCSharp, id___language__, rb_language);
  rb_ivar_set(rb_cCSharp_Query, id___language__, rb_language);
}
