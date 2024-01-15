require 'mkmf'
$INCFLAGS << ' -I$(srcdir)/../core/vendor/include'
$defs.push("-DLANGUAGE_TEMPLATE=LANGUAGE_ID")
create_makefile('template')