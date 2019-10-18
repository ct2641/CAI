SET objdir=..\obj_windows

if not exist %objdir% mkdir %objdir%

del pdparser.exe %objdir%\PDparser.obj %objdir%\parse_tester.obj %objdir%\utilities.obj
