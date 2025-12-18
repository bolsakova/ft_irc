NAME = ft_irc

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17

# Debug mode (make debug)
ifdef DEBUG
    CXXFLAGS += -g3 -fsanitize=address
    LDFLAGS += -fsanitize=address
endif

# Directories
SRCDIR = src
INCDIR = inc
OBJDIR = obj

# Source files
PROTOCOL_SRCS = $(shell find $(SRCDIR) -name '*.cpp')

TEST_SRCS = test_parser.cpp

# All sources
SRCS = $(TEST_SRCS) $(PROTOCOL_SRCS)

# Object files with subdirectory structure
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

# Include paths
INCLUDES = -I$(INCDIR) -I.

# Colors for output
GREEN = \033[0;32m
YELLOW = \033[0;33m
RED = \033[0;31m
BLUE = \033[0;34m
RESET = \033[0m

# Main targets
all: $(NAME)
	@echo "$(GREEN)✓ Build complete: $(NAME)$(RESET)"

$(NAME): $(OBJS)
	@echo "$(BLUE)Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(NAME) $(OBJS)

# Pattern rule for object files (handles subdirectories)
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Create necessary directories
create_dirs:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(OBJDIR)/src/protocol

clean:
	@echo "$(RED)Cleaning object files...$(RESET)"
	@rm -rf $(OBJDIR)

fclean: clean
	@echo "$(RED)Removing $(NAME)...$(RESET)"
	@rm -rf $(NAME)

re: fclean all

# Debug build
debug:
	@$(MAKE) DEBUG=1 re

# Run tests
test: $(NAME)
	@echo "$(BLUE)Running tests...$(RESET)"
	@./$(NAME)

# Check for memory leaks with valgrind
valgrind: $(NAME)
	@echo "$(BLUE)Running valgrind...$(RESET)"
	valgrind ---leak-check=full ---show-leak-kinds=all ./$(NAME)

# Help target
help:
	@echo "$(BLUE)Available targets:$(RESET)"
	@echo "  $(GREEN)all$(RESET)      - Build the project (default)"
	@echo "  $(GREEN)clean$(RESET)    - Remove object files"
	@echo "  $(GREEN)fclean$(RESET)   - Remove object files and executable"
	@echo "  $(GREEN)re$(RESET)       - Rebuild everything"
	@echo "  $(GREEN)debug$(RESET)    - Build with debug symbols and AddressSanitizer"
	@echo "  $(GREEN)test$(RESET)     - Build and run tests"
	@echo "  $(GREEN)valgrind$(RESET) - Run with valgrind memory checker"
	@echo "  $(GREEN)help$(RESET)     - Show this help message"

.PHONY: all clean fclean re debug test valgrind help create_dirs

# Dependencies (automatic generation)
-include $(OBJS:.o=.d)

# Automatic dependency generation
$(OBJDIR)/%.d: %.cpp | create_dirs
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MM -MT $(@:.d=.o) $< > $@
