PREFIX = ../
CC = gcc
INCLUDE_DIRS=-I $(PREFIX)/src/common -I $(PREFIX)/src/host/
CC_OPTS = -Wall -Ofast -std=gnu99 $(INCLUDE_DIRS)

OBJS = packet_handler.o\
       serial_linux.o\
			 

HEADERS = packet_header.h\
	packet_operations.h\
	packet_handler.h\
	serial_linux.h\

	

BINS = host

.phony: clean all

all: $(BINS)

%.o: $(PREFIX)/src/common/%.c 
		 $(CC) $(CC_OPTS) -c $<

%.o: $(PREFIX)/src/host/%.c 
		 $(CC) $(CC_OPTS) -c $<

host: host.o $(OBJS)
			$(CC) $(CC_OPTS) -o $@ $< $(OBJS)	

clean:
	rm -rf $(OBJS) $(BINS) *~ *.d *.o
					
