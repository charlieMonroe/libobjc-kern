
#ifndef OBJC_SELECTOR_H_
#define OBJC_SELECTOR_H_

/*
 * Some basic selectors used throughout the
 * run-time, initialized in objc_selector_init()
 */
PRIVATE extern SEL objc_retain_selector;
PRIVATE extern SEL objc_release_selector;
PRIVATE extern SEL objc_dealloc_selector;
PRIVATE extern SEL objc_autorelease_selector;
PRIVATE extern SEL objc_copy_selector;
PRIVATE extern SEL objc_cxx_construct_selector;
PRIVATE extern SEL objc_cxx_destruct_selector;
PRIVATE extern SEL objc_load_selector;
PRIVATE extern SEL objc_initialize_selector;
PRIVATE extern SEL objc_is_arc_compatible_selector;


/* 
 * Since SEL is just a 16-bit number pointing into the selector table,
 * simple comparison is sufficient.
 */
static inline BOOL
objc_selectors_equal(SEL selector1, SEL selector2)
{
	return selector1 == selector2;
}

struct objc_selector_reference {
	/* Selector name + '\0' + types. */
	const char *selector_name;
		
	/*
	 * Pointer to the variable with the actual SEL, which gets populated by the
	 * runtme.
	 */
	SEL *sel_uid;
};

/*
 * Registers all selectors within the class or method list.
 */
PRIVATE void objc_register_selectors_from_method_list(objc_method_list *list);
PRIVATE void objc_register_selectors_from_class(Class cl, Class meta);
PRIVATE void objc_register_selector_array(struct objc_selector_reference *selectors,
										  unsigned int count);

#endif /* !OBJC_SELECTOR_H_ */
