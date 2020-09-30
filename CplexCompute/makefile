# An Open-Source Toolbox for Computer-Aided Investigation 
# on the Fundamental Limits of Information Systems, Version 0.1
# 
# Chao Tian, James S. Plank and Brent Hurst
# https://github.com/ct2641/CAI/releases/tag/0.1
# October, 2019
# 





#### HERE
# You might need to change this line:
CPLEXDIR = /opt/ibm/ILOG/CPLEX_Studio129/cplex
# CPLEXDIR = /home/grads/r/ruida/ibm/ILOG/CPLEX_Studio1210/cplex






CC = g++
LINKER = g++
CAI_TOOLSDIR = ../CAI_Tools

INCLUDE_CAI_TOOLS = $(CAI_TOOLSDIR)/include
INCLUDE_CPLEX = $(CPLEXDIR)/include
INCLUDE_LOCAL = include

LIBDIR_CAI_TOOLS = $(CAI_TOOLSDIR)/lib
LIBDIR_CPLEX = $(CPLEXDIR)/lib/x86-64_linux/static_pic/

LIB_CAI_TOOLS = cai
LIB_CPLEX = cplex

ARCHIVE = $(LIBDIR_CAI_TOOLS)/lib$(LIB_CAI_TOOLS).a

CFLAGS = -std=c++11 -Wall -Wextra -I$(INCLUDE_CAI_TOOLS) -I$(INCLUDE_CPLEX) -I$(INCLUDE_LOCAL)
LFLAGS = -L$(LIBDIR_CAI_TOOLS) -L$(LIBDIR_CPLEX) -l$(LIB_CAI_TOOLS) -l$(LIB_CPLEX) -ldl -lpthread

EXECUTABLE = cplexcompute.out

OBJDIR = obj_linux_osx
SRCDIR = src

OBJECTS = $(OBJDIR)/transcribe2Cplex.o        \
	      $(OBJDIR)/utilities_Cplex.o         \
	      $(OBJDIR)/computes_Cplex_tester.o \
	      $(OBJDIR)/computes_Cplex.o


all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(ARCHIVE)
	$(LINKER) -o $@ $(OBJECTS) $(LFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean
clean:
	rm -rf $(EXECUTABLE) $(OBJECTS) $(OBJDIR)
