#pragma once
#define CLIENT_SLEEP_TIME 1000
#include "Struktura_Server_info.h"
#include "Struktura_memorija.h"

struct Parametri {
	SOCKET *listenSocket;
	RingBuffer *ringBuffer;
	CRITICAL_SECTION *bufferAccess;
	HANDLE *Empty;
	HANDLE *Full;
	HANDLE *FinishSignal;
	Server_info **serverInfo;
	Memorija **memorija;
	bool *ugasiServer;
};

struct ParametriServer {
	Server_info **serverInfo;
	SOCKET *socket;
	Memorija **memorija;
	int port;
	char adresa[20];
	bool *ugasiServer;
};

DWORD WINAPI NitZaPrihvatanjeZahtevaServera(LPVOID lpParam) {
	SOCKET connectSocket = INVALID_SOCKET;

	ParametriServer parametri = *(ParametriServer*)lpParam;
	Server_info *temp = *(parametri.serverInfo);

	int iResult;
	char recvbuf[512];

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

	iResult = send(connectSocket, (char*)&velicina, 4, 0);
	iResult = send(connectSocket, poruka, velicina, 0);

	free(poruka);

	int duzinaImena = 0, duzinaPrezimena = 0, indeks = 0;
	char ime[20];
	char prezime[20];
	char operacija[20];
	bool izvrsenaFazaPrepare = false;
	int brojacOtkaza = 0;

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
			if (iResult > 4)
			{
				if (!izvrsenaFazaPrepare) {
					printf("Primljena poruka od server-a: %s.\n", recvbuf);

					int primljenaOperacija = *(int*)recvbuf;

					switch (primljenaOperacija) {
					case 1:
						velicina = 5;
						memcpy(operacija, "upis", velicina);
						break;
					case 2:
						velicina = 9;
						memcpy(operacija, "brisanje", velicina);
						break;
					case 3:
						velicina = 7;
						memcpy(operacija, "izmena", velicina);
						break;
					}

					operacija[velicina] = '\0';

					printf("Operacija je : %s\n", operacija);

					duzinaImena = *(int*)(recvbuf + 4);
					duzinaPrezimena = *(int*)(recvbuf + 8);
					indeks = *(int*)(recvbuf + 12);
					memcpy(ime, recvbuf + 16, duzinaImena);
					ime[duzinaImena] = '\0';
					memcpy(prezime, recvbuf + 16 + duzinaImena, duzinaPrezimena);
					prezime[duzinaPrezimena] = '\0';

					char *odgovor;
					velicina = 4;
					odgovor = (char*)malloc(velicina);

					brojacOtkaza++;

					if (brojacOtkaza % 3 == 0) {
						*(int*)odgovor = 0;
					}
					else {
						*(int*)odgovor = 1;
					}

					iResult = send(temp->socket, (char*)&velicina, 4, 0);
					iResult = send(temp->socket, odgovor, velicina, 0);
					free(odgovor);
					izvrsenaFazaPrepare = true;
				}
				else {
					printf("Primljena poruka od server-a: %s.\n", recvbuf);

					char odgovor[20];
					memcpy(odgovor, recvbuf, velicinaPoruke);
					odgovor[velicinaPoruke] = '\0';

					if (strcmp(odgovor, "commit") == 0) {



						if (strcmp(operacija, "upis") == 0) {
							Dodaj(&(*(parametri.memorija)), ime, prezime, indeks);
						}
						else if (strcmp(operacija, "brisanje") == 0) {
							Obrisi(&(*(parametri.memorija)), indeks);
						}
						else if (strcmp(operacija, "izmena") == 0) {
							Izmeni(&(*(parametri.memorija)), ime, prezime, indeks);
						}

						Memorija *memTemp = *(parametri.memorija);

						printf("Memorija :\n");

						while (memTemp != NULL) {
							printf("Ime : %s\n", memTemp->ime);
							printf("Prezime : %s\n", memTemp->prezime);
							printf("Indeks : %d\n", memTemp->indeks);

							memTemp = memTemp->sledeci;
						}
					}

					duzinaImena = 0;
					duzinaPrezimena = 0;
					indeks = 0;
					memset(ime, 0, sizeof(ime));
					memset(prezime, 0, sizeof(prezime));
					memset(operacija, 0, sizeof(operacija));

					izvrsenaFazaPrepare = false;
				}
			}
			else if(iResult > 0) 
			{
				printf("Primljena poruka od server-a: %s.\n", recvbuf);

				int odgovor = *(int*)recvbuf;

				if (odgovor == 1) {
					strcpy(temp->messageBox, "spreman");
				}
				else {
					strcpy(temp->messageBox, "nijeSpreman");
				}

				ReleaseSemaphore(temp->primljenOdgovor, 1, NULL);
				printf("Oslobodjen semafor za port : %d\n", temp->port);
			}
		}
		else if (iResult == 0)
		{
			printf("Connection with server closed.\n");
			closesocket(connectSocket);
			break;
		}
	} while (*(parametri.ugasiServer) != true);
	printf("\nUgasena nit: NitZaPrihvatanjeZahtevaServera\n");
	closesocket(connectSocket);

	return 0;
}

DWORD WINAPI NitZaOsluskivanjeServera(LPVOID lpParam) {
	SOCKET acceptedSocket = INVALID_SOCKET;

	int iResult;
	char recvbuf[512];

	ParametriServer parametri = *(ParametriServer*)lpParam;
	SOCKET listenSocket = *(parametri.socket);

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

		printf("Neko se povezao.\n");

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

		Server_info *glava = *(parametri.serverInfo);
		bool postoji = false;


		while (glava != NULL) {
			if (strcmp(glava->ipAdresa, adresa) == 0 && glava->port == portServera) {
				postoji = true;
				printf("Postoji!\n");
				glava->socket = acceptedSocket;
				break;
			}
			glava = glava->sledeci;
		}

		if (!postoji) {
			Dodaj(&(*(parametri.serverInfo)), adresa, portServera);

			(*parametri.serverInfo)->socket = acceptedSocket;

			ParametriServer parametriServer;
			strcpy(parametriServer.adresa, parametri.adresa);
			parametriServer.port = parametri.port;
			parametriServer.serverInfo = &(*(parametri.serverInfo));
			parametriServer.socket = NULL;
			parametriServer.memorija = &(*(parametri.memorija));
			parametriServer.ugasiServer = parametri.ugasiServer;

			printf("Drugi server se povezao sa mnom!\n");

			(*parametri.serverInfo)->hServerKonekcija = CreateThread(NULL, 0, &NitZaPrihvatanjeZahtevaServera, &parametriServer, 0, (*parametri.serverInfo)->serverID);
		}

	} while (*(parametri.ugasiServer) != true);

	printf("\nUgasena nit: NitZaOsluskivanjeServera\n");
	closesocket(listenSocket);
	//WSACleanup();

	return 0;
}

DWORD WINAPI NitZaPrihvatanjeZahtevaKlijenta(LPVOID lpParam)
{
	SOCKET acceptedSocket = INVALID_SOCKET;

	int iResult;
	char recvbuf[512];

	Parametri parametri = *(Parametri*)lpParam;

	SOCKET listenSocket = *(parametri.listenSocket);
	RingBuffer *ringBuffer = parametri.ringBuffer;
	CRITICAL_SECTION *bufferAccess = parametri.bufferAccess;
	HANDLE *Empty = parametri.Empty;
	HANDLE *Full = parametri.Full;
	HANDLE *FinishSignal = parametri.FinishSignal;

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
		bool konekcijaZatvorena = false;
		int velicinaPoruke = 0;

		iResult = Selekt(&listenSocket);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			continue;
		}

		if (iResult == 0)
		{
			if (_kbhit()) {
				char c = _getch();
				if (c == 'q') {
					*(parametri.ugasiServer) = true;
					ReleaseSemaphore(*FinishSignal, 2, NULL);
				}
			}
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
			iResult = Selekt(&acceptedSocket);

			if (iResult == SOCKET_ERROR)
			{
				fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
				continue;
			}

			if (iResult == 0)
			{
				if (_kbhit()) {
					char c = _getch();
					if (c == 'q') {
						*(parametri.ugasiServer) = true;
						ReleaseSemaphore(*FinishSignal, 2, NULL);
					}
				}
				//printf("Ceka se veza sa klijentom...\n");
				Sleep(1000);
				continue;
			}
			
			iResult = 1;
			iResult = recv(acceptedSocket, recvbuf, 4, 0);
			if (iResult > 0)
			{
				velicinaPoruke = *(int*)recvbuf;
				printf("**************************: %d\n", velicinaPoruke);
				iResult = Selekt(&acceptedSocket);

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
				iResult = recv(acceptedSocket, recvbuf, velicinaPoruke, 0);
				if (iResult > 0) {
					recvbuf[velicinaPoruke] = '\0';
					printf("Message received from client: %s.\n", recvbuf);


					const int semaphore_num = 2;
					HANDLE semaphores[semaphore_num] = { *FinishSignal, *Empty };

					while (WaitForMultipleObjects(semaphore_num, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
					//while (WaitForSingleObject(*Empty, INFINITE) == WAIT_OBJECT_0) {


						EnterCriticalSection(bufferAccess);

						ringBufPutChar(ringBuffer, recvbuf, velicinaPoruke);

						LeaveCriticalSection(bufferAccess);

						//printf("****************************: %s\n", ringBufGetChar(ringBuffer));
						int operacija = *((int*)recvbuf);
						printf("****************************: %d\n", operacija);
						char *p = recvbuf;
						int dr = *((int*)p);
						printf("****************************: %d\n", dr);

						ReleaseSemaphore(*Full, 1, NULL);

						break;
					}
				}
			}
			else if (iResult == 0)
			{
				printf("Connection with client closed.\n");
				konekcijaZatvorena = true;
				closesocket(acceptedSocket);
			}
			else
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(acceptedSocket);
			}
		} while (konekcijaZatvorena == false && *(parametri.ugasiServer) != true);

	} while (*(parametri.ugasiServer) != true);


	printf("\nUgasena nit: NitZaPrihvatanjeZahtevaKlijenta\n");
	closesocket(acceptedSocket);
	//WSACleanup();

	return 0;
}



DWORD WINAPI NitZaIzvrsavanjeZahtevaKlijenta(LPVOID lpParam)
{
	Parametri parametri = *(Parametri*)lpParam;
	RingBuffer *ringBuffer = parametri.ringBuffer;
	CRITICAL_SECTION *bufferAccess = parametri.bufferAccess;
	HANDLE *Empty = parametri.Empty;
	HANDLE *Full = parametri.Full;
	HANDLE *FinishSignal = parametri.FinishSignal;

	const int semaphore_num = 2;
	HANDLE semaphores[semaphore_num] = { *FinishSignal, *Full };

	while (WaitForMultipleObjects(semaphore_num, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {

		EnterCriticalSection(bufferAccess);
		// Citanje kruznog bafera;
		char *zahtev;
		zahtev = ringBufGetChar(ringBuffer);
		LeaveCriticalSection(bufferAccess);
		// Ispis na konzolu;
		printf("\nIzvadjen zahtev iz buffera: %s\n", zahtev);

		int operacija = *((int*)zahtev);
		int duzinaImena = *(int*)(zahtev + 4);
		int duzinaPrezimena = *(int*)(zahtev + 8);
		int indeks = *(int*)(zahtev + 12);
		char ime[20];
		char prezime[20];
		memcpy(ime, zahtev + 16, duzinaImena);
		ime[duzinaImena] = '\0';
		memcpy(prezime, zahtev + 16 + duzinaImena, duzinaPrezimena);
		prezime[duzinaPrezimena] = '\0';

		printf("\n***********************************: %d\n", operacija);
		printf("-----------------------------------: %d\n", duzinaImena);
		printf("++++++++++++++++++++++++++++++++++++: %d\n", duzinaPrezimena);
		printf("++++++++++++++++++++++++++++++++++++: %s\n", ime);
		printf("++++++++++++++++++++++++++++++++++++: %s\n", prezime);

		ReleaseSemaphore(*Empty, 1, NULL);



		Server_info *temp = *(parametri.serverInfo);
		int iResult = 0;
		char *poruka;

		int duzinaPoruke = duzinaImena + duzinaPrezimena + sizeof(int) * 4;
		poruka = (char*)malloc(duzinaPoruke);
		*(int*)poruka = operacija;
		*(int*)(poruka + 4) = duzinaImena;
		*(int*)(poruka + 8) = duzinaPrezimena;
		*(int*)(poruka + 12) = indeks;
		memcpy(poruka + 16, ime, duzinaImena);
		memcpy(poruka + 16 + duzinaImena, prezime, duzinaPrezimena);

		printf("%s\n", poruka);

		HANDLE *semafori;
		semafori = (HANDLE*)malloc(BrojClanova(temp) * sizeof(HANDLE));

		int brojSemafora = 0;

		while (temp != NULL) {
			iResult = send(temp->socket, (char*)&duzinaPoruke, 4, 0);
			printf("%d\n", iResult);
			iResult = send(temp->socket, poruka, duzinaPoruke, 0);
			printf("%d\n", iResult);
			printf("Poslato.\n");

			semafori[brojSemafora] = temp->primljenOdgovor;
			brojSemafora++;

			temp = temp->sledeci;
		}

		free(poruka);

		if (brojSemafora != 0) {
			DWORD vrednost = WaitForMultipleObjects(brojSemafora, semafori, TRUE, INFINITE);
			bool docekaniSemafori = false;

			if (brojSemafora == 1) {
				if (vrednost >= WAIT_OBJECT_0) {
					docekaniSemafori = true;
				}
			}
			else if(vrednost >= WAIT_OBJECT_0 && vrednost < (WAIT_OBJECT_0 + brojSemafora - 1)) {
				docekaniSemafori = true;
			}
			if (docekaniSemafori) {
				printf("Svi su odgovorili.\n");

				Server_info *temp1 = *(parametri.serverInfo);
				int brojPotvrdnihOdgovora = 0;

				while (temp1 != NULL) {
					if (strcmp("spreman", temp1->messageBox) == 0) {
						brojPotvrdnihOdgovora++;
					}
					else {
						break;
					}

					temp1 = temp1->sledeci;
				}

				char *podatak;
				int duzinaPodatka;
				bool svePotvrdo = false;

				if (brojSemafora == brojPotvrdnihOdgovora) {
					printf("Svi su potvrdni.\n");
					duzinaPodatka = 6;
					podatak = (char*)malloc(duzinaPodatka);
					memcpy(podatak, "commit", duzinaPodatka);
					svePotvrdo = true;
				}
				else {
					printf("Postoji negativan odgovor.\n");
					duzinaPodatka = 9;
					podatak = (char*)malloc(duzinaPodatka);
					memcpy(podatak, "rolleback", duzinaPodatka);
				}

				Server_info *temp2 = *(parametri.serverInfo);

				while (temp2 != NULL) {
					iResult = send(temp2->socket, (char *)&duzinaPodatka, 4, 0);
					iResult = send(temp2->socket, podatak, duzinaPodatka, 0);

					temp2 = temp2->sledeci;
				}

				if (operacija == 1) {
					if (svePotvrdo) {
						Dodaj(&(*(parametri.memorija)), ime, prezime, indeks);

						Memorija *memTemp = *(parametri.memorija);

						printf("Memorija :\n");

						while (memTemp != NULL) {
							printf("Ime : %s\n", memTemp->ime);
							printf("Prezime : %s\n", memTemp->prezime);
							printf("Indeks : %d\n", memTemp->indeks);

							memTemp = memTemp->sledeci;
						}
					}
				}
				else if (operacija == 2) {
					if (svePotvrdo) {
						Obrisi(&(*(parametri.memorija)), indeks);

						Memorija *memTemp = *(parametri.memorija);

						printf("Memorija :\n");

						while (memTemp != NULL) {
							printf("Ime : %s\n", memTemp->ime);
							printf("Prezime : %s\n", memTemp->prezime);
							printf("Indeks : %d\n", memTemp->indeks);

							memTemp = memTemp->sledeci;
						}
					}
				}
				else if (operacija == 3) {
					if (svePotvrdo) {
						Izmeni(&(*(parametri.memorija)), ime, prezime, indeks);

						Memorija *memTemp = *(parametri.memorija);

						printf("Memorija :\n");

						while (memTemp != NULL) {
							printf("Ime : %s\n", memTemp->ime);
							printf("Prezime : %s\n", memTemp->prezime);
							printf("Indeks : %d\n", memTemp->indeks);

							memTemp = memTemp->sledeci;
						}
					}
				}
				free(podatak);
			}

			free(semafori);
		}
		else {
			if (operacija == 1) {
				Dodaj(&(*(parametri.memorija)), ime, prezime, indeks);

				Memorija *memTemp = *(parametri.memorija);

				printf("Memorija :\n");

				while (memTemp != NULL) {
					printf("Ime : %s\n", memTemp->ime);
					printf("Prezime : %s\n", memTemp->prezime);
					printf("Indeks : %d\n", memTemp->indeks);

					memTemp = memTemp->sledeci;
				}
			}
			else if (operacija == 2) {
				Obrisi(&(*(parametri.memorija)), indeks);

				Memorija *memTemp = *(parametri.memorija);

				printf("Memorija :\n");

				while (memTemp != NULL) {
					printf("Ime : %s\n", memTemp->ime);
					printf("Prezime : %s\n", memTemp->prezime);
					printf("Indeks : %d\n", memTemp->indeks);

					memTemp = memTemp->sledeci;
				}
			}
			else if (operacija == 3) {
				Izmeni(&(*(parametri.memorija)), ime, prezime, indeks);

				Memorija *memTemp = *(parametri.memorija);

				printf("Memorija :\n");

				while (memTemp != NULL) {
					printf("Ime : %s\n", memTemp->ime);
					printf("Prezime : %s\n", memTemp->prezime);
					printf("Indeks : %d\n", memTemp->indeks);

					memTemp = memTemp->sledeci;
				}
			}
		}
		printf("Izasao.\n");

		if (*parametri.ugasiServer == true) {
			break;
		}
	}

	printf("\nUgasena nit: NitZaIzvrsavanjeZahtevaKlijenta\n");

	return 0;
}