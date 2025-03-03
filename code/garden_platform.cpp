//
// FILE          code\garden_platform.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//

#if defined(_WIN32)
    #include "garden_platform_win32.cpp"
#else
    #error "Unhandled platform! No runtime was included"
#endif
