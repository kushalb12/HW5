# CC=gcc
# CFLAGS= -w -g -pthread
# LDFLAGS=mythread.a
# SOURCES=myqueue.c mysched.c mytest.c
# INCLUDES=mythread.h myqueue.h
# EXECUTABLE=mytest

# default: 
# 	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) $(LDFLAGS) -o $(EXECUTABLE) -lm 

# clean:
# 	rm -f $(EXECUTABLE)
SRCS = p4_main.c p4_server.c
INC = p4.h
OBJS = $(SRCS:.c=.o)
# TEST_SRCS = mytest.c
# TEST_OBJS = $(TEST_SRCS:.c=.o)
EXECUTABLE = p4

CFLAGS = -w -I. -g -pthread 
# LIB = mythread.a
# MYLIB = mysched.a 

CC = gcc

all: test

# lib: $(MYLIB) $(LIB) 

# mysched.a: $(OBJS) $(INC)
# 	$(AR) rcs $(MYLIB) $(OBJS)

%.o: %.c $(INC)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(MYLIB) $(EXECUTABLE) endpoints list*

cleanfile:
	rm -f endpoints list*

test:	$(OBJS) $(INC)
	$(CC) -o $(EXECUTABLE) $(CFLAGS) $(OBJS)  -lm