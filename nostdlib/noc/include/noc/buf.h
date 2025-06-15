#if !defined(NOC_BUF_H_INCLUDED)

#define NOC_BUF_H_INCLUDED

#include <noc/macros.h>
#include <noc/types.h>

typedef struct NOC_Buf_View {
    Byte *data;
    SizeU size;
} NOC_Buf_View;

NOC_DEFINE NOC_Buf_View noc_buf_view_make(Byte *data, SizeU size);

#endif // NOC_BUF_H_INCLUDED
