#if !defined(NOC_HTTP_H_INCLUDED)

#define NOC_HTTP_H_INCLUDED

#include <noc/str.h>
#include <noc/buf.h>

typedef enum NOC_Http_Version {
    NOC_HTTP_VERSION_1P0 = 1,
    NOC_HTTP_VERSION_1P1 = 2,
} NOC_Http_Version;

#define NOC_HTTP_VERSION_1P0_S "HTTP/1.0"
#define NOC_HTTP_VERSION_1P1_S "HTTP/1.1"

typedef enum NOC_Http_Method {
    NOC_HTTP_METHOD_GET,
    NOC_HTTP_METHOD_HEAD,
    NOC_HTTP_METHOD_POST,
    NOC_HTTP_METHOD_PUT,
    NOC_HTTP_METHOD_DELETE,
    NOC_HTTP_METHOD_CONNECT,
    NOC_HTTP_METHOD_TRACE,
    NOC_HTTP_METHOD_PATCH,
} NOC_Http_Method;

#define NOC_HTTP_METHOD_GET_S "GET"

typedef struct NOC_Http_Header {
    struct NOC_Http_Header *next;
    NOC_Str8_View key;
    NOC_Str8_View value;
} NOC_Http_Header;

typedef struct NOC_Http_Status_Line {
    NOC_Http_Method method;
    NOC_Str8_View target;
    NOC_Http_Version version;
} NOC_Http_Status_Line;

typedef struct NOC_Http_Request {
    NOC_Http_Status_Line status;

    struct {
	NOC_Http_Header *head;
	Int64U count;
    } headers;
} NOC_Http_Request;

NOC_DEFINE bool noc_http_request_push_header(NOC_Http_Request *request, Str8Z key, Str8Z value);

NOC_DEFINE bool noc_http_request_write_to_buffer(NOC_Http_Request *request, NOC_Buf_View buffer);

typedef enum NOC_Http_Status_Code {
    NOC_HTTP_STATUS_200_OK = 200,

    NOC_HTTP_STATUS_301_MOVED = 301,

    NOC_HTTP_STATUS_400_BAD_REQUEST = 400,
    NOC_HTTP_STATUS_403_FORBIDDEN = 403,
} NOC_Http_Status_Code;

#define NOC_HTTP_STATUS_200_OK_S "OK"
#define NOC_HTTP_STATUS_301_MOVED_S "Moved"
#define NOC_HTTP_STATUS_400_BAD_REQUEST_S "Bad Request"
#define NOC_HTTP_STATUS_403_FORBIDDEN_S "Forbidden"

typedef struct NOC_Http_Response {
    NOC_Http_Version version;
    NOC_Http_Status_Code status_code;
    NOC_Str8_View status_text;

    struct {
	NOC_Http_Header *head;
	Int64U count;
    } headers;

    NOC_Buf_View buffer;
} NOC_Http_Response;

#define NOC_HTTP_NEWLINE "\r\n"
#define NOC_HTTP_DOUBLE_NEWLINE "\r\n\r\n"

#endif // NOC_HTTP_H_INCLUDED
