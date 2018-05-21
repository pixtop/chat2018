CC=gcc
CFLAGS=#-Wall -O
LDFLAGS=
EXEC1=serveur
EXEC2=console

all: $(EXEC1) $(EXEC2)

serveur: serveur.o
	$(CC) -o $@ $^ $(LDFLAGS)

console: console.o

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC1) $(EXEC2)
