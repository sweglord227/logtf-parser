# Define CFLAGS like this so the user can change them if they like
# Because they can change them, make these options optional, and
# place anything required below
CFLAGS := -g -O2

CC := gcc

# Define all of your sources here, split them up into different
# "SRCS" variables if you're making several different binaries
SRCS = logtf-parser.c

# PHONY means, don't try to find a file named e.g. "all", etc. Since
# make determines if it needs to run a rule if that file exists
.PHONY: all clean

# all is the default rule
all: logtf-parser

# Clean up anything that was produced by this makefile
clean:
	rm logtf-parser.o
	rm test_curl 2>/dev/null || true

# test_curl will be the name of the binary, so it doesn't go in PHONY
# .c=.o means replace all of the sources with their .o counter parts
# Everything to the right of the colon is a requirement *before*
# this rule will run
logtf-parser: $(SRCS:.c=.o)
  # $^ means everything to the right of the colon
  # $@ means the name to the left of the colon
  # See how the *required* library goes here?
	$(CC) $(CFLAGS) -lcurl $^ -o $@

# This is a glob rule, it will create any unmatched .o requirement
# It needs the .c file to exist as a requirement though
# The libs don't get added here since these are object files
# and not a finished binary
# If you have an include folder though, you would still add
# "-Iinclude_folder" here
	%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@
