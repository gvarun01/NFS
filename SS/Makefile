# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Source files
SRCS = storageServer.c SS_NM.c SS_SS.c SS_Client.c SS_Backup.c SS_paths.c ../LinkedList.c SS_try.c

# Output binary
OUT = a.out

# Default target
all: $(OUT)

# Rule to compile and link the source files to create a.out
$(OUT): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(OUT)

# Rule to execute the program with parameters passed to `make run`
run: $(OUT)
	./$(OUT) $(PARAMS)

# Clean up compiled files
clean:
	rm -f $(OUT) *.o

# Phony targets
.PHONY: all run clean
