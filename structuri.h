#ifndef MYSTRUCTS_H
#define MYSTRUCTS_H

using namespace std;

typedef struct mesaj_udp {
	char topic[51];
	unsigned char tip;
	char data[1501];
} mesaj_udp;

typedef struct mesaj_tcp {
	char tip;
	char data[1600];
} mesaj_tcp;

typedef struct mesaj_conectare {
	char mesaj[11];
} mesaj_conectare;

typedef struct mesaj_subscribe {
	uint32_t tip;
	char topic[51];
	char sf;
} mesaj_subscribe;

#endif
