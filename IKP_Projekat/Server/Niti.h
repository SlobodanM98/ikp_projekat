#pragma once

struct Parametri {
	SOCKET *listenSocket;
	RingBuffer *ringBuffer;
	CRITICAL_SECTION *bufferAccess;
	HANDLE *Empty;
	HANDLE *Full;
};

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
			printf("Ceka se veza sa klijentom...\n");
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