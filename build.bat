::
:: FILE          build.bat
::
:: AUTHORS
::               Ilya Akkuzin <gr3yknigh1@gmail.com>
::
:: NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
::

@echo off

set build_type=%1
shift

if [%build_type%]==[] set build_type=Debug

inv build --build-type %build_type%

:: set /p DUMMY=Hit ENTER to continue...
