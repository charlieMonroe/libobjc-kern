#include <sys/cdefs.h>
#include <sys/linker_set.h>

#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "init.h"
#include "loader.h"
#include "utils.h"

SET_DECLARE(objc_module_list_set, struct objc_loader_module);

extern void weak_ref_test(void);

int main(int argc, const char *args[]) {
		/* Attempt to load the runtime's basic classes. */
		if (SET_COUNT(objc_module_list_set) == 0) {
			objc_log("Error loading libobj - cannot load"
				" basic required classes.");
			return 1;
		}
		_objc_load_modules(SET_BEGIN(objc_module_list_set),
						   SET_LIMIT(objc_module_list_set));

		weak_ref_test();
		
	return (0);
}

