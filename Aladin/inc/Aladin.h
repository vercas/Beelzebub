#pragma once
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")

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

// AladinGUI
void Initialize();
void Loop(float Dt);

// Convenience
char** AladinSplitString(char* Str, const char* Delim); 

// AladinWidgets
struct AladinConsole {
private:
	bool AutoScroll;
	//std::vector<std::tuple<const char*, >> Cmds;
	std::unordered_map<const char*, std::function<void(const char*)>> Cmds;

public:
	int ConsoleInputBufferLen;
	char* ConsoleInputBuffer;

	int ConsoleOutputBufferLen;
	char* ConsoleOutputBuffer;
	char* ConsoleOutputBufferPos;

	AladinConsole();

	void Clear();
	void Write(const char* Msg);
	void WriteLine(const char* Msg);

	void RegisterCommand(const char* Name, std::function<void(const char*)> F);
	void Execute(const char* Msg);

	void Draw();
};