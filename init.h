

#ifndef OBJC_INIT_H_
#define OBJC_INIT_H_

PRIVATE extern BOOL  objc_runtime_initialized;

PRIVATE void	objc_arc_destroy(void);
PRIVATE void	objc_arc_init(void);

PRIVATE void	objc_class_destroy(void);
PRIVATE void	objc_class_init(void);

PRIVATE void	objc_dispatch_tables_destroy(void);
PRIVATE void	objc_dispatch_tables_init(void);

PRIVATE void	objc_protocol_destroy(void);
PRIVATE void	objc_protocol_init(void);

PRIVATE void	objc_selector_destroy(void);
PRIVATE void	objc_selector_init(void);

/* Must be initialized after selectors! */
PRIVATE void	objc_associated_objects_init(void);
PRIVATE void	objc_associated_objects_destroy(void);

//PRIVATE void	objc_exceptions_init(void);

PRIVATE void	objc_runtime_init(void);
PRIVATE void	objc_runtime_destroy(void);

#endif /* !OBJC_INIT_H_ */
