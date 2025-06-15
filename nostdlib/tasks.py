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
import os.path

from htask import define_task, Context
from htask import load_env, save_env
from htask.progs import msvc, cmake

from hbuild import compile_project

F = Callable

default_build_type = "Debug"

project_folder = dirname(realpath(__file__))
output_folder  = os.path.sep.join([project_folder, "build"])

configuration_folder: F[[str], str] = lambda build_type: os.path.sep.join([output_folder, build_type]) # noqa


@define_task(name="clean")
def clean_(c: Context, build_type=default_build_type):
    if c.exists(output_folder):
        c.run(f"rmdir /S /Q {output_folder}")


@define_task()
def build(c: Context, build_type=default_build_type, clean=False, reconfigure=False, cmake=False):
    if clean:
        clean_(c, build_type)

    if cmake:
        if not exists(configuration_folder(build_type)) or reconfigure:
            cmake.configure(c)
        cmake.build(c, configuration_name=build_type)

    else:
        compile_project(c, build_file=c.join(project_folder, "build.py"), build_type=build_type, prefix=output_folder)

