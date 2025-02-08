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

del topdown.exe
del topdown.ilk
del topdown.obj
del topdown.pdb

echo I: Done!
