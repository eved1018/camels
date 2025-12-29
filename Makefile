# Define the executable name
EXECUTABLE = $(BINDIR)/camels

# Define the C compiler and flags
CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -std=c11 -Wfloat-equal -Wswitch-default \
          -Wswitch-enum -Wunreachable-code -Wconversion -Wshadow -MMD -MP

# Linker flags
LDFLAGS = 
LDLIBS = -lm

# Define directories for source, object files, and binaries
SRCDIR = src
OBJDIR_MAIN = build/main
OBJDIR_TEST = build/test
BINDIR = bin
HDRDIR = include
TSTDIR = test
TSTOBJDIR = $(TSTDIR)/build
TSTBINDIR = build/test

# Add include directory to CFLAGS
CFLAGS += -I$(HDRDIR) -I$(SRCDIR)

# Find all .c source files and .h header files
SOURCES := $(wildcard $(SRCDIR)/*.c)
HEADERS := $(wildcard $(HDRDIR)/*.h)

# Find all test .c files
TEST_SOURCES := $(wildcard $(TSTDIR)/*.c)

# Generate object file names for main build (in build/main/)
OBJECTS_MAIN := $(patsubst $(SRCDIR)/%.c,$(OBJDIR_MAIN)/%.o,$(SOURCES))

# Generate object file names for test build (in build/test/)
# These are compiled with -DTEST_BUILD flag
OBJECTS_TEST := $(patsubst $(SRCDIR)/%.c,$(OBJDIR_TEST)/%.o,$(SOURCES))

# Generate test object files and executables
TEST_OBJECTS := $(patsubst $(TSTDIR)/%.c,$(TSTOBJDIR)/%.o,$(TEST_SOURCES))
TEST_EXECUTABLES := $(patsubst $(TSTDIR)/%.c,$(TSTBINDIR)/%,$(TEST_SOURCES))

# Include generated dependency files
-include $(OBJECTS_MAIN:.o=.d)
-include $(OBJECTS_TEST:.o=.d)
-include $(TEST_OBJECTS:.o=.d)

# Prevent Make from deleting intermediate object files
.PRECIOUS: $(OBJECTS_MAIN) $(OBJECTS_TEST) $(TEST_OBJECTS)

# Default target: build the executable
default: makedir build

# Build everything and run tests
.PHONY: all
all: makedir build test

# Build the executable
.PHONY: build
build: $(EXECUTABLE)

# Rules to create directories if they don't exist
$(BINDIR):
	@mkdir -p $(BINDIR)

$(OBJDIR_MAIN):
	@mkdir -p $(OBJDIR_MAIN)

$(OBJDIR_TEST):
	@mkdir -p $(OBJDIR_TEST)

$(TSTOBJDIR):
	@mkdir -p $(TSTOBJDIR)

# Rule to create all directories (for manual use)
.PHONY: makedir
makedir:
	@mkdir -p $(BINDIR)
	@mkdir -p $(OBJDIR_MAIN)
	@mkdir -p $(OBJDIR_TEST)
	@mkdir -p $(TSTOBJDIR)

# Rule to link object files into the main executable
$(EXECUTABLE): $(OBJECTS_MAIN) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Rule to compile .c files into .o files for MAIN build (build/main/)
$(OBJDIR_MAIN)/%.o: $(SRCDIR)/%.c | $(OBJDIR_MAIN)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile .c files into .o files for TEST build (build/test/)
# Compiles with -DTEST_BUILD flag to conditionally exclude main()
$(OBJDIR_TEST)/%.o: $(SRCDIR)/%.c | $(OBJDIR_TEST)
	$(CC) $(CFLAGS) -DTEST_BUILD -c $< -o $@

# Rule to compile test .c files into .o files in test/build directory
$(TSTOBJDIR)/%.o: $(TSTDIR)/%.c | $(TSTOBJDIR)
	$(CC) $(CFLAGS) -I$(TSTDIR) -c $< -o $@

# Rule to link test executables in build/test directory
# Test files include all source files, so only link the test object
$(TSTBINDIR)/%: $(TSTOBJDIR)/%.o | $(TSTBINDIR)
	$(CC) $(LDFLAGS) $< -o $@ $(LDLIBS)

# Clean up generated files and directories
.PHONY: clean
clean:
	rm -rf build $(BINDIR)
	rm -rf $(TSTOBJDIR)

# Run the executable
.PHONY: run
run: build
	./$(EXECUTABLE) $(ARGS)

# Build and run tests
.PHONY: test
test: $(TEST_EXECUTABLES)
	@$(foreach test_bin,$(TEST_EXECUTABLES),$(test_bin) || exit 1;)

# Display help information
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  default  - Build the main executable (same as 'build')"
	@echo "  all      - Build executable and run tests"
	@echo "  build    - Build the main executable"
	@echo "  test     - Build and run all tests"
	@echo "  clean    - Remove generated files and directories"
	@echo "  run      - Run the executable (use ARGS=... for arguments)"
	@echo "  makedir  - Create build and bin directories"
	@echo "  help     - Display this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build the executable"
	@echo "  make all          # Build and test"
	@echo "  make run ARGS='--help'"
	@echo "  make test         # Run all tests"
	@echo "  make clean        # Clean all generated files"
	@echo ""
	@echo "Build structure:"
	@echo "  build/main/      - Objects for main executable"
	@echo "  build/test/      - Objects for test builds and test executables"
	@echo "  bin/             - Main executable"
	@echo "  test/build/      - Test source objects"
