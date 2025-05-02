#
# FILE          tasks.py
#
# AUTHORS
#               Ilya Akkuzin <gr3yknigh1@gmail.com>
#
# NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
#
# HOW TO USE?
#
#    * Install `hpipe`:
#
#        > python3 -m pip install git+ssh://git@github.com/gr3yknigh1/hpipe
#
#    * Call build task:
#
#        > htask build --build-type Debug
#
#    * Done!
#

from typing import Callable, Any

from os.path import exists, join, dirname, realpath

from htask import define_task, Context
from htask import load_env, save_env, is_file_busy
from htask.progs import msvc

F = Callable

default_build_type = "Debug"

project_dir: str = dirname(realpath(__file__))
assets_dir: str = join(project_dir, "assets")
code_dir: str = join(project_dir, "code")
output_dir: F[[str], str] = lambda build_type: join(project_dir, "build", build_type) # noqa

#
# Dependencies:
#

glm_folder = join(project_dir, "glm")
glm_configuration = join(glm_folder, "build")
glm_output_dir: F[[str], str] = lambda build_type: join(glm_configuration, "glm", build_type) # noqa
glm_library: F[[str], str] = lambda build_type: join(glm_output_dir(build_type), "glm.lib") # noqa

#
# Tasks:
#

@define_task()
def build(c: Context, build_type=default_build_type, clean=False, reconfigure=False, only_preprocessor=False, perf=False, debug_allocations=True):
    """Builds entire project.
    """
    if clean:
        if c.exists(glm_output_dir(build_type)):
            c.run(f"rmdir /S /Q {glm_output_dir(build_type)}")

        if c.exists(output_dir(build_type)):
            c.run(f"rmdir /S /Q {output_dir(build_type)}")

    #
    # GLM:
    #

    if not clean and c.exists(glm_library(build_type)):
        c.echo("I: GLM already built")
    else:
        c.echo("I: Building GLM...")
        c.run(f"cmake -B {glm_configuration} -S {glm_folder} -D GLM_BUILD_TESTS=OFF -D BUILD_SHARED_LIBS=OFF")
        c.run(f"cmake --build {glm_configuration} --config {build_type}")

    #
    # Garden:
    #

    if not c.exists(output_dir(build_type)):
        c.run(f"mkdir {output_dir(build_type)}")

    cached_env = c.join(output_dir(build_type), "vc_build.env")

    if not reconfigure and exists(cached_env):
        build_env = load_env(cached_env)
    else:
        build_env = msvc.extract_env_from_vcvars(c)
        save_env(cached_env, build_env)


    # TODO(i.akkuzin): Make release configuration build. Currently we using `/Od` flag, which disables any optimizations and link with debug runtime (/MTd). [2025/02/08]
    # TODO(i.akkuzin): Remove _CRT_SECURE_NO_WARNINGS [2025/02/08]


    c.echo("I: Compiling game code...")

    #
    # Common:
    #

    GAMEPLAY_DLL_NAME = "garden_gameplay.dll"

    defines: dict[str, Any] = dict(
        GARDEN_GAMEPLAY_DLL_NAME=GAMEPLAY_DLL_NAME,
        _CRT_SECURE_NO_WARNINGS=1,
    )
    compile_flags = [
        "/MTd",
        "/Zi",
        "/std:c++20",
        "/W4",
        "/Od",   # Disables optimizations
        "/GR-",  # Disables RTTI
        "/nologo"
    ]
    link_flags = ["/DEBUG:FULL"]
    libs = ["kernel32.lib", "user32.lib", "gdi32.lib", glm_library(build_type)]
    includes = [
        code_dir,
        glm_folder,
        c.join(project_dir, "glad"),
        c.join(project_dir, "imgui"),
        c.join(project_dir, "imgui", "backends"),
        c.join(project_dir, "imgui", "misc", "cpp"),
    ]
    sources = [
        c.join(code_dir, "garden_runtime.cpp"),
        c.join(project_dir, "glad", "glad.c"),
        c.join(project_dir, "glad", "glad_wgl.c"),
        c.join(project_dir, "imgui", "imgui.cpp"),
        c.join(project_dir, "imgui", "imgui_demo.cpp"),
        c.join(project_dir, "imgui", "imgui_draw.cpp"),
        c.join(project_dir, "imgui", "imgui_widgets.cpp"),
        c.join(project_dir, "imgui", "imgui_tables.cpp"),
        c.join(project_dir, "imgui", "backends", "imgui_impl_win32.cpp"),
        c.join(project_dir, "imgui", "backends", "imgui_impl_opengl3.cpp"),
    ]

    if perf:
        defines["PERF_ENABLED"] = 1

    if debug_allocations:
        defines["GARDEN_TRACK_ALLOCATIONS"] = 1

    #
    # Gameplay code:
    #
    msvc.compile(
        c, sources,
        output=join(output_dir(build_type), GAMEPLAY_DLL_NAME),
        includes=[*includes],
        defines=dict(
            **defines,
            GARDEN_GAMEPLAY_CODE=1,
        ),
        libs=["kernel32.lib", "user32.lib", "gdi32.lib", glm_library(build_type)],
        link_flags=[*link_flags],
        compile_flags=[*compile_flags],
        env=build_env,
        only_preprocessor=only_preprocessor, unicode_support=True, is_dll=True
    )


    #
    # Runtime code:
    #
    garden_output_exe = c.join(output_dir(build_type), "garden.exe")

    if not is_file_busy(garden_output_exe):
        c.echo("I: Compiling runtime code...")

        msvc.compile(
            c, sources,
            output=garden_output_exe,
            libs=libs,
            includes=includes, link_flags=link_flags, compile_flags=compile_flags,
            defines=dict(
                **defines,
                GARDEN_ASSET_FOLDER=c.quote(assets_dir),
                GARDEN_GAMEPLAY_CODE=0,
            ),
            env=build_env,
            only_preprocessor=only_preprocessor, unicode_support=True
        )
    else:
        c.echo("W: Skipping runtime code because {} is busy...".format(c.quote(garden_output_exe)))

    # link.exe topdown.obj /MACHINE:X64 /SUBSYSTEM:WINDOWS /Fe:topdown.exe
