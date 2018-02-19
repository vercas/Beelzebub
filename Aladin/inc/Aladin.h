#pragma once
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")

#ifndef ALADIN_IMPLEMENTATION
#define ALADIN_EXTERN extern
#else
#define ALADIN_EXTERN
#endif

#include <imgui.h>
#include <vector>
#include <cstdio>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <cstring>

// Aladin
void InitGUI();
void AladinUpdateBegin(float Dt);
void AladinUpdateEnd(float Dt);

void AladinSetDPIAware();

// AladinGUI
void Initialize();
void Loop(float Dt);

// Convenience
char** AladinSplitString(char* Str, const char* Delim); 
int AladinStringArrayLen(const char** Arr);

// AladinWidgets
struct AladinConsole {
private:
	bool AutoScroll;
	//std::vector<std::tuple<const char*, >> Cmds;
	std::unordered_map<const char*, std::function<void(const char*, const char**)>> Cmds;

	int ConsoleInputBufferLen;
	char* ConsoleInputBuffer;

	//std::vector<const char*> Output;
	int ConsoleOutputBufferLen;
	char* ConsoleOutputBuffer;
	char* ConsoleOutputBufferPos;

public:
	bool Show;

	AladinConsole();

	bool RequireArgs(const char** Args, int Cnt);

	void Clear();
	void Write(const char* Msg);
	void WriteLine(const char* Msg);

	void RegisterCommand(const char* Name, std::function<void(const char*, const char**)> F);
	void Execute(const char* Msg, bool Echo = true);

	void Draw();
};

ALADIN_EXTERN AladinConsole Console;