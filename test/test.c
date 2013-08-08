#include "../kernobjc/runtime.h"
#include "../types.h"


static void print_method_list(struct objc_method_list_struct *methods){
	if (methods == NULL){
		printf("\t -- No methods\n");
		return;
	}
	
	for (int i = 0; i < methods->size; ++i){
		printf("\t%s(%d) - %p\n",
		       sel_getName(methods->list[i].selector),
		       (int)methods->list[i].selector,
		       methods->list[i].implementation);
	}
}
static void print_ivar_list(struct objc_ivar_list_struct *ivars){
	if (ivars == NULL){
		printf("\t -- No ivars\n");
		return;
	}
	
	for (int i = 0; i < ivars->size; ++i){
		printf("\t%s - %s - size: %d offset: %d alignment: %d\n",
		       ivars->list[i].name,
		       ivars->list[i].type,
		       (unsigned int)ivars->list[i].size,
		       (unsigned int)ivars->list[i].offset,
		       (unsigned int)ivars->list[i].align);
	}
}

static void print_class(Class cl){
	printf("******** Class %s%s%s [%p]********\n", class_getName(cl),
	       cl->flags.meta ? " (meta)" : "",
	       cl->flags.fake ? " (fake)" : "",
	       cl
	       );
	printf("**** Methods:\n");
	print_method_list(cl->methods);
	printf("**** Ivars:\n");
	print_ivar_list(cl->ivars);
	
	printf("\n\n");
}
static void list_classes(void){
	unsigned int count;
	Class *classes = objc_copyClassList(&count);
	for (int i = 0; i < count; ++i){
		print_class(classes[i]);
		print_class(classes[i]->isa);
	}
	objc_dealloc(classes, NULL);
}


void associated_objects_test(void);
void weak_ref_test(void);
void ivar_test(void);
void handmade_class_test(void);
void compiler_test(void);
//void exception_test(void);

void run_tests(void);
void run_tests(void)
{
	list_classes();
	
	associated_objects_test();
	weak_ref_test();
	ivar_test();
	handmade_class_test();
	compiler_test();
	//exception_test();
	
	printf("Total number of locks created:              %d\n", objc_lock_count);
	printf("Total number of locks destroyed:            %d\n", objc_lock_destroy_count);
	printf("Locks were locked n. times:                 %d\n", objc_lock_locked_count);
}


