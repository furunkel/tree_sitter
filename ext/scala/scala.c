#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_scala(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language, int language_id);

void Init_scala()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));
  VALUE cQuery = rb_const_get(cTree, rb_intern("Query"));

  VALUE rb_cScala = rb_define_class_under(mTreeSitter, "Scala", cTree);
  VALUE rb_cScala_Query = rb_define_class_under(rb_cScala, "Query", cQuery);

  VALUE rb_cScala_s = rb_singleton_class(rb_cScala);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_scala(), LANGUAGE_SCALA);

  rb_ivar_set(rb_cScala, id___language__, rb_language);
  rb_ivar_set(rb_cScala_Query, id___language__, rb_language);
}
