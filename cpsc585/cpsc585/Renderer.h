#pragma once
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "DxErr.lib")

#include <d3d9.h>
#include <d3dx9.h>
#include <DxErr.h>
#include <d3dx9math.h>

#include <vector>
#include <string>

#include "Camera.h"
#include "Drawable.h"
#include "HUD.h"
#include "MenuHandler.h"
#include "Skybox.h"
#include "SmokeSystem.h"
#include "LaserSystem.h"

struct ShadowPoint
{
	D3DXVECTOR4 position;
	D3DCOLOR color;
};


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
	void addDynamicDrawable(Drawable* drawable);
	void setFocus(int drawableIndex);
	IDirect3DDevice9* getDevice();
	HUD* getHUD();
	MenuHandler* getMenuHandler();
	Camera* getCamera();
	
	static Renderer* renderer;
	static IDirect3DDevice9* device;
	static D3DXVECTOR3 lightDir;

private:
	void writeText(std::string text, int line);
	void drawShadows();

	inline DWORD FtoDw(float f)
	{
	    return *((DWORD*)&f);
	}

	IDirect3D9* d3dObject;
	
	D3DXMATRIX projectionMatrix;
	D3DXMATRIX worldMatrix;
	
	ID3DXFont* font;
	
	int numSentences;
	std::string* sentences;

	int numDrawables;
	int currentDrawable;
	Drawable** drawables;
	std::vector<Drawable*>* dynamicDrawables;

	HUD* hud;
	MenuHandler* menuHandler;
	Camera* camera;

	Skybox* skybox;

	IDirect3DVertexBuffer9* shadowQuadVertexBuffer;

	bool useTwoSidedStencils;
	SmokeSystem* smokeSystem;
	LaserSystem* laserSystem;
};
