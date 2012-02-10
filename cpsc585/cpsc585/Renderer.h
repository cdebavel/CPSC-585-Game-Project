#pragma once
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "DxErr.lib")

#include <d3d9.h>
#include <d3dx9.h>
#include <DxErr.h>

#include <string>

#include "Camera.h"
#include "Drawable.h"


class Renderer
{
public:
	Renderer();
	~Renderer();
	bool initialize(int width, int height, HWND hwnd, float zNear, float zFar, int numDrawables, char* msg);
	void shutdown();
	void render();
	void setText(std::string* sentences, int count);
	int addDrawable(Drawable* drawable);
	void setFocus(int drawableIndex);
	IDirect3DDevice9* getDevice();
	
private:
	void writeText(std::string text, int line);

	IDirect3D9* d3dObject;
	IDirect3DDevice9* device;

	D3DXMATRIX projectionMatrix;
	D3DXMATRIX worldMatrix;
	D3DXMATRIX orthoMatrix;

	Camera* camera;
	
	ID3DXFont* font;
	
	int numSentences;
	std::string* sentences;

	int numDrawables;
	int currentDrawable;
	Drawable** drawables;
};
