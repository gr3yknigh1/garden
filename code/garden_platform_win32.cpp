//
// FILE          code\garden_platform_win32.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//
#include <atomic>   // std::atomic_flag

#include <math.h>   // sqrtf
#include <assert.h> // assert
#include <stdio.h>  // puts, printf, FILE, fopen, freopen, fseek, fclose
#include <ctype.h>  // isspace

#include <windows.h>
#include <intrin.h> // __rdtsc

// NOTE(i.akkuzin): For `Camera` struct. [2025/02/09]
#undef near
#undef far

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "garden_gameplay.h"
#include "garden_platform.h"

#if !defined(STRINGIFY_IMPL)
    #define STRINGIFY_IMPL(X) #X
#endif

#if !defined(STRINGIFY)
    #define STRINGIFY(X) STRINGIFY_IMPL(X)
#endif

#if !defined(GARDEN_ASSET_DIR)
    #error "Dev asset directory is not defined!"
#else
    #pragma message( "Using DEV asset dir: '" STRINGIFY(GARDEN_ASSET_DIR) "'" )
#endif

#if !defined(MAKE_FLAG)
    #define MAKE_FLAG(INDEX) (1 << (INDEX))
#endif

#if !defined(HAS_FLAG)
    #define HAS_FLAG(MASK, FLAG) (((MASK) & (FLAG)) == (FLAG))
#endif

#if !defined(LITERAL)
    #if defined(__cplusplus)
        #define LITERAL(X) X
    #else
        #define LITERAL(X) (X)
    #endif
#endif

#if !defined(STATIC_ARRAY_COUNT)
    #define STATIC_ARRAY_COUNT(ARRAY_PTR) (sizeof((ARRAY_PTR)) / sizeof(*(ARRAY_PTR)))
#endif

#if !defined(ZERO_STRUCT)
    #define ZERO_STRUCT(STRUCT_PTR) ZeroMemory((STRUCT_PTR), sizeof(*(STRUCT_PTR)))
#endif

typedef int bool32_t;
typedef unsigned int size32_t;
typedef int packed_rgba_t;

static_assert(sizeof(packed_rgba_t) == 4);

#if !defined(MAKE_PACKED_RGBA)
    #define MAKE_PACKED_RGBA(R, G, B, A) (((R) << 24) + ((G) << 16) + ((B) << 8) + (A))
#endif

#pragma pack(push, 1)
struct Vertex {
    float x, y;
    float s, t;
    packed_rgba_t color;
};
#pragma pack(pop)

static_assert(sizeof(Vertex) == (sizeof(float) * 4 + sizeof(packed_rgba_t)));

#pragma pack(push, 1)
struct Color_RGBA_U8 {
    uint8_t r, g, b, a;
};
#pragma pack(pop)

typedef Color_RGBA_U8 Color4;

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

struct Str8_View8 {
    const char *data;
    size_t length;

    constexpr Str8_View8() noexcept : data(nullptr), length(0) {}
    constexpr Str8_View8(const char *data_) noexcept : data(data_), length(str8_get_length(data_)) {}
    constexpr Str8_View8(const char *data_, size_t length_) noexcept : data(data_), length(length_) {}
};

inline Str8_View8
str8_view_capture_until(const char **cursor, char until)
{
    Str8_View8 sv;

    sv.data = *cursor;

    while (**cursor != until) {
        (*cursor)++;
    }

    sv.length = *cursor - sv.data;

    return sv;
}

inline bool
str8_view_copy_to_nullterminated(Str8_View8 sv, char *out_buffer, size_t out_buffer_size)
{
    if (sv.length + 1 > out_buffer_size) {
        return false;
    }

    memcpy((void *)out_buffer, sv.data, sv.length);
    out_buffer[sv.length] = 0;
    return true;
}

constexpr bool
str8_view_is_equals(Str8_View8 a, Str8_View8 b)
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
str8_view_is_equals(Str8_View8 a, const char *str)
{
    Str8_View8 b(str);
    return str8_view_is_equals(a, b);
}

constexpr size_t
str8_get_length(const wchar_t *s) noexcept
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
    constexpr Str16_View(const wchar_t *data_) noexcept : data(data_), length(str8_get_length(data_)) {}
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


//
// Allocators:
//

struct Arena {
    void *data;
    size_t capacity;
    size_t occupied;
};

Arena make_arena(size_t capacity);
bool  free_arena(Arena *arena);

#define ARENA_ALLOC_BASIC   MAKE_FLAG(0)
#define ARENA_ALLOC_POPABLE MAKE_FLAG(1)

void * arena_alloc(Arena *arena, size_t size, int options);
void * arena_alloc_zero(Arena *arena, size_t size, int options);
bool   arena_pop(Arena *arena, void *data);
size_t arena_reset(Arena *arena);


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

enum class Camera_ViewMode {
    Perspective,
    Orthogonal
};

struct Camera {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;

    float yaw;
    float pitch;

    float speed;
    float sensitivity;
    float fov;

    float near;
    float far;

    Camera_ViewMode view_mode;
};

Camera make_camera(Camera_ViewMode view_mode);

void      camera_rotate(Camera *camera, float x_offset, float y_offset);
glm::mat4 camera_get_view_matrix(Camera *camera);
glm::mat4 camera_get_projection_matrix(Camera *camera, int viewport_width, int viewport_height);

//
// OpenGL API wrappers
//
GLuint compile_shader_from_str8(Arena *arena, const char *string, GLenum type);
GLuint compile_shader_from_file(Arena *arena, const char *file_path, GLenum type);
GLuint link_shader_program(Arena *arena, GLuint vertex_shader, GLuint fragment_shader);

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
bool make_vertex_buffer_layout(Arena *arena, Vertex_Buffer_Layout *layout, size32_t attributes_capacity);

Vertex_Buffer_Attribute *vertex_buffer_layout_push_attr   (Vertex_Buffer_Layout *layout, unsigned int count, GLenum type, size_t size);
Vertex_Buffer_Attribute *vertex_buffer_layout_push_float  (Vertex_Buffer_Layout *layout, unsigned int count);
Vertex_Buffer_Attribute *vertex_buffer_layout_push_integer(Vertex_Buffer_Layout *layout, unsigned int count);

//
// @brief Initializes vertex buffer layout.
//
// @pre
//     glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)
//     glBindVertexArray(vertex_array);
//
void vertex_buffer_layout_build_attrs(const Vertex_Buffer_Layout *layout);


struct Rect_F32 {
    float x;
    float y;
    float width;
    float height;
};


// TODO(gr3yknigh1): Replace with integers? [2025/02/26]
struct Atlas {
    float x_pixel_count;
    float y_pixel_count;
};


//
// @param[out] rect Output array of vertexes
//
size_t generate_rect(Vertex *rect, float x, float y, float width, float height, Color4 color);

//
// @param[out] rect Output array of vertexes
//
size_t generate_rect_with_atlas(Vertex *rect, float x, float y, float width, float height, Rect_F32 atlas_location, Atlas *altas, Color4 color);

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
// Linear:
//

float absolute(float x);
void normalize_vector2f(float *x, float *y);

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

bool load_bitmap_picture_from_file(Arena *arena, Bitmap_Picture *picture, const char *file_path);

//
// @pre
//   - Bind target texture with glBindTexture(GL_TEXTURE_2D, ...);
//
void gl_make_texture_from_bitmap_picture(void *data, size32_t width, size32_t height, Bitmap_Picture_Compression_Method compression_method, GLenum internal_format);


void gl_clear_all_errors();
void gl_die_on_first_error(void);

//
// Tilemaps:
//

enum struct Tilemap_Image_Format {
    Bitmap,
};

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
            Bitmap_Picture *bitmap;
        } u;
    } image;
};

Tilemap *load_tilemap_from_file(Arena *arena, const char *file_path);

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

typedef int watch_ext_mask_t;
typedef void (* watch_notification_routine_t)(const Str16_View file_name, watch_ext_mask_t ext, File_Action action, void *parameter);

struct Watch_Dir_Context {
    std::atomic_flag should_stop;
    const wchar_t *target_dir;
    watch_ext_mask_t watch_exts;
    watch_notification_routine_t notification_routine;
    void *parameter;
};

void make_watch_dir_context(Watch_Dir_Context *context);

void watch_dir_thread_worker(PVOID param);

//
// Assets:
//

struct Reload_Context {
    Arena *arena;
    Bitmap_Picture *picture; //  TODO(gr3yknigh1): Make it generic-asset [2025/03/01]
    GLuint texture_id;
    std::atomic_flag should_reload;
    std::atomic_flag should_reload_gameplay;
};

struct Gameplay {
    HMODULE module;

    Game_On_Init_Fn_Type * on_init;
    Game_On_Tick_Fn_Type * on_tick;
    Game_On_Draw_Fn_Type * on_draw;
    Game_On_Fini_Fn_Type * on_fini;
};

Gameplay load_gameplay(const char *module_path);
void unload_gameplay(Gameplay *gameplay);

int WINAPI
wWinMain(HINSTANCE instance, HINSTANCE previous_instance, PWSTR command_line, int cmd_show)
{
    (void)previous_instance;
    (void)command_line;

    //
    // Window initialization
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

    int window_x = window_rect.left;
    int window_y = window_rect.bottom;
    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;

    //
    // OpenGL settings:
    //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //
    // Game initalization:
    //
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    Arena shader_compilation_arena = make_arena(1024 * 10);

    GLuint basic_vert_shader = compile_shader_from_file(&shader_compilation_arena, STRINGIFY(GARDEN_ASSET_DIR) "\\basic.vert.glsl", GL_VERTEX_SHADER);
    GLuint basic_frag_shader = compile_shader_from_file(&shader_compilation_arena, STRINGIFY(GARDEN_ASSET_DIR) "\\basic.frag.glsl", GL_FRAGMENT_SHADER);
    GLuint basic_shader_program = link_shader_program(&shader_compilation_arena, basic_vert_shader, basic_frag_shader);

    free_arena(&shader_compilation_arena);

    /* After linking shaders no longer needed. */
    glDeleteShader(basic_vert_shader);
    glDeleteShader(basic_frag_shader);

    glUseProgram(basic_shader_program);

    GLint model_uniform_loc = glGetUniformLocation(basic_shader_program, "model");
    assert(model_uniform_loc != -1);

    GLint projection_uniform_loc = glGetUniformLocation(basic_shader_program, "projection");
    assert(projection_uniform_loc != -1);

    Camera camera = make_camera(Camera_ViewMode::Orthogonal);

    glm::mat4 model = glm::identity<glm::mat4>();
    glm::mat4 projection = camera_get_projection_matrix(&camera, window_width, window_height);

    glUniformMatrix4fv(model_uniform_loc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(projection_uniform_loc, 1, GL_FALSE, glm::value_ptr(projection));

    GLuint vertex_array = 0;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

    Arena page_arena = make_arena(1024);

    Vertex_Buffer_Layout vertex_buffer_layout;
    assert(make_vertex_buffer_layout(&page_arena, &vertex_buffer_layout, 4));

    assert(vertex_buffer_layout_push_float(&vertex_buffer_layout, 2));
    assert(vertex_buffer_layout_push_float(&vertex_buffer_layout, 2));
    assert(vertex_buffer_layout_push_integer(&vertex_buffer_layout, 1));

    vertex_buffer_layout_build_attrs(&vertex_buffer_layout);

    arena_reset(&page_arena);

    //
    // Media:
    //

    Arena asset_arena = make_arena(1024000);
    Bitmap_Picture atlas_picture;
    assert(load_bitmap_picture_from_file(&asset_arena, &atlas_picture, STRINGIFY(GARDEN_ASSET_DIR) "\\garden_atlas.bmp"));

    //
    // Setup entity atlas:
    //

    glActiveTexture(GL_TEXTURE0);

    GLuint atlas_texture;
    glGenTextures(1, &atlas_texture);
    glBindTexture(GL_TEXTURE_2D, atlas_texture);

    gl_make_texture_from_bitmap_picture(
        atlas_picture.u.data, atlas_picture.dib_header.width, atlas_picture.dib_header.height,
        atlas_picture.dib_header.compression_method, GL_RGBA8);

    arena_reset(&asset_arena);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //
    // Hot-reload: Setup
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

    Watch_Dir_Context watch_context;
    make_watch_dir_context(&watch_context);

    Reload_Context reload_context;
    reload_context.texture_id = atlas_texture;
    reload_context.arena = &asset_arena;
    reload_context.picture = &atlas_picture;

    watch_context.target_dir = image_directory_buffer; // TODO(gr3yknigh1): Do free somewhere [2025/03/01]
    watch_context.watch_exts = WATCH_EXT_BMP | WATCH_EXT_DLL;
    watch_context.parameter = &reload_context;
    watch_context.notification_routine = []( const Str16_View file_name, watch_ext_mask_t ext, File_Action action, void *parameter ) {
        if (action != File_Action::Modified) {
            return;
        }
        Reload_Context *context = (Reload_Context *)parameter;

        if (ext == WATCH_EXT_BMP && str16_view_is_equals(file_name, L"assets\\garden_atlas.bmp")) {
            context->should_reload.test_and_set();
        }

        if (ext == WATCH_EXT_DLL && str16_view_is_equals(file_name, L"build\\Debug\\garden_gameplay.dll")) {
            context->should_reload_gameplay.test_and_set();
        }
        return;
    };

    DWORD watch_thread_id = 0;
    HANDLE watch_thread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)watch_dir_thread_worker, &watch_context, 0, &watch_thread_id);
    assert(watch_thread && watch_thread != INVALID_HANDLE_VALUE);
    // assert(HeapFree(GetProcessHeap(), 0, image_directory_buffer));

    //
    // Setup tilemap atlas:
    //

    // TODO(gr3yknigh1): Do it later [2025/02/25]
    Tilemap *tilemap = load_tilemap_from_file(&asset_arena, "demo.scene.td");
    glActiveTexture(GL_TEXTURE1);

    GLuint tilemap_texture;
    glGenTextures(1, &tilemap_texture);
    glBindTexture(GL_TEXTURE_2D, tilemap_texture);

    GLint atlas_texture_uniform_loc = glGetUniformLocation(basic_shader_program, "u_texture");
    assert(atlas_texture_uniform_loc != -1);

    glUniform1ui(atlas_texture_uniform_loc, atlas_texture);

    //
    // Game mainloop:
    //
    Arena geometry_arena = make_arena(sizeof(Vertex) * 1024);

    float player_x = 0, player_y = 0, player_speed = 300.0f;

    Clock clock = make_clock();

    Input_State input_state;
    ZERO_STRUCT(&input_state);

    uint64_t frame_counter = 0;

    /// XXX
    static Rect_F32 atlas_location = { 0, 0, 16, 16 };

    //
    // Load game code:
    //

    Gameplay gameplay = load_gameplay(STRINGIFY(GARDEN_GAMEPLAY_DLL_NAME));

    Platform_Context platform_context;
    ZERO_STRUCT(&platform_context);

    Game_Context *game_context = reinterpret_cast<Game_Context *>(gameplay.on_init(&platform_context));

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

                    if (key.vk_code == 0x45) {  // 0x45 - `E` key
                        atlas_location = { 0, 0, 16, 16 };
                    }

                    if (key.vk_code == 0x46) {  // 0x46 - `F` key
                        atlas_location = { 16, 0, 16, 16 };
                    }

                    if (!WIN32_KEYSTATE_IS_RELEASED(&key)) {
                        if (key.vk_code == VK_LEFT) {
                            input_state.x_direction = -1;
                        } else if (key.vk_code == VK_RIGHT) {
                            input_state.x_direction = +1;
                        }

                        if (key.vk_code == VK_DOWN) {
                            input_state.y_direction = -1;
                        } else if (key.vk_code == VK_UP) {
                            input_state.y_direction = +1;
                        }
                    } else {
                        if ((key.vk_code == VK_LEFT   && !win32_is_vk_pressed(VK_RIGHT))
                        || ((key.vk_code == VK_RIGHT) && !win32_is_vk_pressed(VK_LEFT))) {
                            input_state.x_direction = 0;
                        }

                        if ((key.vk_code == VK_DOWN && !win32_is_vk_pressed(VK_UP))
                         || (key.vk_code == VK_UP && !win32_is_vk_pressed(VK_DOWN))) {
                            input_state.y_direction = 0;
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

        gameplay.on_tick(&platform_context, game_context, static_cast<float>(dt));

        PERF_BLOCK_BEGIN(UPDATE);

            //
            // Asset Hot reload:
            //

            if (reload_context.should_reload.test()) {
                reload_context.should_reload.clear();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, reload_context.texture_id);

                assert(load_bitmap_picture_from_file(reload_context.arena, reload_context.picture, STRINGIFY(GARDEN_ASSET_DIR) "\\garden_atlas.bmp"));

                gl_make_texture_from_bitmap_picture(
                    reload_context.picture->u.data, reload_context.picture->dib_header.width, reload_context.picture->dib_header.height,
                    reload_context.picture->dib_header.compression_method, GL_RGBA8);

                arena_reset(reload_context.arena);

                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }

            if (absolute(input_state.x_direction) >= 1.0f && absolute(input_state.y_direction) >= 1.0f) {
                // TODO(gr3yknigh1): Fix strange floating-point bug for diagonal movement [2025/02/20]
                normalize_vector2f(&input_state.x_direction, &input_state.y_direction);
            }

            player_x += player_speed * input_state.x_direction * (float)dt;
            player_y += player_speed * input_state.y_direction * (float)dt;

        PERF_BLOCK_END(UPDATE);

        //
        // Draw:
        //

        gameplay.on_draw(&platform_context, game_context, static_cast<float>(dt));

        PERF_BLOCK_BEGIN(DRAW);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Vertex *vertexes = (Vertex *)arena_alloc_zero(&geometry_arena, sizeof(Vertex) * 6, ARENA_ALLOC_BASIC);
            Color4 rect_color = { 200, 100, 0, 255 };

            // XXX
            static Atlas atlas = { 32, 32 };

            // size_t vertexes_count = generate_rect(vertexes, player_x, player_y, 100, 100, rect_color);
            size_t vertexes_count = generate_rect_with_atlas(vertexes, player_x, player_y, 100, 100, atlas_location, &atlas, rect_color);
            size_t vertex_buffer_size = vertexes_count * sizeof(*vertexes);
            glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, vertexes, GL_DYNAMIC_DRAW);

            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertex_buffer_size / vertex_buffer_layout.stride));

            assert(SwapBuffers(window_device_context));

        PERF_BLOCK_END(DRAW);

        arena_reset(&geometry_arena);

        frame_counter++;
    }

    gameplay.on_fini(&platform_context, game_context);

    unload_gameplay(&gameplay);

    glDeleteProgram(basic_shader_program);

    free_arena(&asset_arena);
    free_arena(&page_arena);
    free_arena(&geometry_arena);

    assert(FreeLibrary(opengl_module));
    CloseWindow(window); // TODO(gr3yknigh1): why it fails? [2025/02/23]

    // WaitForSingleObject(watch_thread, INFINITE); // TODO(gr3yknigh1): Wait for thread [2025/02/23]
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
compile_shader_from_file(Arena *arena, const char *file_path, GLenum type)
{
    // TODO(gr3yknigh1): Check for path existens and that it is file [2024/11/24]

    FILE *file = fopen(file_path, "r");
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(file_size);

    size_t string_buffer_size = file_size + 1;

    char *string_buffer = (char *)arena_alloc_zero(arena, string_buffer_size, ARENA_ALLOC_POPABLE);
    assert(string_buffer);

    fread(string_buffer, string_buffer_size, 1, file);
    fclose(file);

    GLuint id = compile_shader_from_str8(arena, string_buffer, type);

    assert(arena_pop(arena, string_buffer));

    return id;
}

GLuint
compile_shader_from_str8(Arena *arena, const char *string, GLenum type)
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
        char *log_buffer = (char *)arena_alloc_zero(arena, log_buffer_size, ARENA_ALLOC_POPABLE);
        assert(log_buffer);

        glGetShaderInfoLog(id, (GLsizei)log_buffer_size, 0, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to compile OpenGL shader! %s", logBuffer); */
        arena_pop(arena, log_buffer);
    }

    return id;
}

GLuint
link_shader_program(Arena *arena, GLuint vertex_shader, GLuint fragment_shader)
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
        char *log_buffer = (char *)arena_alloc_zero(arena, log_buffer_size, ARENA_ALLOC_POPABLE);
        assert(log_buffer);

        glGetProgramInfoLog(id, (GLsizei)log_buffer_size, NULL, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to link OpenGL program! %s", logBuffer); */
        arena_pop(arena, log_buffer);
    }

    return id;
}

bool
make_vertex_buffer_layout(Arena *arena, Vertex_Buffer_Layout *layout, size32_t attributes_capacity)
{
    ZERO_STRUCT(layout);

    layout->attributes = (Vertex_Buffer_Attribute *)arena_alloc_zero(
        arena, attributes_capacity * sizeof(Vertex_Buffer_Attribute), ARENA_ALLOC_BASIC);

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

Arena
make_arena(size_t capacity)
{
    Arena arena;

    arena.data = VirtualAlloc(
        0, capacity,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);

    arena.capacity = capacity;
    arena.occupied = 0;

    return arena;
}

bool
arena_pop(Arena *arena, void *data)
{
    if (arena == nullptr || data == nullptr) {
        return false;
    }

    if (arena->data == nullptr || arena->occupied < sizeof(arena->occupied) || arena->capacity == 0) {
        return false;
    }

    size_t *data_size = ((size_t *)data) - 1;

    assert((char *)data + *data_size == (char *)arena->data + arena->occupied);
    arena->occupied -= *data_size;
    arena->occupied -= sizeof(*data_size);

    return true;
}

size_t
arena_reset(Arena *arena)
{
    size_t was_occupied = arena->occupied;
    arena->occupied = 0;
    return was_occupied;
}

void *
arena_alloc(Arena *arena, size_t size, int options)
{
    if (arena == nullptr) {
        return nullptr;
    }

    size_t additionals_size = 0;

    if (HAS_FLAG(options, ARENA_ALLOC_POPABLE)) {
        additionals_size = sizeof(size);
    }

    if (arena->occupied + size + additionals_size > arena->capacity) {
        return nullptr;
    }

    void *allocated = ((char *)arena->data) + arena->occupied;

    if (HAS_FLAG(options, ARENA_ALLOC_POPABLE)) {
        *((size_t *)allocated) = size;

        allocated = (char *)allocated + additionals_size;
    }

    arena->occupied += size + additionals_size;
    return allocated;
}

void *
arena_alloc_set(Arena *arena, size_t size, char c, int options)
{
    void *allocated = arena_alloc(arena, size, options);

    if (allocated == nullptr) {
        return allocated;
    }

    memset(allocated, c, size);
    return allocated;
}

void *
arena_alloc_zero(Arena *arena, size_t size, int options)
{
    return arena_alloc_set(arena, size, 0, options);
}

bool
free_arena(Arena *arena)
{
    bool result = VirtualFree(arena->data, 0, MEM_RELEASE);
    ZERO_STRUCT(arena);
    return result;
}

size_t
generate_rect(Vertex *vertexes, float x, float y, float width, float height, Color4 color)
{
    size_t c = 0;

    // bottom-left triangle
    vertexes[c++] = { x + 0    , y + 0,      0, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + 0,      1, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-right
    vertexes[c++] = { x + width, y + height, 1, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right

    // top-right triangle
    vertexes[c++] = { x + 0    , y + 0,      0, 0, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + height, 1, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right
    vertexes[c++] = { x + 0,     y + height, 0, 1, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };

    return c;
}

// TODO(gr3yknigh1): Separate configuration at atlas and mesh size [2025/02/26]
size_t
generate_rect_with_atlas(Vertex *vertexes, float x, float y, float width, float height, Rect_F32 loc, Atlas *atlas, Color4 color)
{
    size_t c = 0;


    // bottom-left triangle
    vertexes[c++] = { x + 0    , y + 0,      (loc.x + 0)         / atlas->x_pixel_count, (loc.y + 0)          / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + 0,      (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + 0)          / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-right
    vertexes[c++] = { x + width, y + height, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right

    // top-right triangle
    vertexes[c++] = { x + 0    , y + 0,      (loc.x + 0)         / atlas->x_pixel_count, (loc.y + 0)          / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + height, (loc.x + loc.width) / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right
    vertexes[c++] = { x + 0,     y + height, (loc.x + 0)         / atlas->x_pixel_count, (loc.y + loc.height) / atlas->y_pixel_count, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };

    return c;
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

float
absolute(float x)
{
    return x < 0 ? -x : x;
}

void
normalize_vector2f(float *x, float *y)
{
    float magnitude = sqrtf(powf(*x, 2) + powf(*y, 2));
    *x /= magnitude;
    *y /= magnitude;
}

bool
win32_is_vk_pressed(int vk)
{
    short state = GetKeyState(vk);
    bool result = state >> 15;
    return result;
}


bool
load_bitmap_picture_from_file(Arena *arena, Bitmap_Picture *picture, const char *file_path)
{
    FILE *file = fopen(file_path, "r");

    if (file == nullptr) {
        return false;
    }

    fread(&picture->header, sizeof(picture->header), 1, file);
    fread(&picture->dib_header, sizeof(picture->dib_header), 1, file);

    picture->u.data = arena_alloc_zero(arena, picture->header.file_size, ARENA_ALLOC_BASIC);
    if (picture->u.data == nullptr) {
        return false;
    }

    fseek(file, picture->header.data_offset, SEEK_SET);
    fread(picture->u.data, picture->header.file_size, 1, file);
    fseek(file, 0, SEEK_SET);

    fclose(file);

    return true;
}

void
gl_make_texture_from_bitmap_picture(void *data, size32_t width, size32_t height, Bitmap_Picture_Compression_Method compression_method, GLenum internal_format)
{
    assert(compression_method == Bitmap_Picture_Compression_Method::Bitfields);

    GLenum format = 0, type = 0;

    if (compression_method == Bitmap_Picture_Compression_Method::Bitfields) {
        format = GL_BGRA;
        type = GL_UNSIGNED_BYTE;
    }

    // TODO(gr3yknigh1): Need to add support for more formats [2025/02/23]

    assert(format && type); // NOTE(gr3yknigh1): Should not be zero [2025/02/23]
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
}

struct Lexer {
    char *cursor;
    char lexeme;

    char *buffer;
    size_t buffer_size;
};

Lexer MakeLexer(char *buffer, size_t buffer_size);

void Lexer_Advance(Lexer *lexer, int32_t count = 1);

char     Lexer_Peek(Lexer *lexer, int32_t offset);
Str8_View8 Lexer_PeekView(Lexer *lexer, int32_t offset);

bool     Lexer_CheckPeeked(Lexer *lexer, const char *s);
bool     Lexer_CheckPeeked(Lexer *lexer, Str8_View8 s);

bool     Lexer_ParseInt(Lexer *lexer, int *result);
bool     Lexer_ParseStrToView(Lexer *lexer, Str8_View8 *sv);

Str8_View8 Lexer_SkipUntil     (Lexer *lexer, char c);
Str8_View8 Lexer_SkipUntilEndline(Lexer *lexer);
void     Lexer_SkipWhitespace(Lexer *lexer);

bool Lexer_IsEndline(Lexer *lexer, bool *is_crlf = nullptr);
bool Lexer_IsEnd(Lexer *lexer);

Tilemap *
load_tilemap_from_file(Arena *arena, const char *file_path)
{
    FILE *file = fopen(file_path, "r");

    if (file == nullptr) {
        return nullptr;
    }

    Tilemap *tilemap = (Tilemap *)arena_alloc_zero(arena, sizeof(Tilemap), ARENA_ALLOC_BASIC);
    if (tilemap == nullptr) {
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(file_size);

    Arena file_arena = make_arena(file_size);

    char *file_buffer = (char *)arena_alloc(&file_arena, file_size, ARENA_ALLOC_BASIC);
    assert(file_buffer);

    fread(file_buffer, file_size, 1, file);
    fclose(file);

    Lexer lexer = MakeLexer(file_buffer, file_size);

    static constexpr Str8_View8 s_tilemap_directive = "@tilemap";

    static constexpr Str8_View8 s_tilemap_image_bmp_format = "bmp";

    Str8_View8 tilemap_image_path_view;

    while (!Lexer_IsEnd(&lexer)) {
        Lexer_SkipWhitespace(&lexer);

        // NOTE(gr3yknigh1): Skipping comments [2025/02/24]
        if (Lexer_CheckPeeked(&lexer, "//")) {
            (void)Lexer_SkipUntilEndline(&lexer);
            continue;
        }

        if (Lexer_CheckPeeked( &lexer, s_tilemap_directive )) {
            // TODO(gr3yknigh1): Fix this cast to signed int [2025/02/24]
            Lexer_Advance(&lexer, (int32_t)s_tilemap_directive.length);
            Lexer_SkipWhitespace(&lexer);

            assert(Lexer_ParseInt(&lexer, &tilemap->row_count));
            Lexer_SkipWhitespace(&lexer);

            assert(Lexer_ParseInt(&lexer, &tilemap->col_count));
            Lexer_SkipWhitespace(&lexer);

            assert(Lexer_ParseStrToView(&lexer, &tilemap_image_path_view));
            Lexer_SkipWhitespace(&lexer);

            // TODO(gr3yknigh1): Add proper error report mechanizm. Error message in window, for example. [2025/02/24]
            assert(Lexer_CheckPeeked( &lexer, s_tilemap_image_bmp_format ));
            tilemap->image.format = Tilemap_Image_Format::Bitmap;
            Lexer_Advance(&lexer, (int32_t)s_tilemap_image_bmp_format.length);

            Lexer_SkipWhitespace(&lexer);

            assert(Lexer_ParseInt(&lexer, &tilemap->tile_x_pixel_count));
            Lexer_SkipWhitespace(&lexer);

            assert(Lexer_ParseInt(&lexer, &tilemap->tile_y_pixel_count));
            Lexer_SkipWhitespace(&lexer);

            // NOTE(gr3yknigh1): Bad assumtion [2025/02/24]
            // assert(Lexer_IsEndline(&lexer));

            // TODO(gr3yknigh1): Move initialization of index array out of here [2025/02/24]
            // NOTE(gr3yknigh1): This can be fixed with adding stage with Token generation, like
            // proper lexers does [2025/02/26]
            tilemap->indexes_count = tilemap->row_count * tilemap->col_count;
            tilemap->indexes = (int *)arena_alloc_zero(arena, tilemap->indexes_count * sizeof(*tilemap->indexes), ARENA_ALLOC_BASIC);

            continue;
        }

        if (isdigit(lexer.lexeme)) {
            assert(tilemap->indexes);

            int *indexes_cursor = tilemap->indexes;

            do {
                size_t current_index_index = indexes_cursor - tilemap->indexes;
                assert(current_index_index + 1 <= tilemap->indexes_count);

                assert(Lexer_ParseInt(&lexer, indexes_cursor));
                Lexer_SkipWhitespace(&lexer);

                ++indexes_cursor;
            } while(isdigit(lexer.lexeme));

            size_t filled_indexes = indexes_cursor - tilemap->indexes;
            assert(filled_indexes == tilemap->indexes_count);
        }

        Lexer_Advance(&lexer);
    }

    tilemap->image.u.bitmap = (Bitmap_Picture *)arena_alloc(arena, sizeof(Bitmap_Picture), ARENA_ALLOC_BASIC);

    // TODO(gr3yknigh1): Factor this out [2025/02/24]
    assert(tilemap_image_path_view.length && tilemap_image_path_view.data);

    char *tilemap_image_path = (char *)HeapAlloc(GetProcessHeap(), (DWORD)(tilemap_image_path_view.length + 1), HEAP_ZERO_MEMORY);
    assert(str8_view_copy_to_nullterminated(tilemap_image_path_view, tilemap_image_path, tilemap_image_path_view.length + 1));

    // TODO(gr3yknigh1): Generalize format validation [2025/02/24]
    assert(tilemap->image.format == Tilemap_Image_Format::Bitmap);
    assert(load_bitmap_picture_from_file(arena, tilemap->image.u.bitmap, tilemap_image_path));

    assert(HeapFree(GetProcessHeap(), 0, tilemap_image_path));

    assert(free_arena(&file_arena));

    return tilemap;
}


Lexer
MakeLexer(char *buffer, size_t buffer_size)
{
    Lexer lexer;

    lexer.cursor = buffer;
    lexer.lexeme = *buffer;

    lexer.buffer = buffer;
    lexer.buffer_size = buffer_size;

    return lexer;
}

void
Lexer_Advance(Lexer *lexer, int32_t count)
{
    size_t cursor_index = lexer->cursor - lexer->buffer;

    if (Lexer_IsEnd(lexer) && cursor_index + count < lexer->buffer_size) {
        return;
    }

    while (count > 0) {
        lexer->cursor++;
        --count;
    }

    lexer->lexeme = *(lexer->cursor);
}

char
Lexer_Peek(Lexer *lexer, int32_t offset)
{
    size_t cursor_index = lexer->cursor - lexer->buffer;

    // TODO(gr3yknigh1): Maybe make it signed? [2025/02/23]
    if (cursor_index + offset < lexer->buffer_size) {
        char *peek = lexer->cursor + offset;
        return *peek;
    }

    return 0;
}

Str8_View8
Lexer_PeekView(Lexer *lexer, int32_t offset)
{
    size_t cursor_index = lexer->cursor - lexer->buffer;

    // TODO(gr3yknigh1): Maybe make it signed? [2025/02/23]
    if (cursor_index + offset < lexer->buffer_size) {
        if (offset < 0) {
            return Str8_View8(lexer->cursor + offset, -offset);
        } else if (offset > 0) {
            return Str8_View8(lexer->cursor, offset);
        }
    }
    return Str8_View8();  // do a better job next time
}

bool
Lexer_CheckPeeked(Lexer *lexer, const char *s)
{
    Str8_View8 sv(s);
    return Lexer_CheckPeeked(lexer, sv);
}

bool
Lexer_CheckPeeked(Lexer *lexer, Str8_View8 s)
{
    Str8_View8 peek_view = Lexer_PeekView(lexer, (int32_t)s.length);
    return str8_view_is_equals(peek_view, s);
}

Str8_View8
Lexer_SkipUntil(Lexer *lexer, char c)
{
    Str8_View8 ret;

    ret.data = lexer->cursor;

    while (!Lexer_IsEnd(lexer) && lexer->lexeme != c) {
        Lexer_Advance(lexer);
        ret.length++;
    }

    return ret;
}

Str8_View8
Lexer_SkipUntilEndline(Lexer *lexer)
{
    Str8_View8 ret;

    ret.data = lexer->cursor;

    bool is_crlf = false;

    while (!Lexer_IsEnd(lexer) && !Lexer_IsEndline(lexer, &is_crlf)) {
        Lexer_Advance(lexer);
        ret.length++;
    }

    if (is_crlf) {
        Lexer_Advance(lexer);
        Lexer_Advance(lexer);
    } else {
        Lexer_Advance(lexer);
    }

    return ret;
}

bool
Lexer_IsEndline(Lexer *lexer, bool *is_crlf)
{
    if (is_crlf != nullptr) {
        *is_crlf = (lexer->cursor[0] == '\r' && lexer->cursor[1] == '\n');
    }
    return lexer->lexeme == '\n' || (lexer->cursor[0] == '\r' && lexer->cursor[1] == '\n');
}

void
Lexer_SkipWhitespace(Lexer *lexer)
{
    while (!Lexer_IsEnd(lexer) && isspace(lexer->lexeme)) {
        Lexer_Advance(lexer);
    }
}

bool
Lexer_IsEnd(Lexer *lexer)
{
    size_t cursor_index = lexer->cursor - lexer->buffer;
    return cursor_index >= lexer->buffer_size;
}


bool
Lexer_ParseInt(Lexer *lexer, int *result)
{
    // TODO(gr3yknigh1): Error handling [2025/02/23]

    int num = 0;

    bool is_negative = false;

    if (lexer->lexeme == '-') {
        is_negative = true;
        Lexer_Advance(lexer);
    }

    while (lexer->lexeme && (lexer->lexeme >= '0' && lexer->lexeme <= '9')) {
        num = num * 10 + (lexer->lexeme - '0');
        Lexer_Advance(lexer);
    }

    if (is_negative) {
        num = -1 * num;
    }

    *result = num;

    return true;
}

bool
Lexer_ParseStrToView(Lexer *lexer, Str8_View8 *sv)
{
    if (lexer->lexeme != '"') {
        return false;
    }

    // NOTE(gr3yknigh1): Skipping '"' [2025/02/24]
    Lexer_Advance(lexer); // TODO(gr3yknigh1): Wrap in `Lexer_AdvanceIf(Lexer *, int32_t count, char c)`

    *sv = Lexer_SkipUntil(lexer, '"');

    // NOTE(gr3yknigh1): Skipping '"' [2025/02/24]
    Lexer_Advance(lexer); // TODO(gr3yknigh1): Wrap in `Lexer_AdvanceIf(Lexer *, int32_t count, char c)`

    return true;
}

bool
path16_get_parent(const wchar_t *path, size_t path_length, Str16_View *out)
{

    out->data = path;

    for (size_t index = path_length - 1; index > 0; --index) {
        wchar_t c = path[index];

        if (c == path16_get_separator()) {
            size_t c_position = path_length - index - 1;
            out->length = c_position - 1;
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
make_watch_dir_context(Watch_Dir_Context *context)
{
    ZERO_STRUCT(context);
    context->should_stop.clear();
}

void
watch_dir_thread_worker(PVOID param)
{
    Watch_Dir_Context *context = (Watch_Dir_Context *)param;

    HANDLE watch_dir = CreateFileW(context->target_dir,
        FILE_LIST_DIRECTORY, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    assert(watch_dir && watch_dir != INVALID_HANDLE_VALUE);

    DWORD file_notify_info_buffer_size = sizeof(FILE_NOTIFY_INFORMATION) * 1024;
    FILE_NOTIFY_INFORMATION *file_notify_info = (FILE_NOTIFY_INFORMATION *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, file_notify_info_buffer_size);
    assert(file_notify_info);
    // TODO(gr3yknigh1): Fix allocation strategy here. Maybe try to
    // iterate in fixed buffer? [2025/03/01]

    while (!context->should_stop.test()) {
        DWORD bytes_returned = 0;
        assert(ReadDirectoryChangesW(
            watch_dir, file_notify_info, file_notify_info_buffer_size, true,
            FILE_NOTIFY_CHANGE_LAST_WRITE, &bytes_returned, nullptr, nullptr
        ));

        if (context->notification_routine != nullptr) {
            Str16_View file_name(file_notify_info->FileName, file_notify_info->FileNameLength / sizeof(*file_notify_info->FileName));

            watch_ext_mask_t file_ext = 0;

            if        (HAS_FLAG(context->watch_exts, WATCH_EXT_BMP)  && str16_view_endswith(file_name, WATCH_EXT_BMP_VIEW)) {
                file_ext = WATCH_EXT_BMP;
            } else if (HAS_FLAG(context->watch_exts, WATCH_EXT_GLSL) && str16_view_endswith(file_name, WATCH_EXT_GLSL_VIEW)) {
                file_ext = WATCH_EXT_GLSL;
            } else if (HAS_FLAG(context->watch_exts, WATCH_EXT_DLL) && str16_view_endswith(file_name, WATCH_EXT_DLL_VIEW)) {
                file_ext = WATCH_EXT_DLL;
            }

            if (file_ext != 0) {
                context->notification_routine(file_name, file_ext, (File_Action)file_notify_info->Action, context->parameter);
            }
        }
    }

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
