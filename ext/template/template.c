#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_template(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language, int language_id);

void Init_template()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));
  VALUE cQuery = rb_const_get(cTree, rb_intern("Query"));

  VALUE rb_cTemplate = rb_define_class_under(mTreeSitter, "Template", cTree);
  VALUE rb_cTemplate_Query = rb_define_class_under(rb_cTemplate, "Query", cQuery);

  VALUE rb_cTemplate_s = rb_singleton_class(rb_cTemplate);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_template(), LANGUAGE_TEMPLATE);

  rb_ivar_set(rb_cTemplate, id___language__, rb_language);
  rb_ivar_set(rb_cTemplate_Query, id___language__, rb_language);
}
