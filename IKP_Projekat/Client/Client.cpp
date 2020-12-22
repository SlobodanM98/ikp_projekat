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

		iResult = recv(connectSocket, recvbuf, 4, 0);
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
					int dobijenPort = *(int*)recvbuf;
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
			printf("Connection with client closed.\n");
			closesocket(connectSocket);
			break;
		}
	} while (!primljenaPoruka);

	while (1) {

	}

	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

