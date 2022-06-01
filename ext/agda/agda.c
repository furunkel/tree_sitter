#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_agda(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_agda()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));
  VALUE cQuery = rb_const_get(cTree, rb_intern("Query"));

  VALUE rb_cAgda = rb_define_class_under(mTreeSitter, "Agda", cTree);
  VALUE rb_cAgda_Query = rb_define_class_under(rb_cAgda, "Query", cQuery);

  VALUE rb_cAgda_s = rb_singleton_class(rb_cAgda);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_agda());

  rb_ivar_set(rb_cAgda, id___language__, rb_language);
  rb_ivar_set(rb_cAgda_Query, id___language__, rb_language);
}
