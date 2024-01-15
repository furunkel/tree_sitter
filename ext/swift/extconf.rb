require 'mkmf'
$INCFLAGS << ' -I$(srcdir)/../core/vendor/include'
$defs.push("-DLANGUAGE_SWIFT=13")
create_makefile('swift')