#pragma once
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")

// Aladin
void InitGUI();
void AladinUpdateBegin(float Dt);
void AladinUpdateEnd(float Dt);

// AladinGUI
void Initialize();
void Loop(float Dt);

// AladinWidgets
void Console_Draw();