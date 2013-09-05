#import "../kernobjc/runtime.h"
#import "../os.h"
#import "../utils.h"

void protocol_as_object_test(void);
void protocol_as_object_test(void)
{
  
  /* This protocol is defined in property-introspection2-test.m */
	Protocol *p = objc_getProtocol("ProtocolTest");
  objc_assert(p != NULL, "Couldn't find the protocol!\n");
  
  /* Try to send a few messages to the protocol. */
  [p retain];
  [p release];
  
  objc_assert([p class] == objc_getClass("Protocol"), "Wrong class!\n");
	
	objc_log("===================\n");
	objc_log("Passed protocol as object test.\n\n");
}
