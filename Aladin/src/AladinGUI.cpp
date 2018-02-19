#include <Aladin.h>
#include <imgui.h>
#include <imgui_memory_editor.h>
#include <stdio.h>
#include <stdlib.h>
#include <Networking.h>

NetInterface* Interface;

void Initialize() {
	Console.RegisterCommand("host", [&](const char* Msg, const char** Args) {
		if (!Console.RequireArgs(Args, 1))
			return;

		if (Interface != NULL) {
			Console.WriteLine("Interface already exists, call 'disconnect'");
			return;
		}

		NamedPipeServerInterface* I = new NamedPipeServerInterface();
		if (!I->Host(Args[1])) {
			Console.Write("Could not spawn ");
			Console.WriteLine(I->Name);

			delete I;
			return;
		}

		Interface = I;

		Console.Write("Hosting ");
		Console.Write(Interface->Name);
		Console.Write(" on '");
		Console.Write(Args[1]);
		Console.WriteLine("'");
	});

	Console.RegisterCommand("disconnect", [&](const char* Msg, const char** Args) {
		if (!Console.RequireArgs(Args, 0))
			return;

		if (Interface == NULL) {
			Console.WriteLine("Nothing to do");
			return;
		}

		delete Interface;
		Interface = NULL;
		Console.WriteLine("Disconnected");
	});
}

void Loop(float Dt) {
	char Data[1024] = { 0 };

	if (Interface != NULL && !Interface->Valid()) {
		Console.WriteLine("Invalid interface");
		Console.Execute("disconnect", false);
	}

	if (Interface != NULL && Interface->Read(Data, 1, false) != -1) {
		/*while (Interface->Read(Data, 1, false) != -1)
			;

		Console.WriteLine("Received a data packet!");*/

		Interface->Read(Data, 4, true);
		int Len = *(int*)Data;

		memset(Data, 0, Len + 1);
		Interface->Read(Data, Len, true);

		Console.WriteLine(Data);
	}

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit"))
				exit(0);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools")) {
			ImGui::MenuItem("Console", NULL, &Console.Show);
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

		if (Interface == NULL)
			ImGui::Text("Interface NULL");
		else
			ImGui::Text(Interface->Name);

		ImGui::Separator();

		if (Console.Show)
			if (ImGui::BeginMenu("Console")) {
				ImGui::SetWindowFocus("Console");
				ImGui::EndMenu();
			}


		ImGui::EndMainMenuBar();
	}

	Console.Draw();
}