CC=gcc
CFLAGS=-g -O0 -Wall

BIN=bin
SRC=src
OBJ=obj

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
TARGET=bin/main

TEST=tests
TESTS=$(wildcard $(TEST)/*.c)
TESTBINS=$(patsubst $(TEST)/%.c, $(TEST)/bin/%, $(TESTS))

all: $(TARGET)

release: CFLAGS=-Wall -O2 -DNDEBUG
release: clean
release: $(TARGET)

$(TARGET): $(OBJ) $(BIN) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST)/bin/%: $(TEST)/%.c
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ -lcriterion

test: $(OBJS) $(TEST)/bin $(TESTBINS)
	for test in $(TESTBINS) ; do ./$$test ; done

$(TEST)/bin:
	mkdir $@

$(OBJ):
	mkdir $@

$(BIN):
	mkdir $@

clean:
	rm -rf $(BIN)/* $(OBJ)/* $(TEST)/bin/*
