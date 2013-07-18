#include "../kernobjc/runtime.h"
#include "../classes/MRObjects.h"
#include "../os.h"

static void print_method_list(struct objc_method_list_struct *methods){
	if (methods == NULL){
		printf("\t -- No methods\n");
		return;
	}
	
	for (int i = 0; i < methods->size; ++i){
		printf("\t%s(%d) - %p\n", sel_getName(methods->method_list[i].selector), methods->method_list[i].selector, methods->method_list[i].implementation);
	}
}
static void print_ivar_list(struct objc_ivar_list_struct *ivars){
	if (ivars == NULL){
		printf("\t -- No ivars\n");
		return;
	}
	
	for (int i = 0; i < ivars->size; ++i){
		printf("\t%s - %s - size: %d offset: %d alignment: %d\n", ivars->ivar_list[i].name, ivars->ivar_list[i].type, ivars->ivar_list[i].size, ivars->ivar_list[i].offset, ivars->ivar_list[i].align);
	}
}

static void print_class(Class cl){
	printf("******** Class %s %s********\n", class_getName(cl), cl->flags.meta ? "(meta)" : "");
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
	objc_dealloc(classes);
}


void associated_objects_test(void);
void weak_ref_test(void);

int main(int argc, const char * argv[])
{
	list_classes();
	
	associated_objects_test();
	weak_ref_test();
	
	return 0;
}

