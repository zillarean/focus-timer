@REM Build with clang-cl

@set OUT_DIR=Build
@set OUT_EXE=focus-timer


@set INCLUDES=/Ithirdparty\imGui /Ithirdparty\imGui\backends /Isrc /I"%WindowsSdkDir%Include\um" /I"%WindowsSdkDir%Include\shared" /I"%DXSDK_DIR%Include"

@set SOURCES=src\main.cpp src\timer.c thirdparty\imGui\backends\imgui_impl_dx11.cpp thirdparty\imGui\backends\imgui_impl_win32.cpp thirdparty\imGui\imgui*.cpp

rem @set LIBS=/link /LIBPATH:"%DXSDK_DIR%\Lib\x86" d3d11.lib d3dcompiler.lib
@@set LIBS=/link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /LIBPATH:"\Lib\x86" d3d11.lib d3dcompiler.lib

mkdir %OUT_DIR%

clang-cl /nologo /MT /utf-8 %INCLUDES% /D UNICODE /D _UNICODE %SOURCES% /Fe%OUT_DIR%\%OUT_EXE%.exe /Fo%OUT_DIR%\ %LIBS%
