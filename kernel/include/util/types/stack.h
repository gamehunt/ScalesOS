#ifndef __K_UTIL_TYPES_STACK_H
#define __K_UTIL_TYPES_STACK_H

#define PUSH(sp, type, value) \
    sp -= sizeof(type);       \
    *((type*)sp) = value;     \

#endif