INCLUDEDIR = include
SRCDIR = src
OBJDIR = obj_linux_osx
LIBDIR = lib
BINDIR = bin
LIB = cai

CC = g++
LINKER = g++
ARCHIVER = ar
ARCHIVERFLAGS = -crsv
LANGUAGE = cpp
CFLAGS = -std=c++11 -Wall -Wextra -I$(INCLUDEDIR) -O3

EXECUTABLE = $(BINDIR)/parser.out
ARCHIVE = $(LIBDIR)/lib$(LIB).a

EXE_OBJECTS = $(OBJDIR)/test_parser.o
ARCHIVE_OBJECTS = $(OBJDIR)/ERfromPD.o \
                  $(OBJDIR)/problem_description.o \
                  $(OBJDIR)/expression_helpers.o \
                  $(OBJDIR)/disjoint_set.o \
                  

EXE_EHT = $(BINDIR)/expression_helpers_tester.out
EHT_OBJECTS = $(OBJDIR)/expression_helpers_tester.o \
			  $(OBJDIR)/expression_helpers.o

HEADERS = $(INCLUDEDIR)/nlohmann/json.hpp  \
		  $(INCLUDEDIR)/problem_description.hpp \
		  $(INCLUDEDIR)/ERfromPD.h \
		  $(INCLUDEDIR)/expression_helpers.hpp

all: $(EXECUTABLE) $(ARCHIVE) $(EXE_EHT)

$(EXECUTABLE): $(ARCHIVE) $(EXE_OBJECTS) | $(BINDIR)
	$(LINKER) -o $@ $(EXE_OBJECTS) -L$(LIBDIR) -l$(LIB)

$(ARCHIVE): $(ARCHIVE_OBJECTS) | $(LIBDIR)
	$(ARCHIVER) $(ARCHIVERFLAGS) $@ $(ARCHIVE_OBJECTS)

$(EXE_EHT): $(EHT_OBJECTS) | $(BINDIR)
	$(LINKER) -o $@ $(EHT_OBJECTS) -L $(LIBDIR) -l$(LIB)

$(OBJDIR)/%.o: $(SRCDIR)/%.$(LANGUAGE) $(HEADERS) | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)


.PHONY: clean
clean:
	rm -rf $(EXECUTABLE) $(EXE_EHT) $(EXE_OBJECTS) $(ARCHIVE_OBJECTS) $(EHT_OBJECTS) $(OBJDIR) $(ARCHIVE) $(LIBDIR)

.PHONY: cleanexe
cleanexe:
	rm -rf $(EXECUTABLE) $(EXE_OBJECTS) $(EXE_EHT)

.PHONY: cleanarchive
cleanarchive:
	rm -rf $(ARCHIVE) $(LIBDIR) $(ARCHIVE_OBJECTS)

.PHONY: cleanobjs
cleanobjs:
	rm -rf $(ARCHIVE_OBJECTS) $(EXE_OBJECTS) $(EHT_OBJECTS) $(OBJDIR)
