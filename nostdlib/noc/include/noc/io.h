#ifndef NOC_IO_H_INCLUDED
#define NOC_IO_H_INCLUDED

#include <noc/macros.h>
#include <noc/str.h>
#include <noc/types.h>
#include <noc/buf.h>

NOC_DEFINE bool noc_print(Str8Z format, ...);

typedef struct NOC_Buf_Writer {
    Byte *cursor;
    Byte *data;
    SizeU capacity;
} NOC_Buf_Writer;

NOC_DEFINE NOC_Buf_Writer noc_buf_writer_make(Byte *data, PointerDiff offset, SizeU capacity);
NOC_DEFINE NOC_Buf_Writer noc_buf_writer_make2(NOC_Buf_View bv, PointerDiff offset);

NOC_DEFINE SizeU noc_buf_writer_bytes_written(const NOC_Buf_Writer *writer);
NOC_DEFINE SizeU noc_buf_writer_space_left(const NOC_Buf_Writer *writer);
NOC_DEFINE bool noc_buf_writer_write_char8(NOC_Buf_Writer *writer, Char8 c);

NOC_DEFINE bool noc_buf_writer_write_str8z(NOC_Buf_Writer *writer, Str8Z s);
NOC_DEFINE bool noc_buf_writer_write_str8_view(NOC_Buf_Writer *writer, NOC_Str8_View sv);


#endif // NOC_IO_H_INCLUDED
