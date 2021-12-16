#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_go(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_go()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cGo = rb_define_class_under(mTreeSitter, "Go", cTree);

  VALUE rb_cGo_s = rb_singleton_class(rb_cGo);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_go());

  rb_ivar_set(rb_cGo, id___language__, rb_language);
}
