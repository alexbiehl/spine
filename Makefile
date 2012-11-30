# Sources
SRCS             = main.c loop.c net.c poll.c stream.c array.c

# Target
TARGET           = spine

# Objektdateien
OBJS             = $(SRCS:.c=.o)

# Debugging-Informationen aktivieren
DEBUG = yes

# Compiler
CC               = gcc

# Precompiler flags
CPPFLAGS_LINUX   = -DLINUX
CPPFLAGS_LINUX64 = -DLINUX
CPPFLAGS_MACOSX  = -DMACOSX
CPPFLAGS_COMMON  = -I./
CPPFLAGS         = $(CPPFLAGS_COMMON) $(CPPFLAGS_$(OS))

# Compiler flags
CFLAGS_LINUX   = 
CFLAGS_LINUX64 = 
CFLAGS_MACOSX  = 
CFLAGS_COMMON  = -Os -pedantic -Wall -Wextra -Werror -std=c99
CFLAGS         = $(CFLAGS_COMMON) $(CFLAGS_$(OS))

# Linker
LD               = gcc

# Linker flags
LDFLAGS_LINUX    = 
LDFLAGS_LINUX64  = 
LDFLAGS_MACOSOX  =
LDFLAGS_COMMON   =
LDFLAGS          = $(LDFLAGS_COMMON) $(LDFLAGS_$(OS))


LDLIBS_LINUX     = 
LDLIBS_LINUX64   = 
LDLIBS_MACOSX    =
LDLIBS_COMMON    = -lm
LDLIBS           = $(LDLIBS_COMMON) $(LDLIBS_$(OS))

# welches Betriebssystem?
ifeq ($(findstring Linux,$(shell uname -s)),Linux)
  OS = LINUX
  ifeq ($(shell uname -m),x86_64)
    OS = LINUX64
  endif
else
  ifeq ($(shell uname -s),Darwin)
    OS = MACOSX
  endif
endif



# Wenn Debugging-Informationen aktiviert werden sollen, entsprechende
# Praeprozessorflags setzen
ifeq ($(DEBUG),yes)
CPPFLAGS_COMMON+=-g -DDEBUG
endif


.SUFFIXES: .o .c
.PHONY: all clean distclean depend


# TARGETS
all: depend $(TARGET)

doc:
	doxygen ../common/Doxyfile

# Linken des ausfuehrbaren Programms
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(TARGET)

# Kompilieren der Objektdateien
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $*.o $*.c

# einfaches Aufraeumen
clean:
	rm -f $(TARGET)
	rm -f $(OBJS)

# alles loeschen, was erstellt wurde
distclean: clean
	rm -f *~
	rm -f Makefile.depend
	rm -rf doc
	rm -rf doxygen.log

# Abhaengigkeiten automatisch ermitteln
depend:
	@rm -f Makefile.depend
	@echo -n "building Makefile.depend ... "
	@$(foreach SRC, $(SRCS), ( $(CC) $(CPPFLAGS) $(SRC) -MM -g0 ) 1>> Makefile.depend;)
	@echo "done"

# zusaetzliche Abhaengigkeiten einbinden
-include Makefile.depend

