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
	bool integrityUpdate;
};

DWORD WINAPI NitZaPrihvatanjeZahtevaServera(LPVOID lpParam) {
	SOCKET connectSocket = INVALID_SOCKET;

	ParametriServer parametri = *(ParametriServer*)lpParam;
	Server_info *temp = *(parametri.serverInfo);

	int iResult;
	char recvbuf[512];

	bool izvrsenIntegrityUpdate = false;

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

	int velicina = strlen(parametri.adresa) + sizeof(int) * 2;
	poruka = (char*)malloc(velicina);

	if (parametri.integrityUpdate) {
		*(int*)poruka = 1;
		printf("Trazim update.\n");
	}
	else {
		*(int*)poruka = 0;
		izvrsenIntegrityUpdate = true;
	}
	
	*(int*)(poruka + 4) = parametri.port;

	for (int i = 0; i < strlen(parametri.adresa); i++) {
		*(poruka + 8 + i) = parametri.adresa[i];
	}

	iResult = PosaljiPoruku(&connectSocket, (char*)&velicina, 4);
	iResult = PosaljiPoruku(&connectSocket, poruka, velicina);


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
			Sleep(CLIENT_SLEEP_TIME);
			continue;
		}

		if (!izvrsenIntegrityUpdate) {
			izvrsenIntegrityUpdate = true;
			iResult = PrimiPoruku(&connectSocket, recvbuf, 8, NULL, false);

			int velicinaPoruke = *(int*)recvbuf;
			int brojClanova = *(int*)(recvbuf + 4);
			printf("Velicina poruke je %d.\n", velicinaPoruke);

			iResult = PrimiPoruku(&connectSocket, recvbuf, velicinaPoruke, NULL, true);

			char ime[20];
			char prezime[20];
			int velicinaPrethodnih = 0;

			for (int i = 0; i < brojClanova; i++) {
				int indeks = *(int*)(recvbuf + velicinaPrethodnih);
				int duzinaImena = *(int*)(recvbuf + velicinaPrethodnih + 4);
				int duzinaPrezimena = *(int*)(recvbuf + velicinaPrethodnih + 8);

				for (int j = 0; j < duzinaImena; j++) {
					ime[j] = *(recvbuf + velicinaPrethodnih + 12 + j);
				}
				ime[duzinaImena] = '\0';

				for (int j = 0; j < duzinaPrezimena; j++) {
					prezime[j] = *(recvbuf + velicinaPrethodnih + duzinaImena + 12 + j);
				}
				prezime[duzinaPrezimena] = '\0';

				Dodaj(&(*(parametri.memorija)), ime, prezime, indeks);

				velicinaPrethodnih += duzinaImena + duzinaPrezimena + 3 * sizeof(int);
			}

			printf("Izvrsen update!\n");
			Memorija *memTemp = *(parametri.memorija);

			printf("Memorija :\n");

			while (memTemp != NULL) {
				printf("Ime : %s\n", memTemp->ime);
				printf("Prezime : %s\n", memTemp->prezime);
				printf("Indeks : %d\n", memTemp->indeks);

				memTemp = memTemp->sledeci;
			}
		}
		else {
			iResult = PrimiPoruku(&connectSocket, recvbuf, 4, NULL, false);

			int velicinaPoruke = *(int*)recvbuf;
			printf("Velicina poruke je %d.\n", velicinaPoruke);

			iResult = PrimiPoruku(&connectSocket, recvbuf, velicinaPoruke, NULL, false);

			if (iResult > 4) {
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

					iResult = PosaljiPoruku(&(temp->socket), (char*)&velicina, 4);
					iResult = PosaljiPoruku(&(temp->socket), odgovor, velicina);

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
			else if (iResult > 0) { 
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
			else if (iResult == 0) {
				printf("Connection with server closed.\n");
				closesocket(connectSocket);
				break;
			}
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

		iResult = PrimiPoruku(&acceptedSocket, recvbuf, 4, NULL, false);
		velicina = *(int*)recvbuf;
		iResult = PrimiPoruku(&acceptedSocket, recvbuf, velicina, NULL, false);

		int integrityUpdate = *(int*)recvbuf;
		portServera = *(int*)(recvbuf + 4);

		for (int i = 0; i < velicina - 8; i++) {
			adresa[i] = *(recvbuf + 8 + i);
		}
		adresa[velicina - 8] = '\0';

		if (integrityUpdate == 1) {
			char *poruka;

			Memorija *temp = *(parametri.memorija);
			int brojKaraktera = 0;
			int brojClanova = 0;

			while (temp != NULL) {
				brojKaraktera += strlen(temp->ime);
				brojKaraktera += strlen(temp->prezime);
				brojClanova++;
				temp = temp->sledeci;
			}

			poruka = (char*)malloc(brojKaraktera + sizeof(int) * 3 * brojClanova);
			int velicinaPoruke = 0;
			temp = *(parametri.memorija);

			for (int i = 0; i < brojClanova; i++) {
				*((int*)(poruka + velicinaPoruke)) = temp->indeks;
				*((int*)(poruka + velicinaPoruke + 4)) = strlen(temp->ime);
				*((int*)(poruka + velicinaPoruke + 8)) = strlen(temp->prezime);

				for (int j = 0; j < strlen(temp->ime); j++) {
					*(poruka + velicinaPoruke + 12 + j) = temp->ime[j];
				}

				for (int j = 0; j < strlen(temp->prezime); j++) {
					*(poruka + strlen(temp->ime) + velicinaPoruke + 12 + j) = temp->prezime[j];
				}

				velicinaPoruke += strlen(temp->ime) + strlen(temp->prezime) + sizeof(int) * 3;

				temp = temp->sledeci;
			}

			char header[9];
			*(int*)header = velicinaPoruke;
			*(int*)(header + 4) = brojClanova;

			iResult = PosaljiPoruku(&acceptedSocket, header, 8);
			iResult = PosaljiPoruku(&acceptedSocket, poruka, velicinaPoruke);

			free(poruka);
		}

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
			parametriServer.serverInfo = parametri.serverInfo;
			parametriServer.socket = NULL;
			parametriServer.memorija = &(*(parametri.memorija));
			parametriServer.ugasiServer = parametri.ugasiServer;
			parametriServer.integrityUpdate = false;

			printf("Drugi server se povezao sa mnom!\n");

			(*parametri.serverInfo)->hServerKonekcija = CreateThread(NULL, 0, &NitZaPrihvatanjeZahtevaServera, &parametriServer, 0, (*parametri.serverInfo)->serverID);
		}

	} while (*(parametri.ugasiServer) != true);

	printf("\nUgasena nit: NitZaOsluskivanjeServera\n");
	closesocket(acceptedSocket);
	closesocket(listenSocket);

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

	bool ugasiServer2 = false;
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
					ugasiServer2 = true;
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
			iResult = PrimiPoruku(&acceptedSocket, recvbuf, 4, &ugasiServer2, false);
			if (ugasiServer2 == true) {
				*(parametri.ugasiServer) = true;
				ReleaseSemaphore(*FinishSignal, 2, NULL);
				continue;
			}
			if (iResult == 0) {
				konekcijaZatvorena = true;
				continue;
			}
			velicinaPoruke = *(int*)recvbuf;
			iResult = PrimiPoruku(&acceptedSocket, recvbuf, velicinaPoruke, &ugasiServer2, false);
			if (ugasiServer2 == true) {
				*(parametri.ugasiServer) = true;
				ReleaseSemaphore(*FinishSignal, 2, NULL);
				continue;
			}
			if (iResult == 0) {
				konekcijaZatvorena = true;
				continue;
			}


			recvbuf[velicinaPoruke] = '\0';
			printf("Message received from client: %s.\n", recvbuf);


			const int semaphore_num = 2;
			HANDLE semaphores[semaphore_num] = { *FinishSignal, *Empty };

			while (WaitForMultipleObjects(semaphore_num, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {

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

		printf("\nProcitana operacija iz kruznog bafera: %d\n", operacija);
		printf("Procitan indeks iz kruznog bafera: %d\n", indeks);
		printf("Procitano ime iz kruznog bafera: %s\n", ime);
		printf("Procitano prezime iz kruznog bafera: %s\n", prezime);

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
			iResult = PosaljiPoruku(&(temp->socket), (char*)&duzinaPoruke, 4);

			printf("%d\n", iResult);
			iResult = PosaljiPoruku(&(temp->socket), poruka, duzinaPoruke);

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
					iResult = PosaljiPoruku(&(temp2->socket), (char *)&duzinaPodatka, 4);
					iResult = PosaljiPoruku(&(temp2->socket), podatak, duzinaPodatka);


					temp2 = temp2->sledeci;
				}


				if (operacija == 1) {
					if (svePotvrdo) {
						Dodaj(&(*(parametri.memorija)), ime, prezime, indeks);

						Memorija *memTemp = *(parametri.memorija);

						printf("Memorija :\n");

						while (memTemp != NULL) {
							printf("Indeks : %d\n", memTemp->indeks);
							printf("Ime : %s\n", memTemp->ime);
							printf("Prezime : %s\n\n", memTemp->prezime);

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
							printf("Indeks : %d\n", memTemp->indeks);
							printf("Ime : %s\n", memTemp->ime);
							printf("Prezime : %s\n\n", memTemp->prezime);

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
							printf("Indeks : %d\n", memTemp->indeks);
							printf("Ime : %s\n", memTemp->ime);
							printf("Prezime : %s\n\n", memTemp->prezime);

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
					printf("Indeks : %d\n", memTemp->indeks);
					printf("Ime : %s\n", memTemp->ime);
					printf("Prezime : %s\n\n", memTemp->prezime);

					memTemp = memTemp->sledeci;
				}
			}
			else if (operacija == 2) {
				Obrisi(&(*(parametri.memorija)), indeks);

				Memorija *memTemp = *(parametri.memorija);

				printf("Memorija :\n");

				while (memTemp != NULL) {
					printf("Indeks : %d\n", memTemp->indeks);
					printf("Ime : %s\n", memTemp->ime);
					printf("Prezime : %s\n\n", memTemp->prezime);

					memTemp = memTemp->sledeci;
				}
			}
			else if (operacija == 3) {
				Izmeni(&(*(parametri.memorija)), ime, prezime, indeks);

				Memorija *memTemp = *(parametri.memorija);

				printf("Memorija :\n");

				while (memTemp != NULL) {
					printf("Indeks : %d\n", memTemp->indeks);
					printf("Ime : %s\n", memTemp->ime);
					printf("Prezime : %s\n\n", memTemp->prezime);

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