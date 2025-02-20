//
// FILE         topdown.cpp
//
// AUTHORS
//               Ilya Akkuzin <gr3yknigh1@gmail.com>
//
// NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//
#include <assert.h> // assert
#include <stdio.h>  // puts, printf, FILE, fopen, freopen, fseek, fclose

#include <windows.h>
#include <intrin.h> // __rdtsc

// NOTE(i.akkuzin): For `Camera` struct. [2025/02/09]
#undef near
#undef far

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>

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

#if !defined(STRIGIFY)
    #define STRINGIFY(X) #X
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
    packed_rgba_t color;
};
#pragma pack(pop)

static_assert(sizeof(Vertex) == (sizeof(float) * 2 + sizeof(packed_rgba_t)));

struct Color4 {
    uint8_t r, g, b, a;
};

//
// Perf helpers:
//

int64_t GetPerfFrequency(void);
int64_t GetPerfCounter(void);
uint64_t GetCyclesCount(void);

#pragma pack(push, 1)
typedef struct Perf_BlockRecord {
    const char *label;
    const char *function;
    const char *file_path;
    uint64_t line_number;
    uint64_t cycles_begin;
    uint64_t cycles_end;
    int64_t counter_begin;
    int64_t counter_end;
} Ignite_Perf_BlockRecord;
#pragma pack(pop)

void Perf_BlockRecord_Print(const Perf_BlockRecord *record);

#define PERF_BLOCK_RECORD(NAME) NAME##__BLOCK_RECORD

#define PERF_BLOCK_BEGIN(NAME) \
    Perf_BlockRecord PERF_BLOCK_RECORD(NAME); \
    do { \
        PERF_BLOCK_RECORD(NAME).label = STRINGIFY(NAME); \
        PERF_BLOCK_RECORD(NAME).function = __FUNCTION__; \
        PERF_BLOCK_RECORD(NAME).file_path = __FILE__; \
        PERF_BLOCK_RECORD(NAME).line_number = __LINE__; \
        PERF_BLOCK_RECORD(NAME).cycles_begin = GetCyclesCount(); \
        PERF_BLOCK_RECORD(NAME).counter_begin = GetPerfCounter(); \
    } while (0)

#define PERF_BLOCK_END(NAME) \
    do { \
        PERF_BLOCK_RECORD(NAME).cycles_end = GetCyclesCount(); \
        PERF_BLOCK_RECORD(NAME).counter_end = GetPerfCounter(); \
        Perf_BlockRecord_Print(&PERF_BLOCK_RECORD(NAME)); \
    } while (0)


//
// Allocators:
//

struct Arena {
    void *data;
    size_t capacity;
    size_t occupied;
};

Arena MakeArena(size_t capacity);

#define ARENA_ALLOC_BASIC   MAKE_FLAG(0)
#define ARENA_ALLOC_POPABLE MAKE_FLAG(1)

void *ArenaAlloc(Arena *arena, size_t size, int options);
void *ArenaAllocSet(Arena *arena, size_t size, char c, int options);
void *ArenaAllocZero(Arena *arena, size_t size, int options);
bool ArenaPop(Arena *arena, void *data);

size_t ResetArena(Arena *arena);
bool FreeArena(Arena *arena);

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

static void      Win32_InitOpenGLContextExtensions(void);
static HGLRC     Win32_InitOpenGLContext(HDC device_context);
LRESULT CALLBACK Win32_WindowEventHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

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

Camera MakeCamera(Camera_ViewMode view_mode);

void RotateCamera(Camera *camera, float x_offset, float y_offset);
glm::mat4 GetCameraViewMatrix(Camera *camera);
glm::mat4 GetCameraProjectionMatrix(Camera *camera, int viewport_width, int viewport_height);

//
// OpenGL API wrappers
//
GLuint CompileShaderFromString(Arena *arena, const char *string, GLenum type);
GLuint CompileShaderFromFile(Arena *arena, const char *file_path, GLenum type);
GLuint LinkShaderProgram(Arena *arena, GLuint vertex_shader, GLuint fragment_shader);

struct VertexBufferAttribute {
    bool is_normalized;
    unsigned int type;
    unsigned int count;
    size32_t size;
};

struct VertexBufferLayout {
    VertexBufferAttribute *attributes;
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
bool MakeVertexBufferLayout(Arena *arena, VertexBufferLayout *layout, size32_t attributes_capacity);
VertexBufferAttribute *VertexBufferLayout_Push(VertexBufferLayout *layout, unsigned int count, GLenum type, size_t size);
VertexBufferAttribute *VertexBufferLayout_PushFloat(VertexBufferLayout *layout, unsigned int count);
VertexBufferAttribute *VertexBufferLayout_PushInt(VertexBufferLayout *layout, unsigned int count);

//
// @brief Initializes vertex buffer layout.
//
// @pre
//     glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)
//     glBindVertexArray(vertex_array);
//
void VertexBufferLayout_BuildAttributes(const VertexBufferLayout *layout);


//
// @param[out] rect Output array of vertexes
//
size_t GenerateRect(Vertex *rect, float x, float y, float width, float height, Color4 color);

struct InputState {
    float x_direction;
    float y_direction;
};

struct Win32_KeyState {
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

Win32_KeyState Win32_ConvertMSGToKeyState(MSG message);

//
// Handle keyboard input for Win32 API layer.
//
// @param[in] message Actual windows message which received in mainloop.
//
// @param[out] input_state Output InputState of the game.
//
void Win32_HandleKeyboardInput(MSG message, InputState *input_state);


int WINAPI
wWinMain(HINSTANCE instance, HINSTANCE previous_instance, PWSTR command_line, int cmd_show)
{
    (void)previous_instance;
    (void)command_line;

    //
    // Window initialization
    //
    const wchar_t *window_class_name = L"topdown";
    const wchar_t *window_title = L"topdown";

    WNDCLASSW window_class;
    ZERO_STRUCT(&window_class);
    window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    //                                             ^^^^^^^^
    // NOTE(ilya.a): Needed by OpenGL. See Khronos's docs [2024/11/10]
    window_class.lpfnWndProc = Win32_WindowEventHandler;
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
    HGLRC window_render_context = Win32_InitOpenGLContext(window_device_context);
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
    // Game initalization
    //
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    Arena shader_compilation_arena = MakeArena(1024 * 10);

    GLuint basic_vert_shader = CompileShaderFromFile(&shader_compilation_arena, "basic.vert.glsl", GL_VERTEX_SHADER);
    GLuint basic_frag_shader = CompileShaderFromFile(&shader_compilation_arena, "basic.frag.glsl", GL_FRAGMENT_SHADER);
    GLuint basic_shader_program = LinkShaderProgram(&shader_compilation_arena, basic_vert_shader, basic_frag_shader);

    FreeArena(&shader_compilation_arena);

    /* After linking shaders no longer needed. */
    glDeleteShader(basic_vert_shader);
    glDeleteShader(basic_frag_shader);

    glUseProgram(basic_shader_program);

    GLint model_uniform_loc = glGetUniformLocation(basic_shader_program, "model");
    assert(model_uniform_loc != -1);

    GLint projection_uniform_loc = glGetUniformLocation(basic_shader_program, "projection");
    assert(projection_uniform_loc != -1);

    Camera camera = MakeCamera(Camera_ViewMode::Orthogonal);

    glm::mat4 model = glm::identity<glm::mat4>();
    glm::mat4 projection = GetCameraProjectionMatrix(&camera, window_width, window_height);

    glUniformMatrix4fv(model_uniform_loc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(projection_uniform_loc, 1, GL_FALSE, glm::value_ptr(projection));

    GLuint vertex_array = 0;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

    Arena page_arena = MakeArena(1024);

    VertexBufferLayout vertex_buffer_layout;
    assert(MakeVertexBufferLayout(&page_arena, &vertex_buffer_layout, 4));

    assert(VertexBufferLayout_PushFloat(&vertex_buffer_layout, 2));
    assert(VertexBufferLayout_PushInt(&vertex_buffer_layout, 1));

    VertexBufferLayout_BuildAttributes(&vertex_buffer_layout);

    ResetArena(&page_arena);

    //
    // Game mainloop
    //

    Arena geometry_arena = MakeArena(sizeof(Vertex) * 1024);

    float player_x = 0, player_y = 0, player_speed = 300.0f;

    float perf_frequency = (float)GetPerfFrequency();
    float ticks_begin = 0;
    float ticks_end = 0;
    float dt = 1.0f;

    while (!global_should_terminate) {

        ticks_begin = (float)GetPerfCounter();


        MSG message;
        ZERO_STRUCT(&message);

        InputState input_state;
        ZERO_STRUCT(&input_state);

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                global_should_terminate = true;
            } else {
                switch (message.message) {
                case WM_KEYUP:
                case WM_KEYDOWN:
                case WM_SYSKEYUP:
                case WM_SYSKEYDOWN:
                    Win32_HandleKeyboardInput(message, &input_state);
                    break;
                }
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // NOTE(i.akkuzin): After WM_QUIT device_context is invalid [2025/02/08]
        if (global_should_terminate) {
            break;
        }

        player_x += player_speed * input_state.x_direction * dt;
        player_y += player_speed * input_state.y_direction * dt;

        PERF_BLOCK_BEGIN(DRAW);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Vertex *vertexes = (Vertex *)ArenaAllocZero(&geometry_arena, sizeof(Vertex) * 6, ARENA_ALLOC_BASIC);
        Color4 rect_color = { 200, 100, 0, 255 };
        size_t vertexes_count = GenerateRect(vertexes, player_x, player_y, 100, 100, rect_color);
        size_t vertex_buffer_size = vertexes_count * sizeof(*vertexes);
        glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, vertexes, GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertex_buffer_size / vertex_buffer_layout.stride));

        assert(SwapBuffers(window_device_context));
        PERF_BLOCK_END(DRAW);

        ResetArena(&geometry_arena);

        ticks_end = (float)GetPerfCounter();
        dt = ticks_end - ticks_begin;
        dt /= perf_frequency;
    }

    glDeleteProgram(basic_shader_program);

    FreeArena(&page_arena);
    FreeArena(&geometry_arena);

    assert(FreeLibrary(opengl_module));
    // TODO(i.akkuzin): CloseWindow [2025/02/08]

    return 0;
}

LRESULT CALLBACK
Win32_WindowEventHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
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
Win32_InitOpenGLContextExtensions(void)
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
Win32_InitOpenGLContext(HDC device_context)
{
    Win32_InitOpenGLContextExtensions();

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
CompileShaderFromFile(Arena *arena, const char *file_path, GLenum type)
{
    // TODO(gr3yknigh1): Check for path existens and that it is file [2024/11/24]

    FILE *file = fopen(file_path, "r");
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(file_size);

    size_t string_buffer_size = file_size + 1;

    char *string_buffer = (char *)ArenaAllocZero(arena, string_buffer_size, ARENA_ALLOC_POPABLE);
    assert(string_buffer);

    memset(string_buffer, 0, string_buffer_size);

    fread(string_buffer, string_buffer_size, 1, file);
    fclose(file);

    GLuint id = CompileShaderFromString(arena, string_buffer, type);

    assert(ArenaPop(arena, string_buffer));

    return id;
}

GLuint
CompileShaderFromString(Arena *arena, const char *string, GLenum type)
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
        char *log_buffer = (char *)ArenaAllocZero(arena, log_buffer_size, ARENA_ALLOC_POPABLE);
        assert(log_buffer);

        glGetShaderInfoLog(id, (GLsizei)log_buffer_size, 0, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to compile OpenGL shader! %s", logBuffer); */
        ArenaPop(arena, log_buffer);
    }

    return id;
}

GLuint
LinkShaderProgram(Arena *arena, GLuint vertex_shader, GLuint fragment_shader)
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
        char *log_buffer = (char *)ArenaAllocZero(arena, log_buffer_size, ARENA_ALLOC_POPABLE);
        assert(log_buffer);

        glGetProgramInfoLog(id, (GLsizei)log_buffer_size, NULL, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to link OpenGL program! %s", logBuffer); */
        ArenaPop(arena, log_buffer);
    }

    return id;
}

bool
MakeVertexBufferLayout(Arena *arena, VertexBufferLayout *layout, size32_t attributes_capacity)
{
    ZERO_STRUCT(layout);

    layout->attributes = (VertexBufferAttribute *)ArenaAllocZero(
        arena, attributes_capacity * sizeof(VertexBufferAttribute), ARENA_ALLOC_BASIC);

    if (layout->attributes == nullptr) {
        return false;
    }

    layout->attributes_capacity = attributes_capacity;

    return true;
}

VertexBufferAttribute *
VertexBufferLayout_Push(VertexBufferLayout *layout, unsigned int count, GLenum type, size32_t size)
{
    if (layout->attributes_count + 1 > layout->attributes_capacity) {
        return nullptr;
    }

    VertexBufferAttribute *attribute = layout->attributes + layout->attributes_count;
    attribute->is_normalized = false;
    attribute->type = type;
    attribute->count = count;
    attribute->size = size;

    layout->attributes_count += 1;
    layout->stride += size * count;

    return attribute;
}

VertexBufferAttribute *
VertexBufferLayout_PushInt(VertexBufferLayout *layout, unsigned int count)
{
    size32_t attribute_size = sizeof(int);
    return VertexBufferLayout_Push(layout, count, GL_INT, attribute_size);
}

VertexBufferAttribute *
VertexBufferLayout_PushFloat(VertexBufferLayout *layout, unsigned int count)
{
    size32_t attribute_size = sizeof(float);
    return VertexBufferLayout_Push(layout, count, GL_FLOAT, attribute_size);
}

void
VertexBufferLayout_BuildAttributes(const VertexBufferLayout *layout)
{
    size_t offset = 0;

    for (unsigned int attribute_index = 0; attribute_index < layout->attributes_count;
         ++attribute_index) {
        VertexBufferAttribute *attribute = layout->attributes + attribute_index;

        glEnableVertexAttribArray(attribute_index);
        glVertexAttribPointer(
            attribute_index, attribute->count, attribute->type,
            attribute->is_normalized, layout->stride, (void *)offset);

        offset += attribute->size * attribute->count;
    }
}


Camera
MakeCamera(Camera_ViewMode view_mode)
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
RotateCamera(Camera *camera, float x_offset, float y_offset)
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
GetCameraViewMatrix(Camera *camera)
{
    return glm::lookAt(
        camera->position, camera->position + camera->front, camera->up);
}

glm::mat4
GetCameraProjectionMatrix(Camera *camera, int viewport_width, int viewport_height)
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
MakeArena(size_t capacity)
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
ArenaPop(Arena *arena, void *data)
{
    if (arena == nullptr || data == nullptr) {
        return false;
    }

    if (arena->data == nullptr || arena->occupied < sizeof(arena->occupied) || arena->capacity == 0) {
        return false;
    }

    size_t *data_size = ((size_t *)data) - 1;
    arena->occupied -= *data_size;
    arena->occupied -= sizeof(*data_size);

    return true;
}

size_t
ResetArena(Arena *arena)
{
    size_t was_occupied = arena->occupied;
    arena->occupied = 0;
    return was_occupied;
}

void *
ArenaAlloc(Arena *arena, size_t size, int options)
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
ArenaAllocSet(Arena *arena, size_t size, char c, int options)
{
    void *allocated = ArenaAlloc(arena, size, options);

    if (allocated == nullptr) {
        return allocated;
    }

    memset(allocated, c, size);
    return allocated;
}

void *
ArenaAllocZero(Arena *arena, size_t size, int options)
{
    return ArenaAllocSet(arena, size, 0, options);
}

bool
FreeArena(Arena *arena)
{
    bool result = VirtualFree(arena->data, 0, MEM_RELEASE);
    ZERO_STRUCT(arena);
    return result;
}

size_t
GenerateRect(Vertex *vertexes, float x, float y, float width, float height, Color4 color)
{
    size_t c = 0;

    // bottom-left triangle
    vertexes[c++] = { x + 0    , y + 0,      MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + 0,      MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-right
    vertexes[c++] = { x + width, y + height, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right

    // top-right triangle
    vertexes[c++] = { x + 0    , y + 0,      MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // bottom-left
    vertexes[c++] = { x + width, y + height, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };  // top-right
    vertexes[c++] = { x + 0,     y + height, MAKE_PACKED_RGBA(color.r, color.g, color.b, color.a) };

    return c;
}

int64_t
GetPerfFrequency(void)
{
    LARGE_INTEGER perf_frequency_result;
    QueryPerformanceFrequency(&perf_frequency_result);
    int64_t perf_frequency = perf_frequency_result.QuadPart;
    return perf_frequency;
}

int64_t
GetPerfCounter(void)
{
    LARGE_INTEGER perf_counter_result;
    QueryPerformanceCounter(&perf_counter_result);
    int64_t perf_counter = perf_counter_result.QuadPart;
    return perf_counter;
}

uint64_t
GetCyclesCount(void)
{
    uint64_t cycles_count = __rdtsc();
    return cycles_count;
}

void
Perf_BlockRecord_Print(const Perf_BlockRecord *record)
{
    int64_t perf_frequency = GetPerfFrequency();

    int64_t counter_elapsed = record->counter_end - record->counter_begin;
    int64_t ms_elapsed = (1000 * counter_elapsed) / perf_frequency;
    uint64_t cycles_elapsed = record->cycles_end - record->cycles_begin;
    uint64_t mega_cycles_elapsed = cycles_elapsed / (1000 * 1000);

    printf(
        "PERF: %s:%llu@%s [%s]: counter = (%llu) ms = (%llu) | Mc = %llu\n", record->file_path, record->line_number,
        record->function, record->label, counter_elapsed, ms_elapsed, mega_cycles_elapsed);
}

Win32_KeyState
Win32_ConvertMSGToKeyState(MSG message)
{
    //
    // @ref <https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input>
    //
    Win32_KeyState key_state;


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

void
Win32_HandleKeyboardInput(MSG message, InputState *input_state)
{
    input_state->x_direction = 0;
    input_state->y_direction = 0;

    Win32_KeyState key = Win32_ConvertMSGToKeyState(message);

    if (key.vk_code == VK_LEFT) {
        input_state->x_direction = -1;
    }

    if (key.vk_code == VK_RIGHT) {
        input_state->x_direction = +1;
    }

    if (key.vk_code == VK_DOWN) {
        input_state->y_direction = -1;
    }

    if (key.vk_code == VK_UP) {
        input_state->y_direction = +1;
    }

}
