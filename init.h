

#ifndef OBJC_INIT_H_
#define OBJC_INIT_H_

#include "types.h" // For BOOL

// TODO rename
PRIVATE void	init_dispatch_tables(void);

PRIVATE void	objc_arc_init(void);
PRIVATE void	objc_class_extra_init(void);
PRIVATE void	objc_class_init(void);
PRIVATE void	objc_install_base_classes(void);
PRIVATE void	objc_protocol_init(void);
PRIVATE void	objc_selector_init(void);


PRIVATE void	objc_runtime_init(void);

#endif /* !OBJC_INIT_H_ */
