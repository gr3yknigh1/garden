#include "noc/buf.h"

NOC_Buf_View
noc_buf_view_make(Byte *data, SizeU size)
{
    NOC_Buf_View ret;
    ret.data = data;
    ret.size = size;
    return ret;
}
