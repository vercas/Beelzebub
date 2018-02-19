#pragma once

struct NetInterface {
public:

	const char* Name;

	virtual ~NetInterface() {};
	virtual bool Valid() = 0;

	virtual int Write(void* Buffer, int Len) = 0;
	virtual int Read(void* Buffer, int Len, bool Block) = 0;
};

struct NamedPipeServerInterface : NetInterface {
public:
	void* PipeHandle;

	NamedPipeServerInterface();
	~NamedPipeServerInterface();
	bool Valid();

	bool Host(const char* Name);
	int Write(void* Buffer, int Len);
	int Read(void* Buffer, int Len, bool Block);
};