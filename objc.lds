SECTIONS
{
	. = ALIGN(4096);
	set_objc_module_list_set : {
		__start_set_objc_module_list_set = .;
		*(set_objc_module_list_set)
		__stop_set_objc_module_list_set = .;
	}
}
