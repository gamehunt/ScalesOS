#ifndef __SHARED_H
#define __SHARED_H

#define C_HDR_START extern "C" {
#define C_HDR_END }

#define UNUSED __attribute__((unused))

#define ALIGN(value, alignment) \
    (((value) + (alignment)) & ~(((typeof(value))(alignment)) - 1))

#define KB(v) ((v)*1024)
#define MB(v) (KB((v)) * 1024)

#define UPPER(data) \
    (data & 0xFFFF0000) >> 16

#define LOWER(data) \
    (data & 0xFFFF)

#define CONCAT(lower, upper) \
    ((lower) >> 16)


#endif
