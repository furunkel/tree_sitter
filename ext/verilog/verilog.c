#include "ruby.h"
#include "tree_sitter/api.h"

extern const void *tree_sitter_verilog(void);
extern void require_core(void);
extern ID id___language__;
extern VALUE rb_new_language(TSLanguage *ts_language);

void Init_verilog()
{
  rb_require("tree_sitter");

  VALUE mTreeSitter = rb_const_get(rb_cObject, rb_intern("TreeSitter"));
  VALUE cTree = rb_const_get(mTreeSitter, rb_intern("Tree"));

  VALUE rb_cVerilog = rb_define_class_under(mTreeSitter, "Verilog", cTree);

  VALUE rb_cVerilog_s = rb_singleton_class(rb_cVerilog);
  VALUE rb_language = rb_new_language((TSLanguage *)tree_sitter_verilog());

  rb_ivar_set(rb_cVerilog, id___language__, rb_language);
}
