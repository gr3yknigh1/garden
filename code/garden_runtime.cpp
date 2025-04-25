//
// FILE          code\garden_platform.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//

#include <ctype.h>  // isspace

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <memory>
#include <list>
#include <unordered_map>
#include <thread>

#include "garden_runtime.h"
#include "media/aseprite.cpp"

U32
generate_rect(Vertex *vertexes, F32 x, F32 y, F32 width, F32 height, Color4 color)
{
    U32 count = 0;

    // bottom-left triangle
    vertexes[count++] = {x + 0, y + 0, 0, 0, pack_rgba_to_int(color.r, color.g, color.b, color.a)};          // bottom-left
    vertexes[count++] = {x + width, y + 0, 1, 0, pack_rgba_to_int(color.r, color.g, color.b, color.a)};      // bottom-right
    vertexes[count++] = {x + width, y + height, 1, 1, pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // top-right

    // top-right triangle
    vertexes[count++] = {x + 0, y + 0, 0, 0, pack_rgba_to_int(color.r, color.g, color.b, color.a)};          // bottom-left
    vertexes[count++] = {x + width, y + height, 1, 1, pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // top-right
    vertexes[count++] = {x + 0, y + height, 0, 1, pack_rgba_to_int(color.r, color.g, color.b, color.a)};

    return count;
}

// TODO(gr3yknigh1): Separate configuration at atlas and mesh size [2025/02/26]
U32
generate_rect_with_atlas(
    Vertex *vertexes, F32 x, F32 y, F32 width, F32 height, Rect_F32 location, Atlas *atlas, Color4 color)
{
    U32 count = 0;

    // bottom-left triangle
    vertexes[count++] = {
        x + 0, y + 0, (location.x + 0) / atlas->x_pixel_count, (location.y + 0) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // bottom-left
    vertexes[count++] = {
        x + width, y + 0, (location.x + location.width) / atlas->x_pixel_count, (location.y + 0) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // bottom-right
    vertexes[count++] = {
        x + width, y + height, (location.x + location.width) / atlas->x_pixel_count, (location.y + location.height) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // top-right

    // top-right triangle
    vertexes[count++] = {
        x + 0, y + 0, (location.x + 0) / atlas->x_pixel_count, (location.y + 0) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // bottom-left
    vertexes[count++] = {
        x + width, y + height, (location.x + location.width) / atlas->x_pixel_count, (location.y + location.height) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)}; // top-right
    vertexes[count++] = {
        x + 0, y + height, (location.x + 0) / atlas->x_pixel_count, (location.y + location.height) / atlas->y_pixel_count,
        pack_rgba_to_int(color.r, color.g, color.b, color.a)};

    return count;
}

U32
generate_geometry_from_tilemap(
    Vertex *vertexes, U32 vertexes_capacity,
    Tilemap *tilemap, F32 origin_x, F32 origin_y,
    Color4 color, Atlas *atlas)
{
    assert(vertexes && tilemap && atlas);

    U32 vertex_count = 0;

    for (int col_index = 0; col_index < tilemap->col_count; ++col_index) {
        for (int row_index = 0; row_index < tilemap->row_count; ++row_index) {

            //! @todo(gr3yknigh1): Improve error handling [2025/04/24] #refactor #error_handling
            assert(vertex_count < vertexes_capacity);


            F32 tile_x = origin_x + col_index * 100; // tilemap->tile_x_pixel_count;
            F32 tile_y = origin_y + row_index * 100; // tilemap->tile_y_pixel_count;

            int tile_index_offset = get_offset_from_coords_of_2d_grid_array_rm(static_cast<int>(tilemap->col_count), col_index, row_index);
            F32 tile_index = static_cast<F32>(tilemap->indexes[tile_index_offset]);

            Rect_F32 tile_location{};
            tile_location.x = floorf(tile_index / tilemap->col_count) * static_cast<F32>(tilemap->tile_x_pixel_count);
            tile_location.y = floorf(fmodf(tile_index, static_cast<F32>(tilemap->col_count))) * static_cast<F32>(tilemap->tile_y_pixel_count);
            tile_location.width = static_cast<F32>(tilemap->tile_x_pixel_count);
            tile_location.height = static_cast<F32>(tilemap->tile_y_pixel_count);

            vertex_count += generate_rect_with_atlas(
                vertexes + vertex_count, tile_x, tile_y, 100, 100
                /* static_cast<F32>(x_pixel_count), static_cast<F32>(y_pixel_count) */,
                tile_location, atlas, color);
        }
    }

    return vertex_count;
}

int
get_offset_from_coords_of_2d_grid_array_rm(int width, int x, int y)
{
    return width * y + x;
}

Static_Arena
make_static_arena(Size capacity)
{
    Static_Arena arena;

    arena.data     = allocate(capacity);
    arena.capacity = capacity;
    arena.occupied = 0;

    return arena;
}

Lexer
make_lexer(char *buffer, size_t buffer_size)
{
    Lexer lexer;

    lexer.cursor = buffer;
    lexer.lexeme = *buffer;

    lexer.buffer = buffer;
    lexer.buffer_size = buffer_size;

    return lexer;
}

void
lexer_advance(Lexer *lexer, int32_t count)
{
    size_t cursor_index = lexer->cursor - lexer->buffer;

    if (lexer_is_end(lexer) && cursor_index + count < lexer->buffer_size) {
        return;
    }

    while (count > 0) {
        lexer->cursor++;
        --count;
    }

    lexer->lexeme = *(lexer->cursor);
}

char
lexer_peek(Lexer *lexer, int32_t offset)
{
    size_t cursor_index = lexer->cursor - lexer->buffer;

    // TODO(gr3yknigh1): Maybe make it signed? [2025/02/23]
    if (cursor_index + offset < lexer->buffer_size) {
        char *peek = lexer->cursor + offset;
        return *peek;
    }

    return 0;
}

Str8_View
lexer_peek_view(Lexer *lexer, int32_t offset)
{
    size_t cursor_index = lexer->cursor - lexer->buffer;

    // TODO(gr3yknigh1): Maybe make it signed? [2025/02/23]
    if (cursor_index + offset < lexer->buffer_size) {
        if (offset < 0) {
            return Str8_View(lexer->cursor + offset, -offset);
        } else if (offset > 0) {
            return Str8_View(lexer->cursor, offset);
        }
    }

    return Str8_View();  // do a better job next time
}

bool
lexer_check_peeked(Lexer *lexer, const char *s)
{
    Str8_View sv{s};
    return lexer_check_peeked(lexer, sv);
}

bool
lexer_check_peeked(Lexer *lexer, Str8_View sv)
{
    Str8_View peek_view{lexer_peek_view(lexer, static_cast<int32_t>(sv.length))};
    return str8_view_is_equals(peek_view, sv);
}

Str8_View
lexer_skip_until(Lexer *lexer, char c)
{
    Str8_View ret{};

    ret.data = lexer->cursor;

    while (!lexer_is_end(lexer) && lexer->lexeme != c) {
        lexer_advance(lexer);
        ret.length++;
    }

    return ret;
}

Str8_View
lexer_skip_until(Lexer *lexer, Str8_View sv)
{
    Str8_View ret{};

    if (sv.empty()) {
        return ret;
    }

    ret.data = lexer->cursor;

    while (!lexer_is_end(lexer) && !lexer_check_peeked(lexer, sv)) {
        lexer_advance(lexer);
        ret.length++;
    }

    return ret;
}


Str8_View
lexer_skip_until_endline(Lexer *lexer)
{
    Str8_View ret;

    ret.data = lexer->cursor;

    bool is_crlf = false;

    while (!lexer_is_end(lexer) && !lexer_is_endline(lexer, &is_crlf)) {
        lexer_advance(lexer);
        ret.length++;
    }

    if (is_crlf) {
        lexer_advance(lexer);
        lexer_advance(lexer);
    } else {
        lexer_advance(lexer);
    }

    return ret;
}

bool
lexer_is_endline(Lexer *lexer, bool *is_crlf)
{
    if (is_crlf != nullptr) {
        *is_crlf = (lexer->cursor[0] == '\r' && lexer->cursor[1] == '\n');
    }
    return lexer->lexeme == '\n' || (lexer->cursor[0] == '\r' && lexer->cursor[1] == '\n');
}

void
lexer_skip_whitespace(Lexer *lexer)
{
    while (!lexer_is_end(lexer) && isspace(lexer->lexeme)) {
        lexer_advance(lexer);
    }
}

bool
lexer_is_end(Lexer *lexer)
{
    size_t cursor_index = lexer->cursor - lexer->buffer;
    return cursor_index >= lexer->buffer_size;
}


bool
lexer_parse_int(Lexer *lexer, int *result)
{
    // TODO(gr3yknigh1): Error handling [2025/02/23]

    int num = 0;

    bool is_negative = false;

    if (lexer->lexeme == '-') {
        is_negative = true;
        lexer_advance(lexer);
    }

    while (lexer->lexeme && (lexer->lexeme >= '0' && lexer->lexeme <= '9')) {
        num = num * 10 + (lexer->lexeme - '0');
        lexer_advance(lexer);
    }

    if (is_negative) {
        num = -1 * num;
    }

    *result = num;

    return true;
}

bool
lexer_parse_str_to_view(Lexer *lexer, Str8_View *sv)
{
    if (lexer->lexeme != '"') {
        return false;
    }

    // NOTE(gr3yknigh1): Skipping '"' [2025/02/24]
    lexer_advance(lexer); // TODO(gr3yknigh1): Wrap in `Lexer_AdvanceIf(Lexer *, int32_t count, char c)`

    *sv = lexer_skip_until(lexer, '"');

    // NOTE(gr3yknigh1): Skipping '"' [2025/02/24]
    lexer_advance(lexer); // TODO(gr3yknigh1): Wrap in `lexer_advance_if(Lexer *, int32_t count, char c)`

    return true;
}

bool
lexer_check_peeked_and_advance(Lexer *lexer, Str8_View sv)
{
    if (lexer_check_peeked(lexer, sv)) {
        lexer_advance(lexer, sv.length); // @cleanup
        return true;
    }
    return false;
}


#if 0
// TODO(gr3yknigh1): Reuse for stack [2025/04/07]
bool
mm::arena_pop(mm::Arena *arena, void *data)
{
    if (arena == nullptr || data == nullptr) {
        return false;
    }

    if (arena->data == nullptr || arena->occupied < sizeof(arena->occupied) || arena->capacity == 0) {
        return false;
    }

    size_t *data_size = static_cast<size_t *>(data) - 1;

    assert(static_cast<mm::byte *>(data) + *data_size == static_cast<mm::byte *>(arena->data) + arena->occupied);
    arena->occupied -= *data_size;
    arena->occupied -= sizeof(*data_size);

    return true;
}
#endif

Size
reset(Static_Arena *arena)
{
    Size was_occupied = arena->occupied;
    arena->occupied = 0;
    return was_occupied;
}

void *
allocate(Static_Arena *arena, Size size, Allocate_Options options)
{
    if (arena == nullptr) {
        return nullptr;
    }

    if (arena->occupied + size > arena->capacity) {
        return nullptr;
    }

    void *allocated = get_offset(arena->data, arena->occupied);

    if (HAS_FLAG(options, ALLOCATE_ZERO_MEMORY)) {
        zero_memory(allocated, size);
    }

    arena->occupied += size;
    return allocated;
}

bool
destroy(Static_Arena *arena)
{
    bool result = deallocate(arena->data);
    zero_struct<Static_Arena>(arena);
    return result;
}

void
zero_memory(void *p, Size size)
{
    std::memset(p, 0, size);
}

void
copy_memory(void *dst, const void *src, Size size)
{
    std::memcpy(dst, src, size);
}

static inline void *
allocate_impl(Size size, Allocate_Options options)
{
    void *result = std::malloc(size);
    if (result && HAS_FLAG(options, ALLOCATE_ZERO_MEMORY)) {
        zero_memory(result, size);
    }
    return result;
}

std::list<Allocation_Record> *
get_allocation_records(void)
{
    static std::list<Allocation_Record> allocation_records{};
    return &allocation_records;
}

bool
dump_allocation_records(bool do_hex_dump)
{
    std::list<Allocation_Record> *records = get_allocation_records();

    Size total_memory = 0;

    for (const Allocation_Record &record : *records) {
        printf("Allocation_Record(options=(%d) size=(%lld) result=(%p) location.file_name=(%s) location.line=(%u) location.function_name=(%s))\n",
            record.options, record.size, record.result, record.location.file_name, record.location.line, record.location.function_name
        );

        total_memory += record.size;

        if (do_hex_dump) {
            hex_dump(record.result, record.size);
            puts("");
        }
    }

    printf("Total memory: %lld bytes\n", total_memory);

    return total_memory > 0;
}

#if defined(GARDEN_TRACK_ALLOCATIONS)
void *
allocate(size_t size, Allocate_Options options, [[maybe_unused]] Source_Location location)
{
    void *result = allocate_impl(size, options);

    Allocation_Record record {options, size, result, location};

    std::list<Allocation_Record> *allocation_records = get_allocation_records();
    allocation_records->push_back(record);

    return result;
}
#else
void *
allocate(size_t size, Allocate_Options options)
{
    return allocate_impl(size, options);
}
#endif

bool
deallocate(void *p)
{

#if defined(GARDEN_TRACK_ALLOCATIONS)

    std::list<Allocation_Record> *allocation_records = get_allocation_records();
    assert(allocation_records->remove_if([p](Allocation_Record &record) {
        return record.result == p;
    }) == 1);

#endif

    // TODO(gr3yknigh1): Use platform functions for allocations [2025/04/07]
    std::free(p);
    return true;
}

Block_Allocator
make_block_allocator(void)
{
    Block_Allocator result;
    zero_struct(&result);
    return result;
}

Block_Allocator
make_block_allocator(U64 blocks_count, Size block_size, Size block_fixed_size, U64 block_count_limit)
{
    assert(blocks_count && block_size);

    if (block_count_limit > 0) {
        assert(blocks_count <= block_count_limit);
    }

    if (block_fixed_size > 0) {
        assert(block_size == block_fixed_size);
    }

    Block_Allocator result;
    zero_struct(&result);

    result.block_fixed_size = block_fixed_size;
    result.block_count_limit = block_count_limit;
    result.blocks.count = blocks_count;

    // TODO(gr3yknigh1): Allocate each block near the actuall data? [2025/03/09]

    if (block_count_limit) {
        result.blocks.head = allocate_structs<Block>(blocks_count);
        zero_structs(result.blocks.head, blocks_count);
    } else {
        result.blocks.head = allocate_struct<Block>();
        zero_struct(result.blocks.head);
    }
    assert(result.blocks.head);

    Byte *data_cursor = nullptr;
    if (block_fixed_size) {
        data_cursor = static_cast<Byte *>(allocate(blocks_count * block_size));
    }

    Block *current_block = result.blocks.head;

    for (U64 block_index = 0; block_index < blocks_count; ++block_index) {

        if (block_fixed_size) {
            current_block->stack = make_stack_view(data_cursor, block_size);
        } else {
            current_block->stack = make_stack_view(block_size);
        }

        if (block_index == blocks_count - 1) {
            result.blocks.tail = current_block;
            continue;
        }

        Block *next_block = nullptr;

        if (block_count_limit) {
            next_block = result.blocks.head + block_index + 1;
        } else {
            next_block = allocate_struct<Block>();
        }

        current_block->next = next_block;
        next_block->previous = current_block;

        if (block_fixed_size) {
            data_cursor += block_size;
        }

        current_block = current_block->next;
    }

    return result;
}



void *
allocate(Block_Allocator *allocator, Size size, Allocate_Options options)
{
    assert(allocator && size);

    if (allocator->block_fixed_size != 0) {
        assert(size == allocator->block_fixed_size);
    }

    FOR_LINKED_LIST(it, &allocator->blocks) {
        void *result = allocate(&it->stack, size);
        if (result != nullptr) {

            if (HAS_FLAG(options, ALLOCATE_ZERO_MEMORY)) {
                zero_memory(result, size);
            }

            return result;
        }
    }

    //
    // Block count limit is exceeded
    //
    if (allocator->block_count_limit && allocator->block_fixed_size) {
        //                              ^^^^^^^^^^^^^^^^^^^^^^^^^^^
        // TODO(gr3yknigh1): Wrap `Block` in allocation-chunks (likewise data blobs). Because we can't free
        // such blocks and semantics can allow only make `Pool`-like allocations or variable-size blocks, where each
        // block and data is a separate allocations. [2025/03/10]
        //
        // TODO(gr3yknigh1): Try make block and data-blob allocation joined, when only `block_fixed_size` parameter is
        // specified [2025/03/10]
        //

        // All blocks should be preallocated
        assert(allocator->blocks.count == allocator->block_count_limit);
        return nullptr;
    }

    //
    // NOTE(gr3yknigh1): Can't pre-allocate more blocks, because semantics will not allow specify block_size
    // for preallocated blocks for grow. We need add function like `block_allocator_pre_allocate(block_size, block_count);` in order to make it more compact.
    // [2025/03/10]
    //
    Block *new_block = allocate_struct<Block>();
    zero_struct(new_block);

    if (allocator->blocks.count > 0) {
        allocator->blocks.tail->next = new_block;
    } else {
        allocator->blocks.head = new_block;
    }

    allocator->blocks.tail = new_block;
    allocator->blocks.count++;

    if (allocator->block_fixed_size) {
        new_block->stack = make_stack_view(allocator->block_fixed_size);
    } else {
        new_block->stack = make_stack_view(page_align(size));
    }

    void *result = allocate(&new_block->stack, size);

    if (result && HAS_FLAG(options, ALLOCATE_ZERO_MEMORY)) {
        zero_memory(result, size);
    }

    return result;
}


bool
destroy_block_allocator(Block_Allocator *allocator)
{
    assert(allocator);

    if (!allocator->blocks.count) {
        return true;
    }

    assert((allocator->block_fixed_size && allocator->block_count_limit)
        || (!allocator->block_fixed_size && !allocator->block_count_limit));

    bool result = true;

    if (allocator->block_fixed_size) {
        result = deallocate( static_cast<void *>( allocator->blocks.head->stack.data ) );
        if (result) {
            result = deallocate( static_cast<void *>( allocator->blocks.head ) );
        }
    } else {

        FOR_LINKED_LIST(it, &allocator->blocks) {
            result = deallocate(it->stack.data);

            if (result && it->previous != nullptr) {
                result = deallocate( static_cast<void *>( it->previous ) );
            }

            if (result && it->next == nullptr) {
                result = deallocate( static_cast<void *>( it ) );
                break;
            }

            if (!result) {
                break;
            }
        }
    }


    return result;
}

bool
can_hold(Stack_View *view, Size size)
{
    assert(view && size);
    bool result = (size + view->occupied) <= view->capacity;
    return result;
}

void *
allocate(Stack_View *view, Size size)
{
    if (!can_hold(view, size)) {
        return nullptr;
    }

    void *result = get_offset(view->data, view->occupied);
    view->occupied += size;
    return result;
}

Stack_View
make_stack_view(void *data, Size capacity)
{
    Stack_View result;
    result.data = data;
    result.capacity = capacity;
    result.occupied = 0;

    return result;
}

Stack_View
make_stack_view(Size capacity)
{
    Stack_View result;
    result.data = allocate(capacity);
    result.capacity = capacity;
    result.occupied = 0;
    return result;
}

Size
align(Size size, Size alignment)
{
    Size result =  size + (alignment - size % alignment);
    return result;
}

Size
page_align(Size size)
{
    Size page_size = get_page_size();

    Size result = align(size, page_size);
    return result;
}

bool
reset(Stack_View *view)
{
    view->occupied = 0;
    return true;
}

bool
reset(Block_Allocator *allocator, void *data)
{
    bool result = false;

    FOR_LINKED_LIST(it, &allocator->blocks) {
        if (it->stack.data == data) {
            result = reset(&it->stack);
            break;
        }
    }

    return result;
}

void *
first(Block_Allocator *allocator)
{
    assert(allocator);

    if (!allocator->blocks.count) {
        return nullptr;
    }

    void *result = allocator->blocks.head->stack.data;
    return result;
}

void *
next(Block_Allocator *allocator, void *data)
{
    assert(allocator && data);

    // NOTE(gr3yknigh1): Very inefficient [2025/03/10]
    // @perf

    FOR_LINKED_LIST(it, &allocator->blocks) {
        if (it->stack.data == data && it->next != nullptr) {
            return it->next->stack.data;
        }
    }
    return nullptr;
}

Size
get_file_size(FILE *file)
{
    assert(file);

    fseek(file, 0, SEEK_END);
    Size file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(file_size);

    return file_size;
}


void
hex_dump(void *buffer, Size buffer_size)
{

    for (Size i = 0; i < buffer_size; i += 16) {
        printf("%06llx: ", i);

        for (Size j = 0; j < 16; j++) {
            if (i + j < buffer_size) {
                printf("%02x ", static_cast<Byte *>(buffer)[i + j]);
            } else {
                printf("   ");
            }
        }

        printf(" ");
        for (Size j = 0; j < 16; j++) {
            if (i + j < buffer_size) {
                printf("%c", isprint(static_cast<Byte *>(buffer)[i + j]) ? static_cast<Byte *>(buffer)[i + j] : '.');
            }
        }
        printf("\n");
    }
}


#if defined(_WIN32)
    #include "garden_runtime_win32.cpp"
#else
    #error "Unhandled platform! No runtime was included"
#endif
