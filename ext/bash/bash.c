#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_bash(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_bash()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cBash = rb_define_class_under(mTreeSitter, "Bash", cTree);

  VALUE rb_cBash_s = rb_singleton_class(rb_cBash);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_bash());

  rb_ivar_set(rb_cBash, id___language__, rb_language);
}
