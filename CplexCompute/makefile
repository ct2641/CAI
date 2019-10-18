# An Open-Source Toolbox for Computer-Aided Investigation 
# on the Fundamental Limits of Information Systems, Version 0.1
# 
# Chao Tian, James S. Plank and Brent Hurst
# https://github.com/ct2641/CAI/releases/tag/0.1
# October, 2019
# 
CC = g++
LINKER = g++
CPLEXDIR = /opt/ibm/ILOG/CPLEX_Studio129/cplex
INCLUDE = $(CPLEXDIR)/include
LIBDIR = $(CPLEXDIR)/lib/x86-64_linux/static_pic/
CFLAGS = -Wall -Wextra -I $(INCLUDE)
LIB = cplex
LFLAGS = -L $(LIBDIR) -l$(LIB) -ldl -lpthread
EXECUTABLE = cplexcompute.out
OBJDIR = ../obj_linux_osx

LANGUAGE = cpp

ERDIR = ..
PARSERDIR = ../PDParser

SRC = computes_Cplex.$(LANGUAGE)           \
	  transcribe2Cplex_tester.$(LANGUAGE)  \
	  transcribe2Cplex.$(LANGUAGE)         \
	  utilities_Cplex.$(LANGUAGE)          \
	  $(ERDIR)/ERfromPD.$(LANGUAGE)        \
	  $(PARSERDIR)/utilities.$(LANGUAGE)   \
	  $(PARSERDIR)/PDparser.$(LANGUAGE)

OBJECTS = $(patsubst %.$(LANGUAGE),$(OBJDIR)/%.o,$(notdir $(SRC)))


define COMPILE_OBJECT
$(OBJDIR)/$(basename $(notdir $(1))).o : $(1) | $(OBJDIR)
	$(CC) $(CFLAGS) -o $(OBJDIR)/$(basename $(notdir $(1))).o -c $(1)
endef


all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $@ $^ $(LFLAGS)

$(foreach src, $(SRC), $(eval $(call COMPILE_OBJECT,$(src))))

$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean
clean:
	rm -f $(EXECUTABLE) $(OBJECTS) 
