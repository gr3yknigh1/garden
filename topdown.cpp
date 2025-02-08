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
#include <glad/glad.h>

#if !defined(STATIC_ARRAY_COUNT)
#define STATIC_ARRAY_COUNT(ARRAY_PTR) (sizeof((ARRAY_PTR)) / sizeof(*(ARRAY_PTR)))
#endif

#if !defined(ZERO_STRUCT)
#define ZERO_STRUCT(STRUCT_PTR) ZeroMemory((STRUCT_PTR), sizeof(*(STRUCT_PTR)))
#endif

typedef int bool32_t;
typedef unsigned int size32_t;

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
// OpenGL API wrappers
//
GLuint CompileShaderFromString(const char *string, GLenum type);
GLuint CompileShaderFromFile(const char *file_path, GLenum type);
GLuint LinkShaderProgram(GLuint vertex_shader, GLuint fragment_shader);

struct VertexBufferAttribute {
    bool is_normalized;
    unsigned int type;
    unsigned int count;
    size32_t size;
};

struct VertexBufferLayout {
    VertexBufferAttribute *attributes;
    unsigned int attributes_count;
    size32_t stride;
};

void VertexBufferLayout_PushFloat(VertexBufferLayout *layout, unsigned int count);


//
// @brief Initializes vertex buffer layout.
//
// @pre
//     glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)
//     glBindVertexArray(vertex_array);
//
void VertexArray_AddBuffer(const GLuint vertex_array, const GLuint vertex_buffer, const VertexBufferLayout *layout);


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
    // OpenGL initialization
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

    //
    // Game initalization
    //
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    GLuint basic_vert_shader = CompileShaderFromFile("basic.vert.glsl", GL_VERTEX_SHADER);
    GLuint basic_frag_shader = CompileShaderFromFile("basic.frag.glsl", GL_FRAGMENT_SHADER);
    GLuint basic_shader_program = LinkShaderProgram(basic_vert_shader, basic_frag_shader);

    /* After linking shaders no longer needed. */
    glDeleteShader(basic_vert_shader);
    glDeleteShader(basic_frag_shader);

    glUseProgram(basic_shader_program);

    static const float vertexes[] = {
        0, 0.5,
        0.5, 0,
        -0.5, 0
    };
    size_t vertexes_count = STATIC_ARRAY_COUNT(vertexes);

    GLuint vertex_array = 0;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

    size_t vertex_buffer_size = vertexes_count * sizeof(*vertexes);
    glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, vertexes, GL_DYNAMIC_DRAW);

    VertexBufferLayout vertex_buffer_layout;
    ZERO_STRUCT(&vertex_buffer_layout);

    vertex_buffer_layout.attributes = (VertexBufferAttribute *)calloc(1, sizeof(VertexBufferAttribute));

    VertexBufferLayout_PushFloat(&vertex_buffer_layout, 2);

    VertexArray_AddBuffer(vertex_array, vertex_buffer, &vertex_buffer_layout);

    //
    // Game mainloop
    //


    while (!global_should_terminate) {
        MSG message;
        ZERO_STRUCT(&message);

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                global_should_terminate = true;
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // NOTE(i.akkuzin): After WM_QUIT device_context is invalid [2025/02/08]
        if (global_should_terminate) {
            break;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertex_buffer_size / vertex_buffer_layout.stride));

        assert(SwapBuffers(window_device_context));
    }

    glDeleteProgram(basic_shader_program);

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
CompileShaderFromFile(const char *file_path, GLenum type)
{
    // TODO(gr3yknigh1): Check for path existens and that it is file [2024/11/24]

    FILE *file = fopen(file_path, "r");
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(file_size);

    size_t string_buffer_size = file_size + 1;
    char *string_buffer = (char *)malloc(string_buffer_size);
    assert(string_buffer);

    memset(string_buffer, 0, string_buffer_size);

    fread(string_buffer, string_buffer_size, 1, file);
    fclose(file);

    GLuint id = CompileShaderFromString(string_buffer, type);

    free(string_buffer);

    return id;
}

GLuint
CompileShaderFromString(const char *string, GLenum type)
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
        char *log_buffer = (char *)malloc(log_buffer_size);
        assert(log_buffer);

        memset(log_buffer, 0, log_buffer_size);

        glGetShaderInfoLog(id, (GLsizei)log_buffer_size, 0, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* DIE_MF("Failed to compile OpenGL shader! %s", logBuffer); */
    }

    return id;
}

GLuint
LinkShaderProgram(GLuint vertex_shader, GLuint fragment_shader)
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
        char *log_buffer = (char *)malloc(log_buffer_size);
        assert(log_buffer);

        memset(log_buffer, 0, log_buffer_size);

        glGetProgramInfoLog(id, (GLsizei)log_buffer_size, NULL, log_buffer);

        assert(false); // TODO(i.akkuzin): Implement DIE macro [2025/02/08]
        /* IGNITE_DIE_MF("Failed to link OpenGL program! %s", logBuffer); */
    }

    return id;
}

void
VertexBufferLayout_PushFloat(VertexBufferLayout *layout, unsigned int count)
{
    size32_t attribute_size = sizeof(float);

    VertexBufferAttribute *attribute = layout->attributes + layout->attributes_count;
    attribute->is_normalized = false;
    attribute->type = GL_FLOAT;
    attribute->count = count;
    attribute->size = attribute_size;

    layout->attributes_count += 1;
    layout->stride += attribute_size * count;
}

void
VertexArray_AddBuffer(const GLuint vertex_array, const GLuint vertex_buffer, const VertexBufferLayout *layout)
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
