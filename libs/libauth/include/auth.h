#ifndef __LIB_AUTH_H
#define __LIB_AUTH_H

#include "kernel/types.h"

int  auth_check_credentials(char* name, char* pass);
void auth_setenv(uid_t uid);

#endif
