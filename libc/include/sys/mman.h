#ifndef __SYS_MMAN
#define __SYS_MMAN

#define PROT_READ  (1 << 0) // Page can be read.
#define PROT_WRITE (1 << 1) // Page can be written.
#define PROT_EXEC  (1 << 2) // Page can be executed.
#define PROT_NONE  0        // Page cannot be accessed.

#define MAP_SHARED    (1 << 0) // Share changes.
#define MAP_PRIVATE   (1 << 1) // Changes are private.
#define MAP_FIXED     (1 << 2) // Interpret addr exactly.
#define MAP_ANONYMOUS (1 << 3) // Don't map to any file
					  
#define MS_ASYNC      (1 << 0) // Perform asynchronous writes.
#define MS_SYNC       (1 << 1) // Perform synchronous writes.
#define MS_INVALIDATE (1 << 2) // Invalidate mappings.

#define MCL_CURRENT 1 // Lock currently mapped pages.
#define MCL_FUTURE  2 // Lock pages that become mapped.

#define MAP_FAILED  -1

#endif
