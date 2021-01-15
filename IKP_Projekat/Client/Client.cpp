#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Metode_klijenta.h"

#define DEFAULT_BUFLEN 512
#define MASTER_PORT 10000
#define CLIENT_SLEEP_TIME 1000
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}
#define BUFFER_SIZE 512
#pragma warning(disable:4996)



int main(void)
{
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptedSocket = INVALID_SOCKET;
	SOCKET connectSocket = INVALID_SOCKET;
	SOCKET connectSocket2 = INVALID_SOCKET;

	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	//char messageToSend[BUFFER_SIZE];

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

	bool izvrsenUnos = false;
	char IPadresaMastera[20];

	while (!izvrsenUnos) {
		printf("1.Master na lokalnom racunaru\n");
		printf("2.Master na drugom racunaru\n");
		char unos[10];
		scanf("%s", unos);
		unos[1] = '\0';
		if (strcmp(unos, "1") == 0) {
			izvrsenUnos = true;
			strcpy(IPadresaMastera, "127.0.0.1");
		}
		else if (strcmp(unos, "2") == 0) {
			izvrsenUnos = true;
			printf("Unesi IP adresu mastera : \n");
			scanf("%s", IPadresaMastera);
		}
	}

	sockaddr_in serverAddress, myAddress;
	serverAddress.sin_family = AF_INET;
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

	int poruka = 0;

	PosaljiPoruku(&connectSocket, (char*)&poruka, 4);

	int dobijenPort = 0;
	char dobijenaAdresa[20];

	iResult = PrimiPoruku(&connectSocket, recvbuf, 8);
	int velicinaPoruke = *(int*)recvbuf;
	do {
		iResult = PrimiPoruku(&connectSocket, recvbuf, velicinaPoruke);
	} while (iResult < 8);

	dobijenPort = *(int*)recvbuf;

	for (int i = 0; i < velicinaPoruke - 4; i++) {
		dobijenaAdresa[i] = *(recvbuf + 4 + i);
	}
	dobijenaAdresa[velicinaPoruke - 4] = '\0';

	printf("Port je : %d\n", dobijenPort);
	printf("Adresa je : %s\n", dobijenaAdresa);


	connectSocket2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connectSocket2 == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(dobijenaAdresa);
	serverAddress.sin_port = htons(dobijenPort);

	if (connect(connectSocket2, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		int liI = getchar();
		closesocket(connectSocket2);
		WSACleanup();
	}

	bool ugasi = false;
	do {
		int operacija = 0;
		char ime[1000];
		ime[0] = '\0';
		char prezime[1000];
		prezime[0] = '\0';
		int indeks;
		char *zahtev;
		int duzinaZahteva = 0;

		int brojTestova = 0;
		bool ugasi = false;

		int br = 0;

		do {
			printf("-------------------- MENU --------------------\n");
			printf("1. Dodaj studenta\n");
			printf("2. Obrisi studenta\n");
			printf("3. Izmeni studenta\n");
			printf("4. Stres test 1 (1kb/0.2s)\n");
			printf("5. Stres test 2 (1mb/0.1s)\n");
			printf("6. Stres test 3 (100b/0.2s)\n");
			printf("7. Ugasi klijenta\n");
			scanf_s("%d", &operacija);
			switch (operacija) {
			case 1:
			{
				printf("\nUnesiste indeks studenta: ");
				scanf("%d", &indeks);
				printf("\nUnesiste ime studenta: ");
				scanf("%s", ime);
				printf("\nUnesiste prezime studenta: ");
				scanf("%s", prezime);
				break;
			}
			case 2:
			{
				printf("\nUnesiste indeks studenta: ");
				scanf_s("%d", &indeks);
				break;
			}
			case 3:
			{
				printf("\nUnesiste indeks studenta: ");
				scanf_s("%d", &indeks);
				printf("\nUnesiste ime studenta: ");
				scanf("%s", ime);
				printf("\nUnesiste prezime studenta: ");
				scanf("%s", prezime);
				break;
			}
			case 4:
				char ime[492];
				char prezime[492];

				for (int i = 0; i < 491; i++) {
					ime[i] = 'I';
					prezime[i] = 'P';
				}

				ime[491] = '\0';
				prezime[491] = '\0';

				while (!ugasi) {
					if (_kbhit()) {
						char c = _getch();
						if (c == 'q') {
							ugasi = true;
							break;
						}
					}

					indeks = brojTestova;

					duzinaZahteva = 1000;
					
					// Redosled: operacija(1,2,3), duzina imena, duzina prezimena, indeks, ime, prezime
					zahtev = (char*)malloc(duzinaZahteva);
					*(int*)zahtev = 1;
					*(int*)(zahtev + 4) = 492;
					*(int*)(zahtev + 8) = 492;
					*(int*)(zahtev + 12) = indeks;
					if (ime[0] != '\0') {
						memcpy(zahtev + 16, ime, 492);
					}
					if (prezime[0] != '\0') {
						memcpy(zahtev + 16 + 492, prezime, 492);
					}

					PosaljiPoruku(&connectSocket2, (char *)&duzinaZahteva, 4);

					PosaljiPoruku(&connectSocket2, zahtev, duzinaZahteva);

					free(zahtev);
					Sleep(200);
					printf("Poslat zahtev %d.\n", indeks);
					brojTestova++;
				}
				operacija = 0;
				ugasi = false;
				break;
			case 5:
				char ime1[499992];
				char prezime1[499992];

				for (int i = 0; i < 499992; i++) {
					ime1[i] = 'I';
					prezime1[i] = 'P';
				}

				while (!ugasi) {
					if (_kbhit()) {
						char c = _getch();
						if (c == 'q') {
							ugasi = true;
							break;
						}
					}

					indeks = brojTestova;

					duzinaZahteva = 1000000;

					// Redosled: operacija(1,2,3), duzina imena, duzina prezimena, indeks, ime, prezime
					zahtev = (char*)malloc(duzinaZahteva);
					*(int*)zahtev = 1;
					*(int*)(zahtev + 4) = 499992;
					*(int*)(zahtev + 8) = 499992;
					*(int*)(zahtev + 12) = indeks;
					memcpy(zahtev + 16, ime1, 499992);
					memcpy(zahtev + 16 + 499992, prezime1, 499992);

					PosaljiPoruku(&connectSocket2, (char *)&duzinaZahteva, 4);

					PosaljiPoruku(&connectSocket2, zahtev, duzinaZahteva);

					free(zahtev);
					Sleep(200);
					printf("Poslat zahtev %d.\n", indeks);
					brojTestova++;
				}
				operacija = 0;
				ugasi = false;
				break;
			case 6:
				char ime2[42];
				char prezime2[42];

				for (int i = 0; i < 42; i++) {
					ime2[i] = 'I';
					prezime2[i] = 'P';
				}

				while (!ugasi) {
					if (_kbhit()) {
						char c = _getch();
						if (c == 'q') {
							ugasi = true;
							break;
						}
					}

					indeks = brojTestova;

					duzinaZahteva = 100;

					// Redosled: operacija(1,2,3), duzina imena, duzina prezimena, indeks, ime, prezime
					zahtev = (char*)malloc(duzinaZahteva);
					*(int*)zahtev = 1;
					*(int*)(zahtev + 4) = 42;
					*(int*)(zahtev + 8) = 42;
					*(int*)(zahtev + 12) = indeks;
					if (ime[0] != '\0') {
						memcpy(zahtev + 16, ime2, 42);
					}
					if (prezime[0] != '\0') {
						memcpy(zahtev + 16 + 42, prezime2, 42);
					}

					PosaljiPoruku(&connectSocket2, (char *)&duzinaZahteva, 4);

					PosaljiPoruku(&connectSocket2, zahtev, duzinaZahteva);

					free(zahtev);
					Sleep(200);
					printf("Poslat zahtev %d.\n", indeks);
					brojTestova++;
				}
				operacija = 0;
				ugasi = false;
				break;
			case 7:
				ugasi = true;
				break;
			default:
				operacija = 0;
				break;
			}
		} while (operacija == 0);

		if (ugasi == true) {
			break;
		}

		int duzinaImena = 0;
		int duzinaPrezimena = 0;

		if (ime[0] != '\0') {
			duzinaImena = strlen(ime);
		}
		if (prezime[0] != '\0') {
			duzinaPrezimena = strlen(prezime);
		}
		duzinaZahteva = duzinaImena + duzinaPrezimena + 4 * sizeof(int);
		// Redosled: operacija(1,2,3), duzina imena, duzina prezimena, indeks, ime, prezime
		zahtev = (char*)malloc(duzinaZahteva);
		*(int*)zahtev = operacija;
		*(int*)(zahtev + 4) = duzinaImena;
		*(int*)(zahtev + 8) = duzinaPrezimena;
		*(int*)(zahtev + 12) = indeks;
		if (ime[0] != '\0') {
			memcpy(zahtev + 16, ime, duzinaImena);
		}
		if (prezime[0] != '\0') {
			memcpy(zahtev + 16 + duzinaImena, prezime, duzinaPrezimena);
		}

		PosaljiPoruku(&connectSocket2, (char *)&duzinaZahteva, 4);

		PosaljiPoruku(&connectSocket2, zahtev, duzinaZahteva);

		free(zahtev);
	} while (true);
	
	closesocket(connectSocket);
	closesocket(connectSocket2);
	WSACleanup();

	return 0;
}

