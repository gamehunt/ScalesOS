#ifndef __SYS_MMAN
#define __SYS_MMAN

#define PROT_READ  (1 << 0) // Page can be read.
#define PROT_WRITE (1 << 1) // Page can be written.
#define PROT_EXEC  (1 << 2) // Page can be executed.
#define PROT_NONE  0        // Page cannot be accessed.

#define MAP_SHARED  1 // Share changes.
#define MAP_PRIVATE 2 // Changes are private.
#define MAP_FIXED   3 // Interpret addr exactly.
					  
#define MS_ASYNC      1 // Perform asynchronous writes.
#define MS_SYNC       2 // Perform synchronous writes.
#define MS_INVALIDATE 3 // Invalidate mappings.

#define MCL_CURRENT 1 // Lock currently mapped pages.
#define MCL_FUTURE  2 // Lock pages that become mapped.

#define MAP_FAILED  -1

#endif
