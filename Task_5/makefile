# Makefile for Sniffer project

all: Sniffer Spoofer Gateway attacker

Sniffer: Sniffer.c
	gcc -o Sniffer Sniffer.c -lpcap
Spoofer: Spoofer.c
	gcc -o Spoofer Spoofer.c -lpcap
Gateway: Gateway.c
	gcc -o Gateway Gateway.c
attacker: attacker.c
	gcc -o attacker attacker.c -lpcap
clean:
	rm -f *.o Sniffer Spoofer Gateway attacker