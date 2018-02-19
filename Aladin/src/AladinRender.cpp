#include <AladinRender.h>

#ifdef ALADIN_RENDERER_TYPE_OPENGL2
#include <GLFW\glfw3.h>
#endif

float DWidth, DHeight;

void alInit() {
#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
}

void alSetView(float Width, float Height, float DisplayWidth, float DisplayHeight) {
	DWidth = DisplayWidth;
	DHeight = DisplayHeight;

#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glViewport(0, 0, Width, Height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, DisplayWidth, DisplayHeight, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
}

void alSetScissor(float X, float Y, float W, float H) {
	if (W == -1)
		W = DWidth;
	if (H == -1)
		H = DHeight;

#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glScissor(X, Y, W, H);
#endif
}

void alClear(float R, float G, float B, float A) {
#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glClearColor(R, G, B, A);
	glClear(GL_COLOR_BUFFER_BIT);
#endif
}

int alCreateTexture(int W, int H, void* RGBA) {
	int TexID = -1;

#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	GLuint GLID;
	glGenTextures(1, &GLID);
	TexID = GLID;

	alUseTexture(GLID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, RGBA);
#endif

	return TexID;
}

void alUseTexture(int ID) {
#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glBindTexture(GL_TEXTURE_2D, ID);
#endif
}

void alDrawTriangles(int ElementCount, void* Indices) {
#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glDrawElements(GL_TRIANGLES, ElementCount, GL_UNSIGNED_INT, Indices);
#endif
}

void alSetVertices(int Size, int Stride, void* Data) {
#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glVertexPointer(Size, GL_FLOAT, Stride, Data);
#endif
}

void alSetTextureCoords(int Stride, void* Data) {
#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glTexCoordPointer(2, GL_FLOAT, Stride, Data);
#endif
}

void alSetColors(int Stride, void* Data) {
#ifdef ALADIN_RENDERER_TYPE_OPENGL2
	glColorPointer(4, GL_UNSIGNED_BYTE, Stride, Data);
#endif
}
