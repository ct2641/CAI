## Compilation Instructions for: "An Open-Source Toolbox for Computer-Aided Investigation on the Fundamental Limits of Information Systems, Version 0.1"

### Chao Tian, James S. Plank and Brent Hurst

For details on how to use this software toolkit,
please refer the included pdf documentation `CAI_doc.pdf`.
This README contains compilation instructions and some additional remarks.

This toolkit is available at [https://github.com/ct2641/CAI/releases/tag/0.1](https://github.com/ct2641/CAI/releases/tag/0.1).  To cite this toolkit, please [click here for citation information](Citation.md).

This work is supported in part by the National Science Foundation through grants CCF-18-16518 and CCF-18-16546.

### Instructions

Linux and OSX: makefiles are included in the package.

- For Gurobi and Cplex, you'll need to edit the makefiles to point to the correct path for `GUROBIDIR` and `CPLEXDIR`.

- To build, enter the directory of the executable you want to compile and type `make`.

  - To clean, type `make clean`.

  - To make sure PDParser/ compiled correctly, type `make test` in the `PDParser/` directory, and it will make and then run three tests that shouldn't have any errors.
    The output should be equal to the file `stresstests/test_output.txt`

  - The `.o` files will be stored in `cai/obj_linux_osx/`.

Windows (Command Line):

- Several *very* simple batch files `.bat` files are included.

  - Assume `cl.exe` and `link.exe` are installed and in the `%PATH%`.

  - For Gurobi and Cplex, you'll need to edit `make.bat` to point to the correct path for `gurobidir` and `cplexdir`,

- To build, enter the directory of the executable you want to compile and type `make` (or maybe `make.bat`)

  - To clean, type `makeclean` (or `makeclean.bat`).

  - To make sure `PDParser/` compiled correctly, type `maketest` (or `maketest.bat`) in the `PDParser/` directory, and it will make and then run three tests that shouldn't have any errors.
    The output should be equal to the file `stresstests/test_output.txt`

  - The `.obj` files will be stored in `cai/obj_windows/`.

Windows (Visual Studio):

- .`sln` and `.vcxproj` files are given for VS 2017

  - These are for x64 only. For win32 version, the configuration need to be adjusted. Paricularly for Gurobi and Cplex libraries, the x64 and win32 versions should not be mixed.

- For other VS versions, new project files can constructed as follows

  - Make an empty project file.

  - Follow the relevant files into the projects according to `make.bat`.

  - Add the Cplex or Gurobi include directory in C/C++ ->General tab.

  - Add the statis linked directory in Linker -> Input tab.

----------

Example problem description files:

- A few example problem description files are given which start with "PD".

- The `GeneratePD/` directory contains simple example matlab scripts to generate problem description files for the regenerating code problem.
