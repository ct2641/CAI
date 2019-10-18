SET objdir=..\obj_windows

if not exist %objdir% mkdir %objdir%

cl /Fo%objdir%\PDparser.obj /c PDparser.cpp
cl /Fo%objdir%\parse_tester.obj /c parse_tester.cpp
cl /Fo%objdir%\utilities.obj /c utilities.cpp

link /out:pdparser.exe %objdir%\PDparser.obj %objdir%\parse_tester.obj %objdir%\utilities.obj
