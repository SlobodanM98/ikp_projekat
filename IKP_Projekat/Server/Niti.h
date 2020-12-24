#pragma once
#define CLIENT_SLEEP_TIME 1000
#include "Struktura_Server_info.h"

struct Parametri {
	SOCKET *listenSocket;
	RingBuffer *ringBuffer;
	CRITICAL_SECTION *bufferAccess;
	HANDLE *Empty;
	HANDLE *Full;
};

struct ParametriServer {
	Server_info *serverInfo;
	SOCKET *socket;
	int port;
	char adresa[20];
};

DWORD WINAPI NitZaPrihvatanjeZahtevaServera(LPVOID lpParam) {
	SOCKET connectSocket = INVALID_SOCKET;

	ParametriServer parametri = *(ParametriServer*)lpParam;
	Server_info *temp = parametri.serverInfo;

	int iResult;
	char recvbuf[512];

	/*if (InitializeWindowsSockets() == false)
	{
		return 1;
	}*/

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(temp->ipAdresa);
	serverAddress.sin_port = htons(temp->port);

	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
		return -1;
	}

	printf("Ja sam se povezao sa drugim serverom.\n");

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(connectSocket, FIONBIO, &nonBlockingMode);

	char* poruka;

	int velicina = strlen(parametri.adresa) + sizeof(int);
	poruka = (char*)malloc(velicina);
	*(int*)poruka = parametri.port;

	for (int i = 0; i < strlen(parametri.adresa); i++) {
		*(poruka + 4 + i) = parametri.adresa[i];
	}
	*(poruka + 4 + strlen(parametri.adresa)) = '\0';

	iResult = send(connectSocket, (char*)&velicina, 4, 0);
	iResult = send(connectSocket, poruka, velicina, 0);

	do {

		iResult = Selekt(&connectSocket);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			break;
		}

		if (iResult == 0)
		{
			//printf("Ceka se zahtev server-a...\n");
			Sleep(CLIENT_SLEEP_TIME);
			continue;
		}

		iResult = recv(connectSocket, recvbuf, 4, 0);
		if (iResult > 0)
		{
			int velicinaPoruke = *(int*)recvbuf;
			printf("Velicina poruke je %d.\n", velicinaPoruke);
			iResult = recv(connectSocket, recvbuf, velicinaPoruke, 0);
			if (iResult > 0)
			{
				printf("Primljena poruka od server-a: %s.\n", recvbuf);
			}
		}
		else if (iResult == 0)
		{
			printf("Connection with server closed.\n");
			closesocket(connectSocket);
			break;
		}
	} while (1);

	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

DWORD WINAPI NitZaOsluskivanjeServera(LPVOID lpParam) {
	SOCKET acceptedSocket = INVALID_SOCKET;

	int iResult;
	char recvbuf[512];

	/*if (InitializeWindowsSockets() == false)
	{
		return 1;
	}*/

	ParametriServer parametri = *(ParametriServer*)lpParam;
	SOCKET listenSocket = *(parametri.socket);
	Server_info *temp = parametri.serverInfo;

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(acceptedSocket, FIONBIO, &nonBlockingMode);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("Server initialized, waiting for servers.\n");

	do
	{
		bool primljenZahtev = false;

		iResult = Selekt(&listenSocket);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			continue;
		}

		if (iResult == 0)
		{
			//printf("Ceka se veza sa serverom...\n");
			Sleep(1000);
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

		

		int velicina, portServera;
		char adresa[20];
		bool primljeno = false;

		do {

			iResult = Selekt(&acceptedSocket);

			if (iResult == SOCKET_ERROR)
			{
				fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
				continue;
			}

			if (iResult == 0)
			{
				printf("Ceka se odgovor server-a...\n");
				Sleep(1000);
				continue;
			}

			iResult = recv(acceptedSocket, recvbuf, 4, 0);
			if (iResult > 0) {
				velicina = *(int*)recvbuf;

				iResult = recv(acceptedSocket, recvbuf, velicina, 0);
				if (iResult > 0) {
					portServera = *(int*)recvbuf;

					for (int i = 0; i < velicina - 4; i++) {
						adresa[i] = *(recvbuf + 4 + i);
					}
					adresa[velicina - 4] = '\0';

					primljeno = true;
				}
			}
		} while (!primljeno);

		Server_info *glava = temp;
		bool postoji = false;


		while (glava != NULL) {
			if (strcmp(glava->ipAdresa, adresa) == 0 && glava->port == portServera) {
				postoji = true;
				printf("Postoji!\n");
				break;
			}
			glava = glava->sledeci;
		}

		if (!postoji) {
			HANDLE semafor = CreateSemaphore(0, 0, 1, NULL);
			LPDWORD serverID = NULL;
			HANDLE hServerKonekcija = NULL;

			Dodaj(&temp, adresa, portServera, semafor, serverID, hServerKonekcija);

			ParametriServer parametriServer;
			strcpy(parametriServer.adresa, parametri.adresa);
			parametriServer.port = parametri.port;
			parametriServer.serverInfo = temp;
			parametriServer.socket = NULL;

			printf("Drugi server se povezao sa mnom!\n");

			temp->hServerKonekcija = CreateThread(NULL, 0, &NitZaPrihvatanjeZahtevaServera, &parametriServer, 0, temp->serverID);
		}

	} while (1);

	closesocket(listenSocket);
	WSACleanup();

	return 0;
}

DWORD WINAPI NitZaPrihvatanjeZahtevaKlijenta(LPVOID lpParam)
{
	SOCKET acceptedSocket = INVALID_SOCKET;

	int iResult;
	char recvbuf[512];

	/*if (InitializeWindowsSockets() == false)
	{
		return 1;
	}*/
	Parametri parametri = *(Parametri*)lpParam;

	SOCKET listenSocket = *(parametri.listenSocket);
	RingBuffer *ringBuffer = parametri.ringBuffer;
	CRITICAL_SECTION *bufferAccess = parametri.bufferAccess;
	HANDLE *Empty = parametri.Empty;
	HANDLE *Full = parametri.Full;

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(acceptedSocket, FIONBIO, &nonBlockingMode);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("Server initialized, waiting for clients.\n");

	do
	{
		bool primljenZahtev = false;

		iResult = Selekt(&listenSocket);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			continue;
		}

		if (iResult == 0)
		{
			//printf("Ceka se veza sa klijentom...\n");
			Sleep(1000);
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

		do
		{
			iResult = recv(acceptedSocket, recvbuf, 512, 0);
			if (iResult > 0)
			{
				recvbuf[11] = '\0';
				printf("Message received from client: %s.\n", recvbuf);
				primljenZahtev = true;
			}
			else if (iResult == 0)
			{
				printf("Connection with client closed.\n");
				closesocket(acceptedSocket);
			}
			else
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(acceptedSocket);
			}
		} while (primljenZahtev == false);


		while (WaitForSingleObject(*Empty, INFINITE) == WAIT_OBJECT_0) {


			EnterCriticalSection(bufferAccess);

			ringBufPutChar(ringBuffer, recvbuf);

			LeaveCriticalSection(bufferAccess);

			//printf("****************************: %s\n", ringBufGetChar(ringBuffer));

			ReleaseSemaphore(*Full, 1, NULL);

			break;
		}

	} while (1);

	closesocket(acceptedSocket);
	WSACleanup();

	return 0;
}



DWORD WINAPI NitZaIzvrsavanjeZahtevaKlijenta(LPVOID lpParam)
{
	Parametri parametri = *(Parametri*)lpParam;
	RingBuffer *ringBuffer = parametri.ringBuffer;
	CRITICAL_SECTION *bufferAccess = parametri.bufferAccess;
	HANDLE *Empty = parametri.Empty;
	HANDLE *Full = parametri.Full;

	while (WaitForSingleObject(*Full, INFINITE) == WAIT_OBJECT_0) {

		EnterCriticalSection(bufferAccess);
		// Citanje kruznog bafera;
		char *zahtev = ringBufGetChar(ringBuffer);
		LeaveCriticalSection(bufferAccess);
		// Ispis na konzolu;
		printf("\nIzvadjen zahtev iz buffera: %s\n", zahtev);

		// V(E) - operacija uvecava semafor E za jedan;
		ReleaseSemaphore(*Empty, 1, NULL);

	}

	return 0;
}