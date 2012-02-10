#include "Renderer.h"


Renderer::Renderer()
{
	d3dObject = NULL;
	device = NULL;
	
	camera = NULL;
	font = NULL;

	sentences = NULL;
	numSentences = 0;
	numDrawables = 0;
	drawables = NULL;
	currentDrawable = 0;
}


Renderer::~Renderer()
{
}

bool Renderer::initialize(int width, int height, HWND hwnd, float zNear, float zFar, int numToDraw, char* msg)
{
	numDrawables = numToDraw;

	drawables = new Drawable*[numToDraw];

	d3dObject = Direct3DCreate9(D3D_SDK_VERSION);
	
	D3DCAPS9 caps;
	d3dObject->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

	if (!(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
	{
		LPCTSTR descr = "This program requires a video card that supports hardware vertex processing!";
		strcpy_s((char*) msg, 128, descr);
		
		return false;
	}


	D3DPRESENT_PARAMETERS params;

	ZeroMemory(&params,sizeof(D3DPRESENT_PARAMETERS));

	params.Windowed = TRUE;
	params.BackBufferCount= 1;
	params.BackBufferFormat = D3DFMT_X8R8G8B8;
	
	params.BackBufferWidth = width;
	params.BackBufferHeight = height;
	params.MultiSampleType=D3DMULTISAMPLE_NONE;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = hwnd;
	params.EnableAutoDepthStencil = TRUE;

	// VSYNC. Change to INTERVAL_IMMEDIATE to turn off VSYNC, change to INTERVAL_ONE to turn on VSYNC
	params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	// Now need to set up the depth stencil format.
	D3DFORMAT formats[] = { D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D16 };
	
	D3DFORMAT format = (D3DFORMAT) 0;
	
	for (int i = 0; i < 3; i++)
	{
		HRESULT result = d3dObject->CheckDeviceFormat(D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE, formats[i]);

		if (result == D3D_OK)
		{
			result = d3dObject->CheckDepthStencilMatch(D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, formats[i]);

			if (result == D3D_OK)
			{
				format = formats[i];
				break;
			}
		}
	}
	
	if (format == (D3DFORMAT) 0)
	{
		LPCTSTR descr = "Couldn't find a suitable depth format!";
		strcpy_s((char*) msg, 128, descr);
		
		return false;
	}
	
	params.AutoDepthStencilFormat = format;

	HRESULT result = d3dObject->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &params, &device);

	if (FAILED(result))
	{
		LPCTSTR descr = DXGetErrorDescription(result);
		strcpy_s((char*) msg, 128, descr);
		
		return false;
	}

	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
	

	D3DVIEWPORT9 viewport;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;
	viewport.X = 0;
	viewport.Y = 0;

	device->SetViewport(&viewport);

	float fieldOfView, screenAspect;

	// Setup the projection matrix.
	fieldOfView = (float)D3DX_PI / 4.0f;
	screenAspect = (float)width / (float)height;

	// Create the projection matrix for 3D rendering.
	D3DXMatrixPerspectiveFovLH(&projectionMatrix, fieldOfView, screenAspect, zNear, zFar);

	D3DXMatrixIdentity(&worldMatrix);

	// Create an orthographic projection matrix for 2D rendering.
	D3DXMatrixOrthoLH(&orthoMatrix, (float)width, (float)height, zNear, zFar);
	
	camera = new Camera;
	
	device->SetRenderState(D3DRS_LIGHTING, TRUE);
	device->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(100, 100, 100));
	
	
	D3DLIGHT9 light;    // create the light struct
	D3DMATERIAL9 material;    // create the material struct
	
	ZeroMemory(&light, sizeof(light));    // clear out the light struct for use
	light.Type = D3DLIGHT_DIRECTIONAL;    // make the light type 'directional light'
	light.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);    // set the light's color
	light.Direction = D3DXVECTOR3(1.0f, -0.5f, 1.0f);
	
    device->SetLight(0, &light);    // send the light struct properties to light #0
	device->LightEnable(0, TRUE);    // turn on light #0

	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

	device->SetMaterial(&material);    // set the globably-used material to &material


	// Set up font stuff
	D3DXCreateFont(device, 0, 10, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DRAFT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "", &font);

	device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTOP_SELECTARG1);
	device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);	// Just to be safe (ignored)

	return true;
}

void Renderer::shutdown()
{
	if (drawables)
	{
		// Clean up drawables
	}

	if (camera)
	{
		delete camera;
		camera = NULL;
	}

	if (font)
	{
		font->Release();
		font = NULL;
	}

	if (d3dObject)
	{
		d3dObject->Release();
		d3dObject = NULL;
	}

	if (device)
	{
		device->Release();
		device = NULL;
	}
}

void Renderer::render()
{
	D3DXMATRIX viewMatrix;
	
	device->SetRenderState(D3DRS_ZENABLE, TRUE);

	camera->update();

	// Get view matrix
	camera->getViewMatrix(viewMatrix);

	device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(10, 10, 80), 1.0f, 0);
	
	device->SetTransform(D3DTS_PROJECTION, &projectionMatrix);
	device->SetTransform(D3DTS_VIEW, &viewMatrix);
	device->SetTransform(D3DTS_WORLD, &worldMatrix);

	device->BeginScene();

	

	for (int i = 0; i < currentDrawable; i++)
	{
		drawables[i]->render(device);
	}


	for (int i = 0; i < numSentences; i++)
	{
		writeText(sentences[i], i);
	}

	device->EndScene();

	device->Present(NULL, NULL, NULL, NULL);

	return;
}

void Renderer::writeText(std::string text, int line)
{
	device->SetTransform(D3DTS_PROJECTION, &orthoMatrix);

	RECT rect;
	SetRect(&rect, 20, 20 + (line*30), 600, 600);
	
	font->DrawText(NULL, text.c_str(), -1, &rect, DT_LEFT|DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
}

void Renderer::setText(std::string* sentenceArray, int count)
{
	if ((sentences) && (numSentences < count))
	{
		delete [] sentences;
		sentences = NULL;
	}
	
	numSentences = count;

	sentences = new std::string[count];

	for (int i = 0; i < count; i++)
	{
		sentences[i] = std::string(sentenceArray[i]);
	}
}


int Renderer::addDrawable(Drawable* drawable)
{
	if (currentDrawable < numDrawables)
	{
		drawables[currentDrawable] = drawable;

		if (currentDrawable == 0)
		{
			camera->setFocus(drawable);
		}

		currentDrawable++;

		return (currentDrawable - 1);
	}

	return -1;
}

void Renderer::setFocus(int drawableIndex)
{
	if ((drawableIndex < 0) || (drawableIndex >= currentDrawable))
	{
		return;
	}

	camera->setFocus(drawables[drawableIndex]);
}

IDirect3DDevice9* Renderer::getDevice()
{
	return device;
}