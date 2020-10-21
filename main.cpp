#include <d3d11.h>
#include <dxgi.h> //DirectX graphics infrastructure
#include <d3dx11.h>
#include <windows.h>
#include <dxerr.h>

#define _XM_NO_INTRINSICS_
#define XM_NO_ALGINMENT
#include <DirectXMath.h>
using namespace DirectX;
//test
//Windows globals and prototypes
HINSTANCE	g_hInst = NULL;
HWND		g_hWnd = NULL;
char		g_TitleName[100] = "Jamie Griffiths";

HRESULT InitialiseWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//Direct3D globals and prototypes
D3D_DRIVER_TYPE			g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL		g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*			g_pD3DDevice = NULL;
ID3D11DeviceContext*	g_pImmediateContext = NULL;
IDXGISwapChain*			g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pBackBufferRTView = NULL; //Render target (Area of memory where pixels can be drawn to) //Viewport is the rectangle a scene is projected onto. Also determines depth of the view frustum
ID3D11Buffer* g_pConstantBuffer0 = NULL;

ID3D11Buffer*			g_pVertexBuffer;
ID3D11VertexShader*		g_pVertexShader;
ID3D11PixelShader*		g_pPixelShader;
ID3D11InputLayout*		g_pInputLayout;

UINT g_SCREENWIDTH = 640;
UINT g_SCREENHEIGHT = 480;

float g_clear_colour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
float x, y;

//Constand Buffer struct. Must be exactly 16 bytes. Float is 4 bytes
struct CONSTANT_BUFFER0
{
	XMFLOAT3 Offset; 
	float packing_bytes; 
};
CONSTANT_BUFFER0 cb0;
float Offset = (0.0f, 0.0f, 0.0f);


//Vertex structure
struct POS_COL_VERTEX
{
	XMFLOAT3 pos;
	XMFLOAT4 Col;
};

POS_COL_VERTEX vertices[] =
{
	{XMFLOAT3(0.9f, 0.9f, 0.0f),   XMFLOAT4(0.1f, 0.5f, 0.9f, 1.0f)},
	{XMFLOAT3(0.9f, -0.9f, 0.0f),  XMFLOAT4(0.3f, 0.3f, 0.1f, 1.0f)},
	{XMFLOAT3(-0.9f, -0.9f, 0.0f), XMFLOAT4(0.6f, 0.2f, 0.9f, 1.0f)},
	{XMFLOAT3(0.9f, 0.9f, 0.0f),   XMFLOAT4(0.9f, 0.1f, 0.1f, 1.0f)},
	{XMFLOAT3(-0.9f, -0.9f, 0.0f), XMFLOAT4(0.1f, 0.9f, 0.8f, 1.0f)},
	{XMFLOAT3(-0.9f, 0.9f, 0.0f),  XMFLOAT4(1.0f, 0.0f, 0.4f, 1.0f)}
};


HRESULT InitialiseD3D();
HRESULT InitialiseGraphics();
void ShutdownD3D();
void RenderFrame();
HRESULT ResizeBuffer();

//Windows probram entry point
int WINAPI WinMain(_In_ HINSTANCE hInstance, //handle to an instance
				   _In_opt_ HINSTANCE hPrevInstance, //leagcy and is always NULL
				   _In_ LPSTR lpCmdLine, //Pointer to a string that contains a command line when app is started
				   _In_ int nCmdShow) //How the window will appear after initalisation
{
	//Unreferenced parameter stops compiler warning about unused variables
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	//Setup, create and display window
	if (FAILED(InitialiseWindow(hInstance, nCmdShow)))
	{
		DXTRACE_MSG("Failed to create Window!");
		return 0;
	}

	//Initialise Direct3D
	if (FAILED(InitialiseD3D()))
	{
		DXTRACE_MSG("Failed to create Device");
		return 0;
	}

	if (FAILED(InitialiseGraphics()))
	{
		DXTRACE_MSG("Failed to initialise graphics");
		return 0;
	}

	//Main message loop
	MSG msg = { 0 };

	while (msg.message != WM_QUIT) //Loop looks for messages on event queue
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) //If message relevant to app is found, it is removed from event queue
		{
			//Translate and sent to application windows procedure
			TranslateMessage(&msg);
			DispatchMessage(&msg); 
		}
		else 
		{
			RenderFrame();
		}
	}

	ShutdownD3D();
	return (int)msg.wParam;

}


void RenderFrame()
{
	//Clear the back buffer
	g_pImmediateContext->ClearRenderTargetView(g_pBackBufferRTView, g_clear_colour);

	//Upload new values for constant buffer
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer0, 0, 0, &cb0, 0, 0);

	//Set vertex buffer
	UINT stride = sizeof(POS_COL_VERTEX);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset); //Sets the active vertex buffer

	//Select which primtive type to use
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //Use triangles

	//Draws the object in vertex buffer
	g_pImmediateContext->Draw(
		6, //Number of vertices to draw
		0	//0 is buffer start position
	);
	//Display what has just been renderered
	g_pSwapChain->Present(0, 0);
}

//Not used but kept to show how to update vertex buffers
void UpdateVertices()
{
	//Copy the vertices into the buffer
	D3D11_MAPPED_SUBRESOURCE ms;

	//Lock the buffer to allow writing
	g_pImmediateContext->Map(g_pVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);

	//Copy the data
	memcpy(ms.pData, vertices, sizeof(vertices)); //Copies the vertex data into the vertex buffer

	//Unlock the buffer
	g_pImmediateContext->Unmap(g_pVertexBuffer, NULL); //Unmap to tell D3D that the copy has finished and free to use.
}

//Called everytime the application receives a message
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) //CALLBACK function - can be called at any time a message is received
{
	PAINTSTRUCT ps;
	HDC hdc;
	float r = 0;

	switch (message)
	{
	case WM_PAINT: //Draws window background
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY: //End the message handling loop in WinMain()
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		if (wParam == 0x41) //A
		{
			cb0.Offset.x -= 0.01;
		}

		if (wParam == 0x44) //D
		{
			cb0.Offset.x += 0.01;
		}

		if (wParam == 0x53) //W
		{
			cb0.Offset.y -= 0.01;
		}

		if (wParam == 0x57) //S
		{
			cb0.Offset.y += 0.01;
		}

	//case WM_KEYDOWN: //Key pressed down
		/*
		if (wParam == VK_LEFT)
		{
			
		}

		if (wParam == VK_RIGHT)
		{
			
		}

		if (wParam == VK_DOWN)
		{
			
		}

		if (wParam == VK_UP)
		{
			
		}
		*/
	case WM_LBUTTONDOWN:
		
	case WM_MOUSEMOVE:

	case WM_SIZE:
		if (g_pSwapChain != NULL)
		{
			ResizeBuffer();
		}
		
	default:
		return DefWindowProc(hWnd, message, wParam, lParam); //Pass message on to the default message handling routines
	}

	return 0;
}


HRESULT ResizeBuffer()
{
	HRESULT hr;

	RECT rc;
	GetClientRect(g_hWnd, &rc); //Returns the rectangle that defines the renderable portion of the window
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	g_pImmediateContext->OMSetRenderTargets(0, 0, 0);

	// Release all outstanding references to the swap chain's buffers.
	if (g_pBackBufferRTView != NULL) { g_pBackBufferRTView->Release(); }

	g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	//Get pointer to back buffer texture
	ID3D11Texture2D* p_BackBufferTexture; //ID3D11Texture2D is a pointer to an image buffer

	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&p_BackBufferTexture); //->GetBuffer returns a pointer to the back buffer in p_BackBufferTexture

	if (FAILED(hr))
		return hr;

	//Use the back buffer texture pointer to create render target view
	hr = g_pD3DDevice->CreateRenderTargetView(p_BackBufferTexture, NULL, &g_pBackBufferRTView);

	p_BackBufferTexture->Release();

	if (FAILED(hr))
		return hr;

	//Set the render target view
	g_pImmediateContext->OMSetRenderTargets(
		1,						//Number of render targets to set
		&g_pBackBufferRTView,	//Pointer to a list of render target views
		NULL					//List of depth views
	);

	//Set the viewport
	D3D11_VIEWPORT viewport;

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (FLOAT)width;
	viewport.Height = (FLOAT)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	g_pImmediateContext->RSSetViewports(
		1,			//Number of viewports to set
		&viewport	//Pointer to the list of viewports
	);

	return S_OK;
}

//Register class and create window
HRESULT InitialiseWindow(HINSTANCE hInstance, int nCmdShow)
{
	//App name
	char Name[100] = "Hello World!";

	//Register class
	WNDCLASSEX wcex = { 0 }; //Register a large structure window class and initialises all elements to 0
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc; //WndProc is the function to processes windows messages
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1); //Required for non-D3D apps and sets a windows background clear colour
	wcex.lpszClassName = Name;

	if (!RegisterClassEx(&wcex)) return E_FAIL; //Registers the window class

	//Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, g_SCREENWIDTH, g_SCREENHEIGHT }; //Creates a rectangle that is used for size of window
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE); //Resize window to take window borders into account

	//Return value is the handle assigned to the new window
	g_hWnd = CreateWindow(
		Name, //Name of the window class (Same as registered class name)
		g_TitleName, //Name of the window in OS
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		NULL,
		NULL,
		hInstance, //Handle to the instance (must be same value that came from WinMain()
		NULL
	);

	if (!g_hWnd)
		return E_FAIL;

	//Takes the window handle (g_hWnd) and nCmdShow and shows the window
	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

//Create D3D device and swap chain
HRESULT InitialiseD3D()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc); //Returns the rectangle that defines the renderable portion of the window
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	//Enables devices debugging capabilties
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	//List of possible driver types to try to create (in order to create them)
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE, //Use graphics hardware
		D3D_DRIVER_TYPE_WARP,	  //Use software rendering
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	//Holds list of Direct3D feature levels to try to create (in order to creat them)
	//Feature level describes the device feature set for Direct3D11 capable hardware
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	//Swap chain is the set of buffers that is rendered to
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd)); //Standard DirectX way of initialising structures
	sd.BufferCount = 1; //Number of back buffers
	sd.BufferDesc.Width = width; //Width of renderable area
	sd.BufferDesc.Height = height; //Height of renderable area
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60; //Refresh rate so 60/1 or 60fps
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //The swap chain buffers can be rendered to
	sd.OutputWindow = g_hWnd; //What window to render to
	sd.SampleDesc.Count = 1; //AntiAliasing properties of buffer. This means no antialiasing
	sd.SampleDesc.Quality = 0;
	sd.Windowed = true; //Start application in windowed mode

	//Attempts to create the device with an associated swap chain based on provided feature levels and swap chain descption.
	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,					//Indicated what graphics adapter to use (NULL uses default)
			g_driverType,			//What type of driver to create
			NULL,					//Not required - NULL
			createDeviceFlags,		//Device creation flags (Used for creating debug friendly device)
			featureLevels,			//Takes list and number of feature levels and creates a device 
			numFeatureLevels,		//^
			D3D11_SDK_VERSION,		//Always use D3D11_SDK_VERSION for DirectX11 developed app
			&sd,					//pointer to the swap chain decription structure
			&g_pSwapChain,			//Pointer to pointer to the swap chain object that gets created. (Used to access the swap chain)
			&g_pD3DDevice,			//Pointer to pointer to the device object. (Represents the physical hardware adapter
			&g_featureLevel,		//Returns the flag of the highest feature level (Lets app know what hardware is available to use)
			&g_pImmediateContext	//Pointer to pointer to the device context object. Device context represents the portion of the device that handles graphics rendering
		);

		if (SUCCEEDED(hr))
			break;
	}


	hr = ResizeBuffer();

	if (FAILED(hr))
		return hr;

	return S_OK;

}

//Setup graphics
HRESULT InitialiseGraphics()
{
	HRESULT hr = S_OK;

	//Setup and create vertex buffer
	D3D11_BUFFER_DESC bufferDesc; //Used to define properties of vertex buffer
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC; //How the buffer will be used (dynamic means both CPU and GPU access is allowed)
	bufferDesc.ByteWidth = sizeof(POS_COL_VERTEX) * sizeof(vertices); //Total size of buffer in bytes (size is calculated from size of single vertex * number of vertices)
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; //Tells D3D that this is vertex buffer
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //Allows cpu access

	//Create the buffer
	hr = g_pD3DDevice->CreateBuffer(
		&bufferDesc,		//Address of the properties struct
		NULL,				//Allows vertex data to be added to buffer on creation
		&g_pVertexBuffer	//Address of buffer object that is returned on successful creation
	);

	if (FAILED(hr)) //Returns error code if failed
	{
		return hr;
	}

	

	//Copy the vertices into the buffer
	D3D11_MAPPED_SUBRESOURCE ms;

	//To allow CPU to add vertices to the vertex buffer you need to use Map and Unmap
	//Lock the buffer to allow writing
	g_pImmediateContext->Map(g_pVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);

	//Copy the data
	memcpy(ms.pData, vertices, sizeof(vertices)); //Copies the vertex data into the vertex buffer

	//Unlock the buffer
	g_pImmediateContext->Unmap(g_pVertexBuffer, NULL); //Unmap to tell D3D that the copy has finished and free to use.

	//Load and compile the pixel and vertex shaders. Use vs_5_0 to target DX11 hardware only
	ID3DBlob* VS;
	ID3DBlob* PS;
	ID3DBlob* error;

	hr = D3DX11CompileFromFile(
		"shaders.hlsl", //Filename where the shaders are
		0,
		0,
		"VShader",		//Name of the shader if that file we want to compile
		"vs_4_0",		//Target pixel shader version
		0,
		0,
		0,
		&VS,			//Returns an object that stores a buffer of data (ID3DBlob)
		&error,			//Returns an error message if the shader fails to compile
		0
	);

	//Check for shader compile error
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) //Dont fail if error is just a warning
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("shaders.hlsl", 0, 0, "PShader", "ps_4_0", 0, 0, 0, &PS, &error, 0);

	//Check for shader compile error
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) //Dont fail if error is just a warning
		{
			return hr;
		}
	}

	//Create shader objects
	hr = g_pD3DDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = g_pD3DDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &g_pPixelShader);
	if (FAILED(hr))
	{
		return hr;
	}

	//Set the shader objects as active
	g_pImmediateContext->VSSetShader(g_pVertexShader, 0, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, 0, 0);

	//Create and set the input layout object
	D3D11_INPUT_ELEMENT_DESC iedesc[] = //Tells D3D the format of vertices so it can correctly interpret them
	{
		//Input layout is a list of the elements of a vertex descibed by a D3D11_INPUT_ELEMENT_DESC
		{
			"POSITION",						//Tells GPU what value is used for
			0,								//Semantic index
			DXGI_FORMAT_R32G32B32_FLOAT,	//Format of the data (Needs to match what was used to create the vertices)
			0,								//Input slot
			0,								//Offset (How many bytes into the struct the element is)
			D3D11_INPUT_PER_VERTEX_DATA,	//What element is used as
			0
		},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
	};

	hr = g_pD3DDevice->CreateInputLayout(iedesc, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &g_pInputLayout); //Uses the struct to return an input layout
	if (FAILED(hr))
	{
		return hr;
	}

	g_pImmediateContext->IASetInputLayout(g_pInputLayout); //Sets input layout

	//Create constant buffer 0
	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));

	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT; //Can use UpdateSubresource() to update
	constant_buffer_desc.ByteWidth = 16; //Must be a multiple of 16 - IMPORTANT (Constant Buffers are uploaded in 16 byte chunks)
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; //Use as constant buffer

	hr = g_pD3DDevice->CreateBuffer(&constant_buffer_desc, NULL, &g_pConstantBuffer0);

	//CONSTANT_BUFFER0 cb0;
	//cb0.RedAmount = 1.0f;	

	//Upload new values for constant buffer
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer0, 0, 0, &cb0, 0, 0);

	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer0);

	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;

}

//Clean up Direct3D objects
void ShutdownD3D()
{
	//Freeup swap chain, device and device context
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pInputLayout) g_pInputLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pConstantBuffer0) g_pConstantBuffer0->Release();
	if (g_pBackBufferRTView) g_pBackBufferRTView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pD3DDevice) g_pD3DDevice->Release();
}
