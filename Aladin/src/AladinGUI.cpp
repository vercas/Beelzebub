#include <Aladin.h>
#include <imgui.h>
#include <imgui_memory_editor.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static MemoryEditor MemEdit;

int ConsoleInputBufferLen;
char* ConsoleInputBuffer;

std::vector<const char*> ConsoleOut;
bool ShowMemEdit, ShowConsole, ShowDemoWindow;

void Initialize() {
	ShowConsole = true;

	ConsoleInputBufferLen = 4096;
	ConsoleInputBuffer = (char*)calloc(ConsoleInputBufferLen, 1);
}

void ConsoleWrite(const char* Str) {
	int Len = strlen(Str);
	char* Str2 = (char*)calloc(Len + 1, 1);
	memcpy(Str2, Str, Len);
	ConsoleOut.push_back(Str2);
}

void DrawConsole() {
	if (ImGui::Begin("Console", NULL)) {
		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text

		ImGui::BeginChild("scrolling", ImVec2(0, -footer_height_to_reserve), true, ImGuiWindowFlags_HorizontalScrollbar);
		for (int i = 0; i < ConsoleOut.size(); i++)
			ImGui::TextUnformatted(ConsoleOut[i]);
		ImGui::EndChild();

		ImGui::Separator();

		ImGui::BeginChild("input", ImVec2(0, 0), false);
		if (ImGui::InputText("Command", ConsoleInputBuffer, ConsoleInputBufferLen, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ConsoleWrite(ConsoleInputBuffer);
			memset(ConsoleInputBuffer, 0, ConsoleInputBufferLen);
		}
		ImGui::EndChild();

	}
	ImGui::End();
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
		DrawConsole();

	if (ShowDemoWindow)
		ImGui::ShowDemoWindow();

	if (ShowMemEdit)
		MemEdit.DrawWindow("Memory Editor", (unsigned char*)ConsoleInputBuffer, ConsoleInputBufferLen);
}