#pragma once
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")

void InitGUI();
void AladinUpdateBegin(float Dt);
void AladinUpdateEnd(float Dt);

// Put initialization code here
void Initialize();

// Main loop, call imgui drawing shit here
void Loop(float Dt);