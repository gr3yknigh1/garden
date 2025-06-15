#include "noc/io.h"

#include "noc/noc.h"

#include <stdio.h> // Temporarly
#include <stdarg.h>

bool
noc_print(Str8Z format, ...)
{

    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
    
    return true;
}

NOC_Buf_Writer
noc_buf_writer_make(Byte *data, PointerDiff offset, SizeU capacity)
{
    NOC_Buf_Writer writer;

    writer.data = data;
    writer.cursor = data + offset;
    writer.capacity = capacity;

    return writer;
}

NOC_Buf_Writer
noc_buf_writer_make2(NOC_Buf_View bv, PointerDiff offset)
{
    NOC_Buf_Writer writer;

    writer.data = bv.data;
    writer.cursor = bv.data + offset;
    writer.capacity = bv.size;

    return writer;
}

bool
noc_buf_writer_write_str8z(NOC_Buf_Writer *writer, Str8Z s)
{
    SizeU length = noc_str8z_length(s);

    if (length > noc_buf_writer_space_left(writer)) {
        return false;
    }

    SizeU bytes_to_write = length * sizeof(*s);

    noc_memory_copy(writer->cursor, s, length);
    writer->cursor += bytes_to_write;

    return true;
}

bool
noc_buf_writer_write_str8_view(NOC_Buf_Writer *writer, NOC_Str8_View sv)
{
    if (sv.length > noc_buf_writer_space_left(writer)) {
        return false;
    }

    SizeU bytes_to_write = sv.length * sizeof(*sv.data);

    noc_memory_copy(writer->cursor, sv.data, bytes_to_write);
    writer->cursor += bytes_to_write;

    return true;
}

SizeU
noc_buf_writer_bytes_written(const NOC_Buf_Writer *writer)
{
    SizeU result = writer->cursor - writer->data;
    return result;
}

SizeU
noc_buf_writer_space_left(const NOC_Buf_Writer *writer)
{
    return writer->capacity - (writer->cursor - writer->data);
}

bool
noc_buf_writer_write_char8(NOC_Buf_Writer *writer, Char8 c)
{
    if (noc_buf_writer_space_left(writer) == 0) {
        return false;
    }

    *writer->cursor = c;
    writer->cursor += 1;

    return true;
}
