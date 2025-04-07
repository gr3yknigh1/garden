//!
//! FILE          code\base\static_assert.h
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#pragma once

#if !defined(__cplusplus)
    // NOTE(gr3yknigh1): Require C11 [2025/04/07]
    #define STATIC_ASSERT(COND, MSG) _Static_assert((COND), MSG)
#else
    #define STATIC_ASSERT(COND, MSG) static_assert((COND), MSG)
#endif

#define EXPECT_TYPE_SIZE(TYPE, SIZE) STATIC_ASSERT(sizeof(TYPE) == (SIZE), "Expected " #TYPE " to be the size of " #SIZE)

