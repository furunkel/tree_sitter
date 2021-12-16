#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_java(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_java()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cJava = rb_define_class_under(mTreeSitter, "Java", cTree);

  VALUE rb_cJava_s = rb_singleton_class(rb_cJava);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_java());

  rb_ivar_set(rb_cJava, id___language__, rb_language);
}
