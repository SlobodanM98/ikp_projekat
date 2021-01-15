#pragma once

#define CLIENT_SLEEP_TIME 1000


int Selekt(SOCKET *socket) {
	FD_SET set;
	timeval timeVal;

	FD_ZERO(&set);

	FD_SET(*socket, &set);

	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	return select(0, &set, NULL, NULL, &timeVal);
}


bool InitializeWindowsSockets()
{
	WSADATA wsaData;
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}


int PrimiPoruku(SOCKET *socket, char *poruka, int duzinaPoruke) {
	int primljeniBajtovi = 0;
	int duzina = duzinaPoruke;
	if (duzinaPoruke > 100) {
		duzina = 100;
	}
	char recvbuf[512];
	do {
		int iResult = Selekt(socket);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			break;
		}

		if (iResult == 0)
		{
			printf("Ceka se odgovor...\n");
			Sleep(CLIENT_SLEEP_TIME);
			continue;
		}

		if (duzinaPoruke - primljeniBajtovi < 100) {
			duzina = duzinaPoruke - primljeniBajtovi;
		}

		iResult = recv(*socket, recvbuf, duzina, 0);
		if (iResult > 0)
		{
			printf("Poruka je: %s.\n", recvbuf);
			memcpy(poruka + primljeniBajtovi, recvbuf, iResult);
			primljeniBajtovi += iResult;
		}
		else if (iResult == 0)
		{
			printf("Connection closed.\n");
			closesocket(*socket);
			break;
		}
	} while (primljeniBajtovi < duzinaPoruke);

	return primljeniBajtovi;
}

int PosaljiPoruku(SOCKET *socket, char *poruka, int duzinaPoruke) {
	int poslatiBajtovi = 0;
	int duzina = duzinaPoruke;
	if (duzinaPoruke > 100) {
		duzina = 100;
	}
	do {
		char bajt = *(poruka + poslatiBajtovi);
		if (duzinaPoruke - poslatiBajtovi < 100) {
			duzina = duzinaPoruke - poslatiBajtovi;
		}
		int iResult = send(*socket, poruka + poslatiBajtovi, duzina, 0);
		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(*socket);
			WSACleanup();
			return 1;
		}
		poslatiBajtovi += iResult;
	} while (poslatiBajtovi < duzinaPoruke);

	return poslatiBajtovi;
}
