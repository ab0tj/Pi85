pi85: monitor.o console.o disk.o file.o
	gcc -o pi85 Pi85.o monitor.o console.o disk.o file.o -lwiringPi

all: pi85 dump fill init run stop

dump: Pi85.o Pi85.h dump.c
	gcc -o dump Pi85.o dump.c -lwiringPi

fill: Pi85.o Pi85.h fill.c
	gcc -o fill Pi85.o fill.c -lwiringPi

init: Pi85.o Pi85.h init.c
	gcc -o init Pi85.o init.c -lwiringPi

run: Pi85.o Pi85.h run.c
	gcc -o run Pi85.o run.c -lwiringPi

stop: Pi85.o Pi85.h stop.c
	gcc -o stop Pi85.o stop.c -lwiringPi

Pi85.o: Pi85.c Pi85.h
	gcc -c Pi85.c

monitor.o: monitor.c Pi85.h console.h disk.h
	gcc -c monitor.c

console.o: console.c console.h Pi85.h disk.h file.h
	gcc -c console.c

disk.o: disk.c disk.h Pi85.h
	gcc -c disk.c

file.o: file.c file.h Pi85.h
	gcc -c file.c

clean:
	rm -f pi85 dump fill init run stop *.o
