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

from typing import Callable, Optional, Any, Dict, List

from os import environ
from os.path import exists, join, dirname, realpath
import subprocess

from invoke import task, Collection

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

def parse_env_file(file: str) -> Dict[str, Any]:
    with open(file, mode="r") as f:
        s = f.read()

        env = {
            item[0] : item[1]
            for item in (
                l.split("=") for l in s.splitlines()
            ) if len(item) == 2
        }
    return env

def store_env_to_file(file: str, env: Dict[str, Any]) -> None:
    with open(file, mode="w") as f:
        for k, v in env.items():
            f.write(f"{k}={v}\n")

def find_vc_bootstrap_script() -> Optional[str]:
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

def extract_environment_from_bootstrap_script(arch="x64", bootstrap_script: Optional[str] = None, environment_vars: Optional[List[str]]=None) -> Dict[str, Any]:
    if bootstrap_script is None:
        bootstrap_script = find_vc_bootstrap_script()

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

@task(default=True)
def build(c, build_type=default_build_type, clean=False, reconfigure=False):
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

    flags = "/MTd /Zi /DEBUG:FULL /std:c++20 /W4 /Od /GR- /Oi"

    defs = " ".join([
        "/D_CRT_SECURE_NO_WARNINGS",
        "/DUNICODE=1",
        "/D_UNICODE=1",
        f"/D GARDEN_ASSET_DIR={assets_dir!s}",
    ])

    libs = " ".join([
        "kernel32.lib",
        "user32.lib",
        "gdi32.lib",
        glm_library(build_type),
    ])

    sources = " ".join([
        join(code_dir, "garden.cpp"),
        join(project_dir, "glad", "glad.c"),
        join(project_dir, "glad", "glad_wgl.c"),
    ])

    includes = " ".join([
        "/I {}".format(join(project_dir, "glad")),
        "/I {}".format(glm_folder),
    ])

    output = join(output_dir(build_type), "garden.exe")

    c.run(f"cl.exe {flags} {defs} {sources} /Fe:{output} {includes} {libs}", env=build_env)
    # link.exe topdown.obj /MACHINE:X64 /SUBSYSTEM:WINDOWS /Fe:topdown.exe
