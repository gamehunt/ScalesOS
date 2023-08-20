#ifndef __K_UTIL_TYPES_STACK_H
#define __K_UTIL_TYPES_STACK_H

#define PUSH(sp, type, value) \
    sp -= sizeof(type);       \
    *((type*)sp) = value;     \

#define POP(sp, type, value) \
	value = *((type*) sp);   \
	sp  += sizeof(type);     \

#endif
