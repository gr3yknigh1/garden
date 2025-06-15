
#include "noc/http.h"

#include "noc/noc.h"


bool
noc_http_request_push_header(NOC_Http_Request *request, Str8Z key, Str8Z value)
{
    // TODO(gr3yknigh1): Replace with external allocator [2025/05/30]
    NOC_Http_Header *new_header = (NOC_Http_Header *)noc_allocate(sizeof(NOC_Http_Header));
    noc_memory_zero(new_header, sizeof(NOC_Http_Header));

    if (new_header == NULL) {  
        return false;
    }

    new_header->key = noc_str8_view_make(key);
    new_header->value = noc_str8_view_make(value);

    if (request->headers.head == NULL) {
        request->headers.head = new_header;
    } else {

        NOC_Http_Header *last_header = request->headers.head; // TODO(gr3yknigh1): Wrap it in macro [2025/05/30]
        while (last_header->next != NULL) {
            last_header = last_header->next;
        }
        last_header->next = new_header;

    }

    request->headers.count += 1;

    return true;
}

bool
noc_http_request_write_to_buffer(NOC_Http_Request *request, NOC_Buf_View buffer)
{
    NOC_Buf_Writer w = noc_buf_writer_make2(buffer, 0);

    if (request->status.method == NOC_HTTP_METHOD_GET) {
        if (!noc_buf_writer_write_str8z(&w, NOC_HTTP_METHOD_GET_S)) {
            return false;
        }
    }

    if (!noc_buf_writer_write_char8(&w, ' ')) {
        return false;
    }

    if (!noc_buf_writer_write_str8_view(&w, request->status.target)) {
        return false;
    }

    if (!noc_buf_writer_write_char8(&w, ' ')) {
        return false;
    }

    if (request->status.version == NOC_HTTP_VERSION_1P0) {
        if (!noc_buf_writer_write_str8z(&w, NOC_HTTP_VERSION_1P0_S)) {
            return false;
        }
    } else if (request->status.version == NOC_HTTP_VERSION_1P1) {
        if (!noc_buf_writer_write_str8z(&w, NOC_HTTP_VERSION_1P1_S)) {
            return false;
        }
    }

    if (!noc_buf_writer_write_str8z(&w, NOC_HTTP_NEWLINE)) {
        return false;
    }

    NOC_Http_Header *current_header = request->headers.head;

    while (current_header != NULL) {
        if (!noc_buf_writer_write_str8_view(&w, current_header->key)) {
            return false;
        }

        if (!noc_buf_writer_write_str8z(&w, ": ")) {
            return false;
        }

        if (!noc_buf_writer_write_str8_view(&w, current_header->value)) {
            return false;
        }

        if (!noc_buf_writer_write_str8z(&w, NOC_HTTP_NEWLINE)) {
            return false;
        }

        current_header = current_header->next;
    }

    if (!noc_buf_writer_write_str8z(&w, NOC_HTTP_NEWLINE)) {
        return false;
    }

    return true;
}
