#include "os.h"


MALLOC_DEFINE(M_AUTORELEASE_POOL_TYPE, "autorelease pool", "Objective-C "
              "Autorelease Pool");

MALLOC_DEFINE(M_FAKE_CLASS_TYPE, "fake class", "Objective-C Associated "
              "Objects Fake Class");

MALLOC_DEFINE(M_REFLIST_TYPE, "reference list", "Objective-C Associated "
              "Objects Reference List");

MALLOC_DEFINE(M_BUFFER_TYPE, "buffer", "Objective-C Buffer");

MALLOC_DEFINE(M_OBJECT_TYPE, "object", "Objective-C Objects");

MALLOC_DEFINE(M_IVAR_TYPE, "ivars", "Objective-C Ivar List");

MALLOC_DEFINE(M_CLASS_EXTRA_TYPE, "class extra", "Objective-C Class Extra");

MALLOC_DEFINE(M_CLASS_TYPE, "class", "Objective-C Class");

MALLOC_DEFINE(M_CLASS_MAP_TYPE, "class_table", "Objective-C Class Table");

MALLOC_DEFINE(M_ENCODING_TYPE, "objc encoding", "Objective-C Encoding Strings");

MALLOC_DEFINE(M_METHOD_LIST_TYPE, "method list", "Objective-C Method List");

MALLOC_DEFINE(M_IVAR_LIST_TYPE, "ivar list", "Objective-C Ivar List");

MALLOC_DEFINE(M_CATEGORY_LIST_TYPE, "category list",
              "Objective-C Category List");

MALLOC_DEFINE(M_METHOD_DESC_LIST_TYPE, "method description list",
              "Objective-C Method Description List");

MALLOC_DEFINE(M_LOAD_MSG_MAP_TYPE, "void_msg_map",
              "Objective-C +load message table");

MALLOC_DEFINE(M_PROTOCOL_LIST_TYPE, "protocol list",
              "Objective-C Protocol List");

MALLOC_DEFINE(M_PROTOCOL_MAP_TYPE, "protocol_table",
              "Objective-C Protocol Table");

MALLOC_DEFINE(M_PROPERTY_LIST_TYPE, "property list",
              "Objective-C Property List");

MALLOC_DEFINE(M_METHOD_TYPE, "objc_methods", "Objective-C method structures");

MALLOC_DEFINE(M_PROPERTY_TYPE, "property", "Objective-C Property");

MALLOC_DEFINE(M_PROTOCOL_TYPE, "protocol", "Objective-C Protocol");

MALLOC_DEFINE(M_SPARSE_ARRAY_TYPE, "sparse_array", "Objective-C Sparse Array");

MALLOC_DEFINE(M_SELECTOR_NAME_TYPE, "selector_names", "Objective-C "
              "Selector Names");

MALLOC_DEFINE(M_SELECTOR_TYPE, "selectors", "Objective-C selectors");

MALLOC_DEFINE(M_SELECTOR_MAP_TYPE, "selector_map", "Objective-C selector map");

MALLOC_DEFINE(M_UTILITIES_TYPE, "objc_utils", "Objective-C run-time utilities");

MALLOC_DEFINE(M_SLOT_POOL_TYPE, "slot_pool", "Objective-C slot pool");
