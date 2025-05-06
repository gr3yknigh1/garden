//!
//! This is platform code which is specific to Windows.
//!
//! FILE          code\garden_platform_win32.cpp
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#include <atomic>   // std::atomic_flag
#include <utility>

#include <assert.h> // assert
#include <stdio.h>  // puts, printf, FILE, fopen, freopen, fseek, fclose
#include <stdarg.h>

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

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#include "imgui_stdlib.h" // ImGui::InputText with std::string instead of char*

#include "media/aseprite.h"

#include "garden_gameplay.h"
#include "garden_runtime.h"

//! @todo(gr3yknigh1): Replace with more non-platform dependent code [2025/03/03] #refactor #renaming
#if !defined(ZERO_STRUCT)
    #define ZERO_STRUCT(STRUCT_PTR) ZeroMemory((STRUCT_PTR), sizeof(*(STRUCT_PTR)))
#endif


typedef int bool32_t;
typedef unsigned int size32_t;


wchar_t path16_get_separator(void);

bool path16_get_parent(const wchar_t *path, SizeU path_length, Str16_View *out);

//
// Perf helpers:
//

Int64S perf_get_counter_frequency(void);
Int64S perf_get_counter(void);
uint64_t perf_get_cycles_count(void);

#pragma pack(push, 1)
struct Perf_Block_Record {
    const char *label;
    const char *function;
    const char *file_path;
    uint64_t line_number;
    uint64_t cycles_begin;
    uint64_t cycles_end;
    Int64S counter_begin;
    Int64S counter_end;
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
GLuint link_shader_program(GLuint vertex_shader, GLuint fragment_shader);

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
bool make_vertex_buffer_layout(mm::Fixed_Arena *arena, Vertex_Buffer_Layout *layout, size32_t attributes_capacity);

Vertex_Buffer_Attribute *vertex_buffer_layout_push_attr   (Vertex_Buffer_Layout *layout, unsigned int count, GLenum type, SizeU size);
Vertex_Buffer_Attribute *vertex_buffer_layout_push_float  (Vertex_Buffer_Layout *layout, unsigned int count);
Vertex_Buffer_Attribute *vertex_buffer_layout_push_integer(Vertex_Buffer_Layout *layout, unsigned int count);


struct Vertex_Buffer {
    GLuint id;
    GLuint vertex_array_id;

    Vertex_Buffer_Layout layout;
};

bool make_vertex_buffer(Vertex_Buffer *buffer);
bool bind_vertex_buffer(Vertex_Buffer *buffer);

//!
//! @brief Initializes vertex buffer layout.
//!
//! @pre
//!     glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)
//!     glBindVertexArray(vertex_array);
//!
void vertex_buffer_layout_build_attrs(const Vertex_Buffer_Layout *layout);


//!
//! @brief Virtual-key codes.
//!
//! @ref https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
//!
enum struct Win32_VK_Code : Int16S {
    //!
    //! @brief Left mouse button.
    //!
    LButton = 0x01,

    //!
    //! @brief Right mouse button.
    //!
    RButton = 0x02,

    //!
    //! @brief Control-break processing.
    //!
    Cancel = 0x03,

    //!
    //! @brief Middle mouse button.
    //!
    MButton = 0x04,

    //!
    //! @brief X1 mouse button.
    //!
    XButton1 = 0x05,

    //!
    //! @brief X2 mouse button.
    //!
    XButton2 = 0x06,

    /* ...skipped */

    //!
    //! @brief Shift key.
    //!
    Shift = 0x10,

    //!
    //! @brief Control key.
    //!
    Control = 0x11,

    //!
    //! @brief Alt key.
    //!
    Menu = 0x12,

    /* ...skipped */

    //!
    //! @brief Left arrow key.
    //!
    Left = 0x25,

    //!
    //! @brief Up arrow key.
    //!
    Up = 0x26,

    //!
    //! @brief Right arrow key.
    //!
    Right = 0x27,

    //!
    //! @brief Down arrow key.
    //!
    Down = 0x28,

    /* ...skipped */

    //!
    //! @brief A key.
    //!
    A = 0x41,

    //!
    //! @brief D key.
    //!
    D = 0x44,

    //!
    //! @brief Q key.
    //!
    Q = 0x51,

    //!
    //! @brief S key.
    //!
    S = 0x53,

    //!
    //! @brief W key.
    //!
    W = 0x57,

    /* ...skipped */
};

struct Win32_Key_State {
    Win32_VK_Code code;

    union {
        /*
         * NOTE(gr3yknigh1): This is lparam layout of @TBD [2025/05/06]
         *
         *               0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
         * repeat_count: ^                                  ^
         * scan_code:                                          ^                    ^
         * extended:                                                                   ^
         * reserved:                                                                      ^        ^
         * context_code:                                                                              ^
         * prev_state:                                                                                   ^
         * trans_state:                                                                                     ^
         *
         */

        struct {
            Int16U repeat_count : 16;
            Int8U scan_code : 8;
            bool extended : 1;
            Int8U reserved : 4;
            bool alt_context : 1;
            bool was_up : 1;
            bool is_up : 1;
        };
        Int32U flags;
    };

    MSG message;
};

#if !defined(WIN32_KEYSTATE_IS_EXTENDED)
    #define WIN32_KEYSTATE_IS_EXTENDED(K_PTR) HAS_FLAG((K_PTR)->flags, KF_EXTENDED)
#endif

#if !defined(WIN32_KEYSTATE_IS_RELEASED)
    #define WIN32_KEYSTATE_IS_RELEASED(K_PTR) HAS_FLAG((K_PTR)->flags, KF_UP)
#endif

Win32_Key_State win32_convert_msg_to_key_state(MSG message);

//!
//! @param[out, optional] changed_key
//!
bool win32_apply_changes_to_key(Input_State *input_state, Win32_Key_State key_state, Key_Code *changed_key = nullptr);
bool win32_is_vk_pressed(Int32S vk);


//
// Time:
//

struct Clock {
    Int64S ticks_begin;
    Int64S ticks_end;
    Int64S frequency;
};

Clock make_clock(void);

double clock_tick(Clock *clock);

//
// Media:
//

enum struct Bitmap_Picture_Header_Type : Int32U {
    BitmapCoreHeader = 12,
    Os22XBitmapHeader_S = 16,
    BitmapInfoHeader = 40,
    BitmapV2InfoHeader = 52,
    BitmapV3InfoHeader = 56,
    Os22XBitmapHeader = 64,
    BitmapV4Header = 108,
    BitmapV5Header = 124,
};

enum struct Bitmap_Picture_Compression_Method : Int32U {
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
    Int32U width;
    Int32U height;
    Int16U planes_count;
    Int16U depth;
    Bitmap_Picture_Compression_Method compression_method;
    Int32U image_size;
    Int32U x_pixel_per_meter;
    Int32U y_pixel_per_meter;
    Int32U color_used;
    Int32U color_important;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Bitmap_Picture_Header {
    Int16U type;
    Int32U file_size;
    Int16U reserved[2];
    Int32U data_offset;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Color_BGRA_U8 {
    Int8U b, g, r, a;
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

bool load_bitmap_picture_info_from_file(Bitmap_Picture *picture, FILE *file);
bool load_bitmap_picture_pixel_data_from_file(Bitmap_Picture *picture, FILE *file);


//
// Media:
//

enum struct Color_Layout {
    Nothing,
    BGRA_U8,
};


//
// @pre
//   - Bind target texture with glBindTexture(GL_TEXTURE_2D, ...);
//
void gl_make_texture_from_pixels(void *pixels, size32_t width, size32_t height, Color_Layout layout, GLenum internal_format);


void gl_clear_all_errors(void);
void gl_die_on_first_error(void);
void gl_print_debug_info(void);

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
    Texture,
    Shader,
    Tilemap,
    Count_
};

enum struct Asset_Store_Place {
    //!
    //! @brief That indicates, that `Asset_Store` represented by some kind of folder. Probably usefull for development[2025/04/06]
    //!
    Folder,

    //!
    //! @brief That indicates, that `Asset_Store` located inside the single file. Probably usefull only on exports.
    //!
    Image,
};

struct Asset_Store {
    static constexpr Int16U max_asset_count = 1024;

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
        for (Asset *IT = static_cast<Asset *>(first(&(ASSET_STORE_PTR)->asset_pool)); IT != nullptr; IT = static_cast<Asset *>(next(&(ASSET_STORE_PTR)->asset_pool, static_cast<void *>(IT)) ))
#endif

struct Reload_Context {
    Asset_Store *store;

    //
    // NOTE(gr3yknigh1): Found no place for this. Think about it later, if it becomes a problem [2025/03/10]
    //
    std::atomic_flag should_reload_gameplay;
};

enum struct Asset_Location_Type {
    None,
    File,
    Buffer,
};

struct File_Info {
    FILE *handle;
    SizeU size;
    Str8 path;

    ~File_Info(void) noexcept {}
};

struct Asset_Location {
    Asset_Location_Type type;

    union Location_Union {
        File_Info file;
        mm::Buffer_View buffer_view;

        ~Location_Union(void) noexcept {}
    } u;

    ~Asset_Location(void) noexcept {}
};


bool make_asset_store_from_folder(Asset_Store *store, const char *folder_path);
bool asset_store_destroy(Asset_Store *store);

struct Texture {
    int width;
    int height;
    Color_Layout layout;

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

    Shader_Module modules[static_cast<SizeU>(Shader_Module_Type::Count_)];
};

struct Asset {
    Asset_Type type;
    Asset_Location location;
    Asset_State state;

    std::atomic_flag should_reload;

    union {
        Texture texture;
        Shader shader;
        Tilemap tilemap;
    } u;

    ~Asset(void) noexcept {}
};

Asset *asset_load(Asset_Store *store, Asset_Type type, const Str8_View file_path);

// helper
bool load_tilemap_from_buffer(Asset_Store *store, char *buffer, SizeU buffer_size, Tilemap *tilemap);
bool asset_image_send_to_gpu(Asset_Store *store, Asset *asset, int unit, Shader *shader);

bool shader_bind(Shader *shader);

bool asset_from_bitmap_picture(Asset *asset, Bitmap_Picture *picture);

bool asset_reload(Asset_Store *store, Asset *asset);
bool asset_unload(Asset_Store *store, Asset *asset);

struct Shader_Compile_Result {
    GLuint shader_program_id;
};

Shader_Compile_Result compile_shader(char *source_code, SizeU file_size);

void asset_watch_routine(Watch_Context *, const Str16_View, File_Action, void *);

#if GARDEN_GAMEPLAY_CODE == 1

#include "garden_gameplay.cpp"

extern "C" BOOL WINAPI
DllMain([[maybe_unused]] HINSTANCE instance, DWORD reason, [[maybe_unused]] LPVOID reserved)
{
    switch(reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

#else

#if !defined(GARDEN_ASSETS_FOLDER)
    #error "Dev asset directory is not defined!"
#else
    #pragma message( "Using DEV asset dir: '" STRINGIFY(GARDEN_ASSETS_FOLDER) "'" )
#endif


struct Console {
    Reporter *reporter;

    bool auto_scroll;
    bool scroll_to_bottom;
};


void gui_show_debug_console(Console *console, bool *p_open);


int WINAPI
wWinMain(HINSTANCE instance, [[maybe_unused]] HINSTANCE previous_instance, [[maybe_unused]] PWSTR command_line, int cmd_show)
{
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
    // ImGui:
    //

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    [[maybe_unused]] ImGuiStyle &style = ImGui::GetStyle();

    ImGui_ImplWin32_InitForOpenGL(window);
    ImGui_ImplOpenGL3_Init();


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

    mm::Fixed_Arena page_arena = mm::make_static_arena(1024);

    Vertex_Buffer_Layout vertex_buffer_layout;
    assert(make_vertex_buffer_layout(&page_arena, &vertex_buffer_layout, 4));

    assert(vertex_buffer_layout_push_float(&vertex_buffer_layout, 2));
    assert(vertex_buffer_layout_push_float(&vertex_buffer_layout, 2));
    assert(vertex_buffer_layout_push_integer(&vertex_buffer_layout, 1));

    vertex_buffer_layout_build_attrs(&vertex_buffer_layout);

    reset(&page_arena);

    //
    // Media:
    //
    Asset_Store store;
    assert(make_asset_store_from_folder(&store, STRINGIFY(GARDEN_ASSETS_FOLDER)));

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
    Asset *atlas_asset = asset_load(&store, Asset_Type::Texture, R"(P:\garden\assets\garden_atlas.bmp)");
    assert(atlas_asset);
    assert(asset_image_send_to_gpu(&store, atlas_asset, 0, basic_shader));

    //
    // Hot-reload: Setup
    //

    Reload_Context reload_context;
    reload_context.store = &store;
    reload_context.should_reload_gameplay.clear();

    Watch_Context watch_context;
    assert(make_watch_context(&watch_context, &reload_context, LR"(P:\garden)", asset_watch_routine));

    HANDLE watch_thread = INVALID_HANDLE_VALUE;
    assert(watch_context_launch_thread(&watch_context, &watch_thread));

    //
    // Setup tilemap atlas:
    //
    Asset *tilemap_asset = asset_load(&store, Asset_Type::Tilemap, R"(P:\garden\assets\demo.tilemap.tp)");
    assert(asset_image_send_to_gpu(&store, tilemap_asset->u.tilemap.texture_asset, 1, basic_shader));

    Vertex_Buffer tilemap_vertex_buffer{};
    assert(make_vertex_buffer(&tilemap_vertex_buffer));
    assert(bind_vertex_buffer(&tilemap_vertex_buffer));

    Vertex_Buffer_Layout tilemap_vertex_buffer_layout{};
    assert(make_vertex_buffer_layout(&page_arena, &tilemap_vertex_buffer_layout, 4));

    assert(vertex_buffer_layout_push_float(&tilemap_vertex_buffer_layout, 2));    // Position
    assert(vertex_buffer_layout_push_float(&tilemap_vertex_buffer_layout, 2));    // UV
    assert(vertex_buffer_layout_push_integer(&tilemap_vertex_buffer_layout, 1));  // Color

    vertex_buffer_layout_build_attrs(&tilemap_vertex_buffer_layout);

    reset(&page_arena);

    Tilemap *tilemap = &tilemap_asset->u.tilemap;
    Float32 tilemap_position_x = 100, tilemap_position_y = 100;
    Texture *tilemap_texture = &tilemap->texture_asset->u.texture;

    Atlas tilemap_atlas{
        static_cast<Float32>(tilemap_texture->width),
        static_cast<Float32>(tilemap_texture->height)
    };

    Vertex *tilemap_vertexes = mm::allocate_structs<Vertex>(tilemap->tiles_count() * TILEMAP_VERTEX_COUNT_PER_TILE); //! @todo Free later. #memory
    Int32U tilemap_vertexes_count = generate_geometry_from_tilemap(
        tilemap_vertexes, tilemap->tiles_count() * TILEMAP_VERTEX_COUNT_PER_TILE, //! @todo #refactor
        tilemap, tilemap_position_x, tilemap_position_y,
        {255, 255, 255, 255}, &tilemap_atlas);

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
    platform_context.persist_arena = mm::make_static_arena(1024);
    platform_context.vertexes_arena = mm::make_static_arena(sizeof(Vertex) * 1024);

    Game_Context *game_context = reinterpret_cast<Game_Context *>(gameplay.on_init(&platform_context));
    gameplay.on_load(&platform_context, game_context);

    bool show_debug_console = true;

    Reporter frame_reporter{};

    Console console{};
    console.reporter = &frame_reporter;

    while (!global_should_terminate) {
        double dt = clock_tick(&clock);

        MSG message;
        ZERO_STRUCT(&message);

        //
        // Game Hot reload:
        //

        if (reload_context.should_reload_gameplay.test()) {
            //puts("I: Gameplay code reload!");
            frame_reporter.report(Severenity::Info, "Gameplay code was reloaded!");

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
                    //
                    // @ref <https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input>
                    //

                    Win32_Key_State win32_key_state {};

                    win32_key_state.code = static_cast<Win32_VK_Code>(LOWORD(message.wParam));
                    win32_key_state.flags = message.lParam;

                    if (WIN32_KEYSTATE_IS_EXTENDED(&win32_key_state)) {
                        win32_key_state.scan_code = MAKEWORD(win32_key_state.scan_code, 0xE0);
                    }

                    win32_key_state.repeat_count = LOWORD(message.lParam);

                    switch (win32_key_state.code) {
                    case Win32_VK_Code::Shift:   // converts to VK_LSHIFT or VK_RSHIFT
                    case Win32_VK_Code::Control: // converts to VK_LCONTROL or VK_RCONTROL
                    case Win32_VK_Code::Menu:    // converts to VK_LMENU or VK_RMENU
                        win32_key_state.code = static_cast<Win32_VK_Code>(LOWORD(MapVirtualKeyW(win32_key_state.scan_code, MAPVK_VSC_TO_VK_EX)));
                        break;
                    }

                    Key_Code changed_key = Key_Code::None;

                    Char8 format_buffer[1024];
                    if (!win32_apply_changes_to_key(&platform_context.input_state, win32_key_state, &changed_key)) {

                        // TODO(gr3yknigh1): Replace wth String_Builder [2025/05/06]
                        sprintf(format_buffer, "VK not handled: scan_code(0x%x)", win32_key_state.scan_code);

                        frame_reporter.report(Severenity::Error, format_buffer);
                        /* TODO(gr3yknigh1): Handle an error [2025/05/06] */
                    } else {
                        sprintf(format_buffer, "Handled key input: scan_code(0x%x) key(%d)", win32_key_state.scan_code, static_cast<Int32S>(changed_key));

                        frame_reporter.report(Severenity::Info, format_buffer);
                    }

                        #if 0
                    if (key.code == VK_ESCAPE || key.code == 0x51) {  // 0x51 - `Q` key.
                        global_should_terminate = true;
                    }

                    if (!key.is_up) {
                        if (key.code == VK_LEFT) {
                            platform_context.input_state.x_direction = -1;
                        } else if (key.code == VK_RIGHT) {
                            platform_context.input_state.x_direction = +1;
                        }

                        if (key.code == VK_DOWN) {
                            platform_context.input_state.y_direction = -1;
                        } else if (key.code == VK_UP) {
                            platform_context.input_state.y_direction = +1;
                        }
                    } else {
                        if ((key.code == VK_LEFT   && !win32_is_vk_pressed(VK_RIGHT))
                        || ((key.code == VK_RIGHT) && !win32_is_vk_pressed(VK_LEFT))) {
                            platform_context.input_state.x_direction = 0;
                        }

                        if ((key.code == VK_DOWN && !win32_is_vk_pressed(VK_UP))
                         || (key.code == VK_UP && !win32_is_vk_pressed(VK_DOWN))) {
                            platform_context.input_state.y_direction = 0;
                        }
                    }
                    #endif

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

        if (IsIconic(window)) {
            Sleep(10);
            continue;
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

                if (it->type == Asset_Type::Texture) {
                    assert(asset_image_send_to_gpu(&store, it, it->u.texture.unit, basic_shader));
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

                    glUniform1i(atlas_texture_uniform_loc, atlas_asset->u.texture.unit);
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
                glActiveTexture(GL_TEXTURE0 + tilemap_asset->u.tilemap.texture_asset->u.texture.unit);
                GLuint texture_uniform_loc = glGetUniformLocation(basic_shader->program_id, "u_texture");
                assert(texture_uniform_loc != -1);
                glUniform1i(texture_uniform_loc, tilemap_asset->u.tilemap.texture_asset->u.texture.unit);

                SizeU vertex_buffer_size = tilemap_vertexes_count * sizeof(*tilemap_vertexes);
                glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, tilemap_vertexes, GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertex_buffer_size / vertex_buffer_layout.stride)); // TODO(gr3yknigh1): Replace layout [2025/03/30]
            }

            if (platform_context.vertexes_count > 0) {
                assert(bind_vertex_buffer(&entity_vertex_buffer));

                /// XXX
                glActiveTexture(GL_TEXTURE0 + atlas_asset->u.texture.unit);
                GLuint texture_uniform_loc = glGetUniformLocation(basic_shader->program_id, "u_texture");
                assert(texture_uniform_loc != -1);
                glUniform1i(texture_uniform_loc, atlas_asset->u.texture.unit);

                SizeU vertex_buffer_size = platform_context.vertexes_count * sizeof(*platform_context.vertexes);
                glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, platform_context.vertexes, GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertex_buffer_size / vertex_buffer_layout.stride));

                platform_context.vertexes_count = 0;
                reset(&platform_context.vertexes_arena);
            }

            //
            // ImGui new frame:
            //
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            static bool show_demo_window = true;
            ImGui::ShowDemoWindow(&show_demo_window);

            gui_show_debug_console(&console, &show_debug_console);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            assert(SwapBuffers(window_device_context));

        PERF_BLOCK_END(DRAW);

        frame_counter++;
    }


    gameplay.on_fini(&platform_context, game_context);

    unload_gameplay(&gameplay);

    //
    // ImGui shutdown:
    //
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    //
    // TODO(gr3yknigh1): Make ReadDirectoryChangesW asyncronous, so you
    // can set the flag to stop the worker: watch_context.should_stop.test_and_set();. [2025/05/25]
    //
    //WaitForSingleObject(watch_thread, INFINITE); // TODO(gr3yknigh1): Wait for thread [2025/02/23]

    glDeleteProgram(basic_shader->program_id); // @cleanup Replace with asset_shader_free

    mm::destroy(&page_arena);
    mm::destroy(&platform_context.vertexes_arena);
    mm::destroy(&platform_context.persist_arena);

    assert(FreeLibrary(opengl_module));
    CloseWindow(window); // TODO(gr3yknigh1): why it fails? [2025/02/23]


    assert(asset_store_destroy(&store));

    mm::dump_allocation_records();

    return 0;
}

static void
gui_show_helper_marker(CStr8 description, ...)
{
    va_list args;
    va_start(args, description);

    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextV(description, args);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    va_end(args);
}


void
gui_show_debug_console(Console *console, bool *p_open)
{
    if (!ImGui::Begin("Debug Console", p_open)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Close Console")) {
            *p_open = false;
        }
        ImGui::EndPopup();
    }

    if (ImGui::SmallButton("Clear")) {
        console->reporter->reports.clear();
    }

    ImGui::SameLine();

    bool copy_to_clipboard = ImGui::SmallButton("Copy");

    ImGui::SameLine();

    if (ImGui::SmallButton("Test Message")) {
        console->reporter->report(Severenity::Trace, "Test message");
    }

    ImGui::Separator();

    if (ImGui::BeginPopup("Options")) {
        ImGui::Checkbox("Auto-scroll", &console->auto_scroll);
        ImGui::EndPopup();
    }

    ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_Tooltip);
    if (ImGui::Button("Options")) {
        ImGui::OpenPopup("Options");
    }

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {

        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::Selectable("Clear")) console->reporter->reports.clear();
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        if (copy_to_clipboard) {
            ImGui::LogToClipboard();
        }

        for (const Report &report : console->reporter->reports) {

            if (report.count > 1) {
                ImGui::Text("%c: %s x%lu", SEVERENITY_LETTERS[static_cast<SizeU>(report.severenity)], report.message.data, report.count);
            } else {
                ImGui::Text("%c: %s", SEVERENITY_LETTERS[static_cast<SizeU>(report.severenity)], report.message.data);
            }

            ImGui::SameLine();

            const Source_Location *location = &report.source_location;
            ImGui::Text/*gui_show_helper_marker*/(
                "%s(%u:%u)@(%s)",
                location->file_name, location->line, location->column, location->function_name);
        }

        if (copy_to_clipboard) {
            ImGui::LogFinish();
        }

        if (console->scroll_to_bottom || (console->auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
            ImGui::SetScrollHereY(1.0f);
        }

        console->scroll_to_bottom = false;

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::Separator();

    ImGui::End();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

LRESULT CALLBACK
win32_window_message_handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam)) {
        return true;
    }

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

#endif

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

        SizeU log_buffer_size = log_length + 1;
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

        SizeU log_buffer_size = log_length + 1;
        char *log_buffer = static_cast<char *>(mm::allocate(log_buffer_size));
        assert(log_buffer);

        glGetProgramInfoLog(id, (GLsizei)log_buffer_size, NULL, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to link OpenGL program! %s", logBuffer); */
        mm::deallocate(log_buffer);
    }

    return id;
}


bool
make_vertex_buffer_layout(mm::Fixed_Arena *arena, Vertex_Buffer_Layout *layout, size32_t attributes_capacity)
{
    ZERO_STRUCT(layout);

    //!
    //! @todo(gr3yknigh1): Make it so on input user provide full description of layout.
    //! vertex_buffer_layout_push_* functions require you to have whole buffer until you
    //! call vertex_buffer_layout_build_attrs [2025/04/25]
    //!

    layout->attributes = allocate_structs<Vertex_Buffer_Attribute>(
        arena, attributes_capacity, ALLOCATE_ZERO_MEMORY);

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
    SizeU offset = 0;

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

Int64S
perf_get_counter_frequency(void)
{
    LARGE_INTEGER perf_frequency_result;
    QueryPerformanceFrequency(&perf_frequency_result);
    Int64S perf_frequency = perf_frequency_result.QuadPart;
    return perf_frequency;
}

Int64S
perf_get_counter(void)
{
    LARGE_INTEGER perf_counter_result;
    QueryPerformanceCounter(&perf_counter_result);
    Int64S perf_counter = perf_counter_result.QuadPart;
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
    Int64S perf_frequency = perf_get_counter_frequency();

    Int64S counter_elapsed = record->counter_end - record->counter_begin;
    Int64S ms_elapsed = (1000 * counter_elapsed) / perf_frequency;
    uint64_t cycles_elapsed = record->cycles_end - record->cycles_begin;
    uint64_t mega_cycles_elapsed = cycles_elapsed / (1000 * 1000);

    printf(
        "PERF: %s:%llu@%s [%s]: counter = (%llu) ms = (%llu) | Mc = %llu\n", record->file_path, record->line_number,
        record->function, record->label, counter_elapsed, ms_elapsed, mega_cycles_elapsed);
}

bool
win32_apply_changes_to_key(Input_State *input_state, Win32_Key_State key_state, Key_Code *changed_key)
{
    Key *key = nullptr;

    switch (key_state.code) {
    case Win32_VK_Code::LButton:
    case Win32_VK_Code::RButton:
    case Win32_VK_Code::Cancel:
    case Win32_VK_Code::MButton:
    case Win32_VK_Code::XButton1:
    case Win32_VK_Code::XButton2:
    case Win32_VK_Code::Shift:
    case Win32_VK_Code::Control:
    case Win32_VK_Code::Menu:
        return false;

    case Win32_VK_Code::Left:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::Left)];
        break;

    case Win32_VK_Code::Up:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::Up)];
        break;

    case Win32_VK_Code::Right:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::Right)];
        break;

    case Win32_VK_Code::Down:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::Down)];
        break;

    case Win32_VK_Code::A:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::A)];
        break;

    case Win32_VK_Code::D:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::D)];
        break;

    case Win32_VK_Code::Q:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::Q)];
        break;

    case Win32_VK_Code::S:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::S)];
        break;

    case Win32_VK_Code::W:
        key = &input_state->keys[static_cast<SizeU>(Key_Code::W)];
        break;

    default:
        return false;
    }

    assert(key);

    key->count = key_state.repeat_count;

    if (key_state.was_up) {
        key->was = Key_State::Up;
    } else {
        key->was = Key_State::Down;
    }

    if (key_state.is_up) {
        key->now = Key_State::Up;
    } else {
        key->now = Key_State::Down;
    }

    if (changed_key) {
        *changed_key = static_cast<Key_Code>(key - input_state->keys);
    }

    return true;
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
win32_is_vk_pressed(Int32S vk)
{
    Int16S state = GetKeyState(vk);
    bool result = state >> 15;
    return result;
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
gl_make_texture_from_pixels(void *pixels, size32_t width, size32_t height, Color_Layout layout, GLenum internal_format)
{
    assert(layout == Color_Layout::BGRA_U8);

    GLenum format = 0, type = 0;

    if (layout == Color_Layout::BGRA_U8) {
        format = GL_BGRA;
        type = GL_UNSIGNED_BYTE;
    }

    // TODO(gr3yknigh1): Need to add support for more formats [2025/02/23]

    assert(format && type); // NOTE(gr3yknigh1): Should not be zero [2025/02/23]
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, pixels);
}

bool
load_tilemap_from_buffer(Asset_Store *store, char *buffer, SizeU buffer_size, Tilemap *tilemap)
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
                SizeU current_index_index = indexes_cursor - tilemap->indexes;
                assert(current_index_index + 1 <= tilemap->indexes_count);

                assert(lexer_parse_int(&lexer, indexes_cursor));
                lexer_skip_whitespace(&lexer);

                ++indexes_cursor;
            } while(isdigit(lexer.lexeme));

            SizeU filled_indexes = indexes_cursor - tilemap->indexes;
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

    tilemap->texture_asset = asset_load(store, Asset_Type::Texture, tilemap_image_path);
    assert(tilemap->texture_asset);

    mm::deallocate(tilemap_image_path);

    return tilemap;
}

bool
path16_get_parent(const wchar_t *path, SizeU path_length, Str16_View *out)
{

    out->data = path;

    for (SizeU index = path_length - 1; index > 0; --index) {
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

    Int32U file_notify_info_capacity = 1024;
    FILE_NOTIFY_INFORMATION *file_notify_info = mm::allocate_structs<FILE_NOTIFY_INFORMATION>(file_notify_info_capacity, ALLOCATE_ZERO_MEMORY);
    assert(file_notify_info);

    // TODO(gr3yknigh1): Replace with String_Builder [2025/03/10]
    uint64_t target_dir_length = str16_get_length(context->target_dir);
    SizeU target_dir_buffer_size = target_dir_length * sizeof(*context->target_dir);
    SizeU file_full_path_buffer_capacity = (target_dir_length + MAX_PATH) * sizeof(*context->target_dir);

    void *file_full_path_buffer = mm::allocate(file_full_path_buffer_capacity, ALLOCATE_ZERO_MEMORY);
    assert(file_full_path_buffer && "Buy more RAM");

    // NOTE(gr3yknigh1): Clunky! [2025/03/10]
    mm::copy_memory(file_full_path_buffer, static_cast<const void *>(context->target_dir), target_dir_buffer_size);

    while (!context->should_stop.test()) {

        DWORD bytes_returned = 0;
        assert(ReadDirectoryChangesW(
            watch_dir, file_notify_info, sizeof(*file_notify_info) * file_notify_info_capacity,
            true, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytes_returned, nullptr, nullptr
        ));

        FILE_NOTIFY_INFORMATION *current_notify_info = file_notify_info;

        for (;;) {

            const wchar_t *changed_file_relative_path = current_notify_info->FileName;
            const uint64_t changed_file_relative_path_length = current_notify_info->FileNameLength / sizeof(*current_notify_info->FileName);
            const SizeU changed_file_relative_path_buffer_size = current_notify_info->FileNameLength;

            if (context->notification_routine != nullptr) {
                // TODO(gr3yknigh1): Replace with some kind of Path_Join function [2025/03/10]
                static_cast<wchar_t *>(file_full_path_buffer)[target_dir_length] = path16_get_separator();
                SizeU separator_size = sizeof(wchar_t);
                mm::copy_memory( mm::get_offset(file_full_path_buffer, target_dir_buffer_size + separator_size), static_cast<const void *>(changed_file_relative_path), changed_file_relative_path_buffer_size );

                Str16_View file_name(static_cast<wchar_t *>(file_full_path_buffer), target_dir_length + 1 + changed_file_relative_path_length);
                //                                                                                    ^^^
                // WARN(gr3yknigh1): BECAUSE WE INSERTED SEPARATOR BEFORE!!! [2025/03/10]
                // Move it to Path_Join as soon as possible
                //


                #if 0
                //
                // TODO(gr3yknigh1): Replace with iterator [2025/04/15]
                //
                for (Size i = 0; i < file_name.length; ++i) {
                    putwchar(file_name.data[i]);
                }
                putchar('\n');
                #endif

                context->notification_routine(context, file_name, (File_Action)current_notify_info->Action, context->parameter);
            }


            if (!current_notify_info->NextEntryOffset) {
                break;
            }

            current_notify_info = mm::get_offset(current_notify_info, current_notify_info->NextEntryOffset);
        }
    }


    mm::deallocate(file_full_path_buffer);
    mm::deallocate(file_notify_info);
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
asset_load(Asset_Store *store, Asset_Type type, const Str8_View file_path)
{
    // TODO(gr3yknigh1): Handle errors and mark asset as failed to load: Asset_State::LoadFailure [2025/03/10]

    assert(store && !file_path.empty());

    Asset *asset = mm::allocate_struct<Asset>(&store->asset_pool, ALLOCATE_ZERO_MEMORY);
    assert(asset);

    asset->type = type;

    Asset_Location *location = &asset->location;

    location->type          = Asset_Location_Type::File;
    location->u.file.path   = Str8(file_path.data, file_path.length) /*asset_store_resolve_file(file)*/;
    location->u.file.handle = fopen(location->u.file.path.data, "r");
    assert(location->u.file.handle);

    location->u.file.size   = get_file_size(location->u.file.handle);

    if (asset->type == Asset_Type::Texture) {
        // NOTE(gr3yknigh1): Assume that file is path to BMP image [2025/03/10]
        Bitmap_Picture picture;
        assert(load_bitmap_picture_info_from_file(&picture, location->u.file.handle));

        // TODO(gr3yknigh1): Allocate less data, because file_size includes size of metadata [2025/03/10]
        //
        picture.u.data = allocate(&store->asset_content, picture.header.file_size);
        assert(load_bitmap_picture_pixel_data_from_file(&picture, location->u.file.handle));

        assert(asset_from_bitmap_picture(asset, &picture));
    } else if (asset->type == Asset_Type::Shader) {

        asset->u.shader.source_code = static_cast<char *>(allocate(&store->asset_content, location->u.file.size));
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

        SizeU buffer_size = asset->location.u.file.size + 1;
        void* buffer = mm::allocate(buffer_size);
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

bool
asset_reload(Asset_Store *store, Asset *asset)
{
    bool result = true;

    assert(store && asset);

    if (asset->state == Asset_State::Loaded) {
        result = asset_unload(store, asset);
    }

    if (result && asset->location.type == Asset_Location_Type::File) {

        if (result && asset->location.u.file.handle == nullptr) {
            // TODO(gr3yknigh1): Wrap fopen in function which accepts Str8_View-s [2025/03/10]
            asset->location.u.file.handle = fopen(asset->location.u.file.path.data, "r");
            asset->location.u.file.size = get_file_size(asset->location.u.file.handle);
            assert(asset->location.u.file.handle);
        }

        if (asset->type == Asset_Type::Texture) {
            Bitmap_Picture picture;
            assert(load_bitmap_picture_info_from_file(&picture, asset->location.u.file.handle));

            //
            // TODO(gr3yknigh1): Allocate less data, because file_size includes size of metadata [2025/03/10]
            //
            picture.u.data = allocate(&store->asset_content, picture.header.file_size);
            assert(load_bitmap_picture_pixel_data_from_file(&picture, asset->location.u.file.handle));

            assert(asset_from_bitmap_picture(asset, &picture));
        } else if (asset->type == Asset_Type::Shader) {

            asset->u.shader.source_code = static_cast<char *>(allocate(&store->asset_content, asset->location.u.file.size));
            mm::zero_memory(asset->u.shader.source_code, asset->location.u.file.size);
            fread(asset->u.shader.source_code, asset->location.u.file.size, 1, asset->location.u.file.handle);

            Shader_Compile_Result compile_result = compile_shader(asset->u.shader.source_code, asset->location.u.file.size);

            glDeleteProgram(asset->u.shader.program_id); // @cleanup
            asset->u.shader.program_id = compile_result.shader_program_id;
            assert(asset->u.shader.program_id);
        } else if (asset->type == Asset_Type::Tilemap) {
            SizeU buffer_size = asset->location.u.file.size + 1;
            void* buffer = mm::allocate(buffer_size);
            mm::zero_memory(buffer, buffer_size);

            fread(buffer, buffer_size - 1, 1, asset->location.u.file.handle);

            assert(load_tilemap_from_buffer(store, static_cast<char *>(buffer), buffer_size, &asset->u.tilemap));

            mm::deallocate(buffer);

        } else {
            result = false;
        }

        if (result && asset->location.u.file.handle != nullptr) {
            fclose(asset->location.u.file.handle);
            asset->location.u.file.handle = nullptr;  // Saying that the file handle is closed
        }
    }


    if (result) {
        asset->state = Asset_State::Loaded;
    }
    return result;
}


bool
asset_from_bitmap_picture(Asset *asset, Bitmap_Picture *picture)
{
    //
    // Copy image data to more generalized structure
    //
    asset->u.texture.width = picture->dib_header.width;
    asset->u.texture.height = picture->dib_header.height;

    asset->u.texture.layout = Color_Layout::Nothing;

    if (picture->dib_header.compression_method ==  Bitmap_Picture_Compression_Method::Bitfields) {
        asset->u.texture.layout = Color_Layout::BGRA_U8;
    }

    assert(asset->u.texture.layout != Color_Layout::Nothing);

    asset->u.texture.pixels.data = picture->u.data;

    return true;
}


bool
asset_unload(Asset_Store *store, Asset *asset)
{
    assert(store && asset);
    assert(asset->state == Asset_State::Loaded);

    bool result = true;

    if (asset->type == Asset_Type::Texture) {
        result = reset(&store->asset_content, asset->u.texture.pixels.data );
    } else if (asset->type == Asset_Type::Shader) {
        result = reset(&store->asset_content, asset->u.shader.source_code);
    } else if (asset->type == Asset_Type::Tilemap) {
        Tilemap *tilemap = &asset->u.tilemap;

        if (tilemap->texture_asset->state == Asset_State::Loaded) {
            result = asset_unload(store, tilemap->texture_asset);
        }

        if (result) {
            mm::deallocate(tilemap->indexes);
        }
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
        wchar_t *image_path_buffer = static_cast<wchar_t *>(mm::allocate(image_path_buffer_size, ALLOCATE_ZERO_MEMORY));
        assert(image_path_buffer);

        DWORD bytes_written = GetModuleFileNameW(nullptr, image_path_buffer, image_path_buffer_size);
        DWORD last_error = GetLastError();
        assert(last_error == ERROR_SUCCESS);

        Str16_View image_directory_view;
        assert(path16_get_parent(image_path_buffer, bytes_written, &image_directory_view));

        wchar_t *image_directory_buffer = static_cast<wchar_t *>(mm::allocate((image_directory_view.length + 1) * sizeof(*image_directory_view.data)));
        assert(image_directory_buffer);

        assert(str16_view_copy_to_nullterminated(image_directory_view, image_directory_buffer, (image_directory_view.length + 1) * sizeof(*image_directory_view.data)));
        mm::deallocate(image_path_buffer);

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

void
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
compile_shader(char *source_code, SizeU file_size)
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
    assert(asset->type == Asset_Type::Texture);

    asset->u.texture.unit = unit;
    glActiveTexture(GL_TEXTURE0 + asset->u.texture.unit);
    if (asset->u.texture.id) {
        glDeleteTextures(1, &asset->u.texture.id);
    }
    glGenTextures(1, &asset->u.texture.id);
    glBindTexture(GL_TEXTURE_2D, asset->u.texture.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_make_texture_from_pixels(
        asset->u.texture.pixels.data, asset->u.texture.width, asset->u.texture.height,
        asset->u.texture.layout, GL_RGBA8);
    glGenerateMipmap(GL_TEXTURE_2D);

    // NOTE: After loading atlas in GPU, we do not need to keep it in RAM.
    assert(asset_unload(store, asset));

    if (shader) {
        GLint texture_uniform_loc = glGetUniformLocation(shader->program_id, "u_texture");
        assert(texture_uniform_loc != -1);

        glUniform1i(texture_uniform_loc, asset->u.texture.unit);
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


SizeU
mm::get_page_size(void)
{
    SizeU result;
    SYSTEM_INFO system_info = {0};
    GetSystemInfo(&system_info);
    result = system_info.dwPageSize;
    return result;
}
