@rem 
@rem  FILE          build.bat
@rem 
@rem  AUTHORS
@rem                Ilya Akkuzin <gr3yknigh1@gmail.com>
@rem 
@rem  NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
@rem 

@echo off

set project_folder=%~dp0

set build_type=%1
shift

if [%build_type%]==[] set build_type=Debug

htask -C %project_folder% build --build-type %build_type%

pause

