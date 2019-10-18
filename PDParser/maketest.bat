CALL make.bat

REM Running unit tests.
REM
REM ~~~~Test 1/3: PD.txt~~~~
pdparser.exe stresstests/PD.txt
REM
REM ~~~~Test 2/3: PD-email.txt~~~~
pdparser.exe stresstests/PD-email.txt
REM
REM ~~~~Test 3/3: PD_RG4x3x3.txt~~~~
pdparser.exe stresstests/PD_RG4x3x3.txt
