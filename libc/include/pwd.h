#ifndef __PWD_H
#define __PWD_H

#include <kernel/types.h>

struct passwd {
        char    *pw_name;       
        char    *pw_passwd;     
        uid_t    pw_uid;         
        gid_t    pw_gid;         
        char    *pw_gecos;      
        char    *pw_dir;        
        char    *pw_shell;      
};

struct passwd *getpwent(void);
void 		   setpwent(void);
void 		   endpwent(void);
struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);

#endif
