#include "Main.h"


bool initialize()
{
	// Create components
	renderer = new Renderer();
	ai = new AI();
	input = new Input();
	sound = new Sound();
	
	quit = false;

	prevTime = 0;

	// TO DO: Load config.ini file to set up resolution and input settings



	// Set up the window (refernced a tutorial at http://www.rastertek.com/dx10tut02.html)
	WNDCLASSEX wc;
	DEVMODE screenSettings;

	
	// Get initial screen settings
	initialScreenSettings.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &initialScreenSettings);

	// Initialize screenSettings
	screenSettings.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &screenSettings);

	hInstance = GetModuleHandle(NULL);
	appName = "CPSC 585 Game";

	memset(&wc, 0, sizeof(WNDCLASSEX));

	wc.style = CS_HREDRAW | CS_VREDRAW;// | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance     = hInstance;
	wc.hIcon		 = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm       = wc.hIcon;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	//wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = appName;
	wc.cbSize        = sizeof(WNDCLASSEX);
	
	// Register the window class.
	RegisterClassEx(&wc);

	unsigned long resx, resy;
	// Get screen resolution
	resx = initialScreenSettings.dmPelsWidth;
	resy = initialScreenSettings.dmPelsHeight;

	screenSettings.dmPelsWidth = resx; // Screen width
	screenSettings.dmPelsHeight = resy; // Screen height
	screenSettings.dmBitsPerPel = initialScreenSettings.dmBitsPerPel; // bit depth
	screenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	
	if (ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
		errorPopup("Changing display settings failed!");
		return false;
	}
	
	// Create the window at position 0, 0, and with the resolution defined just above
	hwnd = CreateWindowEx(WS_EX_APPWINDOW, appName, appName, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
			0, 0, resx, resy, NULL, NULL, hInstance, NULL);

	// Bring up the window and set focus
	ShowWindow(hwnd, SW_SHOWNORMAL);
	

	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	// Capture the mouse
	SetCapture(hwnd);

	// Keep the mouse in the application area (maybe not necessary since we're doing fullscreen)
	RECT rect;
	GetWindowRect(hwnd, &rect);
	ClipCursor(&rect);

	// Hide the mouse
	ShowCursor(false);
	

	// Initialize components
	if (!renderer || !ai || !sound || !input)
	{
		errorPopup("Creating Renderer, AI, Sound, or Input component failed! [!renderer || !ai || !sound || !input]");
		return false;
	}

	// TO DO: Run the 'initialize' method for each component
	char* errorMsg = new char[128];
	if (!(renderer->initialize(resx, resy, hwnd, 1.0f, 1000.0f, 10, errorMsg)))
	{
		errorPopup(errorMsg);
		errorPopup("Renderer initialization failed!");
		return false;
	}

	ai->initialize(renderer, input);


	return true;
}

void run()
{
	// (refernced a tutorial at http://www.rastertek.com/dx10tut02.html)
	MSG msg;
	bool quit = false;

	// Zero-out the memory used by msg
	ZeroMemory(&msg, sizeof(MSG));

	SYSTEMTIME st;
	FILETIME ft;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	ULARGE_INTEGER largeInt;
	largeInt.LowPart = ft.dwLowDateTime;
	largeInt.HighPart = ft.dwHighDateTime;

	prevTime = largeInt.QuadPart / 10000;

	while (!quit)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		if (msg.message == WM_QUIT)
		{
			quit = true;
		}
		else
		{
			quit = mainLoop();
		}

	}


	return;
}

void shutdown()
{
	ClipCursor(NULL);
	ReleaseCapture();
	ShowCursor(true);
	ChangeDisplaySettings(NULL, 0);
	DestroyWindow(hwnd);
	hwnd = NULL;

	UnregisterClass(appName, hInstance);
	hInstance = NULL;

	// TO DO: Shut down and free up ai, renderer, sound, input

	ai->shutdown();

	ai = NULL;

	renderer->shutdown();
	
	renderer = NULL;

	ChangeDisplaySettings(&initialScreenSettings, NULL);

	return;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	// (refernced a tutorial at http://www.rastertek.com/dx10tut02.html)
	switch(umessage)
	{
		// Prevent entering powersave/screensaver
		case WM_SYSCOMMAND:
		{
			switch (wparam)
			{
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
				return 0;
			}
			
			return 0;
		}
		// Check if the window is being destroyed.
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}

		// Check if the window is being closed.
		case WM_CLOSE:
		{
			PostQuitMessage(0);		
			return 0;
		}

		case WM_KEYDOWN: case WM_KEYUP : case WM_MOUSEMOVE: case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN: case WM_LBUTTONUP: case WM_RBUTTONUP: 
		// Pass to handler for all input, that will work alongside XInput
		{
			if (input)
				input->processWindowsMsg(umessage, wparam);
			return 0;
		}

		// All other messages pass to the default message handler
		default:
		{
			return DefWindowProc(hwnd, umessage, wparam, lparam);
		}
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	if (initialize())
	{
		run();
	}
	else
	{
		errorPopup("Initialization failed!");
		return 0;
	}

	shutdown();

	return 0;
}



// Main program loop. Return "true" when you want to break out of the loop
bool mainLoop()
{
	SYSTEMTIME st;
	FILETIME ft;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	ULARGE_INTEGER largeInt;
	largeInt.LowPart = ft.dwLowDateTime;
	largeInt.HighPart = ft.dwHighDateTime;

	ULONGLONG currentTime(largeInt.QuadPart / 10000);

	if (currentTime == prevTime)
	{
		renderer->render();
		return false;
	}
	
	quit = input->update();
	ai->simulate((currentTime - prevTime) / 1000.0f);
	renderer->render();

	prevTime = currentTime;

	if (quit)
		ai->shutdown();

	return quit;
}


void errorPopup(LPCTSTR errorMsg)
{
	MessageBox(hwnd, errorMsg, NULL, MB_OK);
}

