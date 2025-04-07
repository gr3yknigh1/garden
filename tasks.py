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
def build(c: Context, build_type=default_build_type, clean=False, reconfigure=False, only_preprocessor=False):
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

    GAMEPLAY_DLL_NAME = "garden_gameplay.dll"

    c.echo("I: Compiling game code...")

    common_compile_flags = ["/MTd", "/Zi", "/std:c++20", "/W4", "/Od", "/GR-"]
    common_link_flags = ["/DEBUG:FULL"]
    common_defines: dict[str, Any] = dict(
        GARDEN_GAMEPLAY_DLL_NAME=GAMEPLAY_DLL_NAME,
    )

    common_sources = [
        c.join(code_dir, "garden_memory.cpp"),
        c.join(code_dir, "garden_platform.cpp"),
    ]

    if build_type == "Debug":
        common_defines["GARDEN_BUILD_TYPE_DEBUG"] = 1

    if build_type == "Release":
        common_defines["GARDEN_BUILD_TYPE_RELEASE"] = 1

    msvc.compile(
        c, [ join(code_dir, "garden.cpp"), *common_sources ], # TODO(gr3yknigh1): Move garden_platform.cpp away in library [2025/03/03]
        output=join(output_dir(build_type), GAMEPLAY_DLL_NAME),
        includes=[
            join(project_dir, "glad"),
            glm_folder,
        ],
        defines=dict(
            **common_defines,
            GARDEN_GAMEPLAY_CODE=1,
        ),
        libs=[],
        link_flags=[*common_link_flags],
        compile_flags=[*common_compile_flags],
        env=build_env,
        only_preprocessor=only_preprocessor, unicode_support=True, is_dll=True
    )

    garden_output_exe = c.join(output_dir(build_type), "garden.exe")
    garden_platform_sources = [
        c.join(project_dir, "glad", "glad.c"),
        c.join(project_dir, "glad", "glad_wgl.c"),
        *common_sources
    ]

    if not is_file_busy(garden_output_exe):
        c.echo("I: Compiling platform runtime...")

        msvc.compile(
            c, garden_platform_sources,
            output=garden_output_exe,
            libs=["kernel32.lib", "user32.lib", "gdi32.lib", glm_library(build_type)],
            defines=dict(
                **common_defines,
                _CRT_SECURE_NO_WARNINGS=1,
                GARDEN_ASSET_FOLDER=c.quote(assets_dir),
                GARDEN_GAMEPLAY_CODE=0,
            ),
            includes=[
                c.join(project_dir, "glad"),
                glm_folder,
            ],
            link_flags=[*common_link_flags],
            compile_flags=[*common_compile_flags],
            env=build_env,
            only_preprocessor=only_preprocessor, unicode_support=True
        )
    else:
        c.echo("W: Skipping platform runtime because {} is busy...".format(garden_output_exe))

    # link.exe topdown.obj /MACHINE:X64 /SUBSYSTEM:WINDOWS /Fe:topdown.exe
