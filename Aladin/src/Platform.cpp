#include <Aladin.h>

#ifdef _WIN32 || _WIN64
#include <Windows.h>
#endif

void AladinSetDPIAware() {
#ifdef _WIN32 || _WIN64
	SetProcessDPIAware();
#endif
}