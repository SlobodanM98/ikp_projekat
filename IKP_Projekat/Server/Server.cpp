#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Metode_servera.h"
#include "Kruzni_bafer.h"
#include "Niti.h"
#include "Struktura_Server_info.h"
#include "Struktura_memorija.h"

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
	bool ugasiServer = false;
	bool integrityUpdate = true;
	bool izvrsavaSe2FC = false;
	bool izvrsenUpdate = true;

	printf("Klikni dugme da se povezes na master-a...\n");
	_getch();

	Server_info *glava;
	glava = (Server_info*)malloc(sizeof(Server_info));
	if (glava == NULL) {
		return 1;
	}

	Inicijalizacija(&glava);

	Memorija *glavaMemorije;
	glavaMemorije = (Memorija*)malloc(sizeof(Memorija));
	if (glavaMemorije == NULL) {
		return 1;
	}

	Inicijalizacija(&glavaMemorije);

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

	bool izvrsenUnos = false;
	char IPadresaMastera[20];
	char IPadresa[20];

	while (!izvrsenUnos) {
		printf("1.Master na lokalnom racunaru\n");
		printf("2.Master na drugom racunaru\n");
		char unos[10];
		scanf("%s", unos);
		unos[1] = '\0';
		if (strcmp(unos, "1") == 0) {
			izvrsenUnos = true;
			strcpy(IPadresaMastera, "127.0.0.1");
			strcpy(IPadresa, "127.0.0.1");
		}
		else if (strcmp(unos, "2") == 0) {
			izvrsenUnos = true;
			printf("Unesi IP adresu mastera : \n");
			scanf("%s", IPadresaMastera);
			printf("Unesi tvoju IP adresu : \n");
			scanf("%s", IPadresa);
		}
	}

	serverAddress.sin_addr.s_addr = inet_addr(IPadresaMastera);
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

	PosaljiPoruku(&connectSocket, (char*)&poruka, 4);

	iResult = PrimiPoruku(&connectSocket, recvbuf, 8, &ugasiServer, false);
	int velicinaPoruke = *(int*)recvbuf;
	int brojElemenata = *(int*)(recvbuf + 4);
	iResult = PrimiPoruku(&connectSocket, recvbuf, velicinaPoruke, &ugasiServer, false);

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


			Dodaj(&glava, dobijenaAdresa, dobijenPort);

			duzinaElementa += 2 * sizeof(int) + duzinaAdrese;
		}
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr(IPadresa);
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

	nonBlockingMode = 1;
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

	int len = sizeof(myAddress);
	getsockname(listenSocketServera, (sockaddr*)&myAddress, &len);
	inet_ntop(AF_INET, &myAddress.sin_addr, adresa, sizeof(adresa));

	portServera = ntohs(myAddress.sin_port);

	getsockname(listenSocketKlijenta, (sockaddr*)&myAddress, &len);
	inet_ntop(AF_INET, &myAddress.sin_addr, adresa, sizeof(adresa));

	portKlijenta = ntohs(myAddress.sin_port);

	int velicina = strlen(adresa) + 2 * sizeof(int);
	char* poruka2 = (char*)malloc(velicina);
	*(int*)poruka2 = portServera;
	*((int*)(poruka2 + 4)) = portKlijenta;

	for (int i = 0; i < strlen(adresa); i++) {
		*(poruka2 + 2 * 4 + i) = adresa[i];
	}
	adresa[strlen(adresa)] = '\0';
	printf("Port servera: %d\n", portServera);
	printf("Port klijenta: %d\n", portKlijenta);

	iResult = send(connectSocket, (char*)&velicina, 4, 0);
	iResult = send(connectSocket, poruka2, velicina, 0);
	free(poruka2);

	closesocket(connectSocket);

	RingBuffer *ringBuffer = (RingBuffer*)malloc(sizeof(RingBuffer));
	ringBuffer->head = 0;
	ringBuffer->tail = 0;
	ringBuffer->data = (char*)malloc(10000000);
	ringBuffer->alociranaMemorija = 10000000;
	ringBuffer->zauzetaMemorija = 0;
	ringBuffer->brojDozvoljenihPoruka = 10;
	ringBuffer->velicinaPrethodnihPoruka = (char*)malloc(10 * sizeof(int));

	CRITICAL_SECTION bufferAccess;
	InitializeCriticalSection(&bufferAccess);

	HANDLE Empty = CreateSemaphore(0, RING_SIZE, RING_SIZE, NULL);
	HANDLE Full = CreateSemaphore(0, 0, RING_SIZE, NULL);
	HANDLE FinishSignal = CreateSemaphore(0, 0, 2, NULL);

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
	parametri.FinishSignal = &FinishSignal;
	parametri.serverInfo = NULL;
	parametri.memorija = NULL;
	parametri.ugasiServer = &ugasiServer;
	parametri.izvrsavaSe2FC = &izvrsavaSe2FC;
	parametri.izvrsenUpdate = &izvrsenUpdate;

	hklijentkonekcija = CreateThread(NULL, 0, &NitZaPrihvatanjeZahtevaKlijenta, &parametri, 0, &klijentID);

	Parametri parametri2;
	parametri2.listenSocket = NULL;
	parametri2.ringBuffer = ringBuffer;
	parametri2.bufferAccess = &bufferAccess;
	parametri2.Empty = &Empty;
	parametri2.Full = &Full;
	parametri2.FinishSignal = &FinishSignal;
	parametri2.serverInfo = &glava;
	parametri2.memorija = &glavaMemorije;
	parametri2.ugasiServer = &ugasiServer;
	parametri2.izvrsavaSe2FC = &izvrsavaSe2FC;
	parametri2.izvrsenUpdate = &izvrsenUpdate;

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
		parametriServer.serverInfo = &temp;
		parametriServer.socket = NULL;
		parametriServer.memorija = &glavaMemorije;
		parametriServer.ugasiServer = &ugasiServer;
		parametriServer.izvrsavaSe2FC = &izvrsavaSe2FC;
		parametriServer.izvrsenUpdate = &izvrsenUpdate;

		if (integrityUpdate) {
			parametriServer.integrityUpdate = true;
			integrityUpdate = false;
		}
		else {
			parametriServer.integrityUpdate = false;
		}

		temp->hServerKonekcija = CreateThread(NULL, 0, &NitZaPrihvatanjeZahtevaServera, &parametriServer, 0, temp->serverID);

		Sleep(100);

		temp = temp->sledeci;
	}

	ParametriServer parametri3;
	parametri3.socket = &listenSocketServera;
	parametri3.serverInfo = &glava;
	strcpy(parametri3.adresa, adresa);
	parametri3.port = portServera;
	parametri3.memorija = &glavaMemorije;
	parametri3.ugasiServer = &ugasiServer;
	parametri3.izvrsavaSe2FC = &izvrsavaSe2FC;
	parametri3.izvrsenUpdate = &izvrsenUpdate;

	hServerKonekcija = CreateThread(NULL, 0, &NitZaOsluskivanjeServera, &parametri3, 0, &serverID);

	WaitForSingleObject(hklijentkonekcija, INFINITE);
	WaitForSingleObject(hklijentIzvrsavanje, INFINITE);
	WaitForSingleObject(hServerKonekcija, INFINITE);

	temp = glava;

	while (temp != NULL) {
		WaitForSingleObject(temp->hServerKonekcija, INFINITE);
		temp = temp->sledeci;
	}

	temp = glava;

	while (temp != NULL) {
		SAFE_DELETE_HANDLE(temp->hServerKonekcija);
		SAFE_DELETE_HANDLE(temp->primljenOdgovor);
		temp = temp->sledeci;
	}

	SAFE_DELETE_HANDLE(hklijentkonekcija);
	SAFE_DELETE_HANDLE(hklijentIzvrsavanje);
	SAFE_DELETE_HANDLE(hServerKonekcija);
	SAFE_DELETE_HANDLE(Empty);
	SAFE_DELETE_HANDLE(Full);
	SAFE_DELETE_HANDLE(FinishSignal);

	DeleteCriticalSection(&bufferAccess);

	closesocket(listenSocketKlijenta);
	closesocket(listenSocketServera);
	closesocket(acceptedSocket);
	WSACleanup(); 

	temp = glava;

	while ((temp = glava) != NULL) {
		glava = glava->sledeci;
		free(temp);
	}

	free(temp);

	Memorija *tempMem = glavaMemorije;

	while ((tempMem = glavaMemorije) != NULL) {
		glavaMemorije = glavaMemorije->sledeci;
		free(tempMem->ime);
		free(tempMem->prezime);
		free(tempMem);
	}

	free(tempMem);

	free(ringBuffer->data);
	free(ringBuffer->velicinaPrethodnihPoruka);
	free(ringBuffer);

	return 0;
}
