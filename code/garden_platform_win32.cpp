//
// FILE          code\garden_platform_win32.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//
#include <atomic>   // std::atomic_flag

#include <assert.h> // assert
#include <stdio.h>  // puts, printf, FILE, fopen, freopen, fseek, fclose
#include <ctype.h>  // isspace

#if defined(GARDEN_USE_CRT_ALLOCATIONS)
    #include <crtdbg.h>
#endif

#if !defined(UNICODE)
    #define UNICODE
#endif

#if !defined(NOMINMAX)
    #define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <intrin.h> // __rdtsc

#if defined(far)
    #undef far
#endif

#if defined(near)
    #undef near
#endif


#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "garden_gameplay.h"
#include "garden_platform.h"

#if !defined(GARDEN_ASSET_FOLDER)
    #error "Dev asset directory is not defined!"
#else
    #pragma message( "Using DEV asset dir: '" STRINGIFY(GARDEN_ASSET_FOLDER) "'" )
#endif

// TODO(gr3yknigh1): Replace with more non-platform dependent code [2025/03/03]
#if !defined(ZERO_STRUCT)
    #define ZERO_STRUCT(STRUCT_PTR) ZeroMemory((STRUCT_PTR), sizeof(*(STRUCT_PTR)))
#endif


typedef int bool32_t;
typedef unsigned int size32_t;

//
// String handling:
//

constexpr size_t
str8_get_length(const char *s) noexcept
{
    size_t result = 0;

    while (s[result] != 0) {
        result++;
    }

    return result;
}

struct Str8_View {
    const char *data;
    size_t length;

    constexpr inline bool empty(void) const noexcept { return this->length == 0; }

    constexpr inline Str8_View() noexcept : data(nullptr), length(0) {}
    constexpr inline Str8_View(const char *data_) noexcept : data(data_), length(str8_get_length(data_)) {}
    constexpr inline Str8_View(const char *data_, size_t length_) noexcept : data(data_), length(length_) {}
};

inline Str8_View
str8_view_capture_until(const char **cursor, char until)
{
    Str8_View sv;

    sv.data = *cursor;

    while (**cursor != until) {
        (*cursor)++;
    }

    sv.length = *cursor - sv.data;

    return sv;
}

inline bool
str8_view_copy_to_nullterminated(Str8_View sv, char *out_buffer, size_t out_buffer_size)
{
    if (sv.length + 1 > out_buffer_size) {
        return false;
    }

    memcpy((void *)out_buffer, sv.data, sv.length);
    out_buffer[sv.length] = 0;
    return true;
}

constexpr bool
str8_view_is_equals(Str8_View a, Str8_View b)
{
    if (a.length != b.length) {
        return false;
    }

    // TODO(gr3yknigh1): Do vectorization [2025/01/03]
    for (size_t i = 0; i < a.length; ++i) {
        if (a.data[i] != b.data[i]) {
            return false;
        }
    }

    return true;
}

constexpr bool
str8_view_is_equals(Str8_View a, const char *str)
{
    Str8_View b(str);
    return str8_view_is_equals(a, b);
}


constexpr size_t
str16_get_length(const wchar_t *s) noexcept
{
    size_t result = 0;

    while (s[result] != 0) {
        result++;
    }

    return result;
}

struct Str16_View {
    const wchar_t *data;
    size_t length;

    constexpr Str16_View() noexcept : data(nullptr), length(0) {}
    constexpr Str16_View(const wchar_t *data_) noexcept : data(data_), length(str16_get_length(data_)) {}
    constexpr Str16_View(const wchar_t *data_, size_t length_) noexcept : data(data_), length(length_) {}
};

inline bool
str16_view_copy_to_nullterminated(Str16_View view, wchar_t *out_buffer, size_t out_buffer_size) noexcept
{
    size_t required_buffer_size = (view.length + 1) * sizeof(*view.data);

    if (required_buffer_size > out_buffer_size) {
        return false;
    }

    memcpy((void *)out_buffer, view.data, view.length * sizeof(*view.data));
    out_buffer[view.length] = 0;
    return true;
}

inline bool
str16_view_endswith(Str16_View view, Str16_View end) noexcept
{
    if (view.length < end.length) {
        return false;
    }

    for (size_t end_index = end.length - 1, view_index = view.length - 1; end_index > 0; --end_index, --view_index) {
        if (end.data[end_index] != view.data[view_index]) {
            return false;
        }
    }

    return true;
}

inline bool
str16_view_endswith(Str16_View view, Str8_View end) noexcept
{
    if (view.length < end.length) {
        return false;
    }

    for (size_t end_index = end.length - 1, view_index = view.length - 1; end_index > 0; --end_index, --view_index) {

        const char *c16 = reinterpret_cast<const char *>(view.data + view_index);

        if (end.data[end_index] != c16[0]) {
            return false;
        }

        if (c16[1] != 0) {
            return false;
        }
    }

    return true;
}

constexpr bool
str16_view_is_equals(Str16_View a, Str16_View b) noexcept
{
    if (a.length != b.length) {
        return false;
    }

    // TODO(gr3yknigh1): Do vectorization [2025/01/03]
    for (size_t i = 0; i < a.length; ++i) {
        if (a.data[i] != b.data[i]) {
            return false;
        }
    }

    return true;
}

constexpr bool
str16_view_is_equals(Str16_View a, const wchar_t *str) noexcept
{
    Str16_View b(str);
    return str16_view_is_equals(a, b);
}

constexpr bool
str16_view_is_equals(Str16_View a, Str8_View b) noexcept
{
    if (a.length != b.length) {
        return false;
    }

    // TODO(gr3yknigh1): Do vectorization [2025/01/03]
    for (size_t i = 0; i < a.length; ++i) {
        const mm::byte *c16 = reinterpret_cast<const mm::byte *>(a.data + i);

        if (c16[1] != 0) {
            return false;
        }

        mm::byte c8 = b.data[i];

        if (c16[0] != c8) {
            return false;
        }
    }

    return true;
}

constexpr bool
str16_view_is_equals(Str16_View a, const char *str) noexcept
{
    Str8_View b(str);
    return str16_view_is_equals(a, b);
}

wchar_t path16_get_separator();

bool path16_get_parent(const wchar_t *path, size_t path_length, Str16_View *out);

//
// Perf helpers:
//

int64_t perf_get_counter_frequency(void);
int64_t perf_get_counter(void);
uint64_t perf_get_cycles_count(void);

#pragma pack(push, 1)
struct Perf_Block_Record {
    const char *label;
    const char *function;
    const char *file_path;
    uint64_t line_number;
    uint64_t cycles_begin;
    uint64_t cycles_end;
    int64_t counter_begin;
    int64_t counter_end;
};
#pragma pack(pop)

void perf_block_record_print(const Perf_Block_Record *record);

#define PERF_BLOCK_RECORD(NAME) NAME##__BLOCK_RECORD

#if defined(PERF_ENABLED)

    #define PERF_BLOCK_BEGIN(NAME) \
        Perf_Block_Record PERF_BLOCK_RECORD(NAME); \
        do { \
            PERF_BLOCK_RECORD(NAME).label = STRINGIFY(NAME); \
            PERF_BLOCK_RECORD(NAME).function = __FUNCTION__; \
            PERF_BLOCK_RECORD(NAME).file_path = __FILE__; \
            PERF_BLOCK_RECORD(NAME).line_number = __LINE__; \
            PERF_BLOCK_RECORD(NAME).cycles_begin = perf_get_cycles_count(); \
            PERF_BLOCK_RECORD(NAME).counter_begin = perf_get_counter(); \
        } while (0)

    #define PERF_BLOCK_END(NAME) \
        do { \
            PERF_BLOCK_RECORD(NAME).cycles_end = perf_get_cycles_count(); \
            PERF_BLOCK_RECORD(NAME).counter_end = perf_get_counter(); \
            perf_block_record_print(&PERF_BLOCK_RECORD(NAME)); \
        } while (0)

#else

    #define PERF_BLOCK_BEGIN(NAME)

    #define PERF_BLOCK_END(NAME)

#endif // if defined(PERF_ENABLED)

//
// WGL: Context initialization.
//

#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023

#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202B

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

typedef HGLRC    WINAPI WGL_CreateContext_T(HDC);
typedef bool32_t WINAPI WGL_DeleteContext_T(HGLRC);
typedef bool32_t WINAPI WGL_MakeCurrent_T(HDC, HGLRC);
typedef void *   WINAPI WGL_GetProcAddress_T(const char *);

static WGL_CreateContext_T  *WGL_CreateContext;
static WGL_DeleteContext_T  *WGL_DeleteContext;
static WGL_MakeCurrent_T    *WGL_MakeCurrent;
static WGL_GetProcAddress_T *WGL_GetProcAddress;

//
// WGL: OpenGL extentions initialization.
//

// TODO(gr3yknigh1): Replace _T with _FnType [2025/03/03]

typedef HGLRC    WINAPI WGL_CreateContextAttribARB_T(HDC, HGLRC, const int *);
typedef bool32_t WINAPI WGL_ChoosePixelFormatARB_T(HDC, const int *, const float *, unsigned int, int *, unsigned int *);
typedef bool32_t WINAPI WGL_SwapIntervalEXT_T(int);

static WGL_CreateContextAttribARB_T *WGL_CreateContextAttribARB;
static WGL_ChoosePixelFormatARB_T   *WGL_ChoosePixelFormatARB;
static WGL_SwapIntervalEXT_T        *WGL_SwapIntervalEXT;

//
// Globals
//

static bool global_should_terminate = false;

static void      win32_init_opengl_context_extensions(void);
static HGLRC     win32_init_opengl_context(HDC device_context);
LRESULT CALLBACK win32_window_message_handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

//
// Camera
//


Camera make_camera(Camera_ViewMode view_mode);

void      camera_rotate(Camera *camera, float x_offset, float y_offset);
glm::mat4 camera_get_view_matrix(Camera *camera);
glm::mat4 camera_get_projection_matrix(Camera *camera, int viewport_width, int viewport_height);

//
// OpenGL API wrappers
//
GLuint compile_shader_from_str8(const char *string, GLenum type);
GLuint compile_shader_from_str8(mm::Arena *arena, const char *string, GLenum type);
GLuint compile_shader_from_file(mm::Arena *arena, const char *file_path, GLenum type);
GLuint link_shader_program(GLuint vertex_shader, GLuint fragment_shader);
GLuint link_shader_program(mm::Arena *arena, GLuint vertex_shader, GLuint fragment_shader);

struct Vertex_Buffer_Attribute {
    bool is_normalized;
    unsigned int type;
    unsigned int count;
    size32_t size;
};

struct Vertex_Buffer_Layout {
    Vertex_Buffer_Attribute *attributes;
    unsigned int attributes_count;
    unsigned int attributes_capacity;
    size32_t stride;
};

//
// @brief Initializes struct which stores information about vertex buffer's attributes layout.
//
// @param[in] arena Pointer to arena on which will be allocated vertex buffer's attributes array.
//
// @param[out] layout Layout which should be initialized.
//
// @param[in] attributes_capacity Count of attributes for which should be reserved memory.
//
// @return True if allocation of the array was succesfull. Otherwise buy more RAM.
//
bool make_vertex_buffer_layout(mm::Arena *arena, Vertex_Buffer_Layout *layout, size32_t attributes_capacity);

Vertex_Buffer_Attribute *vertex_buffer_layout_push_attr   (Vertex_Buffer_Layout *layout, unsigned int count, GLenum type, size_t size);
Vertex_Buffer_Attribute *vertex_buffer_layout_push_float  (Vertex_Buffer_Layout *layout, unsigned int count);
Vertex_Buffer_Attribute *vertex_buffer_layout_push_integer(Vertex_Buffer_Layout *layout, unsigned int count);


struct Vertex_Buffer {
    GLuint id;
    GLuint vertex_array_id;

    Vertex_Buffer_Layout layout;
};

bool make_vertex_buffer(Vertex_Buffer *buffer);
bool bind_vertex_buffer(Vertex_Buffer *buffer);

//
// @brief Initializes vertex buffer layout.
//
// @pre
//     glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)
//     glBindVertexArray(vertex_array);
//
void vertex_buffer_layout_build_attrs(const Vertex_Buffer_Layout *layout);


struct Win32_Key_State {
    short vk_code;
    short flags;
    short scan_code;
    short repeat_count;

    MSG native_message;
};

#if !defined(WIN32_KEYSTATE_IS_EXTENDED)
    #define WIN32_KEYSTATE_IS_EXTENDED(K_PTR) HAS_FLAG((K_PTR)->flags, KF_EXTENDED)
#endif

#if !defined(WIN32_KEYSTATE_IS_RELEASED)
    #define WIN32_KEYSTATE_IS_RELEASED(K_PTR) HAS_FLAG((K_PTR)->flags, KF_UP)
#endif

Win32_Key_State win32_convert_msg_to_key_state(MSG message);

bool win32_is_vk_pressed(int vk);

//
// Handle keyboard input for Win32 API layer.
//
// @param[in] message Actual windows message which received in mainloop.
//
// @param[out] input_state Output Input_State of the game.
//
void Win32_HandleKeyboardInput(MSG message, Input_State *input_state);  // TODO

//
// Time:
//

struct Clock {
    int64_t ticks_begin;
    int64_t ticks_end;
    int64_t frequency;
};

Clock make_clock(void);

double clock_tick(Clock *clock);

//
// Media:
//

enum struct Bitmap_Picture_Header_Type : uint32_t {
    BitmapCoreHeader = 12,
    Os22XBitmapHeader_S = 16,
    BitmapInfoHeader = 40,
    BitmapV2InfoHeader = 52,
    BitmapV3InfoHeader = 56,
    Os22XBitmapHeader = 64,
    BitmapV4Header = 108,
    BitmapV5Header = 124,
};

enum struct Bitmap_Picture_Compression_Method : uint32_t {
    RGB = 0,
    RLE8 = 1,
    RLE4 = 2,
    Bitfields = 3,
    JPEG = 4,
    PNG = 5,
    AlphaBitfields = 6,
    CMYK = 11,
    CMYKRLE8 = 12,
    CMYKRLE4 = 13,
};

#pragma pack(push, 1)
struct Bitmap_Picture_DIB_Header {
    Bitmap_Picture_Header_Type header_size;
    uint32_t width;
    uint32_t height;
    uint16_t planes_count;
    uint16_t depth;
    Bitmap_Picture_Compression_Method compression_method;
    uint32_t image_size;
    uint32_t x_pixel_per_meter;
    uint32_t y_pixel_per_meter;
    uint32_t color_used;
    uint32_t color_important;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Bitmap_Picture_Header {
    uint16_t type;
    uint32_t file_size;
    uint16_t reserved[2];
    uint32_t data_offset;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Color_BGRA_U8 {
    uint8_t b, g, r, a;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Bitmap_Picture {
    Bitmap_Picture_Header header;
    Bitmap_Picture_DIB_Header dib_header;

    union {
        void *data;
        Color_BGRA_U8 *bgra;
    } u;
};
#pragma pack(pop)

bool load_bitmap_picture_from_file(mm::Arena *arena, Bitmap_Picture *picture, const char *file_path);
bool load_bitmap_picture_from_file(mm::Arena *arena, Bitmap_Picture *picture, FILE *file);

bool load_bitmap_picture_info_from_file(Bitmap_Picture *picture, FILE *file);
bool load_bitmap_picture_pixel_data_from_file(Bitmap_Picture *picture, FILE *file);


//
// Media:
//

enum struct Image_Color_Layout {
    Nothing,
    BGRA_U8,
};


//
// @pre
//   - Bind target texture with glBindTexture(GL_TEXTURE_2D, ...);
//
void gl_make_texture_from_image(void *data, size32_t width, size32_t height, Image_Color_Layout layout, GLenum internal_format);


void gl_clear_all_errors(void);
void gl_die_on_first_error(void);
void gl_print_debug_info(void);

//
// Tilemaps:
//

enum struct Tilemap_Image_Format {
    Bitmap,
};

struct Asset;

struct Tilemap {
    // TODO(gr3yknigh1): Implement uint parsing [2025/02/23]

    int row_count;
    int col_count;

    int tile_x_pixel_count;
    int tile_y_pixel_count;

    int *indexes;
    size_t indexes_count;

    struct {
        Tilemap_Image_Format format;
        union {
            Asset *asset;
        } u;
    } image;
};

Tilemap *load_tilemap_from_file(mm::Arena *arena, const char *file_path);

#define WATCH_EXT_NONE MAKE_FLAG(0)
#define WATCH_EXT_BMP  MAKE_FLAG(1)
#define WATCH_EXT_GLSL MAKE_FLAG(2)
#define WATCH_EXT_DLL  MAKE_FLAG(3)

constexpr Str16_View WATCH_EXT_BMP_VIEW = L"bmp";
constexpr Str16_View WATCH_EXT_GLSL_VIEW = L"glsl";
constexpr Str16_View WATCH_EXT_DLL_VIEW = L"dll";

/* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-file_notify_information#members */
enum struct File_Action {
    Added           = FILE_ACTION_ADDED,
    Removed         = FILE_ACTION_REMOVED,
    Modified        = FILE_ACTION_MODIFIED,
    RenamedOldName  = FILE_ACTION_RENAMED_OLD_NAME,
    RenamedNewName  = FILE_ACTION_RENAMED_NEW_NAME,
};

struct Watch_Context;

typedef int watch_ext_mask_t;
typedef void (* watch_notification_routine_t)(Watch_Context *watch_context, const Str16_View file_name, File_Action action, void *parameter);

struct Watch_Context {
    std::atomic_flag should_stop;
    const wchar_t *target_dir;
    watch_notification_routine_t notification_routine;
    void *parameter;
};

bool make_watch_context(Watch_Context *context, void *parameter = nullptr, const wchar_t *target_dir = nullptr, watch_notification_routine_t notification_routine = nullptr);
DWORD watch_context_launch_thread(Watch_Context *context, HANDLE *out_thread_handle = nullptr);
void watch_thread_worker(PVOID param);

//
// Assets:
//

struct Gameplay {
    HMODULE module;

    Game_On_Init_Fn_Type *on_init;
    Game_On_Load_Fn_Type *on_load;
    Game_On_Tick_Fn_Type *on_tick;
    Game_On_Draw_Fn_Type *on_draw;
    Game_On_Fini_Fn_Type *on_fini;
};

Gameplay load_gameplay(const char *module_path);
void unload_gameplay(Gameplay *gameplay);

enum struct Asset_Type {
    Image,
    Shader,
    Tilemap,
    Count_
};

enum struct Asset_Store_Place {
    Folder,
    ImageFile,
};

struct Asset_Store {
    static constexpr uint16_t max_asset_count = 1024;

    mm::Block_Allocator asset_pool;
    mm::Block_Allocator asset_content;

    // TODO(gr3yknigh1): Add support for using `store image-file` (single file) [2025/03/06]
    // TODO(gr3yknigh1): Support for utf-8 or wide paths? [2025/03/06]

    Asset_Store_Place place;

    union {
        const char *folder;
    } u;
};

#if !defined(FOR_EACH_ASSET)
    #define FOR_EACH_ASSET(IT, ASSET_STORE_PTR) \
        for (Asset *IT = static_cast<Asset *>(mm::first(&(ASSET_STORE_PTR)->asset_pool)); IT != nullptr; IT = static_cast<Asset *>(mm::next(&(ASSET_STORE_PTR)->asset_pool, static_cast<void *>(IT)) ))
#endif

struct Reload_Context {
    Asset_Store *store;

    //
    // NOTE(gr3yknigh1): Found no place for this. Think about it later, if it becomes a problem [2025/03/10]
    //
    std::atomic_flag should_reload_gameplay;
};

struct Buffer_View {
    mm::byte *data;
    size_t size;

    constexpr Buffer_View() : data(nullptr), size(0) {}
};

enum struct Asset_Location_Type {
    None,
    File,
    Buffer,
};

struct Asset_Location {
    Asset_Location_Type type;

    union Data {
        mm::byte dummy;  // Dummy for zero initialization of union in default constructor

        struct {
            FILE *handle;
            size_t size;
            Str8_View path;
        } file;

        Buffer_View buffer_view;

        constexpr Data() : dummy(0) {}
    } u;

    constexpr Asset_Location() : type(Asset_Location_Type::None) {}
};

size_t get_file_size(FILE *file);

bool make_asset_store_from_folder(Asset_Store *store, const char *folder_path);
bool asset_store_destroy(Asset_Store *store);

struct Image {
    int width;
    int height;
    Image_Color_Layout layout;

    GLuint unit;
    GLuint id;

    union {
        void *data;
        Color_BGRA_U8 *bgra_u8;
    } pixels;
};

enum struct Asset_State {
    NotLoaded,
    LoadFailure,
    Loaded,
    UnloadFailure,
    Unloaded
};


enum struct Shader_Module_Type {
    Vertex,
    Fragment,

    Count_
};

struct Shader_Module {
    GLuint id;
};

struct Shader {
    GLuint program_id;
    char *source_code;

    Shader_Module modules[static_cast<size_t>(Shader_Module_Type::Count_)];
};

struct Asset {
    Asset_Type type;
    Asset_Location location;
    Asset_State state;

    std::atomic_flag should_reload;

    union {
        Image image;
        Shader shader;
        Tilemap tilemap;
    } u;
};

Asset *asset_load(Asset_Store *store, Asset_Type type, const char *file);
#if 0
Asset *asset_load_image(Asset_Store *store, const char *file);
Asset *asset_load_shader(Asset_Store *store, const char *file);
Asset *asset_load_tilemap(Asset_Store *store, const char *file);
#endif

// helper
bool load_tilemap_from_buffer(Asset_Store *store, char *buffer, size_t buffer_size, Tilemap *tilemap);
bool asset_image_send_to_gpu(Asset_Store *store, Asset *asset, int unit, Shader *shader);

bool shader_bind(Shader *shader);

bool asset_from_bitmap_picture(Asset *asset, Bitmap_Picture *picture);

bool asset_reload(Asset_Store *store, Asset *asset);
bool asset_unload_content(Asset_Store *store, Asset *asset);

struct Shader_Compile_Result {
    GLuint shader_program_id;
};

Shader_Compile_Result compile_shader(char *source_code, size_t file_size);

struct Lexer {
    char *cursor;
    char lexeme;

    char *buffer;
    size_t buffer_size;
};

//!
//! @todo Make buffer be const!
//!
Lexer make_lexer(char *buffer, size_t buffer_size);

void lexer_advance(Lexer *lexer, int32_t count = 1);

char      lexer_peek(Lexer *lexer, int32_t offset);
Str8_View lexer_peek_view(Lexer *lexer, int32_t offset);

bool      lexer_check_peeked(Lexer *lexer, const char *s);
bool      lexer_check_peeked(Lexer *lexer, Str8_View sv);
bool      lexer_check_peeked_and_advance(Lexer *lexer, Str8_View sv);

bool      lexer_parse_int(Lexer *lexer, int *result);
bool      lexer_parse_str_to_view(Lexer *lexer, Str8_View *sv);

// TODO(gr3yknigh1): Rename lexer_skip_until* to lexer_advance_until* [2025/03/28]
Str8_View lexer_skip_until(Lexer *lexer, char c);
Str8_View lexer_skip_until(Lexer *lexer, Str8_View sv);
Str8_View lexer_skip_until_endline(Lexer *lexer);
void      lexer_skip_whitespace(Lexer *lexer);

bool      lexer_is_endline(Lexer *lexer, bool *is_crlf = nullptr);
bool      lexer_is_end(Lexer *lexer);


static void asset_watch_routine(Watch_Context *, const Str16_View, File_Action, void *);

int get_offset_from_coords_of_2d_grid_array_rm(int width, int x, int y);


int WINAPI
wWinMain(HINSTANCE instance, HINSTANCE previous_instance, PWSTR command_line, int cmd_show)
{
    (void)previous_instance;
    (void)command_line;

    //
    // Window initialization:
    //
    const wchar_t *window_class_name = L"garden";
    const wchar_t *window_title = L"garden";

    WNDCLASSW window_class;
    ZERO_STRUCT(&window_class);
    window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    //                                             ^^^^^^^^
    // NOTE(ilya.a): Needed by OpenGL. See Khronos's docs [2024/11/10]
    window_class.lpfnWndProc = win32_window_message_handler;
    window_class.hInstance = instance;
    window_class.lpszClassName = window_class_name;
    assert(RegisterClassW(&window_class));

    HWND window = CreateWindowEx(
        0, window_class.lpszClassName, window_title,
        WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, instance, 0);
    assert(window);

    HDC window_device_context = GetDC(window);
    assert(window_device_context);

    assert(ShowWindow(window, cmd_show));
    assert(UpdateWindow(window));

    //
    // OpenGL initialization:
    //
    HMODULE opengl_module = LoadLibraryW(L"opengl32.dll");
    assert(opengl_module);

    WGL_GetProcAddress = (WGL_GetProcAddress_T *)GetProcAddress(opengl_module, "wglGetProcAddress");
    assert(WGL_GetProcAddress);

    WGL_CreateContext = (WGL_CreateContext_T *)GetProcAddress(opengl_module, "wglCreateContext");
    assert(WGL_CreateContext);

    WGL_DeleteContext = (WGL_DeleteContext_T *)GetProcAddress(opengl_module, "wglDeleteContext");
    assert(WGL_DeleteContext);

    WGL_MakeCurrent = (WGL_MakeCurrent_T *)GetProcAddress(opengl_module, "wglMakeCurrent");
    assert(WGL_MakeCurrent);

    // NOTE(gr3yknigh1): In order to list all procs in `opengl32.dll`:
    // ```cmd
    // dumpbin /exports C:\Windows\System32\opengl32.dll | findstr wgl*
    // ```
    // [2024/11/11]
    HGLRC window_render_context = win32_init_opengl_context(window_device_context);
    assert(window_render_context);

    assert(gladLoadGL());

    UpdateWindow(window);

    RECT window_rect;
    assert(GetClientRect(window, &window_rect));

    [[maybe_unused]] int window_x = window_rect.left;
    [[maybe_unused]] int window_y = window_rect.bottom;
    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;

    //
    // OpenGL settings:
    //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl_print_debug_info();

    //
    // Game initalization:
    //
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    Vertex_Buffer entity_vertex_buffer{};
    assert(make_vertex_buffer(&entity_vertex_buffer));
    assert(bind_vertex_buffer(&entity_vertex_buffer));

    mm::Arena page_arena = mm::make_arena(1024);

    Vertex_Buffer_Layout vertex_buffer_layout;
    assert(make_vertex_buffer_layout(&page_arena, &vertex_buffer_layout, 4));

    assert(vertex_buffer_layout_push_float(&vertex_buffer_layout, 2));
    assert(vertex_buffer_layout_push_float(&vertex_buffer_layout, 2));
    assert(vertex_buffer_layout_push_integer(&vertex_buffer_layout, 1));

    vertex_buffer_layout_build_attrs(&vertex_buffer_layout);

    mm::arena_reset(&page_arena);

    //
    // Media:
    //
    Asset_Store store;
    assert(make_asset_store_from_folder(&store, STRINGIFY(GARDEN_ASSET_FOLDER)));

    Asset *basic_shader_asset = asset_load(&store, Asset_Type::Shader, R"(P:\garden\assets\basic.sl)");
    assert(basic_shader_asset);

    Shader *basic_shader = &basic_shader_asset->u.shader;

    glUseProgram(basic_shader->program_id);

    GLint model_uniform_loc = glGetUniformLocation(basic_shader->program_id, "model");
    assert(model_uniform_loc != -1);

    GLint projection_uniform_loc = glGetUniformLocation(basic_shader->program_id, "projection");
    assert(projection_uniform_loc != -1);

    Camera camera = make_camera(Camera_ViewMode::Orthogonal);

    glm::mat4 model = glm::identity<glm::mat4>();
    glm::mat4 projection = camera_get_projection_matrix(&camera, window_width, window_height);

    glUniformMatrix4fv(model_uniform_loc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(projection_uniform_loc, 1, GL_FALSE, glm::value_ptr(projection));

    //
    // Atlas:
    //
    Asset *atlas_asset = asset_load(&store, Asset_Type::Image, R"(P:\garden\assets\garden_atlas.bmp)");
    assert(atlas_asset);
    assert(asset_image_send_to_gpu(&store, atlas_asset, 0, basic_shader));

    mm::Arena asset_arena = mm::make_arena(1024000);  // @cleanup

    //
    // Hot-reload: Setup
    //

    Reload_Context reload_context;
    reload_context.store = &store;
    reload_context.should_reload_gameplay.clear();

    Watch_Context watch_context;
    assert(make_watch_context(&watch_context, &reload_context, LR"(P:\garden)", asset_watch_routine));

    assert(watch_context_launch_thread(&watch_context));

    //
    // Setup tilemap atlas:
    //
    Asset *tilemap_asset = asset_load(&store, Asset_Type::Tilemap, R"(P:\garden\assets\demo.tilemap.tp)");
    assert(asset_image_send_to_gpu(&store, tilemap_asset->u.tilemap.image.u.asset, 1, basic_shader));

    Vertex_Buffer tilemap_vertex_buffer{};
    assert(make_vertex_buffer(&tilemap_vertex_buffer));
    assert(bind_vertex_buffer(&tilemap_vertex_buffer));

    Vertex_Buffer_Layout tilemap_vertex_buffer_layout{};
    assert(make_vertex_buffer_layout(&page_arena, &tilemap_vertex_buffer_layout, 4));

    assert(vertex_buffer_layout_push_float(&tilemap_vertex_buffer_layout, 2));    // Position
    assert(vertex_buffer_layout_push_float(&tilemap_vertex_buffer_layout, 2));    // UV
    assert(vertex_buffer_layout_push_integer(&tilemap_vertex_buffer_layout, 1));  // Color

    vertex_buffer_layout_build_attrs(&tilemap_vertex_buffer_layout);

    mm::arena_reset(&page_arena);

    Tilemap *tilemap = &tilemap_asset->u.tilemap;

    static size_t TILEMAP_VERTEX_COUNT_PER_TILE = 6; // two triangles
    float tilemap_position_x = 100, tilemap_position_y = 100;
    size_t tilemap_tiles_count = tilemap->row_count * tilemap->col_count;
    size_t tilemap_vertexes_buffer_size = tilemap_tiles_count * TILEMAP_VERTEX_COUNT_PER_TILE * sizeof(Vertex);
    Vertex *tilemap_vertexes = static_cast<Vertex *>(mm::allocate(tilemap_vertexes_buffer_size)); // TODO: Free later
    size_t tilemap_vertexes_count = 0;
    Image *tilemap_texture = &tilemap->image.u.asset->u.image;

    Atlas tilemap_atlas{
        #if 1
        static_cast<float>(tilemap_texture->width),
        static_cast<float>(tilemap_texture->height)
        #else
        static_cast<float>(tilemap->tile_x_pixel_count),
        static_cast<float>(tilemap->tile_y_pixel_count)
        #endif
    };

    for (int col_idx = 0; col_idx < tilemap->col_count; ++col_idx) {
        for (int row_idx = 0; row_idx < tilemap->row_count; ++row_idx) {

            float tile_x = tilemap_position_x + col_idx * 100; // tilemap->tile_x_pixel_count;
            float tile_y = tilemap_position_y + row_idx * 100; // tilemap->tile_y_pixel_count;


            int index_offset = get_offset_from_coords_of_2d_grid_array_rm(tilemap->col_count, col_idx, row_idx);
            float index = static_cast<float>(tilemap->indexes[index_offset]);

            Rect_F32 tile_atlas_location{
                /* x: */ floorf(index / tilemap->col_count) * static_cast<float>(tilemap->tile_x_pixel_count),
                /* y: */ floorf(fmodf(index, static_cast<float>(tilemap->col_count))) * static_cast<float>(tilemap->tile_y_pixel_count),
                /* width: */ static_cast<float>(tilemap->tile_x_pixel_count),
                /* height: */ static_cast<float>(tilemap->tile_y_pixel_count)
            };

            static Color4 tile_color = { 200, 100, 0, 255 };

            tilemap_vertexes_count += generate_rect_with_atlas(
                tilemap_vertexes + tilemap_vertexes_count, tile_x, tile_y, 100, 100 /* static_cast<float>(tilemap->tile_x_pixel_count), static_cast<float>(tilemap->tile_y_pixel_count) */, tile_atlas_location, &tilemap_atlas, tile_color);

        }
    }

    //
    // Game mainloop:
    //
    Clock clock = make_clock();

    uint64_t frame_counter = 0;

    //
    // Load game code:
    //

    Gameplay gameplay = load_gameplay(STRINGIFY(GARDEN_GAMEPLAY_DLL_NAME));

    Platform_Context platform_context;
    ZERO_STRUCT(&platform_context);

    platform_context.camera = &camera;
    platform_context.persist_arena = mm::make_arena(1024);
    platform_context.vertexes_arena = mm::make_arena(sizeof(Vertex) * 1024);

    Game_Context *game_context = reinterpret_cast<Game_Context *>(gameplay.on_init(&platform_context));
    gameplay.on_load(&platform_context, game_context);

    while (!global_should_terminate) {
        double dt = clock_tick(&clock);

        MSG message;
        ZERO_STRUCT(&message);

        //
        // Game Hot reload:
        //

        if (reload_context.should_reload_gameplay.test()) {
            reload_context.should_reload_gameplay.clear();
            unload_gameplay(&gameplay);
            gameplay = load_gameplay(STRINGIFY(GARDEN_GAMEPLAY_DLL_NAME));
            gameplay.on_load(&platform_context, game_context);
        }

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                global_should_terminate = true;
            } else {
                switch (message.message) {
                case WM_KEYUP:
                case WM_KEYDOWN:
                case WM_SYSKEYUP:
                case WM_SYSKEYDOWN: {
                    Win32_Key_State key = win32_convert_msg_to_key_state(message);

                    if (key.vk_code == VK_ESCAPE || key.vk_code == 0x51) {  // 0x51 - `Q` key.
                        global_should_terminate = true;
                    }

                    // if (key.vk_code == 0x45) {  // 0x45 - `E` key
                    //     atlas_location = { 0, 0, 16, 16 };
                    // }

                    // if (key.vk_code == 0x46) {  // 0x46 - `F` key
                    //     atlas_location = { 16, 0, 16, 16 };
                    // }

                    if (!WIN32_KEYSTATE_IS_RELEASED(&key)) {
                        if (key.vk_code == VK_LEFT) {
                            platform_context.input_state.x_direction = -1;
                        } else if (key.vk_code == VK_RIGHT) {
                            platform_context.input_state.x_direction = +1;
                        }

                        if (key.vk_code == VK_DOWN) {
                            platform_context.input_state.y_direction = -1;
                        } else if (key.vk_code == VK_UP) {
                            platform_context.input_state.y_direction = +1;
                        }
                    } else {
                        if ((key.vk_code == VK_LEFT   && !win32_is_vk_pressed(VK_RIGHT))
                        || ((key.vk_code == VK_RIGHT) && !win32_is_vk_pressed(VK_LEFT))) {
                            platform_context.input_state.x_direction = 0;
                        }

                        if ((key.vk_code == VK_DOWN && !win32_is_vk_pressed(VK_UP))
                         || (key.vk_code == VK_UP && !win32_is_vk_pressed(VK_DOWN))) {
                            platform_context.input_state.y_direction = 0;
                        }
                    }

                } break;
                }
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // NOTE(i.akkuzin): After WM_QUIT device_context is invalid [2025/02/08]
        if (global_should_terminate) {
            break;
        }


        //
        // Update:
        //

        PERF_BLOCK_BEGIN(UPDATE);

            gameplay.on_tick(&platform_context, game_context, static_cast<float>(dt));

            //
            // Asset Hot reload:
            //

            FOR_EACH_ASSET(it, &store) {
                if (it->state == Asset_State::NotLoaded) {
                    continue;
                }

                if (it->location.type != Asset_Location_Type::File) {
                    continue;
                }

                if (!it->should_reload.test()) {
                    continue;
                }

                assert(asset_reload(&store, it));

                if (it->type == Asset_Type::Image) {
                    assert(asset_image_send_to_gpu(&store, it, it->u.image.unit, basic_shader));
                }

                if (it->type == Asset_Type::Shader) {
                    Shader *shader = &it->u.shader;

                    glUseProgram(shader->program_id);

                    model_uniform_loc = glGetUniformLocation(shader->program_id, "model");
                    assert(model_uniform_loc != -1);

                    projection_uniform_loc = glGetUniformLocation(shader->program_id, "projection");
                    assert(projection_uniform_loc != -1);

                    model = glm::identity<glm::mat4>();
                    projection = camera_get_projection_matrix(&camera, window_width, window_height);

                    glUniformMatrix4fv(model_uniform_loc, 1, GL_FALSE, glm::value_ptr(model));
                    glUniformMatrix4fv(projection_uniform_loc, 1, GL_FALSE, glm::value_ptr(projection));

                    GLuint atlas_texture_uniform_loc = glGetUniformLocation(shader->program_id, "u_texture");
                    assert(atlas_texture_uniform_loc != -1);

                    glUniform1i(atlas_texture_uniform_loc, atlas_asset->u.image.unit);
                }

                it->should_reload.clear();
            }


        PERF_BLOCK_END(UPDATE);

        //
        // Draw:
        //

        PERF_BLOCK_BEGIN(DRAW);

            model = glm::identity<glm::mat4>();
            model = glm::translate(model, camera.position);
            model = glm::translate(model, glm::vec3(window_width / 2, window_height / 2, 0));

            projection = camera_get_projection_matrix(&camera, window_width, window_height);
            glUniformMatrix4fv(model_uniform_loc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(projection_uniform_loc, 1, GL_FALSE, glm::value_ptr(projection));

            gameplay.on_draw(&platform_context, game_context, static_cast<float>(dt));

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (tilemap_vertexes_count > 0) {
                assert(bind_vertex_buffer(&tilemap_vertex_buffer));

                /// XXX
                glBindTexture(GL_TEXTURE_2D, tilemap_asset->u.tilemap.image.u.asset->u.image.id);
                GLuint texture_uniform_loc = glGetUniformLocation(basic_shader->program_id, "u_texture");
                assert(texture_uniform_loc != -1);
                glUniform1i(texture_uniform_loc, tilemap_asset->u.tilemap.image.u.asset->u.image.unit);

                size_t vertex_buffer_size = tilemap_vertexes_count * sizeof(*tilemap_vertexes);
                glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, tilemap_vertexes, GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertex_buffer_size / vertex_buffer_layout.stride)); // TODO(gr3yknigh1): Replace layout [2025/03/30]
            }

            if (platform_context.vertexes_count > 0) {
                assert(bind_vertex_buffer(&entity_vertex_buffer));

                /// XXX
                glBindTexture(GL_TEXTURE_2D, atlas_asset->u.image.id);
                GLuint texture_uniform_loc = glGetUniformLocation(basic_shader->program_id, "u_texture");
                assert(texture_uniform_loc != -1);
                glUniform1i(texture_uniform_loc, atlas_asset->u.image.unit);

                size_t vertex_buffer_size = platform_context.vertexes_count * sizeof(*platform_context.vertexes);
                glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, platform_context.vertexes, GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertex_buffer_size / vertex_buffer_layout.stride));

                platform_context.vertexes_count = 0;
                mm::arena_reset(&platform_context.vertexes_arena);
            }

            assert(SwapBuffers(window_device_context));

        PERF_BLOCK_END(DRAW);

        frame_counter++;
    }

    gameplay.on_fini(&platform_context, game_context);

    unload_gameplay(&gameplay);

    glDeleteProgram(basic_shader->program_id); // @cleanup Replace with asset_shader_free

    free_arena(&asset_arena);
    free_arena(&page_arena);
    free_arena(&platform_context.vertexes_arena);
    free_arena(&platform_context.persist_arena);

    assert(FreeLibrary(opengl_module));
    CloseWindow(window); // TODO(gr3yknigh1): why it fails? [2025/02/23]

    // WaitForSingleObject(watch_thread, INFINITE); // TODO(gr3yknigh1): Wait for thread [2025/02/23]

    assert(asset_store_destroy(&store));

#if defined(GARDEN_USE_CRT_ALLOCATIONS)
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}

LRESULT CALLBACK
win32_window_message_handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {

    case WM_CREATE:
        assert(AllocConsole());
        freopen("CONOUT$", "w+", stdout);
        freopen("CONOUT$", "w+", stderr);
        freopen("CONIN$", "r+", stdin);
        break;

    case WM_DESTROY:
        assert(FreeConsole());
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(window, message, wparam, lparam);
    }

    return 0;
}

static void
win32_init_opengl_context_extensions(void)
{
    WNDCLASSA dummy_window_class;
    ZERO_STRUCT(&dummy_window_class);

    dummy_window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    dummy_window_class.lpfnWndProc = DefWindowProcA;
    dummy_window_class.hInstance = GetModuleHandle(0);
    dummy_window_class.lpszClassName = "__dummy_window_class";
    assert(RegisterClassA(&dummy_window_class));

    HWND dummy_window = CreateWindowExA(
        0, dummy_window_class.lpszClassName, "__dummy_window",
        WS_CLIPSIBLINGS | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, dummy_window_class.hInstance, 0);
    assert(dummy_window);

    HDC dummy_device_context = GetDC(dummy_window);

    // NOTE(ilya.a): The worst struct I ever met [2024/09/07]
    PIXELFORMATDESCRIPTOR pfd;
    ZERO_STRUCT(&pfd);

    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;  // Should be zero?
    pfd.cDepthBits = 24; // Number of bits for the depthbuffer
    pfd.cStencilBits = 8;

    int pf_index = ChoosePixelFormat(dummy_device_context, &pfd);
    assert(pf_index);
    assert(SetPixelFormat(dummy_device_context, pf_index, &pfd));

    HGLRC dummy_render_context = WGL_CreateContext(dummy_device_context);
    assert(dummy_render_context);

    assert(WGL_MakeCurrent(dummy_device_context, dummy_render_context));

#if 0
    assert(gladLoadWGL(dummy_device_context));
#endif

    WGL_CreateContextAttribARB = (WGL_CreateContextAttribARB_T *)WGL_GetProcAddress("wglCreateContextAttribsARB");
    assert(WGL_CreateContextAttribARB);

    WGL_ChoosePixelFormatARB = (WGL_ChoosePixelFormatARB_T *)WGL_GetProcAddress("wglChoosePixelFormatARB");
    assert(WGL_ChoosePixelFormatARB);

    WGL_SwapIntervalEXT = (WGL_SwapIntervalEXT_T *)WGL_GetProcAddress("wglSwapIntervalEXT");
    assert(WGL_SwapIntervalEXT);

    WGL_MakeCurrent(dummy_device_context, 0);
    WGL_DeleteContext(dummy_render_context);
}

static HGLRC
win32_init_opengl_context(HDC device_context)
{
    win32_init_opengl_context_extensions();

    int pixel_format_attribs[] = {
        WGL_DRAW_TO_WINDOW_ARB,
        true,
        WGL_SUPPORT_OPENGL_ARB,
        true,
        WGL_DOUBLE_BUFFER_ARB,
        true,
        WGL_ACCELERATION_ARB,
        WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,
        WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,
        32,
        WGL_DEPTH_BITS_ARB,
        24,
        WGL_STENCIL_BITS_ARB,
        8,
        0};

    int pixel_format;
    unsigned int num_formats;

    assert(WGL_ChoosePixelFormatARB(device_context, pixel_format_attribs, 0, 1, &pixel_format, &num_formats));
    assert(num_formats);

    PIXELFORMATDESCRIPTOR pixel_format_descriptor;
    assert(DescribePixelFormat(device_context, pixel_format, sizeof(pixel_format_descriptor), &pixel_format_descriptor));
    assert(SetPixelFormat(device_context, pixel_format, &pixel_format_descriptor));

    // Specify that we want to create an OpenGL 3.3 core profile context
    int gl_attribs[] = {
#ifdef _DEBUG
        WGL_CONTEXT_FLAGS_ARB,
        WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        WGL_CONTEXT_MAJOR_VERSION_ARB,
        3,
        WGL_CONTEXT_MINOR_VERSION_ARB,
        3,
        WGL_CONTEXT_PROFILE_MASK_ARB,
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };

    HGLRC render_context = WGL_CreateContextAttribARB(device_context, 0, gl_attribs);
    assert(render_context);

    assert(WGL_MakeCurrent(device_context, render_context));
    assert(WGL_SwapIntervalEXT(1));

    return render_context;
}

GLuint
compile_shader_from_file(mm::Arena *arena, const char *file_path, GLenum type)
{
    // TODO(gr3yknigh1): Check for path existens and that it is file [2024/11/24]

    FILE *file = fopen(file_path, "r");
    assert(file);

    size_t file_size = get_file_size(file);
    assert(file_size);

    size_t string_buffer_size = file_size + 1;

    char *string_buffer = static_cast<char *>(mm::arena_alloc_zero(arena, string_buffer_size, ARENA_ALLOC_POPABLE));
    assert(string_buffer);

    fread(string_buffer, string_buffer_size, 1, file);
    fclose(file);

    GLuint id = compile_shader_from_str8(arena, string_buffer, type);

    assert(arena_pop(arena, string_buffer));

    return id;
}

GLenum
gl_convert_shader_module_type_to_gl_enum(Shader_Module_Type type) {

    if (type == Shader_Module_Type::Vertex) {
        return GL_VERTEX_SHADER;
    }

    if (type == Shader_Module_Type::Fragment) {
        return GL_FRAGMENT_SHADER;
    }

    return 0;
}

GLuint
compile_shader_from_str8(const char *string, Shader_Module_Type type)
{
    GLenum gl_shader_type = gl_convert_shader_module_type_to_gl_enum(type);

    GLuint id = glCreateShader(gl_shader_type);
    glShaderSource(id, 1, &string, 0);
    glCompileShader(id);

    GLint status = GL_TRUE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        //
        // TODO(gr3yknigh1): Make Shader_Compiler struct with which you should report compile errors [2025/03/28]
        //
        GLint log_length = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
        assert(log_length);

        size_t log_buffer_size = log_length + 1;
        char *log_buffer = static_cast<char *>(mm::allocate(log_buffer_size));
        assert(log_buffer);

        glGetShaderInfoLog(id, (GLsizei)log_buffer_size, 0, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to compile OpenGL shader! %s", logBuffer); */
        mm::deallocate(log_buffer);
    }

    return id;
}

GLuint
compile_shader_from_str8(mm::Arena *arena, const char *string, GLenum type)
{
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &string, 0);
    glCompileShader(id);

    GLint status = GL_TRUE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        GLint log_length = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
        assert(log_length);

        size_t log_buffer_size = log_length + 1;
        char *log_buffer = static_cast<char *>(mm::arena_alloc_zero(arena, log_buffer_size, ARENA_ALLOC_POPABLE));
        assert(log_buffer);

        glGetShaderInfoLog(id, (GLsizei)log_buffer_size, 0, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to compile OpenGL shader! %s", logBuffer); */
        mm::arena_pop(arena, log_buffer);
    }

    return id;
}

GLuint
link_shader_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint id = glCreateProgram();

    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    glLinkProgram(id);

    glValidateProgram(id);

    GLint status = GL_TRUE;
    glGetProgramiv(id, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint log_length{0};
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);

        size_t log_buffer_size = log_length + 1;
        char *log_buffer = static_cast<char *>(mm::allocate(log_buffer_size));
        assert(log_buffer);

        glGetProgramInfoLog(id, (GLsizei)log_buffer_size, NULL, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to link OpenGL program! %s", logBuffer); */
        mm::deallocate(log_buffer);
    }

    return id;
}

GLuint
link_shader_program(mm::Arena *arena, GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint id = glCreateProgram();

    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    glLinkProgram(id);

    glValidateProgram(id);

    GLint status = GL_TRUE;
    glGetProgramiv(id, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint log_length;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);

        size_t log_buffer_size = log_length + 1;
        char *log_buffer = static_cast<char *>(mm::arena_alloc_zero(arena, log_buffer_size, ARENA_ALLOC_POPABLE));
        assert(log_buffer);

        glGetProgramInfoLog(id, (GLsizei)log_buffer_size, NULL, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to link OpenGL program! %s", logBuffer); */
        mm::arena_pop(arena, log_buffer);
    }

    return id;
}

bool
make_vertex_buffer_layout(mm::Arena *arena, Vertex_Buffer_Layout *layout, size32_t attributes_capacity)
{
    ZERO_STRUCT(layout);

    layout->attributes = static_cast<Vertex_Buffer_Attribute *>(
        mm::arena_alloc_zero(arena, attributes_capacity * sizeof(Vertex_Buffer_Attribute), ARENA_ALLOC_BASIC));

    if (layout->attributes == nullptr) {
        return false;
    }

    layout->attributes_capacity = attributes_capacity;

    return true;
}

Vertex_Buffer_Attribute *
vertex_buffer_layout_push_attr(Vertex_Buffer_Layout *layout, unsigned int count, GLenum type, size32_t size)
{
    if (layout->attributes_count + 1 > layout->attributes_capacity) {
        return nullptr;
    }

    Vertex_Buffer_Attribute *attribute = layout->attributes + layout->attributes_count;
    attribute->is_normalized = false;
    attribute->type = type;
    attribute->count = count;
    attribute->size = size;

    layout->attributes_count += 1;
    layout->stride += size * count;

    return attribute;
}

Vertex_Buffer_Attribute *
vertex_buffer_layout_push_integer(Vertex_Buffer_Layout *layout, unsigned int count)
{
    size32_t attribute_size = sizeof(int);
    return vertex_buffer_layout_push_attr(layout, count, GL_INT, attribute_size);
}

Vertex_Buffer_Attribute *
vertex_buffer_layout_push_float(Vertex_Buffer_Layout *layout, unsigned int count)
{
    size32_t attribute_size = sizeof(float);
    return vertex_buffer_layout_push_attr(layout, count, GL_FLOAT, attribute_size);
}

void
vertex_buffer_layout_build_attrs(const Vertex_Buffer_Layout *layout)
{
    size_t offset = 0;

    for (unsigned int attribute_index = 0; attribute_index < layout->attributes_count;
         ++attribute_index) {
        Vertex_Buffer_Attribute *attribute = layout->attributes + attribute_index;

        glEnableVertexAttribArray(attribute_index);
        glVertexAttribPointer(
            attribute_index, attribute->count, attribute->type,
            attribute->is_normalized, layout->stride, (void *)offset);

        offset += attribute->size * attribute->count;
    }
}

Camera
make_camera(Camera_ViewMode view_mode)
{
    Camera camera;
    ZERO_STRUCT(&camera);

    camera.position = {0, 0, 3.0f};
    camera.front = {0, 0, -1.0f};
    camera.up = {0, 1.0f, 0};

    camera.yaw = -90.0f;
    camera.pitch = 0.0f;

    camera.speed = 10.0f;
    camera.sensitivity = 0.5f;
    camera.fov = 45.0f;

    camera.near = 0.1f;
    camera.far = 100.0f;

    camera.view_mode = view_mode;

    return camera;
}

void
camera_rotate(Camera *camera, float x_offset, float y_offset)
{
    x_offset *= camera->sensitivity;
    y_offset *= camera->sensitivity;

    camera->yaw += x_offset * 1;
    camera->pitch += y_offset * -1;

    camera->pitch = glm::clamp(camera->pitch, -89.0f, 89.0f);

    float yaw_rad = glm::radians(camera->yaw);
    float pitch_rad = glm::radians(camera->pitch);

    glm::vec3 direction = LITERAL(glm::vec3) {
        glm::cos(yaw_rad) * glm::cos(pitch_rad),
        glm::sin(pitch_rad),
        glm::sin(yaw_rad) * glm::cos(pitch_rad),
    };
    camera->front = glm::normalize(direction);
}

glm::mat4
camera_get_view_matrix(Camera *camera)
{
    return glm::lookAt(
        camera->position, camera->position + camera->front, camera->up);
}

glm::mat4
camera_get_projection_matrix(Camera *camera, int viewport_width, int viewport_height)
{
    if (camera->view_mode == Camera_ViewMode::Perspective) {
        return glm::perspective(
            glm::radians(camera->fov),
            (float)viewport_width / (float)viewport_height,
            camera->near, camera->far);
    }

    if (camera->view_mode == Camera_ViewMode::Orthogonal) {
        return glm::ortho(
            0.0f, (float)viewport_width,
            0.0f, (float)viewport_height,
            camera->near, camera->far);
    }

    assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/09]
    exit(1);
}

int64_t
perf_get_counter_frequency(void)
{
    LARGE_INTEGER perf_frequency_result;
    QueryPerformanceFrequency(&perf_frequency_result);
    int64_t perf_frequency = perf_frequency_result.QuadPart;
    return perf_frequency;
}

int64_t
perf_get_counter(void)
{
    LARGE_INTEGER perf_counter_result;
    QueryPerformanceCounter(&perf_counter_result);
    int64_t perf_counter = perf_counter_result.QuadPart;
    return perf_counter;
}

uint64_t
perf_get_cycles_count(void)
{
    uint64_t cycles_count = __rdtsc();
    return cycles_count;
}

void
perf_block_record_print(const Perf_Block_Record *record)
{
    int64_t perf_frequency = perf_get_counter_frequency();

    int64_t counter_elapsed = record->counter_end - record->counter_begin;
    int64_t ms_elapsed = (1000 * counter_elapsed) / perf_frequency;
    uint64_t cycles_elapsed = record->cycles_end - record->cycles_begin;
    uint64_t mega_cycles_elapsed = cycles_elapsed / (1000 * 1000);

    printf(
        "PERF: %s:%llu@%s [%s]: counter = (%llu) ms = (%llu) | Mc = %llu\n", record->file_path, record->line_number,
        record->function, record->label, counter_elapsed, ms_elapsed, mega_cycles_elapsed);
}

Win32_Key_State
win32_convert_msg_to_key_state(MSG message)
{
    //
    // @ref <https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input>
    //
    Win32_Key_State key_state;


    key_state.vk_code = LOWORD(message.wParam);
    key_state.flags = HIWORD(message.lParam);
    key_state.scan_code = LOBYTE(key_state.flags);

    if (WIN32_KEYSTATE_IS_EXTENDED(&key_state)) {
        key_state.scan_code = MAKEWORD(key_state.scan_code, 0xE0);
    }

    key_state.repeat_count = LOWORD(message.lParam);

    switch (key_state.vk_code) {
    case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
    case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
    case VK_MENU:    // converts to VK_LMENU or VK_RMENU
        key_state.vk_code = LOWORD(MapVirtualKeyW(key_state.scan_code, MAPVK_VSC_TO_VK_EX));
        break;
    }

    return key_state;
}

Clock
make_clock(void)
{
    Clock clock;
    clock.ticks_begin = perf_get_counter();
    clock.ticks_end = 0;
    clock.frequency = perf_get_counter_frequency();
    return clock;
}


double
clock_tick(Clock *clock)
{
    clock->ticks_end = perf_get_counter();
    double elapsed = (double)(clock->ticks_end - clock->ticks_begin) / (double)clock->frequency;
    clock->ticks_begin = perf_get_counter();
    return elapsed;
}

bool
win32_is_vk_pressed(int vk)
{
    short state = GetKeyState(vk);
    bool result = state >> 15;
    return result;
}


bool
load_bitmap_picture_from_file(mm::Arena *arena, Bitmap_Picture *picture, const char *file_path)
{
    FILE *file = fopen(file_path, "r");

    if (file == nullptr) {
        return false;
    }

    bool result = load_bitmap_picture_from_file(arena, picture, file);

    fclose(file);

    return result;
}

bool
load_bitmap_picture_from_file(mm::Arena *arena, Bitmap_Picture *picture, FILE *file)
{
    if (!load_bitmap_picture_info_from_file(picture, file)) {
        return false;
    }

    picture->u.data = mm::arena_alloc_zero(arena, picture->header.file_size, ARENA_ALLOC_BASIC);
    if (picture->u.data == nullptr) {
        return false;
    }

    if (!load_bitmap_picture_pixel_data_from_file(picture, file)) {
        return false;
    }

    return true;
}

bool
load_bitmap_picture_info_from_file(Bitmap_Picture *picture, FILE *file)
{
    // TODO(gr3yknigh1): Make sure that SEEK_SET is in position [2025/03/10]
    fread(&picture->header, sizeof(picture->header), 1, file);
    fread(&picture->dib_header, sizeof(picture->dib_header), 1, file);

    // TODO(gr3yknigh1): Handle read errors [2025/03/10]
    return true;
}

bool
load_bitmap_picture_pixel_data_from_file(Bitmap_Picture *picture, FILE *file)
{
    fseek(file, picture->header.data_offset, SEEK_SET);
    fread(picture->u.data, picture->header.file_size, 1, file);
    fseek(file, 0, SEEK_SET);

    // TODO(gr3yknigh1): Handle read errors [2025/03/10]
    return true;
}

void
gl_make_texture_from_image(void *data, size32_t width, size32_t height, Image_Color_Layout layout, GLenum internal_format)
{
    assert(layout == Image_Color_Layout::BGRA_U8);

    GLenum format = 0, type = 0;

    if (layout == Image_Color_Layout::BGRA_U8) {
        format = GL_BGRA;
        type = GL_UNSIGNED_BYTE;
    }

    // TODO(gr3yknigh1): Need to add support for more formats [2025/02/23]

    assert(format && type); // NOTE(gr3yknigh1): Should not be zero [2025/02/23]
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
}

bool
load_tilemap_from_buffer(Asset_Store *store, char *buffer, size_t buffer_size, Tilemap *tilemap)
{
    Lexer lexer = make_lexer(buffer, buffer_size);

    static constexpr Str8_View s_tilemap_directive = "@tilemap";
    static constexpr Str8_View s_tilemap_image_bmp_format = "bmp";

    Str8_View tilemap_image_path_view;

    while (!lexer_is_end(&lexer)) {
        lexer_skip_whitespace(&lexer);

        // NOTE(gr3yknigh1): Skipping comments [2025/02/24]
        if (lexer_check_peeked(&lexer, "//")) {
            lexer_skip_until_endline(&lexer);
            continue;
        }

        if (lexer_check_peeked( &lexer, s_tilemap_directive )) {
            // TODO(gr3yknigh1): Fix this cast to signed int [2025/02/24]
            lexer_advance(&lexer, (int32_t)s_tilemap_directive.length);
            lexer_skip_whitespace(&lexer);

            assert(lexer_parse_int(&lexer, &tilemap->row_count));
            lexer_skip_whitespace(&lexer);

            assert(lexer_parse_int(&lexer, &tilemap->col_count));
            lexer_skip_whitespace(&lexer);

            assert(lexer_parse_str_to_view(&lexer, &tilemap_image_path_view));
            lexer_skip_whitespace(&lexer);

            // TODO(gr3yknigh1): Add proper error report mechanizm. Error message in window, for example. [2025/02/24]
            assert(lexer_check_peeked( &lexer, s_tilemap_image_bmp_format ));
            tilemap->image.format = Tilemap_Image_Format::Bitmap;
            lexer_advance(&lexer, (int32_t)s_tilemap_image_bmp_format.length);

            lexer_skip_whitespace(&lexer);

            assert(lexer_parse_int(&lexer, &tilemap->tile_x_pixel_count));
            lexer_skip_whitespace(&lexer);

            assert(lexer_parse_int(&lexer, &tilemap->tile_y_pixel_count));
            lexer_skip_whitespace(&lexer);

            // NOTE(gr3yknigh1): Bad assumtion [2025/02/24]
            // assert(Lexer_IsEndline(&lexer));

            // TODO(gr3yknigh1): Move initialization of index array out of here [2025/02/24]
            // NOTE(gr3yknigh1): This can be fixed with adding stage with Token generation, like
            // proper lexers does [2025/02/26]
            tilemap->indexes_count = tilemap->row_count * tilemap->col_count;
            tilemap->indexes = static_cast<int *>(mm::allocate(tilemap->indexes_count * sizeof(*tilemap->indexes)));

            continue;
        }

        if (isdigit(lexer.lexeme)) {
            assert(tilemap->indexes);

            int *indexes_cursor = tilemap->indexes;

            do {
                size_t current_index_index = indexes_cursor - tilemap->indexes;
                assert(current_index_index + 1 <= tilemap->indexes_count);

                assert(lexer_parse_int(&lexer, indexes_cursor));
                lexer_skip_whitespace(&lexer);

                ++indexes_cursor;
            } while(isdigit(lexer.lexeme));

            size_t filled_indexes = indexes_cursor - tilemap->indexes;
            assert(filled_indexes == tilemap->indexes_count);
        }

        lexer_advance(&lexer);
    }

    // TODO(gr3yknigh1): Factor this out [2025/02/24]
    assert(tilemap_image_path_view.length && tilemap_image_path_view.data);

    char *tilemap_image_path = (char *)mm::allocate(tilemap_image_path_view.length + 1);
    mm::zero_memory(tilemap_image_path, tilemap_image_path_view.length + 1);
    assert(str8_view_copy_to_nullterminated(tilemap_image_path_view, tilemap_image_path, tilemap_image_path_view.length + 1));

    // TODO(gr3yknigh1): Generalize format validation [2025/02/24]

    tilemap->image.u.asset = asset_load(store, Asset_Type::Image, tilemap_image_path);
    assert(tilemap->image.u.asset);

    mm::deallocate(tilemap_image_path);

    return tilemap;
}

Asset *
asset_load_tilemap(Asset_Store *store, const char *file)
{
    assert(store && file);

    auto *asset = mm::allocate_struct<Asset>(&store->asset_pool);
    assert(asset);


    asset->type = Asset_Type::Tilemap;
    asset->location.type = Asset_Location_Type::File;
    asset->location.u.file.path = file /*asset_store_resolve_file(file)*/;
    asset->location.u.file.handle = fopen(asset->location.u.file.path.data, "r");
    asset->location.u.file.size = get_file_size(asset->location.u.file.handle);
    assert(asset->location.u.file.handle);

    size_t buffer_size = asset->location.u.file.size + 1;
    void* buffer = mm::allocate(asset->location.u.file.size);
    mm::zero_memory(buffer, buffer_size);

    fread(buffer, buffer_size - 1, 1, asset->location.u.file.handle);

    assert(load_tilemap_from_buffer(store, static_cast<char *>(buffer), buffer_size, &asset->u.tilemap));

    mm::deallocate(buffer);

    asset->state = Asset_State::Loaded;
    fclose(asset->location.u.file.handle);
    asset->location.u.file.handle = nullptr;  // Saying that the file handle is closed

    return asset;
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

bool
path16_get_parent(const wchar_t *path, size_t path_length, Str16_View *out)
{

    out->data = path;

    for (size_t index = path_length - 1; index > 0; --index) {
        wchar_t c = path[index];

        if (c == path16_get_separator()) {
            out->length = index;
            break;
        }
    }

    return true;
}

wchar_t
path16_get_separator()
{
    // TODO(gr3yknigh1): Add support for other platform [2025/03/01]
    return L'\\';
}

void
watch_thread_worker(PVOID param)
{
    Watch_Context *context = (Watch_Context *)param;

    HANDLE watch_dir = CreateFileW(context->target_dir,
        FILE_LIST_DIRECTORY, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    assert(watch_dir && watch_dir != INVALID_HANDLE_VALUE);

    DWORD file_notify_info_buffer_size = sizeof(FILE_NOTIFY_INFORMATION) * 1024;
    FILE_NOTIFY_INFORMATION *file_notify_info = static_cast<FILE_NOTIFY_INFORMATION *>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, file_notify_info_buffer_size));
    assert(file_notify_info);
    // TODO(gr3yknigh1): Fix allocation strategy here. Maybe try to
    // iterate in fixed buffer? [2025/03/01]

    // TODO(gr3yknigh1): Replace with String_Builder [2025/03/10]
    uint64_t target_dir_length = str16_get_length(context->target_dir);
    size_t target_dir_buffer_size = target_dir_length * sizeof(*context->target_dir);
    size_t file_full_path_buffer_capacity = (target_dir_length + MAX_PATH) * sizeof(*context->target_dir);

    void *file_full_path_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, file_full_path_buffer_capacity);
    assert(file_full_path_buffer && "Buy more RAM");

    // NOTE(gr3yknigh1): Clunky! [2025/03/10]
    mm::copy_memory(file_full_path_buffer, static_cast<const void *>(context->target_dir), target_dir_buffer_size);


    while (!context->should_stop.test()) {
        DWORD bytes_returned = 0;
        assert(ReadDirectoryChangesW(
            watch_dir, file_notify_info, file_notify_info_buffer_size, true,
            FILE_NOTIFY_CHANGE_LAST_WRITE, &bytes_returned, nullptr, nullptr
        ));

        const wchar_t *changed_file_relative_path = file_notify_info->FileName;
        const uint64_t changed_file_relative_path_length = file_notify_info->FileNameLength / sizeof(*file_notify_info->FileName);
        const size_t changed_file_relative_path_buffer_size = file_notify_info->FileNameLength;


        if (context->notification_routine != nullptr) {
            // TODO(gr3yknigh1): Replace with some kind of Path_Join function [2025/03/10]
            static_cast<wchar_t *>(file_full_path_buffer)[target_dir_length] = path16_get_separator();
            size_t separator_size = sizeof(wchar_t);
            mm::copy_memory( static_cast<mm::byte *>(file_full_path_buffer) + target_dir_buffer_size + separator_size, static_cast<const void *>(changed_file_relative_path), changed_file_relative_path_buffer_size );

            Str16_View file_name(static_cast<wchar_t *>(file_full_path_buffer), target_dir_length + 1 + changed_file_relative_path_length);
            //                                                                                    ^^^
            // WARN(gr3yknigh1): BECAUSE WE INSERTED SEPARATOR BEFORE!!! [2025/03/10]
            // Move it to Path_Join as soon as possible
            //

            context->notification_routine(context, file_name, (File_Action)file_notify_info->Action, context->parameter);
        }
    }

    assert(HeapFree(GetProcessHeap(), 0, file_full_path_buffer));
    assert(HeapFree(GetProcessHeap(), 0, file_notify_info));
}

void
gl_clear_all_errors(void)
{
    while (glGetError() != GL_NO_ERROR) {
    }
}

void
gl_die_on_first_error(void)
{
    GLenum errorCode = GL_NO_ERROR;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        printf("E: GL: Error code=(%i)\n", errorCode);
        DebugBreak(); // XXX
        ExitProcess(1);
    }
}

void
gl_print_debug_info(void)
{
    const char *vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    const char *shading_language_version = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    printf("Graphics info: %s (Vendor %s) \nOpenGL %s, GLSL %s\n", renderer, vendor, version, shading_language_version);
}

Gameplay
load_gameplay(const char *module_path)
{
    Gameplay gameplay;

    const char *loaded_gameplay = "garden_gameplay.loaded.dll";
    if (!CopyFileA(module_path, loaded_gameplay, FALSE)) {
        DWORD last_error = GetLastError();
        assert(!last_error);
    }

    // TODO: add move

    gameplay.module = LoadLibraryA(loaded_gameplay);
    assert(gameplay.module && gameplay.module != INVALID_HANDLE_VALUE);

    gameplay.on_init = reinterpret_cast<Game_On_Init_Fn_Type *>(GetProcAddress(gameplay.module, GAME_ON_INIT_FN_NAME));
    assert(gameplay.on_init);

    gameplay.on_load = reinterpret_cast<Game_On_Load_Fn_Type *>(GetProcAddress(gameplay.module, GAME_ON_LOAD_FN_NAME));
    assert(gameplay.on_load);

    gameplay.on_tick = reinterpret_cast<Game_On_Tick_Fn_Type *>(GetProcAddress(gameplay.module, GAME_ON_TICK_FN_NAME));
    assert(gameplay.on_tick);

    gameplay.on_draw = reinterpret_cast<Game_On_Draw_Fn_Type *>(GetProcAddress(gameplay.module, GAME_ON_DRAW_FN_NAME));
    assert(gameplay.on_draw);

    gameplay.on_fini = reinterpret_cast<Game_On_Fini_Fn_Type *>(GetProcAddress(gameplay.module, GAME_ON_FINI_FN_NAME));
    assert(gameplay.on_fini);

    return gameplay;
}

void
unload_gameplay(Gameplay *gameplay)
{
    assert(FreeLibrary(gameplay->module));
}

size_t
get_file_size(FILE *file)
{
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(file_size);

    return file_size;
}

bool
make_asset_store_from_folder(Asset_Store *store, const char *folder_path)
{
    assert(store && folder_path);

    mm::zero_struct(store);

    store->place = Asset_Store_Place::Folder;
    store->asset_pool = mm::make_block_allocator(Asset_Store::max_asset_count, sizeof(Asset), sizeof(Asset), Asset_Store::max_asset_count);
    store->u.folder = folder_path;

    store->asset_content = mm::make_block_allocator();

    return true;
}

bool
asset_store_destroy(Asset_Store *store)
{
    assert(store);

    assert(mm::destroy_block_allocator(&store->asset_pool));
    assert(mm::destroy_block_allocator(&store->asset_content));

    mm::zero_struct(store);

    return true;
}

Asset *
asset_load(Asset_Store *store, Asset_Type type, const char *file)
{
    // TODO(gr3yknigh1): Handle errors and mark asset as failed to load: Asset_State::LoadFailure [2025/03/10]

    assert(store && file);

    Asset *asset = mm::allocate_struct<Asset>(&store->asset_pool);
    assert(asset);

    asset->type = type;

    Asset_Location *location = &asset->location;
    location->type          = Asset_Location_Type::File;
    location->u.file.path   = file /*asset_store_resolve_file(file)*/;
    location->u.file.handle = fopen(location->u.file.path.data, "r");
    assert(location->u.file.handle);

    location->u.file.size   = get_file_size(location->u.file.handle);

    if (asset->type == Asset_Type::Image) {
        // NOTE(gr3yknigh1): Assume that file is path to BMP image [2025/03/10]
        Bitmap_Picture picture;
        assert(load_bitmap_picture_info_from_file(&picture, location->u.file.handle));

        // TODO(gr3yknigh1): Allocate less data, because file_size includes size of metadata [2025/03/10]
        //
        picture.u.data = mm::allocate(&store->asset_content, picture.header.file_size);
        assert(load_bitmap_picture_pixel_data_from_file(&picture, location->u.file.handle));

        assert(asset_from_bitmap_picture(asset, &picture));
    } else if (asset->type == Asset_Type::Shader) {

        asset->u.shader.source_code = static_cast<char *>(mm::allocate(&store->asset_content, location->u.file.size));
        mm::zero_memory(asset->u.shader.source_code, location->u.file.size);
        fread(asset->u.shader.source_code, location->u.file.size, 1, location->u.file.handle);

        Shader_Compile_Result result = compile_shader(asset->u.shader.source_code, location->u.file.size);
        asset->u.shader.program_id = result.shader_program_id;
        assert(asset->u.shader.program_id);

        //
        // TODO(gr3yknigh1): Delete shader modules. They are no longer needed. [2025/03/28]
        //

        #if 0
        /* After linking shaders no longer needed. */
        glDeleteShader(vertex_module->id);
        glDeleteShader(fragment_module->id);
        #endif
    } else if (asset->type == Asset_Type::Tilemap) {

        size_t buffer_size = asset->location.u.file.size + 1;
        void* buffer = mm::allocate(asset->location.u.file.size);
        mm::zero_memory(buffer, buffer_size);

        fread(buffer, buffer_size - 1, 1, asset->location.u.file.handle);

        assert(load_tilemap_from_buffer(store, static_cast<char *>(buffer), buffer_size, &asset->u.tilemap));

        mm::deallocate(buffer);

    } else {
        // TODO(gr3yknigh1): Handle an error [2025/04/06]
    }

    asset->state = Asset_State::Loaded;
    fclose(location->u.file.handle);
    location->u.file.handle = nullptr;  // Saying that the file handle is closed

    return asset;
}

#if 0
Asset *
asset_load_image(Asset_Store *store, const char *file)
{
    // TODO(gr3yknigh1): Handle errors and mark asset as failed to load: Asset_State::LoadFailure [2025/03/10]

    assert(store && file);

    auto *asset = mm::allocate_struct<Asset>(&store->asset_pool);
    assert(asset);

    asset->type = Asset_Type::Image;
    asset->location.type = Asset_Location_Type::File;
    asset->location.u.file.path = file /*asset_store_resolve_file(file)*/;
    asset->location.u.file.handle = fopen(asset->location.u.file.path.data, "r");
    asset->location.u.file.size = get_file_size(asset->location.u.file.handle);
    assert(asset->location.u.file.handle);

    // NOTE(gr3yknigh1): Assume that file is path to BMP image [2025/03/10]

    Bitmap_Picture picture;
    assert(load_bitmap_picture_info_from_file(&picture, asset->location.u.file.handle));

    //
    // TODO(gr3yknigh1): Allocate less data, because file_size includes size of metadata [2025/03/10]
    //
    picture.u.data = mm::allocate(&store->asset_content, picture.header.file_size);
    assert(load_bitmap_picture_pixel_data_from_file(&picture, asset->location.u.file.handle));

    assert(asset_from_bitmap_picture(asset, &picture));

    asset->state = Asset_State::Loaded;

    fclose(asset->location.u.file.handle);
    asset->location.u.file.handle = nullptr;  // Saying that the file handle is closed

    return asset;
}

Asset *
asset_load_shader(Asset_Store *store, const char *file)
{
    assert(store && file);

    auto *asset = mm::allocate_struct<Asset>(&store->asset_pool);
    assert(asset);

    asset->type = Asset_Type::Shader;
    asset->location.type = Asset_Location_Type::File;
    asset->location.u.file.path = file /*asset_store_resolve_file(file)*/;
    asset->location.u.file.handle = fopen(asset->location.u.file.path.data, "r");
    asset->location.u.file.size = get_file_size(asset->location.u.file.handle);
    assert(asset->location.u.file.handle);

    asset->u.shader.source_code = static_cast<char *>(mm::allocate(&store->asset_content, asset->location.u.file.size));
    mm::zero_memory(asset->u.shader.source_code, asset->location.u.file.size);

    fread(asset->u.shader.source_code, asset->location.u.file.size, 1, asset->location.u.file.handle);

    Shader_Compile_Result result = compile_shader(asset->u.shader.source_code, asset->location.u.file.size);
    asset->u.shader.program_id = result.shader_program_id;
    assert(asset->u.shader.program_id);

    //
    // TODO(gr3yknigh1): Delete shader modules. They are no longer needed. [2025/03/28]
    //

    #if 0
    /* After linking shaders no longer needed. */
    glDeleteShader(vertex_module->id);
    glDeleteShader(fragment_module->id);
    #endif


    fclose(asset->location.u.file.handle);
    asset->location.u.file.handle = nullptr;  // Saying that the file handle is closed
    asset->state = Asset_State::Loaded;
    return asset;
}
#endif

bool
asset_reload(Asset_Store *store, Asset *asset)
{
    assert(store && asset);

    if (asset->state == Asset_State::Loaded) {
        assert(asset_unload_content(store, asset));
    }

    if (asset->location.type == Asset_Location_Type::File) {
        if (asset->location.u.file.handle == nullptr) {
            // TODO(gr3yknigh1): Wrap fopen in function which accepts Str8_View-s [2025/03/10]
            asset->location.u.file.handle = fopen(asset->location.u.file.path.data, "r");
            asset->location.u.file.size = get_file_size(asset->location.u.file.handle);
            assert(asset->location.u.file.handle);
        }

        if (asset->type == Asset_Type::Image) {
            Bitmap_Picture picture;
            assert(load_bitmap_picture_info_from_file(&picture, asset->location.u.file.handle));

            //
            // TODO(gr3yknigh1): Allocate less data, because file_size includes size of metadata [2025/03/10]
            //
            picture.u.data = mm::allocate(&store->asset_content, picture.header.file_size);
            assert(load_bitmap_picture_pixel_data_from_file(&picture, asset->location.u.file.handle));

            assert(asset_from_bitmap_picture(asset, &picture));
        }

        if (asset->type == Asset_Type::Shader) {

            asset->u.shader.source_code = static_cast<char *>(mm::allocate(&store->asset_content, asset->location.u.file.size));
            mm::zero_memory(asset->u.shader.source_code, asset->location.u.file.size);
            fread(asset->u.shader.source_code, asset->location.u.file.size, 1, asset->location.u.file.handle);

            Shader_Compile_Result result = compile_shader(asset->u.shader.source_code, asset->location.u.file.size);

            glDeleteProgram(asset->u.shader.program_id); // @cleanup
            asset->u.shader.program_id = result.shader_program_id;
            assert(asset->u.shader.program_id);
        }

        if (asset->location.u.file.handle != nullptr) {
            fclose(asset->location.u.file.handle);
            asset->location.u.file.handle = nullptr;  // Saying that the file handle is closed
        }
    }

    asset->state = Asset_State::Loaded;
    return true;
}

bool
asset_from_bitmap_picture(Asset *asset, Bitmap_Picture *picture)
{
    //
    // Copy image data to more generalized structure
    //
    asset->u.image.width = picture->dib_header.width;
    asset->u.image.height = picture->dib_header.height;

    asset->u.image.layout = Image_Color_Layout::Nothing;
    if (picture->dib_header.compression_method ==  Bitmap_Picture_Compression_Method::Bitfields) {
        asset->u.image.layout = Image_Color_Layout::BGRA_U8;
    }
    assert(asset->u.image.layout != Image_Color_Layout::Nothing);

    asset->u.image.pixels.data = picture->u.data;

    return true;
}


bool
asset_unload_content(Asset_Store *store, Asset *asset)
{
    assert(store && asset);

    assert(asset->state == Asset_State::Loaded);

    bool result = true;

    if (asset->type == Asset_Type::Image) {
        result = mm::reset(&store->asset_content, asset->u.image.pixels.data );
    } else if (asset->type == Asset_Type::Shader) {
        result = mm::reset(&store->asset_content, asset->u.shader.source_code);
    } else {
        result = false;
    }

    if (result) {
        asset->state = Asset_State::Unloaded;
    } else {
        asset->state = Asset_State::UnloadFailure;
    }

    return result;
}

bool make_watch_context(Watch_Context *context, void *parameter, const wchar_t *target_dir, watch_notification_routine_t notification_routine)
{
    assert(context);

    mm::zero_struct(context);

    context->should_stop.clear();

    if (!target_dir) {

        //
        // TODO(gr3yknigh1): Wrap all string manipulation in more simple utility functions. String_Builder? [2025/03/01]
        //

        DWORD image_path_buffer_size = MAX_PATH * sizeof(wchar_t);
        wchar_t *image_path_buffer = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, image_path_buffer_size);
        assert(image_path_buffer);

        DWORD bytes_written = GetModuleFileNameW(nullptr, image_path_buffer, image_path_buffer_size);
        DWORD last_error = GetLastError();
        assert(last_error == ERROR_SUCCESS);

        Str16_View image_directory_view;
        assert(path16_get_parent(image_path_buffer, bytes_written, &image_directory_view));

        wchar_t *image_directory_buffer = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, (image_directory_view.length + 1) * sizeof(*image_directory_view.data));
        assert(image_directory_buffer);

        assert(str16_view_copy_to_nullterminated(image_directory_view, image_directory_buffer, (image_directory_view.length + 1) * sizeof(*image_directory_view.data)));
        assert(HeapFree(GetProcessHeap(), 0, image_path_buffer));

        target_dir = image_directory_buffer;
    }

    context->target_dir = target_dir;  // TODO(gr3yknigh1): Do free somewhere [2025/03/01]
    context->parameter = parameter;
    context->notification_routine = notification_routine;

    return true;
}

DWORD
watch_context_launch_thread(Watch_Context *context, PHANDLE out_thread_handle)
{
    DWORD watch_thread_id = 0;
    HANDLE watch_thread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)watch_thread_worker, context, 0, &watch_thread_id);
    assert(watch_thread && watch_thread != INVALID_HANDLE_VALUE);

    if (out_thread_handle != nullptr) {
        *out_thread_handle = watch_thread;
    }
    return watch_thread_id;
}

static void
asset_watch_routine([[maybe_unused]] Watch_Context *watch_context, const Str16_View file_name, File_Action action, void *parameter)
{
    if (action != File_Action::Modified) {
        return;
    }

    Reload_Context *reload_context = reinterpret_cast<Reload_Context *>(parameter);

    if (reload_context->store->place == Asset_Store_Place::Folder) {
        FOR_EACH_ASSET(it, reload_context->store) {
            if (it->location.type != Asset_Location_Type::File) {
                continue;
            }

            if (str16_view_is_equals(file_name, it->location.u.file.path)) {
                it->should_reload.test_and_set();
                return;
            }
        }
    }

    if (str16_view_endswith(file_name, STRINGIFY(GARDEN_GAMEPLAY_DLL_NAME))) {
        reload_context->should_reload_gameplay.test_and_set();
    }
}

Shader_Compile_Result
compile_shader(char *source_code, size_t file_size)
{
    Lexer lexer = make_lexer(source_code, file_size);

    constexpr static Str8_View s_line_comment = "//";
    constexpr static Str8_View s_begin_directive = "#begin";
    constexpr static Str8_View s_vertex_literal = "vertex";
    constexpr static Str8_View s_fragment_literal = "fragment";

    Str8_View vertex_source{};
    Str8_View fragment_source{};

    while (!lexer_is_end(&lexer)) {
        lexer_skip_whitespace(&lexer);

        // NOTE(gr3yknigh1): Skipping comments [2025/02/24]
        // TODO(gr3yknigh1): Skip /**/ comments [2025/03/19]
        if (lexer_check_peeked(&lexer, s_line_comment)) {
            lexer_skip_until_endline(&lexer);
            continue;
        }

        if (lexer_check_peeked_and_advance(&lexer, s_begin_directive)) {
            lexer_skip_whitespace(&lexer); // NOTE: Skipping spaces

            if (lexer_check_peeked_and_advance(&lexer, s_vertex_literal)) {
                assert(vertex_source.empty());
                lexer_skip_whitespace(&lexer);

                vertex_source = lexer_skip_until(&lexer, s_begin_directive);
                continue;
            }  else if (lexer_check_peeked_and_advance(&lexer, s_fragment_literal)) {
                assert(fragment_source.empty());
                lexer_skip_whitespace(&lexer);

                fragment_source = lexer_skip_until(&lexer, s_begin_directive);
                continue;
            }
        }

        lexer_advance(&lexer);
    }

    assert(!vertex_source.empty() && !fragment_source.empty());

    // TODO(gr3yknigh1): Move to arena [2025/03/28]
    char *vertex_source_buffer = static_cast<char *>(mm::allocate(vertex_source.length + 1));
    mm::zero_memory(vertex_source_buffer, vertex_source.length + 1);
    assert(str8_view_copy_to_nullterminated(vertex_source, vertex_source_buffer, vertex_source.length + 1));
    GLuint vertex_module_id = compile_shader_from_str8(vertex_source_buffer, Shader_Module_Type::Vertex);
    assert(vertex_module_id);
    mm::deallocate(vertex_source_buffer);

    // TODO(gr3yknigh1): Move to arena [2025/03/28]
    char *fragment_source_buffer = static_cast<char *>(mm::allocate(fragment_source.length + 1));
    mm::zero_memory(fragment_source_buffer, fragment_source.length + 1);
    assert(str8_view_copy_to_nullterminated(fragment_source, fragment_source_buffer, fragment_source.length + 1));
    GLuint fragment_module_id = compile_shader_from_str8(fragment_source_buffer, Shader_Module_Type::Fragment);
    assert(fragment_module_id);
    mm::deallocate(fragment_source_buffer);

    Shader_Compile_Result result{};
    result.shader_program_id = link_shader_program(vertex_module_id, fragment_module_id);
    return result;
}

bool
asset_image_send_to_gpu(Asset_Store *store, Asset *asset, int unit, Shader *shader)
{
    assert(asset->type == Asset_Type::Image);

    asset->u.image.unit = unit;
    glActiveTexture(GL_TEXTURE0 + asset->u.image.unit);
    glGenTextures(1, &asset->u.image.id);
    glBindTexture(GL_TEXTURE_2D, asset->u.image.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_make_texture_from_image(
        asset->u.image.pixels.data, asset->u.image.width, asset->u.image.height,
        asset->u.image.layout, GL_RGBA8);
    glGenerateMipmap(GL_TEXTURE_2D);

    // NOTE: After loading atlas in GPU, we do not need to keep it in RAM.
    assert(asset_unload_content(store, asset));

    if (shader) {
        GLint texture_uniform_loc = glGetUniformLocation(shader->program_id, "u_texture");
        assert(texture_uniform_loc != -1);

        glUniform1i(texture_uniform_loc, asset->u.image.unit);
    }

    return true;
}

bool
make_vertex_buffer(Vertex_Buffer *buffer)
{
    assert(buffer);

    glGenVertexArrays(1, &buffer->vertex_array_id);
    glBindVertexArray(buffer->vertex_array_id);

    glGenBuffers(1, &buffer->id);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->id);

    return true;
}

bool
bind_vertex_buffer(Vertex_Buffer *buffer)
{
    assert(buffer);
    glBindVertexArray(buffer->vertex_array_id);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->id);

    return true;
}

int
get_offset_from_coords_of_2d_grid_array_rm(int width, int x, int y)
{
    return width * y + x;
}
