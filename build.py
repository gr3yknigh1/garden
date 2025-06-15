from __future__ import annotations

from os.path import dirname, realpath
import os.path

from hbuild import *


project_folder = dirname(realpath(__file__))
assets_folder  = os.path.sep.join([project_folder, "assets"])

noc = add_external_library("noc", location=r"nostdlib")
                           #location="git+https://github.com/gr3yknigh1/nostdlib@bb611ec")
# NOTE(gr3yknigh1): While there is no package management and no option being propagated, NOC_LIBC_WRAPPERS are defined
# by default [2025/06/10]
target_macros(noc, Access.PUBLIC, macros=dict(NOC_LIBC_WRAPPERS="1"))

glm = add_external_library("glm", location="glm", tool=BuildTool.CMAKE)
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

target_links(garden_runtime, links=[glad, imgui, glm, noc])
target_macros(garden_runtime, macros=dict(
    GARDEN_GAMEPLAY_DLL_NAME="garden_gameplay.dll",
    GARDEN_ASSETS_FOLDER=f"{assets_folder!r}", # TODO(gr3yknigh1): Expose to the tasks.py level [2025/05/02] #buildsystem
    _CRT_SECURE_NO_WARNINGS="1",
))

garden_gameplay = add_library("garden_gameplay", dynamic=True, sources=(
    "code/garden_runtime.cpp",
))
target_links(garden_gameplay, links=[glad, imgui, glm, noc])
target_macros(garden_gameplay, macros=dict(
    GARDEN_GAMEPLAY_CODE="1",
    GARDEN_GAMEPLAY_DLL_NAME="garden_gameplay.dll",
    _CRT_SECURE_NO_WARNINGS="1",
))

add_package("garden", targets=[
    garden_runtime, garden_gameplay,
])
