#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "structuri.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

string convertToString(char *source, int size)
{
	int i;
	string s = "";

	for (int i = 0; i < size; i++)
	{
		s += source[i];
	}

	return s;
}

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		usage(argv[0]);
	}
	int ret;

	// dezactivare buffering
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// numar port
	int portno = atoi(argv[1]);
	DIE(portno == 0, "port");

	// setez socket udp
	int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_sock < 0, "socket UDP");

	struct sockaddr_in udp_addr;
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(portno);
	udp_addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr));
	DIE(ret < 0, "bind UDP");

	// setez socket tcp
	int tcp_sock;
	tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock < 0, "socket TCP");

	struct sockaddr_in tcp_addr;
	memset((char *)&tcp_addr, 0, sizeof(tcp_addr));
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(portno);
	tcp_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(tcp_sock, (struct sockaddr *)&tcp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind TCP");

	ret = listen(tcp_sock, 150);
	DIE(ret < 0, "listen");

	// dezactivare nagle
	int flag = 1;
	ret = setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	DIE(ret < 0, "nagle");

	// set porturi
	fd_set read_fs;
	fd_set tmp_fds;
	int fdmax;

	FD_ZERO(&read_fs);
	FD_ZERO(&tmp_fds);

	FD_SET(0, &read_fs);
	FD_SET(udp_sock, &read_fs);
	FD_SET(tcp_sock, &read_fs);

	// get fdmax
	if (udp_sock > tcp_sock)
	{
		fdmax = udp_sock;
	}
	else
	{
		fdmax = tcp_sock;
	}

	int ok = 1;

	// map pentru id si socket
	unordered_map<string, int> id_socket;

	// map pentru topic si id
	unordered_map<string, vector<string>> topic_clienti;

	// map pentru socket si id
	unordered_map<int, string> socket_id;

	// astept mesaje
	while (ok)
	{
		tmp_fds = read_fs;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (int i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &tmp_fds))
			{
				//udp
				if (i == udp_sock)
				{
					char buf[1600];
					memset(buf, 0, sizeof(buf));
					struct sockaddr_in udp_client;
					socklen_t udp_len = sizeof(udp_client);
					ret = recvfrom(udp_sock, buf, sizeof(buf), 0, (struct sockaddr *)&udp_client, &udp_len);
					DIE(ret < 0, "primire udp");

					struct mesaj_tcp de_trimis;
					memset(&de_trimis, 0, sizeof(struct mesaj_tcp));
					memcpy(de_trimis.data, buf, sizeof(struct mesaj_tcp));
					de_trimis.tip = 1;

					memcpy(de_trimis.data + sizeof(struct mesaj_udp), &udp_client.sin_addr, sizeof(struct in_addr));
					memcpy(de_trimis.data + sizeof(struct mesaj_udp) + sizeof(in_addr), &udp_client.sin_port, 2);

					char aux[51];
					memset(aux, 0, 51);
					memcpy(aux, de_trimis.data, 50);

					string de_cautat = convertToString(aux, 50);

					if (topic_clienti.find(aux) == topic_clienti.end())
					{
						continue;
					}

					vector<string> clienti_ce_primesc = topic_clienti[aux];

					for (int j = 0; j < clienti_ce_primesc.size(); j++)
					{
						string client_de_trimis = clienti_ce_primesc[j];

						int sock_sent = id_socket[client_de_trimis];
						if (sock_sent != -1)
						{

							ret = send(sock_sent, &de_trimis, sizeof(struct mesaj_tcp), 0);
							DIE(ret < 0, "sent tcp client");
						}
					}

					break;
				}

				//tcp
				if (i == tcp_sock)
				{
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int new_client = accept(tcp_sock, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(new_client < 0, "accept");

					// dezactivare nagle
					int flag = 1;
					ret = setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
					DIE(ret < 0, "nagle");

					mesaj_tcp msg_client;
					memset(&msg_client, 0, sizeof(mesaj_tcp));

					// verificare id
					ret = recv(new_client, &msg_client, sizeof(mesaj_tcp), 0);
					if (msg_client.tip != 0)
					{
						close(new_client);
						continue;
					}

					mesaj_conectare msg_id;
					memset(&msg_id, 0, sizeof(mesaj_conectare));
					memcpy(&msg_id, &msg_client.data, sizeof(msg_id));
					string source_id = convertToString(msg_id.mesaj, 10);

					if (id_socket.find(source_id) != id_socket.end() && id_socket[source_id] != -1)
					{
						close(new_client);
						printf("Client %s already connected.\n", msg_id.mesaj);
						continue;
					}

					int ok = 0;
					if (id_socket.find(source_id) != id_socket.end())
					{
						id_socket[source_id] = new_client;
						socket_id.insert(make_pair(new_client, source_id));

						ok = 1;
					}
					else
					{
						id_socket.insert(make_pair(source_id, new_client));
						socket_id.insert(make_pair(new_client, source_id));
					}

					FD_SET(new_client, &read_fs);
					if (new_client > fdmax)
					{
						fdmax = new_client;
					}

					printf("New client %s connected from %d.\n", msg_id.mesaj, ntohs(cli_addr.sin_port));
					continue;
				}

				//stdin
				if (i == 0)
				{
					char buf[10];
					memset(buf, 0, 10);
					fgets(buf, 10, stdin);
					if (strncmp(buf, "exit", 4) == 0 && strlen(buf) == 5 && buf[4] == '\n')
					{
						ok = 0;
						for (int j = 0; j <= fdmax; j++)
						{
							if (FD_ISSET(j, &read_fs) && j != 0 && j != tcp_sock && j != udp_sock)
							{
								FD_CLR(j, &read_fs);
								close(j);
							}
						}
						break;
					}
					continue;
				}

				// tcp client
				struct mesaj_tcp mes_client;
				memset(&mes_client, 0, sizeof(struct mesaj_tcp));
				ret = recv(i, &mes_client, sizeof(mesaj_tcp), 0);
				DIE(ret < 0, "primire mesaj subscriber");

				if (ret == 0)
				{
					printf("Client %s disconnected.\n", socket_id[i].c_str());
					id_socket[socket_id[i]] = -1;
					socket_id.erase(i);
					close(i);
					FD_CLR(i, &read_fs);
					FD_CLR(i, &tmp_fds);
					continue;
				}

				if (mes_client.tip != 1)
				{
					continue;
				}

				struct mesaj_subscribe de_verificat;
				memset(&de_verificat, 0, sizeof(struct mesaj_subscribe));
				memcpy(&de_verificat, &mes_client.data, sizeof(struct mesaj_subscribe));

				if (de_verificat.tip == 1)
				{
					if (topic_clienti.find(de_verificat.topic) != topic_clienti.end())
					{
						vector<string> v = topic_clienti[de_verificat.topic];
						if (find(v.begin(), v.end(), socket_id[i]) == v.end())
						{
							v.push_back(socket_id[i]);
						}
						topic_clienti[de_verificat.topic] = v;
					}
					else
					{
						vector<string> v;
						v.push_back(socket_id[i]);
						topic_clienti.insert(make_pair(convertToString(de_verificat.topic, strlen(de_verificat.topic)), v));
					}

					char verificare = 1;
					ret = send(i, &verificare, 1, 0);
					DIE(ret < 0, "send akc sub");

					continue;
				}

				if (de_verificat.tip == 0)
				{
					if (topic_clienti.find(de_verificat.topic) != topic_clienti.end())
					{
						vector<string> v = topic_clienti[de_verificat.topic];
						if (find(v.begin(), v.end(), socket_id[i]) != v.end())
						{
							v.erase(find(v.begin(), v.end(), socket_id[i]));
						}
						topic_clienti[de_verificat.topic] = v;
					}

					char verificare = 0;
					ret = send(i, &verificare, 1, 0);
					DIE(ret < 0, "send akc unsub");
				}
			}
		}
	}

	close(udp_sock);
	close(tcp_sock);

	return 0;
}
