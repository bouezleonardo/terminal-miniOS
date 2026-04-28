include sources.mk

# Replace the .c with .o (for compiling all object files)
OBJS = $(SOURCES:.c=.o)

# Target final executable
TARGET = terminal

# C code flags
CFLAGS = -Wall #-Werror

# Linker flags
LDFLAGS = -lm -lncurses

# Generate preprocessed files
%.i : %.c
	gcc -E $(CFLAGS) $(HEADERS) $< -o $@ $(LDFLAGS)
	
# Generate assembly files
%.s : %.c
	gcc -S $(CFLAGS) $(HEADERS) $< -o $@ $(LDFLAGS)

# Generate object files (not linking)
%.o : %.c
	gcc -c $(CFLAGS) $(HEADERS) $< -o $@ $(LDFLAGS)

# Compile all (not linking)
.PHONY: compile-all
compile-all: $(OBJS)

# Link everything
$(TARGET).out: $(OBJS)
	gcc $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Build into a final executable
.PHONY: build
build: $(TARGET).out

# Cleaning all generated files
.PHONY: clean
clean:
	rm -f src/*.i src/*.s src/*.o *.out


