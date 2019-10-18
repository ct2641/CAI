SET objdir=..\obj_windows

if not exist %objdir% mkdir %objdir%

del cplexcompute.exe %objdir%\computes_Cplex.obj %objdir%\transcribe2Cplex_tester.obj %objdir%\transcribe2Cplex.obj %objdir%\utilities_Cplex.obj %objdir%\ERfromPD.obj %objdir%\utilities.obj %objdir%\PDparser.obj
