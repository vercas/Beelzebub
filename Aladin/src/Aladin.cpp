//#define GLFW_INCLUDE_GLEXT
#include <GLFW\glfw3.h>
#include <Aladin.h>

GLFWwindow* Window;

int main(int argc, const char* argv[]) {
	if (!glfwInit())
		return -1;

	// TODO: Fetch desktop resolution and create something less gay
	Window = glfwCreateWindow(800, 600, "Aladin", NULL, NULL);
	if (!Window) {
		glfwTerminate();
		return -1;
	}

	while (!glfwWindowShouldClose(Window)) {
		Update(0);
		Render(0);
	}

	glfwTerminate();
	return 0;
}

void Update(float Dt) {
	glfwPollEvents();
}

void Render(float Dt) {
	glfwSwapBuffers(Window);
}