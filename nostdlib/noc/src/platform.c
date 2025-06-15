

#include "noc/noc.h"


#if defined(NOC_LIBC_WRAPPERS)

NOC_DEFINE SizeU
noc_get_file_size(FILE *file)
{
    SizeU file_size = 0;

    long position = ftell(file);

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);

    fseek(file, position, SEEK_SET);

    return file_size;
}

#endif // NOC_LIBC_WRAPPERS


#if NOC_DETECT_PLATFORM_WINDOWS

    #include "platform_win32.c"

#endif


#if NOC_DETECT_PLATFORM_LINUX

    #include "platform_linux.c"

#endif
