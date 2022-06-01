#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_typescript(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_typescript()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));
  VALUE cQuery = rb_const_get(cTree, rb_intern("Query"));

  VALUE rb_cTypescript = rb_define_class_under(mTreeSitter, "Typescript", cTree);
  VALUE rb_cTypescript_Query = rb_define_class_under(rb_cTypescript, "Query", cQuery);

  VALUE rb_cTypescript_s = rb_singleton_class(rb_cTypescript);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_typescript());

  rb_ivar_set(rb_cTypescript, id___language__, rb_language);
  rb_ivar_set(rb_cTypescript_Query, id___language__, rb_language);
}
