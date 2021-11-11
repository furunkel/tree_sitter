#pragma once

#include "ruby.h"

#define ts_malloc ruby_xmalloc
#define ts_calloc ruby_xcalloc
#define ts_realloc ruby_xrealloc
#define ts_free    ruby_xfree