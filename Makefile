MAINSRC = shoot_hex.c

SRC = $(MAINSRC)

LIBS = -lpthread -ltermcap

OBJ = $(SRC:.c=.o)

CC = gcc

all: game

.c.o:
	$(CC) -c $<

game: $(OBJ)
	gcc -o $@ $^ $(LIBS)

clean:
	rm -f game *.o
