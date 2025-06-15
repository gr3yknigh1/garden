
#include "noc/platform.h"

#if !defined(UNICODE)
    #define UNICODE
#endif

#if !defined (_UNICODE)
    #define _UNICODE
#endif

#if !defined (NOMINMAX)
    #define NOMINMAX
#endif

#if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
#endif


#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include "noc/noc.h"

#pragma comment(lib, "Ws2_32.lib")  // TODO(gr3yknigh1): move this to build system [2025/05/30]

typedef struct NOC_Native_Net_State {
    // TODO(gr3yknigh1): Syncronize [2025/05/30]
    bool was_initialized;
    WSADATA wsa;
} NOC_Native_Net_State;

typedef struct NOC_Native_Socket {
    SOCKET handle;
} NOC_Native_Socket;

static NOC_Native_Net_State global_native_net_state;


SizeU
noc_get_page_size(void)
{
    SizeU result = 0;

    SYSTEM_INFO system_info = {0};
    GetSystemInfo(&system_info);
    result = system_info.dwPageSize;

    return result;
}


NOC_NODISCARD void *
noc_native_allocate(SizeU size)
{
    void *data = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    //                              ^^^^
    // NOTE(ilya.a): So, here I am reserving `size` amount of bytes, but
    // accually `VirtualAlloc` will round up this number to next page.
    // [2024/05/26]
    // TODO(ilya.a): Do something about waste of unused memory in Arena.
    // [2024/05/26]
    return data;
}

NOC_NODISCARD bool
noc_native_free(void *data, SizeU size)
{
    if (VirtualFree(data, size, MEM_RELEASE) == 0) {
        //                      ^^^^^^^^^^^
        // NOTE(ilya.a): Might be more reasonable to use MEM_DECOMMIT instead
        // for MEM_RELEASE. Because in that case it's will be keep buffer
        // around, until we use it again. P.S. Also will be good to try protect
        // buffer after deallocating or other stuff.
        //
        return false;
    }
    return true;
}

NOC_NORETURN void
noc_exit_process(Int32S exit_code)
{
    ExitProcess(exit_code);
}


NOC_NODISCARD bool
noc_native_net_state_init(void)
{
    if (global_native_net_state.was_initialized) {
	return true;
    }

    noc_memory_zero(&global_native_net_state.wsa, sizeof(global_native_net_state.wsa));

    int result = WSAStartup(MAKEWORD(2, 2), &global_native_net_state.wsa);

    if (result != 0) {
	// TODO(gr3yknigh1): Crash [2025/05/30]
	return false;
    }

    global_native_net_state.was_initialized = true;

    return true;
}

NOC_NODISCARD bool
noc_native_net_state_destroy(void)
{
    WSACleanup();
    return true;
}

NOC_NODISCARD NOC_Native_Socket *
noc_native_socket_open(Str8Z hostname, Int16U port)
{
    int result = 0;

    Char8 port_format_buffer[5] = {0};
    noc_str8z_format(port_format_buffer, "%hd", port);

    struct addrinfo addrinfo_hints;
    noc_memory_zero(&addrinfo_hints, sizeof(addrinfo_hints));
    addrinfo_hints.ai_family = AF_INET;
    addrinfo_hints.ai_socktype = SOCK_STREAM;
    addrinfo_hints.ai_protocol = IPPROTO_TCP;
    addrinfo_hints.ai_flags = AI_PASSIVE;

    struct addrinfo *addrinfo_result = NULL;

    // TODO(gr3yknigh1): Add support for wchar_t [2025/01/03]
    result = GetAddrInfoA(hostname, port_format_buffer, &addrinfo_hints, &addrinfo_result);

    if (result != 0) {
        FreeAddrInfoA(addrinfo_result);
        return NULL;
    }

    SOCKET socket_handle = INVALID_SOCKET;

    for (struct addrinfo *it = addrinfo_result; it != NULL; it = it->ai_next) {
        socket_handle = socket(it->ai_family, it->ai_socktype, it->ai_protocol);

        if (socket_handle == INVALID_SOCKET) {
            break;
        }

        result = connect(socket_handle, it->ai_addr, (int)(it->ai_addrlen));

        if (result != SOCKET_ERROR) {
            break;
        }

        result = WSAGetLastError(); // NOTE(gr3yknigh1): For debugging inspection [2025/01/04]

        // TODO(gr3yknigh1): check here for WSAECONNREFUSED(10061) error code [2025/01/05]

        closesocket(socket_handle);
        socket_handle = INVALID_SOCKET;
    }

    if (socket_handle == INVALID_SOCKET) {
        FreeAddrInfoA(addrinfo_result);
        return NULL;
    }

    NOC_Native_Socket *socket = (NOC_Native_Socket *)noc_allocate(sizeof(NOC_Native_Socket));
    socket->handle = socket_handle;

    return socket;
}

bool
noc_native_socket_shutdown(NOC_Native_Socket *socket)
{
    int result = shutdown(socket->handle, SD_SEND); // XXX
    return result == 0;
}

bool
noc_native_socket_close(NOC_Native_Socket *socket)
{
    closesocket(socket->handle);  // TODO:

    noc_free(socket);
    return true;
}

bool
noc_native_socket_send(NOC_Native_Socket *socket, NOC_Buf_View buffer)
{
    const Byte *data_cursor = buffer.data;
    SizeU remain_bytes = buffer.size;

    while (remain_bytes > 0) {
        int sent = send(socket->handle, data_cursor, remain_bytes, 0);
        if (sent == SOCKET_ERROR) {
            return false;
	}

        data_cursor += sent;
        remain_bytes -= sent;
    }

    return true;
}

bool
noc_native_socket_recv(NOC_Native_Socket *socket, Byte *buffer, SizeU capacity)
{
    int result = 0;

    Byte *buffer_end  = buffer;

    do {
	SizeU space_left = capacity - (buffer_end - buffer);

	result = recv(socket->handle, buffer_end, space_left, 0);

	if (result < 0) {
	    return false;
	}

	buffer_end = buffer_end + result;

	if (capacity == 0 && result == 0) {
	    break;
	}

	if (result == space_left) {
	    // TODO(gr3yknigh1): reallocate! [2025/05/31]
	    NOC_DIE_M("Reallocation of the buffer isn't implemented on socket recv.");
	}
    
    } while (result > 0);

    return result == 0;
}
