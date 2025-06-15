#if !defined(NOC_DEBUG_H_INCLUDED)

#define NOC_DEBUG_H_INCLUDED

#include <noc/platform.h>

#if !defined(NOC_EMMIT_DEBUG_MESSAGE_BUFFER_SIZE)
    #define NOC_EMMIT_DEBUG_MESSAGE_BUFFER_SIZE 1024
#endif

#if !defined(NOC_EMMIT_DEBUG_MESSAGE_ALLOCATE_FUNCTION)
    #define NOC_EMMIT_DEBUG_MESSAGE_ALLOCATE_FUNCTION noc_allocate
#endif

#if !defined(NOC_EMMIT_DEBUG_MESSAGE_ZERO_BUFFER_FUNCTION)
    #define NOC_EMMIT_DEBUG_MESSAGE_ZERO_BUFFER_FUNCTION noc_memory_zero
#endif

#if !defined(NOC_EMMIT_DEBUG_MESSAGE_FREE_FUNCTION)
    #define NOC_EMMIT_DEBUG_MESSAGE_FREE_FUNCTION noc_free
#endif

#if !defined(NOC_EMMIT_DEBUG_MESSAGE_FORMAT_FUNCTION)
    #define NOC_EMMIT_DEBUG_MESSAGE_FORMAT_FUNCTION noc_str8z_format
#endif

#if !defined(NOC_EMMIT_DEBUG_MESSAGE)

// TODO(gr3yknigh1): Add stack_strace, last platform errors and message box [2025/05/30]
#define NOC_EMMIT_DEBUG_MESSAGE(...)					\
    do {								\
	void *__noc_assertion_message_buffer =                          \
	    NOC_EMMIT_DEBUG_MESSAGE_ALLOCATE_FUNCTION(NOC_EMMIT_DEBUG_MESSAGE_BUFFER_SIZE); \
	NOC_EMMIT_DEBUG_MESSAGE_ZERO_BUFFER_FUNCTION(__noc_assertion_message_buffer, NOC_EMMIT_DEBUG_MESSAGE_BUFFER_SIZE); \
	NOC_EMMIT_DEBUG_MESSAGE_FORMAT_FUNCTION(__noc_assertion_message_buffer, __VA_ARGS__); \
	noc_print("%s\n", __noc_assertion_message_buffer);		\
	NOC_EMMIT_DEBUG_MESSAGE_FREE_FUNCTION(__noc_assertion_message_buffer); \
    } while (0)

#endif

#if !defined(NOC_DIE)

    #define NOC_DIE()							\
	do {								\
	    NOC_EMMIT_DEBUG_MESSAGE(					\
		"E: %s:%d@%s - Died.\n", __FILE__, __LINE__, __FUNCTION__); \
	    noc_exit_process(1);					\
	} while (0)

#endif

#if !defined(NOC_DIE_M)

    #define NOC_DIE_M(MESSAGE)						\
	do {								\
	    NOC_EMMIT_DEBUG_MESSAGE(					\
		"E: %s:%d@%s - Died: %s.\n", __FILE__, __LINE__, __FUNCTION__, MESSAGE); \
	    noc_exit_process(1);					\
	} while (0)

#endif

#if !defined(NOC_DIE_MF)

    #define NOC_DIE_MF(FORMAT, ...)					\
	do {								\
	    NOC_EMMIT_DEBUG_MESSAGE(					\
		"E: %s:%d@%s - Died: " FORMAT ".\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
	    noc_exit_process(1);					\
	} while (0)

#endif

#endif // NOC_DEBUG_H_INCLUDED
