#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_python(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language, int language_id);

void Init_python()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));
  VALUE cQuery = rb_const_get(cTree, rb_intern("Query"));

  VALUE rb_cPython = rb_define_class_under(mTreeSitter, "Python", cTree);
  VALUE rb_cPython_Query = rb_define_class_under(rb_cPython, "Query", cQuery);

  VALUE rb_cPython_s = rb_singleton_class(rb_cPython);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_python(), LANGUAGE_PYTHON);

  rb_ivar_set(rb_cPython, id___language__, rb_language);
  rb_ivar_set(rb_cPython_Query, id___language__, rb_language);
}
