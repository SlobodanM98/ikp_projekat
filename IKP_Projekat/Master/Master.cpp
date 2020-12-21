#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "10000"
#define SERVER_SLEEP_TIME 1000
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}

bool InitializeWindowsSockets();

typedef struct clan {
	char ipAdresa[20];
	int port;
	struct clan *sledeci;
}Clan;

void Dodaj(Clan **glava, char *novaAdresa, int noviPort) {
	Clan *novi;
	novi = (Clan*)malloc(sizeof(Clan));
	strcpy(novi->ipAdresa, novaAdresa);
	novi->port = noviPort;
	novi->sledeci = *glava;
	*glava = novi;
}

int BrojClanova(Clan *glava) {
	int broj = 0;
	Clan *trenutni = glava;
	while (trenutni != NULL) {
		broj++;
		trenutni = trenutni->sledeci;
	}
	return broj;
}

int  main(void)
{
	Clan *glava = NULL;
	glava = (Clan*)malloc(sizeof(Clan));
	if (glava == NULL) {
		return 1;
	}

	glava->port = NULL;
	glava->sledeci = NULL;

	bool praznaLista = true;
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptedSocket = INVALID_SOCKET;

	int iResult;
	char recvbuf[DEFAULT_BUFLEN];

	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	addrinfo *resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;     

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return 1;
	}

	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(resultingAddress);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("Server initialized, waiting for clients.\n");

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(acceptedSocket, FIONBIO, &nonBlockingMode);
	iResult = ioctlsocket(listenSocket, FIONBIO, &nonBlockingMode);

	do
	{

		FD_SET set;
		timeval timeVal;

		FD_ZERO(&set);

		FD_SET(listenSocket, &set);

		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		iResult = select(0, &set, NULL, NULL, &timeVal);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			continue;
		}

		if (iResult == 0)
		{
			printf("Ceka se veza sa serverom...\n");
			Sleep(SERVER_SLEEP_TIME);
			continue;
		}

		acceptedSocket = accept(listenSocket, NULL, NULL);

		if (acceptedSocket == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		int brojClanova = BrojClanova(glava);
		int velicinaPoruke = 0;
		char* poruka;

		if (!praznaLista) {
			bool praznaPoruka = true;

			Clan *temp = glava;

			poruka = (char*)malloc(strlen(temp->ipAdresa) + sizeof(int));

			*(int*)poruka = temp->port;

			for (int i = 0; i < strlen(temp->ipAdresa); i++) {
				*(poruka + 4 + i) = temp->ipAdresa[i];
			}

			velicinaPoruke += strlen(temp->ipAdresa) + sizeof(int);
		}
		else {
			velicinaPoruke = 7;
			poruka = (char*)malloc(velicinaPoruke);
			strcpy(poruka, "prazno");
		}

		iResult = send(acceptedSocket, (char*)&velicinaPoruke, 4, 0);
		iResult = send(acceptedSocket, poruka, velicinaPoruke, 0);

		free(poruka);

		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket);
			WSACleanup();
			return 1;
		}

		bool primljeno = false;

		do {

			FD_SET set;
			timeval timeVal;

			FD_ZERO(&set);

			FD_SET(acceptedSocket, &set);

			timeVal.tv_sec = 0;
			timeVal.tv_usec = 0;

			iResult = select(0, &set, NULL, NULL, &timeVal);

			if (iResult == SOCKET_ERROR)
			{
				fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
				continue;
			}

			if (iResult == 0)
			{
				printf("Ceka se odgovor sa serverom...\n");
				Sleep(SERVER_SLEEP_TIME);
				continue;
			}

			int velicina = 0, port = 0;
			char adresa[20];

			iResult = recv(acceptedSocket, recvbuf, 4, 0);
			if (iResult > 0) {
				velicina = *(int*)recvbuf;
				printf("Velicina je : %d\n", velicina);

				iResult = recv(acceptedSocket, recvbuf, velicina, 0);
				if (iResult > 0) {
					printf("Poruka je : %s\n", recvbuf);
					port = *(int*)recvbuf;
					
					for (int i = 0; i < velicina - 4; i++) {
						adresa[i] = *(recvbuf + 4 + i);
					}
					adresa[velicina - 4] = '\0';

					printf("Port je : %d\n", port);
					printf("Adresa je : %s\n", adresa);
					primljeno = true;

					if (glava->port == NULL) {
						strcpy(glava->ipAdresa, adresa);
						glava->port = port;
						praznaLista = false;
					}
					else {
						Dodaj(&glava, adresa, port);
						praznaLista = false;
					}

					Clan *temp = glava;

					printf("Lista :\n");
					while (temp != NULL) {
						printf("Adresa : %s\n", temp->ipAdresa);
						printf("Port : %d\n\n", temp->port);
						temp = temp->sledeci;
					}

				}
			}
		} while (!primljeno);
	} while (1);

	iResult = shutdown(acceptedSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptedSocket);
		WSACleanup();
		return 1;
	}

	closesocket(listenSocket);
	closesocket(acceptedSocket);
	WSACleanup();

	return 0;
}

bool InitializeWindowsSockets()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}
