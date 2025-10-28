# Compiler
CXX      = g++
CXXFLAGS = -Wall -g -Iinc

# Directories
SRC_DIR  = src
OUT_DIR  = out
MISC_DIR = misc

# Bison/Flex
LEXER     = $(MISC_DIR)/lexer.l
PARSER    = $(MISC_DIR)/parser.y
LEX_CPP   = $(OUT_DIR)/lexer.cpp
PARSER_CPP= $(OUT_DIR)/parser.cpp
PARSER_HPP= $(OUT_DIR)/parser.hpp

# Sources
ALL_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
ALL_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OUT_DIR)/%.o,$(ALL_SRCS))

# Executables
ASM_EXEC    = $(OUT_DIR)/assembler
LINK_EXEC   = $(OUT_DIR)/linker
EMUL_EXEC   = $(OUT_DIR)/emulator

# Default target
all: $(ASM_EXEC) $(LINK_EXEC) $(EMUL_EXEC)

# Ensure output directory exists
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# Generate parser
$(PARSER_CPP) $(PARSER_HPP): $(PARSER) | $(OUT_DIR)
	bison -d --defines=$(PARSER_HPP) -o $(PARSER_CPP) $(PARSER)

# Generate lexer (depends on parser header)
$(LEX_CPP): $(LEXER) $(PARSER_HPP) | $(OUT_DIR)
	flex -o $(LEX_CPP) $(LEXER)

# Compile all source files
$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile generated lexer and parser
$(OUT_DIR)/lexer.o: $(LEX_CPP)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/parser.o: $(PARSER_CPP)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Object files for assembler: all except linker.o
ASM_OBJS = $(filter-out $(OUT_DIR)/linker.o $(OUT_DIR)/emulator.o, $(ALL_OBJS))

# Build assembler (includes parser/lexer)
$(ASM_EXEC): $(ASM_OBJS) $(LEX_CPP:.cpp=.o) $(PARSER_CPP:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $(ASM_OBJS) $(LEX_CPP:.cpp=.o) $(PARSER_CPP:.cpp=.o)

# Build linker (only relevant objects, no parser/lexer)
LINK_OBJS = $(filter-out $(OUT_DIR)/parser.o $(OUT_DIR)/lexer.o $(OUT_DIR)/emulator.o,$(ALL_OBJS))
$(LINK_EXEC): $(LINK_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(LINK_OBJS)

EMUL_OBJS = $(OUT_DIR)/emulator.o

$(EMUL_EXEC): $(EMUL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(EMUL_OBJS) -pthread
# Clean
clean:
	rm -f $(OUT_DIR)/*.o $(OUT_DIR)/lexer.cpp $(OUT_DIR)/parser.cpp $(OUT_DIR)/parser.hpp $(ASM_EXEC) $(LINK_EXEC) ${EMUL_EXEC}
	rmdir $(OUT_DIR) 2>/dev/null || true
