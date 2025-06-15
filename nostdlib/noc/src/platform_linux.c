
#include <unistd.h>  // getpagesize
#include <sys/mman.h>  // mmap, munmap

#include <netinet/in.h> // ...
#include <netdb.h>


#include "noc/noc.h"


// NOTE(gr3yknigh1): This is just dummy in order to support API like in WSA on Windows. :clown:
// [2025/06/01]
typedef struct NOC_Native_Net_State {
    bool was_initialized;
} NOC_Native_Net_State;

typedef struct NOC_Native_Socket {
    int descriptor;
} NOC_Native_Socket;

static NOC_Native_Net_State global_native_net_state;


SizeU
noc_get_page_size(void)
{
    SizeU result = 0;

    result = getpagesize();

    return result;
}

NOC_NODISCARD void *
noc_native_allocate(SizeU size)
{
    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    return data;
}

NOC_NODISCARD bool
noc_native_free(void *data, SizeU size)
{
    int result = munmap(data, size);
    return result == 0;
}

NOC_NORETURN void
noc_exit_process(Int32S exit_code)
{
    _exit(exit_code);
}

NOC_NODISCARD bool
noc_native_net_state_init(void)
{
    return true;
}

NOC_NODISCARD bool
noc_native_net_state_destroy(void)
{
    return true;
}

NOC_NODISCARD NOC_Native_Socket *
noc_native_socket_open(Str8Z hostname, Int16U port)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct hostent *host = gethostbyname(hostname);

    struct sockaddr_in host_addr;
    noc_memory_zero(&host_addr, sizeof(host_addr));

    host_addr.sin_family = AF_INET;
    noc_memory_copy(host->h_addr, &host_addr.sin_addr.s_addr, host->h_length);
    host_addr.sin_port = htons(port);

    if (connect(socket_fd, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
	return NULL;
    }

    NOC_Native_Socket *socket = noc_allocate(sizeof(NOC_Native_Socket));
    socket->descriptor = socket_fd;

    return socket;
}

bool
noc_native_socket_shutdown(NOC_Native_Socket *socket)
{
    int result = shutdown(socket->descriptor, SHUT_RD); // XXX
    return result == 0;
}

bool
noc_native_socket_close(NOC_Native_Socket *socket)
{
    close(socket->descriptor); 

    noc_free(socket);
    return true;
}

// TODO(gr3yknigh1): Rewrite noc_native_socket_recv so that it would be just thin wrapper around platform-specific routine [2025/06/01]
bool
noc_native_socket_recv(NOC_Native_Socket *socket, Byte *buffer, SizeU capacity)
{
    int result = 0;

    Byte *buffer_end  = buffer;

    do {
	SizeU space_left = capacity - (buffer_end - buffer);

	result = recv(socket->descriptor, buffer_end, space_left, 0);

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


bool
noc_native_socket_send(NOC_Native_Socket *socket, NOC_Buf_View buffer)
{
    const Byte *data_cursor = buffer.data;
    SizeU remain_bytes = buffer.size;

    while (remain_bytes > 0) {
        int sent = send(socket->descriptor, data_cursor, remain_bytes, 0);
        if (sent == -1) {
            return false;
	}

        data_cursor += sent;
        remain_bytes -= sent;
    }

    return true;
}
