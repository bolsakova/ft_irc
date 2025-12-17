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
PROTOCOLDIR = src/protocol
PROTOCOL_SRCS = $(addsuffix .cpp, $(PROTOCOLDIR))

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

.PHONY: all clean fclean re
