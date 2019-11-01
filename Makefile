CC      = g++
CFLAGS  = -pthread -std=c++11 -Wall -Werror -O2
SOURCES_CC = $(wildcard *.cc)
SOURCES_C = $(wildcard *.c)
OBJECTS_CC = $(SOURCES_CC:%.cc=%.o)
OBJECTS_C = $(SOURCES_C:%.c=%.o)

all: wc

wc: wordcount

wordcount: $(OBJECTS_CC) $(OBJECTS_C)
	$(CC) -o wordcount $(OBJECTS_CC) $(OBJECTS_C) -pthread

compile: $(OBJECTS_CC) $(OBJECTS_C)

%.o: %.cc
	${CC} ${CFLAGS} -c $^

%.o: %.c
	${CC} ${CFLAGS} -c $^

clean:
	@rm -f *.o wordcount result-*.txt

clean-result:
	@rm result-*.txt

compress:
	zip mapreduce.zip README.md Makefile *.cc *.c *.h

