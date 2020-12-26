#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Metode_servera.h"
#include "Kruzni_bafer.h"
#include "Niti.h"
#include "Struktura_Server_info.h"

#define DEFAULT_BUFLEN 512
#define MASTER_PORT 10000
#define SERVER_SLEEP_TIME 1000
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}
#define BUFFER_SIZE 512
#pragma warning(disable:4996)

int  main(void)
{
	int portServera = 0;
	int portKlijenta = 0;
	char adresa[20];

	Server_info *glava;
	glava = (Server_info*)malloc(sizeof(Server_info));
	if (glava == NULL) {
		return 1;
	}

	Inicijalizacija(&glava);

	SOCKET listenSocketKlijenta = INVALID_SOCKET;
	SOCKET listenSocketServera = INVALID_SOCKET;
	SOCKET acceptedSocket = INVALID_SOCKET;
	SOCKET connectSocket = INVALID_SOCKET;

	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	char messageToSend[BUFFER_SIZE];

	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddress, myAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(MASTER_PORT);

	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
	}

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(connectSocket, FIONBIO, &nonBlockingMode);

	int poruka = 1;

	iResult = send(connectSocket, (char*)&poruka, 4, 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(acceptedSocket);
		WSACleanup();
		return 1;
	}

	bool primljenaPoruka = false;

	do {

		iResult = Selekt(&connectSocket);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			break;
		}

		if (iResult == 0)
		{
			printf("Ceka se odgovor master-a...\n");
			Sleep(SERVER_SLEEP_TIME);
			continue;
		}

		iResult = recv(connectSocket, recvbuf, 8, 0);
		if (iResult > 0)
		{
			int velicinaPoruke = *(int*)recvbuf;
			int brojElemenata = *(int*)(recvbuf + 4);
			iResult = recv(connectSocket, recvbuf, velicinaPoruke, 0);
			if (iResult > 0)
			{
				primljenaPoruka = true;

				if (iResult > 7) {
					char dobijenaAdresa[20];
					int duzinaElementa = 0;
					for (int i = 0; i < brojElemenata; i++) {
						int dobijenPort = *(int*)(recvbuf + duzinaElementa);
						int duzinaAdrese = *(int*)(recvbuf + duzinaElementa + 4);

						for (int j = 0; j < duzinaAdrese; j++) {
							dobijenaAdresa[j] = *((recvbuf + duzinaElementa) + 8 + j);
						}
						dobijenaAdresa[duzinaAdrese] = '\0';

						HANDLE semafor = CreateSemaphore(0, 0, 1, NULL);
						LPDWORD serverID = NULL;
						HANDLE hServerKonekcija = NULL;

						Dodaj(&glava, dobijenaAdresa, dobijenPort, semafor, serverID, hServerKonekcija);

						duzinaElementa += 2 * sizeof(int) + duzinaAdrese;
					}
				}

				//Pravimo listenSocketKlijenta da bi ga poslali masteru
				sockaddr_in service;
				service.sin_family = AF_INET;
				service.sin_addr.s_addr = inet_addr("127.0.0.1");
				service.sin_port = htons(0);

				listenSocketKlijenta = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

				if (listenSocketKlijenta == INVALID_SOCKET)
				{
					printf("socket failed with error: %ld\n", WSAGetLastError());
					WSACleanup();
					return 1;
				}

				iResult = bind(listenSocketKlijenta, (SOCKADDR *)&service, sizeof(service));
				if (iResult == SOCKET_ERROR)
				{
					printf("bind failed with error: %d\n", WSAGetLastError());
					closesocket(listenSocketKlijenta);
					WSACleanup();
					return 1;
				}

				unsigned long int nonBlockingMode = 1;
				iResult = ioctlsocket(listenSocketKlijenta, FIONBIO, &nonBlockingMode);



				//Pravimo listenSoketServera da bi ga poslali masteru
				service.sin_port = htons(0);

				listenSocketServera = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

				if (listenSocketServera == INVALID_SOCKET)
				{
					printf("socket failed with error: %ld\n", WSAGetLastError());
					WSACleanup();
					return 1;
				}

				iResult = bind(listenSocketServera, (SOCKADDR *)&service, sizeof(service));
				if (iResult == SOCKET_ERROR)
				{
					printf("bind failed with error: %d\n", WSAGetLastError());
					closesocket(listenSocketServera);
					WSACleanup();
					return 1;
				}

				nonBlockingMode = 1;
				iResult = ioctlsocket(listenSocketServera, FIONBIO, &nonBlockingMode);

				//Slanje poruke masteru
				char* poruka;

				int len = sizeof(myAddress);
				getsockname(listenSocketServera, (sockaddr*)&myAddress, &len);
				inet_ntop(AF_INET, &myAddress.sin_addr, adresa, sizeof(adresa));

				portServera = ntohs(myAddress.sin_port);

				getsockname(listenSocketKlijenta, (sockaddr*)&myAddress, &len);
				inet_ntop(AF_INET, &myAddress.sin_addr, adresa, sizeof(adresa));

				portKlijenta = ntohs(myAddress.sin_port);

				int velicina = strlen(adresa) + 2 * sizeof(int);
				poruka = (char*)malloc(velicina);
				*(int*)poruka = portServera;
				*((int*)(poruka + 4)) = portKlijenta;

				for (int i = 0; i < strlen(adresa); i++) {
					*(poruka + 2 * 4 + i) = adresa[i];
				}
				*(poruka + 2 * 4 + strlen(adresa)) = '\0';
				adresa[strlen(adresa)] = '\0';

				iResult = send(connectSocket, (char*)&velicina, 4, 0);
				iResult = send(connectSocket, poruka, velicina, 0);

			}
		}
		else if (iResult == 0)
		{
			printf("Connection with master closed.\n");
			closesocket(connectSocket);
			break;
		}
	} while (!primljenaPoruka);

	closesocket(connectSocket);

	RingBuffer *ringBuffer = (RingBuffer*)malloc(sizeof(RingBuffer));;
	ringBuffer->head = 0;
	ringBuffer->tail = 0;

	CRITICAL_SECTION bufferAccess;
	InitializeCriticalSection(&bufferAccess);

	HANDLE Empty = CreateSemaphore(0, RING_SIZE, RING_SIZE, NULL);
	HANDLE Full = CreateSemaphore(0, 0, RING_SIZE, NULL);

	DWORD klijentID;
	DWORD izvrsavanjeID;
	DWORD serverID;
	HANDLE hklijentkonekcija;
	HANDLE hklijentIzvrsavanje;
	HANDLE hServerKonekcija;

	Parametri parametri;
	parametri.listenSocket = &listenSocketKlijenta;
	parametri.ringBuffer = ringBuffer;
	parametri.bufferAccess = &bufferAccess;
	parametri.Empty = &Empty;
	parametri.Full = &Full;

	hklijentkonekcija = CreateThread(NULL, 0, &NitZaPrihvatanjeZahtevaKlijenta, &parametri, 0, &klijentID);

	Parametri parametri2;
	parametri2.listenSocket = NULL;
	parametri2.ringBuffer = ringBuffer;
	parametri2.bufferAccess = &bufferAccess;
	parametri2.Empty = &Empty;
	parametri2.Full = &Full;

	hklijentIzvrsavanje = CreateThread(NULL, 0, &NitZaIzvrsavanjeZahtevaKlijenta, &parametri2, 0, &izvrsavanjeID);

	Server_info *temp = glava;

	printf("Lista :\n");
	while (temp != NULL) {
		printf("Adresa : %s\n", temp->ipAdresa);
		printf("Port Servera : %d\n", temp->port);
		printf("MessageBox : %s\n\n", temp->messageBox);

		ParametriServer parametriServer;
		strcpy(parametriServer.adresa, adresa);
		parametriServer.port = portServera;
		parametriServer.serverInfo = temp;
		parametriServer.socket = NULL;

		temp->hServerKonekcija = CreateThread(NULL, 0, &NitZaPrihvatanjeZahtevaServera, &parametriServer, 0, temp->serverID);

		temp = temp->sledeci;
	}

	ParametriServer parametri3;
	parametri3.socket = &listenSocketServera;
	parametri3.serverInfo = glava;
	strcpy(parametri3.adresa, adresa);
	parametri3.port = portServera;

	hServerKonekcija = CreateThread(NULL, 0, &NitZaOsluskivanjeServera, &parametri3, 0, &serverID);

	temp = glava;

	while (temp != NULL) {
		WaitForSingleObject(temp->hServerKonekcija, INFINITE);
		temp = temp->sledeci;
	}

	WaitForSingleObject(hklijentkonekcija, INFINITE);
	WaitForSingleObject(hklijentIzvrsavanje, INFINITE);
	WaitForSingleObject(hServerKonekcija, INFINITE);

	temp = glava;

	while (temp != NULL) {
		CloseHandle(temp->hServerKonekcija);
		temp = temp->sledeci;
	}

	CloseHandle(hklijentkonekcija);
	CloseHandle(hklijentIzvrsavanje);
	CloseHandle(hServerKonekcija);
	SAFE_DELETE_HANDLE(Empty);
	SAFE_DELETE_HANDLE(Full);

	DeleteCriticalSection(&bufferAccess);

	closesocket(listenSocketKlijenta);
	closesocket(listenSocketServera);
	closesocket(acceptedSocket);
	WSACleanup(); 

	return 0;
}
