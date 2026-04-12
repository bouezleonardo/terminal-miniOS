include sources.mk

# Replace the .c with .o (for compiling all object files)
OBJS = $(SOURCES:.c=.o)

# Target final executable
TARGET = terminal

# Generate preprocessed files
%.i : %.c
	gcc -E $(HEADERS) $< -o $@
	
# Generate assembly files
%.s : %.c
	gcc -S $(HEADERS) $< -o $@

# Generate object files (not linking)
%.o : %.c
	gcc -c $(HEADERS) $< -o $@

# Compile all (not linking)
.PHONY: compile-all
compile-all: $(OBJS)

# Link everything and show sizes of memory segments
$(TARGET).out: $(OBJS)
	$(CC) $(OBJS) -o $@

# Build into a final executable
.PHONY: build
build: $(TARGET).out

# Cleaning all generated files
.PHONY: clean
clean:
	rm -f *.i *.s *.o


