SET objdir=..\obj_windows
SET gurobidir=C:\gurobi811\win64
SET gurobiincludedir=%gurobidir%\include
SET gurobilibdir=/libpath:%gurobidir%\lib
SET otherwindowslibdirs=/libpath:"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\amd64" /libpath:"C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x64" /libpath:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.10240.0\ucrt\x64"
SET lib=gurobi81.lib

if not exist %objdir% mkdir %objdir%

@REM g++ -o gurobicompute.exe -I C:\gurobi811\win64\include -L C:\gurobi811\win64\lib computes_Gurobi.cpp transcribe2Gurobi_tester.cpp transcribe2Gurobi.cpp utilities_Gurobi.cpp ..\ERfromPD.cpp ..\PDParser\utilities.cpp ..\PDParser\PDparser.cpp -l gurobi81

cl /I %gurobiincludedir% /Fo%objdir%\computes_Gurobi.obj /c computes_Gurobi.cpp
cl /I %gurobiincludedir% /Fo%objdir%\transcribe2Gurobi_tester.obj /c transcribe2Gurobi_tester.cpp
cl /I %gurobiincludedir% /Fo%objdir%\transcribe2Gurobi.obj /c transcribe2Gurobi.cpp
cl /I %gurobiincludedir% /Fo%objdir%\utilities_Gurobi.obj /c utilities_Gurobi.cpp
cl /I %gurobiincludedir% /Fo%objdir%\ERfromPD.obj /c ..\ERfromPD.cpp
cl /I %gurobiincludedir% /Fo%objdir%\utilities.obj /c ..\PDParser\utilities.cpp
cl /I %gurobiincludedir% /Fo%objdir%\PDparser.obj /c ..\PDParser\PDparser.cpp

link /out:gurobicompute.exe %objdir%\computes_Gurobi.obj %objdir%\transcribe2Gurobi_tester.obj %objdir%\transcribe2Gurobi.obj %objdir%\utilities_Gurobi.obj %objdir%\ERfromPD.obj %objdir%\utilities.obj %objdir%\PDparser.obj %gurobilibdir% %otherwindowslibdirs% %lib%
