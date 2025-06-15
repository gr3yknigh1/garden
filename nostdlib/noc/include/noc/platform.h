#if !defined(NOC_PLATFORM_H_INCLUDED)
#define NOC_PLATFORM_H_INCLUDED

#include <noc/macros.h>
#include <noc/types.h>

NOC_DEFINE SizeU noc_get_page_size(void);

NOC_DEFINE NOC_NODISCARD void *noc_native_allocate(SizeU size);
NOC_DEFINE NOC_NODISCARD bool  noc_native_free    (void *data, SizeU size);


NOC_DEFINE NOC_NORETURN void noc_exit_process(Int32S exit_code);

#if defined(NOC_LIBC_WRAPPERS)

#include <stdio.h>

NOC_DEFINE SizeU noc_get_file_size(FILE *file);

#endif // NOC_LIBC_WRAPPERS

#endif // NOC_PLATFORM_H_INCLUDED
