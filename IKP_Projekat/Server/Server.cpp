#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define MASTER_PORT 10000
#define SERVER_SLEEP_TIME 1000
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);}
#define BUFFER_SIZE 512
#pragma warning(disable:4996)

bool InitializeWindowsSockets();

typedef struct clan {
	char *ipAdresa;
	char *port;
	struct clan *sledeci;
}Clan;

int port = 0;

int  main(void)
{
	SOCKET listenSocket = INVALID_SOCKET;
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
		FD_SET set;
		timeval timeVal;

		FD_ZERO(&set);

		FD_SET(connectSocket, &set);

		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		iResult = select(0, &set, NULL, NULL, &timeVal);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			break;
		}

		if (iResult == 0)
		{
			printf("Ceka se master...\n");
			Sleep(SERVER_SLEEP_TIME);
			continue;
		}

		do {

			FD_SET set;
			timeval timeVal;

			FD_ZERO(&set);

			FD_SET(connectSocket, &set);

			timeVal.tv_sec = 0;
			timeVal.tv_usec = 0;

			iResult = select(0, &set, NULL, NULL, &timeVal);

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

					char* poruka;
					char adresa[20];

					int len = sizeof(myAddress);
					getsockname(connectSocket, (sockaddr*)&myAddress, &len);
					inet_ntop(AF_INET, &myAddress.sin_addr, adresa, sizeof(adresa));

					port = ntohs(myAddress.sin_port);

					int velicina = strlen(adresa) + sizeof(int);
					poruka = (char*)malloc(velicina);
					*(int*)poruka = port;

					for (int i = 0; i < strlen(adresa); i++) {
						*(poruka + 4 + i) = adresa[i];
					}
					*(poruka + 4 + strlen(adresa)) = '\0';

					iResult = send(connectSocket, (char*)&velicina, 4, 0);
					iResult = send(connectSocket, poruka, velicina, 0);

				}
			}
			else if (iResult == 0)
			{
				printf("Connection with client closed.\n");
				closesocket(connectSocket);
				break;
			}
		} while (!primljenaPoruka);
	} while (!primljenaPoruka);

	closesocket(connectSocket);

	addrinfo *resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; 

	char stringPort[10];
	itoa(port, stringPort, 10);

	iResult = getaddrinfo(NULL, stringPort, &hints, &resultingAddress);
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
			printf("Ceka se veza sa klijentom...\n");
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

		do
		{
			iResult = recv(acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
			if (iResult > 0)
			{
				printf("Message received from client: %s.\n", recvbuf);
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
		} while (iResult > 0);
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
