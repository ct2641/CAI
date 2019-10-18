# An Open-Source Toolbox for Computer-Aided Investigation 
# on the Fundamental Limits of Information Systems, Version 0.1
# 
# Chao Tian, James S. Plank and Brent Hurst
# https://github.com/ct2641/CAI/releases/tag/0.1
# October, 2019
# 
CC = g++
LINKER = g++
LANGUAGE = cpp
CFLAGS = -Wall -Wextra
LFLAGS =

EXECUTABLE = pdparser.out
OBJDIR = ../obj_linux_osx
OBJECTS = $(OBJDIR)/PDparser.o      \
		  $(OBJDIR)/parse_tester.o  \
		  $(OBJDIR)/utilities.o
HEADERS = PDparser.h    \
		  problemdes.h  \
		  utilities.h

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LINKER) $(LFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.$(LANGUAGE) $(HEADERS) | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR):
	mkdir -p $(OBJDIR)


.PHONY: clean
clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

NUMTESTS = 3
TESTDIR = stresstests

.PHONY: test
test: $(EXECUTABLE)
	@echo "Running unit tests."
	@echo ""
	@echo "~~~~Test 1/$(NUMTESTS): PD.txt~~~~"
	$(EXECUTABLE) $(TESTDIR)/PD.txt
	@echo ""
	@echo "~~~~Test 2/$(NUMTESTS): PD-email.txt~~~~"
	$(EXECUTABLE) $(TESTDIR)/PD-email.txt
	@echo ""
	@echo "~~~~Test 3/$(NUMTESTS): PD_RG4x3x3.txt~~~~"
	$(EXECUTABLE) $(TESTDIR)/PD_RG4x3x3.txt
