#ifndef NOC_MATH_ROUND_H_INCLUDED
#define NOC_MATH_ROUND_H_INCLUDED

#include <noc/macros.h>
#include <noc/types.h>

// NOC_DEF f32 noc_f32_round(f32 x);
// NOC_DEF f64 noc_f64_round(f64 x);

#include <math.h>

#define NOC_ROUND(__X)                                                         \
    _Generic((__X), Float32 : roundf, Float64 : round, default : roundf)(__X)

#endif // NOC_MATH_ROUND_H_INCLUDED
