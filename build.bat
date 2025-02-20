::
:: FILE          build.bat
::
:: AUTHORS
::               Ilya Akkuzin <gr3yknigh1@gmail.com>
::
:: NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
::

@echo off


set project_folder=%~dp0
set configuration_folder=%project_folder%\build

set build_type=%1
shift

if [%build_type%]==[] set build_type=Debug

:: Detect vcvarsall for x64 build...
set vc2022_bootstrap="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
set vc2019_bootstrap="C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Auxiliary\Build\vcvarsall.bat"

echo I: Configuring...

if exist %vc2022_bootstrap% (
  echo I: Found VC 2022 bootstrap script!
  call %vc2022_bootstrap% amd64
) else (
  if exist %vc2019_bootstrap% (
    echo I: No script for VC 2022, but found VC 2019 bootstrap script!
    call %vc2019_bootstrap% amd64
  ) else (
    echo W: Failed to find nor VC 2019, nor VC 2022 bootstrap scripts!
  )
)

echo I: Compiling...

pushd %project_folder%

:: TODO(i.akkuzin): Make release configuration build. Currently we using `/Od` flag, which disables any optimizations and link with debug runtime (/MTd). [2025/02/08]
:: TODO(i.akkuzin): Remove _CRT_SECURE_NO_WARNINGS [2025/02/08]

set glm_folder=%project_folder%glm
set glm_configuration=%glm_folder%\build
set glm_lib=%glm_configuration%\glm\%build_type%\glm.lib

if exist %glm_lib% (
  echo I: GLM already compiled!
) else (
  cmake -B %glm_configuration% -S %glm_folder% -D GLM_BUILD_TESTS=OFF -D BUILD_SHARED_LIBS=OFF
  cmake --build %glm_configuration% --config %build_type%
)

cl.exe ^
 /MTd /D_CRT_SECURE_NO_WARNINGS /DUNICODE=1 /D_UNICODE=1 /Zi /DEBUG:FULL /std:c++20 /W4 /Od /GR- /Oi ^
 /Fe:topdown.exe topdown.cpp glad\glad.c glad\glad_wgl.c ^
 /I%project_folder%glad /I%project_folder%glm ^
 kernel32.lib user32.lib gdi32.lib ^
 %glm_lib%

:: link.exe topdown.obj /MACHINE:X64 /SUBSYSTEM:WINDOWS /Fe:topdown.exe

popd

echo I: Done!

:: set /p DUMMY=Hit ENTER to continue...
