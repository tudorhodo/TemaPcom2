#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include "structuri.h"
#include <cctype>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s id_client ip_server port_server\n", file);
	exit(0);
}

int main(int argc, char **argv)
{
	if (argc < 4)
	{
		usage(argv[0]);
	}

	// dezactivare buffering
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// numar port
	int portno = atoi(argv[3]);
	DIE(portno == 0, "port");

	// socket server
	int socket_server, ret;

	socket_server = socket(AF_INET, SOCK_STREAM, 0);
	DIE(socket_server < 0, "socket");

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(socket_server, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// dezactivare nagle
	int flag = 1;
	ret = setsockopt(socket_server, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	DIE(ret < 0, "nagle");

	// mesaj conectare
	mesaj_conectare send_id;
	mesaj_tcp msg_serv;
	memset(&send_id, 0, sizeof(mesaj_conectare));
	memset(&msg_serv, 0, sizeof(mesaj_tcp));

	memcpy(send_id.mesaj, argv[1], 10);
	memcpy(msg_serv.data, &send_id, sizeof(send_id));

	msg_serv.tip = 0;
	ret = send(socket_server, &msg_serv, sizeof(mesaj_tcp), 0);
	DIE(ret < 0, "connect id");

	fd_set read_fds;
	fd_set tmp_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(socket_server, &read_fds);
	FD_SET(0, &read_fds);

	int fdmax = socket_server;
	while (1)
	{
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(0, &tmp_fds))
		{
			char buffer[100];
			memset(buffer, 0, 100);
			fgets(buffer, 100, stdin);

			if (strncmp(buffer, "exit", 4) == 0)
			{
				break;
			}

			if (strncmp(buffer, "subscribe", 9) == 0)
			{
				struct mesaj_subscribe sub;
				memset(&sub, 0, sizeof(mesaj_subscribe));
				sub.tip = 1;
				char *token = strtok(buffer, " \n");
				token = strtok(NULL, " \n");
				if (token == NULL || strlen(token) > 50)
				{
					continue;
				}

				memcpy(sub.topic, token, strlen(token));
				token = strtok(NULL, " \n");

				if (token == NULL || !isdigit(token[0]))
				{
					continue;
				}

				if (token[0] == '0')
				{
					sub.sf = 0;
				}
				else
				{
					sub.sf = 1;
				}

				struct mesaj_tcp de_trimis;
				memset(&de_trimis, 0, sizeof(struct mesaj_tcp));
				memcpy(&de_trimis.data, &sub, sizeof(struct mesaj_subscribe));

				de_trimis.tip = 1;

				ret = send(socket_server, &de_trimis, sizeof(struct mesaj_tcp), 0);
				DIE(ret < 0, "cerere sub");

				char verificare;
				ret = recv(socket_server, &verificare, 1, 0);
				DIE(ret < 0, "akc sub");

				if (verificare == 1)
				{
					printf("Subscribed to topic.\n");
				}

				continue;
			}

			if (strncmp(buffer, "unsubscribe", 11) == 0)
			{
				struct mesaj_subscribe unsub;
				memset(&unsub, 0, sizeof(struct mesaj_subscribe));
				unsub.tip = 0;
				char *token = strtok(buffer, " \n");
				token = strtok(NULL, " \n");

				if (token == NULL || strlen(token) > 50)
				{
					continue;
				}

				memcpy(unsub.topic, token, strlen(token));
				token = strtok(NULL, " \n");

				struct mesaj_tcp de_trimis;
				memset(&de_trimis, 0, sizeof(struct mesaj_tcp));
				memcpy(&de_trimis.data, &unsub, sizeof(struct mesaj_subscribe));
				de_trimis.tip = 1;

				ret = send(socket_server, &de_trimis, sizeof(struct mesaj_tcp), 0);
				DIE(ret < 0, "cerere unsub");

				char verificare;
				ret = recv(socket_server, &verificare, 1, 0);
				DIE(ret < 0, "akc unsub");

				if (verificare == 0)
				{
					printf("Unsubscribed from topic.\n");
				}
			}
		}
		else
		{
			mesaj_tcp msg_server;
			memset(&msg_server, 0, sizeof(mesaj_tcp));
			ret = recv(socket_server, &msg_server, sizeof(msg_server), 0);
			if (ret == 0)
			{
				break;
			}
			DIE(ret < 0, "primire notificare");

			if (msg_server.tip != 1)
			{
				continue;
			}

			struct mesaj_udp m;
			memset(&m, 0, sizeof(struct mesaj_udp));
			memcpy(m.topic, msg_server.data, 50);

			memcpy(&m.tip, msg_server.data + 50, 1);

			memcpy(m.data, msg_server.data + 51, 1500);
			m.data[1500] = '\0';

			struct in_addr udp_addr;
			memset(&udp_addr, 0, sizeof(struct in_addr));
			memcpy(&udp_addr, msg_server.data + sizeof(struct mesaj_udp), sizeof(struct in_addr));

			uint16_t udp_port;
			memset(&udp_port, 0, 2);
			memcpy(&udp_port, msg_server.data + sizeof(struct mesaj_udp) + sizeof(in_addr), 2);

			if (m.tip == 0)
			{
				uint8_t semn;
				uint32_t modul;

				memcpy(&semn, m.data, 1);
				memset(&modul, 0, 4);
				memcpy(&modul, m.data + 1, 4);
				modul = ntohl(modul);

				if (semn == 1)
				{
					modul = -1 * modul;
				}

				printf("%s:%d - %s - INT - %d\n", inet_ntoa(udp_addr), ntohs(udp_port), m.topic, modul);
			}

			if (m.tip == 1)
			{
				uint16_t modul;
				memset(&modul, 0, 2);
				memcpy(&modul, m.data, 2);
				modul = ntohs(modul);

				printf("%s:%d - %s - SHORT_REAL - %.2f\n", inet_ntoa(udp_addr), ntohs(udp_port), m.topic, (float)(modul / 100.00));
			}

			if (m.tip == 2)
			{
				uint8_t semn;
				uint32_t modul;
				uint8_t rest;

				memset(&modul, 0, 4);
				memcpy(&semn, m.data, 1);
				memcpy(&modul, m.data + 1, 4);
				memcpy(&rest, m.data + 5, 1);

				modul = ntohl(modul);
				float modul2 = modul;

				for (int i = 0; i < rest; i++)
				{
					modul2 = modul2 / 10;
				}

				if (semn == 1)
				{
					modul2 = -1 * modul2;
				}

				printf("%s:%d - %s - FLOAT - %.*f\n", inet_ntoa(udp_addr), ntohs(udp_port), m.topic, rest, modul2);
			}

			if (m.tip == 3)
			{
				printf("%s:%d - %s - STRING - %s\n", inet_ntoa(udp_addr), ntohs(udp_port), m.topic, m.data);
			}
		}
	}

	close(socket_server);

	return 0;
}
