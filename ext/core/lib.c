// The Tree-sitter library can be built by compiling this one source file.
//
// The following directories must be added to the include path:
//   - include

#define _POSIX_C_SOURCE 200112L

#include "alloc.h"
#include "vendor/src/get_changed_ranges.c"
#include "vendor/src/language.c"
#include "vendor/src/lexer.c"
#include "vendor/src/node.c"
#include "vendor/src/parser.c"
#include "vendor/src/query.c"
#include "vendor/src/stack.c"
#include "vendor/src/subtree.c"
#include "vendor/src/tree_cursor.c"
#include "vendor/src/tree.c"
