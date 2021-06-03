SET objdir=..\obj_windows

if not exist %objdir% mkdir %objdir%

del gurobicompute.exe %objdir%\computes_Gurobi.obj %objdir%\computes_Gurobi_tester.obj %objdir%\transcribe2Gurobi.obj %objdir%\utilities_Gurobi.obj %objdir%\ERfromPD.obj %objdir%\utilities.obj %objdir%\PDparser.obj
