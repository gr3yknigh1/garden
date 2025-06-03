
from hbuild import *


glm = add_external_library("glm", location="glm", tool=ExternalTool.CMAKE)
target_includes(glm, Access.PUBLIC, includes=["glm"])


imgui = add_library("imgui", sources=(
    "imgui/imgui.cpp",
    "imgui/imgui_demo.cpp",
    "imgui/imgui_draw.cpp",
    "imgui/imgui_widgets.cpp",
    "imgui/imgui_tables.cpp",
    "imgui/backends/imgui_impl_win32.cpp",
    "imgui/backends/imgui_impl_opengl3.cpp",
))
target_includes(imgui, Access.PUBLIC, includes=[
    "imgui",
    "imgui/backends",
    "imgui/misc/cpp",
])


glad = add_library("glad", sources=(
    "glad/glad.c",
    "glad/glad_wgl.c",
))
target_includes(glad, Access.PUBLIC, includes=[
    "glad"
])


garden_runtime = add_executable("garden", sources=(
    "code/garden_runtime.cpp",
))

target_links(garden_runtime, links=[glad, imgui, glm])
target_macros(garden_runtime, macros=dict(
    GARDEN_GAMEPLAY_DLL_NAME="garden_gameplay.dll",
    GARDEN_ASSETS_FOLDER="${PROJECT_SOURCE_DIR}/assets", # TODO(gr3yknigh1): Expose to the tasks.py level [2025/05/02] #buildsystem
    _CRT_SECURE_NO_WARNINGS="1",
))

garden_gameplay = add_library("garden_gameplay", dynamic=True, sources=(
    "code/garden_runtime.cpp",
))
target_links(garden_gameplay, links=[glad, imgui, glm])
target_macros(garden_gameplay, macros=dict(
    GARDEN_GAMEPLAY_CODE="1",
    GARDEN_GAMEPLAY_DLL_NAME="garden_gameplay.dll",
    _CRT_SECURE_NO_WARNINGS="1",
))
