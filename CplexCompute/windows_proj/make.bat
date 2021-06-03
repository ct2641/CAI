SET objdir=..\obj_windows
SET cplexsrcdir=..\src
SET cplexincdir=..\include
SET caitoolssrcdir= ..\..\CAI_Tools\src
SET caitoolsincdir= ..\..\CAI_Tools\include
SET cplexdir=C:\CPLEX_Studio129\cplex
SET cplexincludedir=%cplexdir%\include
SET cplexlibdir=/libpath:%cplexdir%\lib\x64_windows_vs2017\stat_mda
SET otherwindowslibdirs=/libpath:"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\amd64" /libpath:"C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x64" /libpath:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.10240.0\ucrt\x64"
SET lib=cplex1290.lib

if not exist %objdir% mkdir %objdir%

cl /EHcs /I %cplexincludedir% /I %caitoolsincdir% /I %cplexincdir% /Fo%objdir%\computes_Cplex.obj /c %cplexsrcdir%\computes_Cplex.cpp
cl /EHcs /I %cplexincludedir% /I %caitoolsincdir% /I %cplexincdir% /Fo%objdir%\computes_Cplex_tester.obj /c %cplexsrcdir%\computes_Cplex_tester.cpp
cl /EHcs /I %cplexincludedir% /I %caitoolsincdir% /I %cplexincdir% /Fo%objdir%\transcribe2Cplex.obj /c %cplexsrcdir%\transcribe2Cplex.cpp
cl /EHcs /I %cplexincludedir% /I %caitoolsincdir% /I %cplexincdir% /Fo%objdir%\utilities_Cplex.obj /c %cplexsrcdir%\utilities_Cplex.cpp

cl /EHcs /I %cplexincludedir% /I %caitoolsincdir% /I %cplexincdir% /Fo%objdir%\ERfromPD.obj /c %caitoolssrcdir%\ERfromPD.cpp
cl /EHcs /I %cplexincludedir% /I %caitoolsincdir% /I %cplexincdir% /Fo%objdir%\disjoint_set.obj /c %caitoolssrcdir%\disjoint_set.cpp
cl /EHcs /I %cplexincludedir% /I %caitoolsincdir% /I %cplexincdir% /Fo%objdir%\expression_helpers.obj /c %caitoolssrcdir%\expression_helpers.cpp
cl /EHcs /I %cplexincludedir% /I %caitoolsincdir% /I %cplexincdir% /Fo%objdir%\problem_description.obj /c %caitoolssrcdir%\problem_description.cpp

link /out:cplexcompute.exe %objdir%\computes_Cplex.obj %objdir%\computes_Cplex_tester.obj %objdir%\transcribe2Cplex.obj %objdir%\utilities_Cplex.obj %objdir%\ERfromPD.obj %objdir%\disjoint_set.obj %objdir%\expression_helpers.obj %objdir%\problem_description.obj %cplexlibdir% %otherwindowslibdirs% %lib%
