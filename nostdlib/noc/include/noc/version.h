#ifndef NOC_VERSION_H_INCLUDED
#define NOC_VERSION_H_INCLUDED

#include <noc/macros.h>

#if !defined(NOC_VERSION_MAJOR)
    #define NOC_VERSION_MAJOR NOC_EXPAND(0)
#endif
 
#if !defined(NOC_VERSION_MINOR)
    #define NOC_VERSION_MINOR NOC_EXPAND(0)
#endif

#if !defined(NOC_VERSION_PATCH)
    #define NOC_VERSION_PATCH NOC_EXPAND(1)
#endif
    
#if !defined(NOC_VERSION_MICRO)
    #define NOC_VERSION_MICRO NOC_EXPAND(0)
#endif

#endif // NOC_VERSION_H_INCLUDED
