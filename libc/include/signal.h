#ifndef __SIGNAL_H
#define __SIGNAL_H

#include "sys/signal.h"

#define SIG_DFL ((_signal_handler_ptr)0) /* Default action */
#define SIG_IGN ((_signal_handler_ptr)1) /* Ignore action */
#define SIG_ERR ((_signal_handler_ptr)-1)/* Error return */

#define SA_NOCLDSTOP (1 << 0)
#define SA_NOCLDWAIT (1 << 1)
#define SA_NODEFER   (1 << 2)
#define SA_ONSTACK   (1 << 3)
#define SA_RESETHAND (1 << 4)
#define SA_RESTART   (1 << 5)
#define SA_SIGINFO   (1 << 6)

typedef _signal_handler_ptr signal_handler_t;
typedef int      sig_atomic_t;
typedef uint64_t sigset_t;

union sigval {            /* Data passed with notification */
    int     sival_int;    /* Integer value */
    void   *sival_ptr;    /* Pointer value */
};

struct sigevent {
    int    sigev_notify;  /* Notification method */
    int    sigev_signo;   /* Notification signal */
    union sigval sigev_value;
                          /* Data passed with notification */
    void (*sigev_notify_function)(union sigval);
                          /* Function used for thread
                             notification (SIGEV_THREAD) */
    void  *sigev_notify_attributes;
                          /* Attributes for notification thread
                             (SIGEV_THREAD) */
    pid_t  sigev_notify_thread_id;
                          /* ID of thread to signal
                             (SIGEV_THREAD_ID); Linux-specific */
};

typedef struct {
    int      si_signo;     /* Signal number */
    int      si_errno;     /* An errno value */
    int      si_code;      /* Signal code */
    int      si_trapno;    /* Trap number that caused
                              hardware-generated signal
                              (unused on most architectures) */
    pid_t    si_pid;       /* Sending process ID */
    uid_t    si_uid;       /* Real user ID of sending process */
    int      si_status;    /* Exit value or signal */
    clock_t  si_utime;     /* User time consumed */
    clock_t  si_stime;     /* System time consumed */

    union sigval si_value; /* Signal value */
    int      si_int;       /* POSIX.1b signal */
    void    *si_ptr;       /* POSIX.1b signal */
    int      si_overrun;   /* Timer overrun count;
                              POSIX.1b timers */
    int      si_timerid;   /* Timer ID; POSIX.1b timers */
    void    *si_addr;      /* Memory location which caused fault */
    long     si_band;      /* Band event (was int in
                              glibc 2.3.2 and earlier) */
    int      si_fd;        /* File descriptor */
    short    si_addr_lsb;  /* Least significant bit of address
                              (since Linux 2.6.32) */
    void    *si_lower;     /* Lower bound when address violation
                              occurred (since Linux 3.19) */
    void    *si_upper;     /* Upper bound when address violation
                              occurred (since Linux 3.19) */
    int      si_pkey;      /* Protection key on PTE that caused
                              fault (since Linux 4.6) */
    void    *si_call_addr; /* Address of system call instruction
                              (since Linux 3.5) */
    int      si_syscall;   /* Number of attempted system call
                              (since Linux 3.5) */
    unsigned int si_arch;  /* Architecture of attempted system call
                              (since Linux 3.5) */
} siginfo_t;

struct sigaction {
    signal_handler_t sa_handler;
    void       (*sa_sigaction)(int, siginfo_t*, void *);
    sigset_t   sa_mask;
    int        sa_flags;
};

signal_handler_t signal(int sig, signal_handler_t handler);
int  raise(int sig);

int  sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);

#endif
