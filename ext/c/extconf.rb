require 'mkmf'
$INCFLAGS << ' -I$(srcdir)/../core/vendor/include'
$defs.push("-DLANGUAGE_C=15")
create_makefile('c')