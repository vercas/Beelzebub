#include <Aladin.h>
#include <imgui.h>
#include <imgui_memory_editor.h>
#include <stdio.h>
#include <stdlib.h>

static MemoryEditor MemEdit;
bool ShowMemEdit, ShowConsole, ShowDemoWindow;

AladinConsole Console;

void Initialize() {
	ShowConsole = true;

	Console.RegisterCommand("exit", [&](const char* Msg) {
		ShowConsole = false;
	});

	Console.RegisterCommand("shite", [&](const char* Msg) {
		Console.WriteLine("Hello! You executed the \"shite\" command!");
	});
}

void Loop(float Dt) {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit"))
				exit(0);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools")) {
			ImGui::MenuItem("Memory Editor", NULL, &ShowMemEdit);
			ImGui::MenuItem("Console", NULL, &ShowConsole);
			ImGui::MenuItem("Demo", NULL, &ShowDemoWindow);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Style")) {
			if (ImGui::MenuItem("Dark"))
				ImGui::StyleColorsDark();
			if (ImGui::MenuItem("Classic"))
				ImGui::StyleColorsClassic();
			if (ImGui::MenuItem("Light"))
				ImGui::StyleColorsLight();

			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ShowConsole)
			if (ImGui::BeginMenu("Console")) {
				ImGui::SetWindowFocus("Console");
				ImGui::EndMenu();
			}

		if (ShowMemEdit)
			if (ImGui::BeginMenu("Memory Editor")) {
				ImGui::SetWindowFocus("Memory Editor");
				ImGui::EndMenu();
			}

		ImGui::EndMainMenuBar();
	}

	if (ShowConsole)
		Console.Draw();

	if (ShowDemoWindow)
		ImGui::ShowDemoWindow();

	/*if (ShowMemEdit)
		MemEdit.DrawWindow("Memory Editor", (unsigned char*)ConsoleInputBuffer, ConsoleInputBufferLen);*/
}