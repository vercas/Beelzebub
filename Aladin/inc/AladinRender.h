#pragma once

#define ALADIN_RENDERER_TYPE_OPENGL2

void alInit();
void alSetView(float Width, float Height, float DisplayWidth, float DisplayHeight);

void alSetScissor(float X, float Y, float W, float H);
void alClear(float R, float G, float B, float A);

// Objects
int alCreateTexture(int W, int H, void* RGBA);
void alUseTexture(int ID);

//void alSetVertices3D();
void alSetVertices(int Size, int Stride, void* Data);
void alSetTextureCoords(int Stride, void* Data);
void alSetColors(int Stride, void* Data);

// Rendering
void alDrawTriangles(int ElementCount, void* Indices);