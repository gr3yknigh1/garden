
#include <noc/noc.h>


#include <stdlib.h>

int
main(void)
{
    Str8Z message = (Str8Z)noc_allocate(1024);
    noc_memory_zero(message, 1024);

    noc_str8z_format(message, "Hello there! There is '%d' reasons why you are jay!", 10);

    noc_free(message);

    noc_native_net_state_init();

    NOC_Native_Socket *socket = noc_native_socket_open("localhost", 6969);

    if (!socket) {
	NOC_DIE_M("Failed to open a connection!");
    }

    NOC_Buf_View buffer;
    buffer.size = 1024;
    buffer.data = noc_allocate(buffer.size);

    NOC_Http_Request r = {};
    r.status.method = NOC_HTTP_METHOD_GET;
    r.status.target = noc_str8_view_make("/");
    r.status.version = NOC_HTTP_VERSION_1P1;

    noc_http_request_push_header(&r, "User-Agent", "nostdlib/noc 0.0.0 (Windows NT; Win64, x64) Dev");
    noc_http_request_write_to_buffer(&r, buffer);

    noc_native_socket_send(socket, buffer);
    noc_native_socket_recv(socket, NULL, 0);

    noc_free(buffer.data);

    NOC_EMMIT_DEBUG_MESSAGE("Hi hi!");

    noc_native_socket_shutdown(socket);
    noc_native_socket_close(socket);
    noc_native_net_state_destroy();
    NOC_DIE_MF("Got interesting: %d", 120);
}
