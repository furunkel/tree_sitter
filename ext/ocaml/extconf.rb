require 'mkmf'
$INCFLAGS << ' -I$(srcdir)/../core/vendor/include'
$defs.push("-DLANGUAGE_OCAML=18")
create_makefile('ocaml')