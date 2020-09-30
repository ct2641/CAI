# An Open-Source Toolbox for Computer-Aided Investigation 
# on the Fundamental Limits of Information Systems, Version 0.1
# 
# Chao Tian, James S. Plank and Brent Hurst
# https://github.com/ct2641/CAI/releases/tag/0.1
# October, 2019
# 





#### HERE
# If you don't have GUROBI_HOME already set,
# you'll need to change this line:
GUROBI_HOME ?= $(HOME)/gurobi811/mac46
# GUROBI_HOME = /home/grads/r/ruida/gurobi811/linux64





CC = g++
LINKER = g++
GUROBIDIR = $(GUROBI_HOME)
CAI_TOOLSDIR = ../CAI_Tools

INCLUDE_CAI_TOOLS = $(CAI_TOOLSDIR)/include
INCLUDE_GUROBI = $(GUROBIDIR)/include
INCLUDE_LOCAL = include

LIBDIR_CAI_TOOLS = $(CAI_TOOLSDIR)/lib
LIBDIR_GUROBI = $(GUROBIDIR)/lib

LIB_CAI_TOOLS = cai
LIB_GUROBI = gurobi81

ARCHIVE = $(LIBDIR_CAI_TOOLS)/lib$(LIB_CAI_TOOLS).a

CFLAGS = -Wall -Wextra -std=c++11 -I$(INCLUDE_CAI_TOOLS) -I$(INCLUDE_GUROBI) -I$(INCLUDE_LOCAL)
LFLAGS = -L$(LIBDIR_CAI_TOOLS) -L$(LIBDIR_GUROBI) -l$(LIB_CAI_TOOLS) -l$(LIB_GUROBI)

EXECUTABLE = gurobicompute.out

OBJDIR = obj_linux_osx
SRCDIR = src

OBJECTS = $(OBJDIR)/transcribe2Gurobi.o        \
	      $(OBJDIR)/utilities_Gurobi.o         \
	      $(OBJDIR)/computes_Gurobi_tester.o \
	      $(OBJDIR)/computes_Gurobi.o


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
