#include "../class_registry.h"
#import "../classes/KKObjects.h"
#include "../os.h"
#include "../kernobjc/types.h"
#include "../types.h"
#include "../associative.h"

#include "../kernobjc/object.h"


void associated_objects_test(void){
  typedef struct {
    Class isa;
    int retain_count;
  } Object;
  
  KKObject *obj = [[KKObject alloc] init];
	
	void *key = (void*)0x123456;
	
  KKObject *obj_to_associate = [[KKObject alloc] init];
	
	objc_set_associated_object((id)obj, key, (id)obj_to_associate, OBJC_ASSOCIATION_ASSIGN);
	objc_set_associated_object((id)obj, key, nil, OBJC_ASSOCIATION_ASSIGN);
	
	objc_assert(((Object*)obj)->isa->flags.fake, "The object should now be of a fake class!\n");
	objc_assert(!object_getClass((id)obj)->flags.fake, "objc_object_get_class() returned a fake class!\n");
	objc_assert(((Object*)obj_to_associate)->retain_count == 0, "The associated object shouldn't have been released!\n");
	
	objc_set_associated_object((id)obj, key, (id)obj_to_associate, OBJC_ASSOCIATION_RETAIN);
	objc_set_associated_object((id)obj, key, nil, OBJC_ASSOCIATION_ASSIGN);
	
	objc_assert(((Object*)obj_to_associate)->retain_count == 0, "The associated object should have been released!\n");
	
  [obj release];
  
	objc_log("===================\n");
	objc_log("Passed associated object tests.\n\n");
}
