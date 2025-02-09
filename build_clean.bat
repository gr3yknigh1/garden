::
:: FILE          build_clean.bat
::
:: AUTHORS
::               Ilya Akkuzin <gr3yknigh1@gmail.com>
::
:: NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
::

@echo off

set project_folder=%~dp0

echo I: Cleaning...

del %project_folder%*.exe
del %project_folder%*.ilk
del %project_folder%*.obj
del %project_folder%*.pdb

rmdir /S /Q %project_folder%glm\build

echo I: Done!

set /p DUMMY=Hit ENTER to continue...