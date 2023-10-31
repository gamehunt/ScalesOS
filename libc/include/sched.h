#ifndef __SCHED_H
#define __SCHED_H

#define CLONE_CHILD_CLEARTID (1 << 0)
#define CLONE_CHILD_SETTID   (1 << 1)
#define CLONE_CLEAR_SIGHAND  (1 << 2)
#define CLONE_PARENT_SETTID  (1 << 3)
#define CLONE_FILES          (1 << 4)
#define CLONE_FS             (1 << 5)
#define CLONE_PARENT         (1 << 6)
#define CLONE_SETTLS         (1 << 7)
#define CLONE_SIGHAND        (1 << 8)
#define CLONE_THREAD         (1 << 9)
#define CLONE_VM             (1 << 10)

#endif
