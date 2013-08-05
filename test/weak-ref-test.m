#include "../os.h"
#include "../kernobjc/types.h"
#include "../types.h"
#include "../class_registry.h"
#include "../class.h"
#import "../KKObjects.h"
#include "../selector.h"
#include "../kernobjc/runtime.h"
#include "../message.h"
#include "../associative.h"

static void weak_ref_test_manual(void){
  typedef struct {
    id isa;
    int retain_count;
  } Object;
  
  
  KKObject *obj = [[KKObject alloc] init];
	
	id weak_ref = (id)0x1234;
	
	objc_storeWeak(&weak_ref, (id)obj);
	
	objc_assert(weak_ref == (id)obj, "The obj wasn't stored in the weak ref!\n");
	
	objc_release((id)obj);
	
	objc_assert(((Object*)obj)->retain_count == -1, "The associated object should have been deallocated!\n");
	objc_assert(weak_ref == 0, "The weak ref isn't zeroed out!\n")
	
	objc_log("===================\n");
	objc_log("Passed weak ref tests.\n\n");
  
}

static void weak_ref_test_compiler(void){
  // TODO
}


void weak_ref_test(void){
  weak_ref_test_manual();
  weak_ref_test_compiler();
}
