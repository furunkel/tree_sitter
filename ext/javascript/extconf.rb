require 'mkmf'
$INCFLAGS << ' -I$(srcdir)/../core/vendor/include'
create_makefile('javascript')