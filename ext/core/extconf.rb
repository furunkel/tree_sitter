require 'mkmf'

$INCFLAGS << ' -I$(srcdir)/vendor/include'
#$LDFLAGS = '-fsanitize=address -lasan ' + $LDFLAGS
#$CFLAGS += ' -fsanitize=address'
CONFIG['debugflags'] << ' -ggdb3 -O0'

create_makefile('core')
