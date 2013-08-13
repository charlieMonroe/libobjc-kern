/* Provide a real file - not a symlink - as it would cause multiarch conflicts
   when multiple different arch releases are installed simultaneously.  */

#if defined __x86_64__
# include "libunwind-x86_64.h"
#else
# error "Unsupported arch"
#endif
