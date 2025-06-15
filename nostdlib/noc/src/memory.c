#include "noc/memory.h"

#include "noc/macros.h"
#include "noc/types.h"
#include "noc/platform.h"

#include "noc/detect.h"

#include <stdlib.h> // malloc, free

void *
noc_allocate(SizeU size)
{
    void *result = malloc(size);
    return result;
}

void
noc_free(void *p)
{
    free(p);
}

void
noc_memory_zero(void *buffer, SizeU size)
{
    while (size-- > 0) {
        ((char *)buffer)[size] = 0;
    }
}

void
noc_memory_set(void *buffer, SizeU size, Byte value)
{
    while (size-- > 0) {
        ((char *)buffer)[size] = value;
    }
}

void
noc_memory_copy(void *destination, const void *source, SizeU size)
{
    for (SizeU i = 0; i < size; ++i) {
        ((Byte *)destination)[i] = ((Byte *)source)[i];
    }
}

const void *
noc_memory_find(const void *buffer, SizeU size, Byte value)
{
    for (SizeU i = 0; i < size; ++i) {
        if (((Byte *)buffer)[i] == value) {
            return buffer;
        }
    }
    return NULL;
}

SizeU
noc_align_to_page_size(SizeU size)
{
    SizeU page_size = noc_get_page_size();
    SizeU aligned = NOC_ALIGN_TO(size, page_size);
    return aligned;
}

SizeU noc_align_to_page_size(SizeU size);

NOC_NODISCARD NOC_Arena
noc_make_arena(SizeU size)
{
    size = noc_align_to_page_size(size);

    NOC_Arena ret = {0};
    ret.data = noc_allocate(size);
    ret.capacity = size;
    ret.occupied = 0;

    return ret;
}

NOC_NODISCARD void *
noc_arena_alloc(NOC_Arena *arena, SizeU size)
{
    if (arena == NULL || arena->data == NULL) {
        return NULL;
    }

    if (arena->occupied + size > arena->capacity) {
        return NULL;
    }

    void *data = ((Byte *)arena->data) + arena->occupied;
    arena->occupied += size;
    return data;
}

void
noc_destroy_arena(NOC_Arena *arena)
{
    if (arena == NULL || arena->data == NULL) {
        return;
    }

    noc_free(arena->data);

    arena->data = NULL;
    arena->capacity = 0;
    arena->occupied = 0;
}

