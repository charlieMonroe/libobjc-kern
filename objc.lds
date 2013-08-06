SECTIONS
{
	objc_module_list : {
		__start_objc_module_list = .;
		*(objc_module_list)
		__end_objc_module_list = .;
	}
}
