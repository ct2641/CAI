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
GUROBIDIR = /home/brent/gurobi811/linux64
INCLUDE = $(GUROBIDIR)/include
LIBDIR = $(GUROBIDIR)/lib
LIB = gurobi81
CFLAGS = -Wall -Wextra -I $(INCLUDE)
LFLAGS = -L $(LIBDIR) -l $(LIB)

ERDIR = ..
PARSERDIR = ../PDParser
EXECUTABLE = gurobicompute.out

SRC = transcribe2Gurobi.$(LANGUAGE)        \
	  utilities_Gurobi.$(LANGUAGE)         \
	  transcribe2Gurobi_tester.$(LANGUAGE) \
	  computes_Gurobi.$(LANGUAGE)          \
	  $(ERDIR)/ERfromPD.$(LANGUAGE)        \
	  $(PARSERDIR)/utilities.$(LANGUAGE)   \
	  $(PARSERDIR)/PDparser.$(LANGUAGE)

OBJDIR = ../obj_linux_osx
OBJECTS = $(patsubst %.$(LANGUAGE),$(OBJDIR)/%.o,$(notdir $(SRC)))

define COMPILE_OBJECT
$(OBJDIR)/$(basename $(notdir $(1))).o : $(1) | $(OBJDIR)
	$(CC) $(CFLAGS) -o $(OBJDIR)/$(basename $(notdir $(1))).o -c $(1)
endef

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LINKER) -o $@ $^ $(LFLAGS)

$(foreach src, $(SRC), $(eval $(call COMPILE_OBJECT,$(src))))

$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean
clean:
	rm -f $(EXECUTABLE) $(OBJECTS) 
