#if !defined(NOC_NET_H_INCLUDED)
#define NOC_NET_H_INCLUDED

#include <noc/buf.h>

typedef struct NOC_Native_Net_State NOC_Native_Net_State;

NOC_DEFINE NOC_NODISCARD bool noc_native_net_state_init(void);
NOC_DEFINE NOC_NODISCARD bool noc_native_net_state_destroy(void);

typedef struct NOC_Native_Socket NOC_Native_Socket;

NOC_DEFINE NOC_NODISCARD NOC_Native_Socket *noc_native_socket_open(Str8Z hostname, Int16U port);

NOC_DEFINE bool noc_native_socket_send(NOC_Native_Socket *socket, NOC_Buf_View buffer);
NOC_DEFINE bool noc_native_socket_recv(NOC_Native_Socket *socket, Byte *buffer, SizeU capacity);

NOC_DEFINE bool noc_native_socket_shutdown(NOC_Native_Socket *socket);
NOC_DEFINE bool noc_native_socket_close(NOC_Native_Socket *socket);

#endif // NOC_NET_H_INCLUDED
