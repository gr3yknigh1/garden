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
#    * Install `pyinvoke`:
#
#        > python3 -m pip install invoke
#
#    * Call build task:
#
#        > inv build --build-type Debug
#
#    * Done!
#

from typing import Callable, Any, TYPE_CHECKING

from os import environ
from os.path import exists, join, dirname, realpath
import subprocess
import time

from invoke import task, Collection, Context

F = Callable

default_build_type = "Debug"

project_dir: str = dirname(realpath(__file__))
assets_dir: str = join(project_dir, "assets")
code_dir: str = join(project_dir, "code")
output_dir: F[[str], str] = lambda build_type: join(project_dir, "build", build_type)

#
# VS utilities
#

DEFAULT_VC_BOOSTRAP_VARS = ["INCLUDE", "LIB", "LIBPATH", "PATH"]

def parse_env_file(file: str) -> dict[str, Any]:
    with open(file, mode="r") as f:
        s = f.read()

        env = {
            item[0] : item[1]
            for item in (
                l.split("=") for l in s.splitlines()
            ) if len(item) == 2
        }
    return env

def store_env_to_file(file: str, env: dict[str, Any]) -> None:
    with open(file, mode="w") as f:
        for k, v in env.items():
            f.write(f"{k}={v}\n")

def find_vc_bootstrap_script() -> str | None:
    #
    # Detect vcvarsall for x64 build...
    #

    default_vc2022_bootstrap = r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    default_vc2019_bootstrap = r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Auxiliary\Build\vcvarsall.bat"

    if exists(default_vc2022_bootstrap):
        return default_vc2022_bootstrap

    if exists(default_vc2019_bootstrap):
        return default_vc2019_bootstrap

    return None

def extract_environment_from_bootstrap_script(arch="x64", bootstrap_script: str | None = None, environment_vars: list[str] | None=None) -> dict[str, Any]:
    if bootstrap_script is None:
        bootstrap_script = find_vc_bootstrap_script()

        if bootstrap_script is None:
            return {}

    if environment_vars is None:
        environment_vars = DEFAULT_VC_BOOSTRAP_VARS

    process = subprocess.Popen([bootstrap_script, arch, "&&", "set"], stdout=subprocess.PIPE)
    output = process.stdout.read().decode("utf-8")

    # NOTE(gr3yknigh1): Ha-ha, nasty, but it is on purpose! [2025/02/03]
    env = {
        item[0].upper() : item[1]
        for item in (
            l.split("=") for l in output.splitlines()
        ) if len(item) == 2 and item[0].upper() in environment_vars
    }

    return env

#
# Dependencies:
#

glm_folder = join(project_dir, "glm")
glm_configuration = join(glm_folder, "build")
glm_output_dir: F[[str], str] = lambda build_type: join(glm_configuration, "glm", build_type)
glm_library: F[[str], str] = lambda build_type: join(glm_output_dir(build_type), "glm.lib")

#
# Tasks:
#

def quote_path(s: str) -> str:
    s = repr(s)
    s = s.replace("'", '"')
    return s


@task(default=True)
def build(c, build_type=default_build_type, clean=False, reconfigure=False, only_preprocessor=False, enable_crt_alloc=False):
    """Builds entire project.
    """
    if clean:

        if exists(glm_output_dir(build_type)):
            c.run(f"rmdir /S /Q {glm_output_dir(build_type)}")

        if exists(output_dir(build_type)):
            c.run(f"rmdir /S /Q {output_dir(build_type)}")

    #
    # GLM:
    #

    if not clean and exists(glm_library(build_type)):
        print("I: GLM already built")
    else:
        print("I: Building GLM...")

        c.run(f"cmake -B {glm_configuration} -S {glm_folder} -D GLM_BUILD_TESTS=OFF -D BUILD_SHARED_LIBS=OFF")
        c.run(f"cmake --build {glm_configuration} --config {build_type}")

    #
    # Garden:
    #

    if not exists(output_dir(build_type)):
        c.run(f"mkdir {output_dir(build_type)}")

    cached_env = join(output_dir(build_type), "vc_build.env")

    if not reconfigure and exists(cached_env):
        build_env = parse_env_file(cached_env)
    else:
        build_env = extract_environment_from_bootstrap_script()
        store_env_to_file(cached_env, build_env)


    # TODO(i.akkuzin): Make release configuration build. Currently we using `/Od` flag, which disables any optimizations and link with debug runtime (/MTd). [2025/02/08]
    # TODO(i.akkuzin): Remove _CRT_SECURE_NO_WARNINGS [2025/02/08]

    GAMEPLAY_DLL_NAME = "garden_gameplay.dll"

    c.run("echo I: Compiling game code...")

    if False:
        msvc_conf = MsvcConf(
            runtime=MsvcRuntime.StaticDebug,
            debug=MsvcDebugOpt.Full,
        )

    common_compile_flags = ["/MTd", "/Zi", "/std:c++20", "/W4", "/Od", "/GR-"]
    common_link_flags = ["/DEBUG:FULL"]
    common_defines = dict(
        GARDEN_GAMEPLAY_DLL_NAME=GAMEPLAY_DLL_NAME,
    )

    if enable_crt_alloc:
        common_defines.update(dict(GARDEN_USE_CRT_ALLOCATIONS=1))

    msvc_compile(
        c, [ join(code_dir, "garden.cpp"), join(code_dir, "garden_platform.cpp") ], # TODO(gr3yknigh1): Move garden_platform.cpp away in library [2025/03/03]
        output=join(output_dir(build_type), GAMEPLAY_DLL_NAME),
        includes=[
            join(project_dir, "glad")
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

    garden_output_exe = join(output_dir(build_type), "garden.exe")
    garden_platform_sources = [
        join(code_dir, "garden_platform.cpp"),
        join(project_dir, "glad", "glad.c"),
        join(project_dir, "glad", "glad_wgl.c")
    ]

    if not is_file_busy(garden_output_exe):
        c.run("echo I: Compiling platform runtime...")

        msvc_compile(
            c, garden_platform_sources,
            output=garden_output_exe,
            libs=["kernel32.lib", "user32.lib", "gdi32.lib", glm_library(build_type)],
            defines=dict(
                **common_defines,
                _CRT_SECURE_NO_WARNINGS=1,
                GARDEN_ASSET_FOLDER=quote_path(assets_dir),
                GARDEN_GAMEPLAY_CODE=0,
            ),
            includes=[
                join(project_dir, "glad"),
                glm_folder,
            ],
            link_flags=[*common_link_flags],
            compile_flags=[*common_compile_flags],
            env=build_env,
            only_preprocessor=only_preprocessor, unicode_support=True
        )
    else:
        c.run("echo W: Skipping platform runtime because {} is busy...".format(garden_output_exe))

    # link.exe topdown.obj /MACHINE:X64 /SUBSYSTEM:WINDOWS /Fe:topdown.exe

#
# MSVC command line helpers:
#

def msvc_format_defines(defines: dict[str, Any]) -> str:

    result = " ".join(
        [f"/D {k}={v}" for k, v in defines.items() ]
    )

    return result

def msvc_format_includes(includes: list[str]) -> str:

    result = " ".join(
        [f"/I {include}" for include in includes]
    )

    return result

def msvc_compile(
    c: Context,
    sources: list[str],
    *,
    output: str,
    libs: list[str] | None=None,
    defines: dict[str, Any] | None=None,
    includes: list[str] | None=None,
    compile_flags: list[str] | None=None,
    link_flags: list[str] | None=None,
    env: dict[str, Any] | None=None,
    only_preprocessor=False,
    unicode_support=False,
    is_dll=False,
    **kw
):
    if env is None:
        env = {}

    if libs is None:
        libs = []

    if compile_flags is None:
        compile_flags = []

    if link_flags is None:
        link_flags = []

    if defines is None:
        defines = {}

    if includes is None:
        includes = []

    if unicode_support:
        defines.update(dict(
            UNICODE=1,
            _UNICODE=1,
        ))

    if only_preprocessor:
        compile_flags.append("/P")

    if is_dll:
        link_flags.append("/DLL")

    compile_flags_formatted = " ".join(compile_flags)
    defines_formatted = msvc_format_defines(defines)
    includes_formatted = msvc_format_includes(includes)
    libs_formatted = " ".join(libs)
    sources_formatted = " ".join(sources)
    link_flags_formatted = "/link {}".format(" ".join(link_flags))
    output_formatted = f"/Fe:{output}"

    options = " ".join([
        compile_flags_formatted, defines_formatted, sources_formatted, output_formatted, includes_formatted, libs_formatted, link_flags_formatted
    ])

    return c.run(
        f"cl.exe {options}",
        env=env, **kw
    )


def is_file_busy(file: str, mode: str="w") -> bool:
    try:
        with open(file, mode=mode):
            return False
    except IOError:
        return True
