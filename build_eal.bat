@echo off
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86
) else (
echo No compiler found: please set one!
)
set compilerflags=/Od /Zi /EHsc /std:c++latest /I include
set linkerflags=/OUT:bin\eal.dll /DLL /DYNAMICBASE "advapi32.lib" "Shell32.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "version.lib"
cl.exe %compilerflags% src\eal\*.cpp /link %linkerflags%
del bin\*.ilk *.obj *.pdb