require 'mkmf'
$INCFLAGS << ' -I$(srcdir)/../core/vendor/include'
$defs.push("-DLANGUAGE_JSON=19")
create_makefile('json')