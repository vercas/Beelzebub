#include <Aladin.h>
#include <imgui.h>

AladinConsole::AladinConsole() {
	Show = true;

	ConsoleInputBufferLen = 1024;
	ConsoleInputBuffer = (char*)calloc(ConsoleInputBufferLen, 1);

	ConsoleOutputBufferLen = 4096 * 2;
	//ConsoleOutputBufferLen = 500;
	ConsoleOutputBuffer = (char*)calloc(ConsoleOutputBufferLen, 1);
	Clear();

	RegisterCommand("clear", [&](const char* Msg, const char** Args) {
		Clear();
	});

	RegisterCommand("exit", [&](const char* Msg, const char** Args) {
		Console.Show = false;
	});
}

bool AladinConsole::RequireArgs(const char** Args, int Cnt) {
	int ArgCnt = 0;

	if ((ArgCnt = AladinStringArrayLen(Args)) != Cnt + 1) {
		char Bfr[1024] = { 0 };
		sprintf(Bfr, "Command requires %i arguments, received %i", Cnt, ArgCnt - 1);
		WriteLine(Bfr);
		return false;
	}

	return true;
}

void AladinConsole::Clear() {
	memset(ConsoleOutputBuffer, 0, ConsoleOutputBufferLen);
	ConsoleOutputBufferPos = ConsoleOutputBuffer;
}

void AladinConsole::Write(const char* Msg) {
	int Len = strlen(Msg);
	if (ConsoleOutputBufferPos - ConsoleOutputBuffer + Len >= ConsoleOutputBufferLen)
		Clear();

	memcpy(ConsoleOutputBufferPos, Msg, Len);
	ConsoleOutputBufferPos += Len;
}

void AladinConsole::WriteLine(const char* Msg) {
	Write(Msg);
	Write("\n");
}

void AladinConsole::RegisterCommand(const char* Name, std::function<void(const char*, const char**)> F) {
	Cmds.insert({ Name, F });
}

void AladinConsole::Execute(const char* Msg, bool Echo) {
	if (strlen(Msg) == 0)
		return;

	if (Echo) {
		Write(">> ");
		WriteLine(Msg);
	}

	char* MsgDup = strdup(Msg);
	char** Args = AladinSplitString(MsgDup, " ");

	for each (auto C in Cmds) {
		if (!strcmp(*Args, C.first)) {
			C.second(Msg, (const char**)Args);

			goto END; // You're gonna hate me for this :troll:
		}
	}

	Write("Unknown command '");
	Write(*Args);
	WriteLine("'");

END:
	free(MsgDup);
	free(Args);
}

void AladinConsole::Draw() {
	if (!Show)
		return;

	if (ImGui::Begin("Console", NULL, ImGuiWindowFlags_MenuBar)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Options")) {
				ImGui::MenuItem("Auto Scroll", NULL, &AutoScroll);
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		const float ReserveHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
		ImGui::BeginChild("scrolling", ImVec2(0, -ReserveHeight));
		{
			ImGui::InputTextMultiline("##hidelabel", ConsoleOutputBuffer, ConsoleOutputBufferLen, ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);

			if (AutoScroll)
				ImGui::SetScrollHere();
		}
		ImGui::EndChild();
		ImGui::Separator();
		ImGui::BeginChild("input");
		{
			ImGui::Text("Command");
			ImGui::SameLine();
			ImGui::PushItemWidth(-10);

			if (ImGui::InputText("##hidelabel", ConsoleInputBuffer, ConsoleInputBufferLen, ImGuiInputTextFlags_EnterReturnsTrue)) {
				Execute(ConsoleInputBuffer);
				memset(ConsoleInputBuffer, 0, ConsoleInputBufferLen);
				ImGui::SetKeyboardFocusHere(-1); // Focus input box after pressed enter
			}

			ImGui::PopItemWidth();
		}
		ImGui::EndChild();

	}
	ImGui::End();
}