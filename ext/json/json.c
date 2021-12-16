#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_json(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_json()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJson = rb_define_class_under(mTreeSitter, "Json", cTree);

  VALUE rb_cJson_s = rb_singleton_class(rb_cJson);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_json());

  rb_ivar_set(rb_cJson, id___language__, rb_language);
}
