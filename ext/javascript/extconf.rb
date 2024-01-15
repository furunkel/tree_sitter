require 'mkmf'
$INCFLAGS << ' -I$(srcdir)/../core/vendor/include'
$defs.push("-DLANGUAGE_JAVASCRIPT=0")
create_makefile('javascript')