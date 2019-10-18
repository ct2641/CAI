SET objdir=..\obj_windows
SET cplexdir=C:\CPLEX_Studio129\cplex
SET cplexincludedir=%cplexdir%\include
SET cplexlibdir=/libpath:%cplexdir%\lib\x64_windows_vs2017\stat_mda
SET otherwindowslibdirs=/libpath:"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\amd64" /libpath:"C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x64" /libpath:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.10240.0\ucrt\x64"
SET lib=cplex1290.lib

if not exist %objdir% mkdir %objdir%

cl /I %cplexincludedir% /Fo%objdir%\computes_Cplex.obj /c computes_Cplex.cpp
cl /I %cplexincludedir% /Fo%objdir%\transcribe2Cplex_tester.obj /c transcribe2Cplex_tester.cpp
cl /I %cplexincludedir% /Fo%objdir%\transcribe2Cplex.obj /c transcribe2Cplex.cpp
cl /I %cplexincludedir% /Fo%objdir%\utilities_Cplex.obj /c utilities_Cplex.cpp
cl /I %cplexincludedir% /Fo%objdir%\ERfromPD.obj /c ..\ERfromPD.cpp
cl /I %cplexincludedir% /Fo%objdir%\utilities.obj /c ..\PDParser\utilities.cpp
cl /I %cplexincludedir% /Fo%objdir%\PDParser.obj /c ..\PDParser\PDparser.cpp

link /out:cplexcompute.exe %objdir%\computes_Cplex.obj %objdir%\transcribe2Cplex_tester.obj %objdir%\transcribe2Cplex.obj %objdir%\utilities_Cplex.obj %objdir%\ERfromPD.obj %objdir%\utilities.obj %objdir%\PDparser.obj %cplexlibdir% %otherwindowslibdirs% %lib%
