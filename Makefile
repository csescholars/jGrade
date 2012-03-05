CC = gcc
CCFLAGS = -Wall -g

BINS = autograder

AG_OBJS = main.o mkpath.o grade.o diff.o csv.o

all: $(BINS)

autograder: $(AG_OBJS)
	$(CC) $(CCFLAGS) $^ -o $@ -lcgic
copy:
	cp ./autograder ../public_html/autograder/autograder.cgi

%.o: %.c
	$(CC) $(CCFLAGS) -c -o $@ $<

clean:
	rm -f *.o
	rm -f $(BINS)
