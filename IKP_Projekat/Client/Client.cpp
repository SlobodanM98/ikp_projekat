#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
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

	int poruka = 0;

	iResult = send(connectSocket, (char*)&poruka, 4, 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(acceptedSocket);
		WSACleanup();
		return 1;
	}


	bool primljenaPoruka = false;
	int dobijenPort = 0;

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
			Sleep(CLIENT_SLEEP_TIME);
			continue;
		}

		iResult = recv(connectSocket, recvbuf, 8, 0);
		if (iResult > 0)
		{
			int velicinaPoruke = *(int*)recvbuf;
			printf("Velicina poruke je %d.\n", velicinaPoruke);
			iResult = recv(connectSocket, recvbuf, velicinaPoruke, 0);
			if (iResult > 0)
			{
				printf("Primljena poruka od master-a: %s.\n", recvbuf);
				primljenaPoruka = true;

				if (iResult > 7) {
					dobijenPort = *(int*)recvbuf;
					char dobijenaAdresa[20];

					for (int i = 0; i < velicinaPoruke - 4; i++) {
						dobijenaAdresa[i] = *(recvbuf + 4 + i);
					}
					dobijenaAdresa[velicinaPoruke - 4] = '\0';

					printf("Port je : %d\n", dobijenPort);
					printf("Adresa je : %s\n", dobijenaAdresa);
				}

			}
		}
		else if (iResult == 0)
		{
			printf("Connection with master closed.\n");
			closesocket(connectSocket);
			break;
		}
	} while (!primljenaPoruka && dobijenPort == 0);

	

	connectSocket2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connectSocket2 == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(dobijenPort);

	if (connect(connectSocket2, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		int liI = getchar();
		closesocket(connectSocket2);
		WSACleanup();
	}
	int operacija = 0;
	char ime[20];
	ime[0] = '\0';
	char prezime[20];
	prezime[0] = '\0';
	int indeks;
	char *zahtev;
	int duzinaZahteva = 0;

	do {
		printf("-------------------- MENU --------------------\n");
		printf("1. Dodaj studenta\n");
		printf("2. Obrisi studenta\n");
		printf("3. Izmeni studenta\n");
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
		default:
		{
			operacija = 0;
			break;
		}
		}
	} while (operacija == 0);

	int duzinaImena = 0;
	int duzinaPrezimena = 0;

	if (ime[0] != '\0') {
		duzinaImena = strlen(ime);
	}
	if (prezime[0] != '\0') {
		duzinaPrezimena = strlen(prezime);
	}
	/*printf("%s", strlen(prezime));
	printf("%d", strlen(ime));

	printf("uspesno");
	while(1) {}*/

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
	//int duzinaZahteva = 11;
	iResult = send(connectSocket2, (char *)&duzinaZahteva, 4, 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(acceptedSocket);
		WSACleanup();
		return 1;
	}
	//char p[12] = "Hello World";
	iResult = send(connectSocket2, zahtev, duzinaZahteva, 0);

	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(acceptedSocket);
		WSACleanup();
		return 1;
	}

	free(zahtev);

	while(1) {}

	closesocket(connectSocket);
	closesocket(connectSocket2);
	WSACleanup();

	return 0;
}

