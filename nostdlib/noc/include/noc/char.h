#if !defined(NOC_CHAR_H_INCLUDED)
#define NOC_CHAR_H_INCLUDED

#include <noc/macros.h>
#include <noc/types.h>


#define NOC_IS_DIGIT(C) NOC_IN_RANGE('0', C, '9')
#define NOC_IS_ALPHABET(C) NOC_IN_RANGE('a', C, 'Z')
#define NOC_IS_UPPER(C) NOC_IN_RANGE('A', C, 'Z')
#define NOC_IS_LOWER(C) NOC_IN_RANGE('a', C, 'z')

#endif // NOC_CHAR_H_INCLUDED
