#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Struktura_clan.h"
#include "Metode_mastera.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "10000"
#define SERVER_SLEEP_TIME 1000
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}



int  main(void)
{
	Clan *glava;
	glava = (Clan*)malloc(sizeof(Clan));
	if (glava == NULL) {
		return 1;
	}

	Inicijalizacija(&glava);

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

	printf("Master initialized, waiting for clients.\n");

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(acceptedSocket, FIONBIO, &nonBlockingMode);
	iResult = ioctlsocket(listenSocket, FIONBIO, &nonBlockingMode);

	int random_brojac = 0;

	do
	{

		iResult = Selekt(&listenSocket);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			continue;
		}

		if (iResult == 0)
		{
			//printf("Ceka se veza sa serverom...\n");
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

		bool primljeno = false;
		bool klijent = false;

		//Primanje flega da znamo da li je klijent(0) ili server(1)
		do {

			iResult = Selekt(&acceptedSocket);

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

			int fleg = 0;

			iResult = recv(acceptedSocket, recvbuf, 4, 0);
			if (iResult > 0) {
				fleg = *(int*)recvbuf;
				primljeno = true;
				printf("Prikljucio se: %d\n", fleg);
				//0 znaci da je klijent
				if (fleg == 0) {
					klijent = true;
				}
			}
		} while (!primljeno);

		int brojClanova = BrojClanova(glava);
		int velicinaPoruke = 0;
		char* poruka;

		if (brojClanova <= random_brojac) {
			random_brojac = 0;
		}

		if (klijent == true) {
			if (!praznaLista) {
				bool praznaPoruka = true;

				Clan *temp = glava;
				for (int i = 1; i <= random_brojac; i++) {
					temp = glava->sledeci;
				}
				random_brojac++;

				poruka = (char*)malloc(strlen(temp->ipAdresa) + sizeof(int));

				*(int*)poruka = temp->portKlijenta;

				for (int i = 0; i < strlen(temp->ipAdresa); i++) {
					*(poruka + 4 + i) = temp->ipAdresa[i];
				}

				velicinaPoruke += strlen(temp->ipAdresa) + sizeof(int);
				brojClanova = 0;
			}
			else {
				velicinaPoruke = 7;
				poruka = (char*)malloc(velicinaPoruke);
				strcpy(poruka, "prazno");
			}
		}  //SERVER
		else {
			if (!praznaLista) {
				bool praznaPoruka = true;

				Clan *temp = glava;

				int duzinaAdresa = DuzinaAdresa(glava);
				poruka = (char*)malloc(duzinaAdresa + brojClanova * 2 * sizeof(int));

				int velicinaElementa = 0;

				for (int i = 0; i < brojClanova; i++) {

					*((int*)(poruka + velicinaElementa))= temp->portServera;
					*((int*)(poruka + velicinaElementa + 4)) = strlen(temp->ipAdresa);

					for (int j = 0; j < strlen(temp->ipAdresa); j++) {
						*((poruka + velicinaElementa) + 8 + j) = temp->ipAdresa[j];
					}

					velicinaPoruke += 2 * sizeof(int) + strlen(temp->ipAdresa);
					velicinaElementa += strlen(temp->ipAdresa) + 2 * sizeof(int);

					temp = temp->sledeci;
				}
			}
			else {
				velicinaPoruke = 7;
				brojClanova = 0;
				poruka = (char*)malloc(velicinaPoruke);
				strcpy(poruka, "prazno");
			}
		}

		char prvaPoruka[9];
		*(int*)prvaPoruka = velicinaPoruke;
		*(int*)(prvaPoruka + 4) = brojClanova;
		iResult = send(acceptedSocket, prvaPoruka, 8, 0);
		iResult = send(acceptedSocket, poruka, velicinaPoruke, 0);

		free(poruka);

		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket);
			WSACleanup();
			return 1;
		}

		primljeno = false;

		do {
			//ako je klijent onda ne prima ip adresu i port
			if (klijent == true) {
				break;
			}

			iResult = Selekt(&acceptedSocket);

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

			int velicina = 0, portServera = 0, portKlijenta = 0;
			char adresa[20];

			iResult = recv(acceptedSocket, recvbuf, 4, 0);
			if (iResult > 0) {
				velicina = *(int*)recvbuf;
				printf("Velicina je : %d\n", velicina);

				iResult = recv(acceptedSocket, recvbuf, velicina, 0);
				if (iResult > 0) {
					printf("Poruka je : %s\n", recvbuf);
					portServera = *(int*)recvbuf;                //Port servera je prvi u recfbuf
					portKlijenta = *((int*)(recvbuf + 4));
					
					for (int i = 0; i < velicina - 2 * 4; i++) {
						adresa[i] = *(recvbuf + 2 * 4 + i);
					}
					adresa[velicina - 2 * 4] = '\0';

					printf("Port servera je : %d\n", portServera);
					printf("Port klijenta je : %d\n", portKlijenta);
					printf("Adresa je : %s\n", adresa);
					primljeno = true;

					Dodaj(&glava, adresa, portServera, portKlijenta);
					praznaLista = false;

					Clan *temp = glava;

					printf("Lista :\n");
					while (temp != NULL) {
						printf("Adresa : %s\n", temp->ipAdresa);
						printf("Port Servera : %d\n", temp->portServera);
						printf("Port Klijenta : %d\n\n", temp->portKlijenta);
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

