IDIR=include
LIBS=lib
CC=g++-8
# CC=g++
CFLAGS=-I $(IDIR) -I $(LIBS) -std=c++11 -pthread -static-libstdc++ -DASIO_STANDALONE -Wall

ODIR=obj


_DEPS = tcp_server.h client.h command.h spreadsheet.h JSON_message.h spreadsheet_server.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = server.o tcp_server.o client.o command.o spreadsheet.o JSON_message.o spreadsheet_server.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) 

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o server

