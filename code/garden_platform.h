#if !defined(GARDEN_PLATFORM_H)
//
// FILE          code\platform.h
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//
#define GARDEN_PLATFORM_H

struct Input_State {
    float x_direction;
    float y_direction;
};

struct Platform_Context {
    Input_State input_state;
};

#endif // GARDEN_PLATFORM_H