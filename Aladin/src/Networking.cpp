#include <Aladin.h>
#include <Networking.h>

#ifdef _WIN32 || _WIN64
#include <iostream>
#include <Windows.h>
#endif

NamedPipeServerInterface::NamedPipeServerInterface() {
	Name = "Named Pipe Server";
}

NamedPipeServerInterface::~NamedPipeServerInterface() {
	if (PipeHandle != NULL) {
		CloseHandle(PipeHandle);
		PipeHandle = NULL;
	}
}

bool NamedPipeServerInterface::Host(const char* Name) {
	char FullPipeName[MAX_PATH] = { 0 };
	if (sprintf_s(FullPipeName, "\\\\.\\pipe\\%s", Name) == -1)
		return false;

	PipeHandle = CreateNamedPipeA(FullPipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, NULL);

	if (PipeHandle == NULL || PipeHandle == INVALID_HANDLE_VALUE)
		return false;

	OVERLAPPED Ol = { 0, 0, 0, 0, NULL };
	Ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	BOOL Ret = FALSE;

	if (!(Ret = ConnectNamedPipe(PipeHandle, &Ol))) { // Will fail
		switch (GetLastError()) {
		case ERROR_PIPE_CONNECTED:
			Ret = TRUE;
			break;

		case ERROR_IO_PENDING:
			if (Ret = WaitForSingleObject(Ol.hEvent, 30000)) {
				DWORD Ign;
				Ret = GetOverlappedResult(PipeHandle, &Ol, &Ign, FALSE);

				if (Ret == 0)
					Ret = GetLastError();
			}
			else
				Ret = TRUE;
		}
	}

	CloseHandle(Ol.hEvent);
	if (Ret == TRUE || Ret == ERROR_IO_PENDING)
		return true;

	return false;
}

bool NamedPipeServerInterface::Valid() {
	if (PipeHandle == NULL)
		return false;

	if (!PeekNamedPipe(PipeHandle, NULL, 0, NULL, NULL, NULL)) {
		switch (GetLastError()) {
		case ERROR_BROKEN_PIPE:
			return false;
		}
	}

	return true;
}

static bool NamedPipeHasData(HANDLE NamedPipe, int* Len) {
	DWORD DataLen = 0;
	*Len = 0;

	if (PeekNamedPipe(NamedPipe, NULL, 0, NULL, &DataLen, NULL)) {
		*Len = (int)DataLen;
		return true;
	}

	return false;
}

int NamedPipeServerInterface::Write(void* Buffer, int Len) {
	DWORD Written;
	if (WriteFile(PipeHandle, Buffer, Len, &Written, NULL))
		return (int)Written;

	return -1;
}

int NamedPipeServerInterface::Read(void* Buffer, int Len, bool Block) {
	DWORD Read;
	int AvailableLen;

	if (!Block && NamedPipeHasData(PipeHandle, &AvailableLen) && AvailableLen < Len)
		return -1;

	if (ReadFile(PipeHandle, Buffer, Len, &Read, NULL))
		return (int)Read;

	return -1;
}