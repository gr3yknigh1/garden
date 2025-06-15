#ifndef NOSTDLIB_BITS_H_
#define NOSTDLIB_BITS_H_

#include <noc/io.h>
#include <noc/macros.h>
#include <noc/types.h>

#define NOC_BITS_WRITE(X, FD)                                               \
    do {                                                                       \
        for (Int64U index = sizeof(X) * CHAR_BITS; index > 0; --index) {  \
            noc_write_char(FD, (X) & (1 << (index - 1)) ? '1' : '0');    \
        }                                                                      \
    } while (0)

#define NOC_BITS_PRINT(X) NOC_BITS_WRITE(noc_stdout_fileno, (X))

#define NOC_BITS_PUTS(X)                                                       \
    do {                                                                       \
        NOC_BITS_PRINT(X);                                                    \
        noc_write_char(noc_stdout_fileno, '\n');                               \
    } while (0)

#if !defined(NOC_MAKE_FLAG)
    #define NOC_MAKE_FLAG(INDEX) (1 << (INDEX))
#endif

#if !defined(NOC_HAS_FLAG)
    #define NOC_HAS_FLAG(MASK, FLAG) (((MASK) & (FLAG)) == (FLAG))
#endif

#if !defined(NOC_ADD_FLAG)
    #define NOC_ADD_FLAG(MASK, X) ((MASK) |= (X))
#endif

#if !defined(NOC_DEL_FLAG)
    #define NOC_DEL_FLAG(MASK, X) ((MASK) ^= (X))
#endif

#endif // NOCTDLIB_BITS_H_
