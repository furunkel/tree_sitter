require 'mkmf'

$INCFLAGS << ' -I$(srcdir)/vendor/include'

create_makefile('core')
