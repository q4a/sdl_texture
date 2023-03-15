
#include "d3d_utility.h"

#include <string.h>
#include <SDL2/SDL.h>

// Globals

IDirect3DDevice9* Device = 0;
const int Width = 640;
const int Height = 480;
const int SECOND = 1000;

IDirect3DVertexBuffer9* Quad = 0;
IDirect3DTexture9*      Tex  = 0;

// Classes and Structures

struct Vertex
{
	Vertex() {}
	Vertex(
		float x, float y, float z,
		float nx, float ny, float nz,
		float u, float v)
	{
		_x = x;  _y = y;  _z = z;
		_nx = nx; _ny = ny; _nz = nz;
		_u = u;  _v = v;
	}
	float _x, _y, _z;
	float _nx, _ny, _nz;
	float _u, _v; // texture coordinates

	static const DWORD FVF;
};
const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

// Additional math functions

static inline D3DMATRIX* MatrixIdentity(D3DMATRIX* pout)
{
	if (!pout) return nullptr;
	pout->m[0][1] = 0.0f;
	pout->m[0][2] = 0.0f;
	pout->m[0][3] = 0.0f;
	pout->m[1][0] = 0.0f;
	pout->m[1][2] = 0.0f;
	pout->m[1][3] = 0.0f;
	pout->m[2][0] = 0.0f;
	pout->m[2][1] = 0.0f;
	pout->m[2][3] = 0.0f;
	pout->m[3][0] = 0.0f;
	pout->m[3][1] = 0.0f;
	pout->m[3][2] = 0.0f;
	pout->m[0][0] = 1.0f;
	pout->m[1][1] = 1.0f;
	pout->m[2][2] = 1.0f;
	pout->m[3][3] = 1.0f;
	return pout;
}

D3DMATRIX* MatrixPerspectiveFovLH(D3DMATRIX* pout, float fovy, float aspect, float zn, float zf)
{
	MatrixIdentity(pout);
	pout->m[0][0] = 1.0f / (aspect * tanf(fovy / 2.0f));
	pout->m[1][1] = 1.0f / tanf(fovy / 2.0f);
	pout->m[2][2] = zf / (zf - zn);
	pout->m[2][3] = 1.0f;
	pout->m[3][2] = (zf * zn) / (zn - zf);
	pout->m[3][3] = 0.0f;
	return pout;
}

// Some d3dx9 functions

#ifdef _WIN32
#include <d3dx9.h>
#else
#include <unordered_map>
#include <gli/gli.hpp>

const std::unordered_map<gli::format, D3DFORMAT> gli_format_map{
	{ gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16, D3DFMT_DXT5 },
	{ gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16, D3DFMT_DXT5 },
};
#endif

HRESULT CreateTextureFromFile(
	IDirect3DDevice9 *device,
	const char *srcfile,
	IDirect3DTexture9 **texture)
{
#ifdef _WIN32
	return D3DXCreateTextureFromFile(device, srcfile, texture);
#else

	gli::texture tex = gli::load(srcfile);
	const auto dimensions = tex.extent();
	HRESULT hr;

	hr = Device->CreateTexture(dimensions.x, dimensions.y, 1, 0, gli_format_map.at(tex.format()), D3DPOOL_MANAGED, texture, nullptr);
	if (FAILED(hr))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "LockRect failed", nullptr);
		return hr;
	}

	D3DLOCKED_RECT rect;
	hr = (*texture)->LockRect( 0, &rect, 0, D3DLOCK_DISCARD );
	if (FAILED(hr))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "LockRect failed", nullptr);
		return hr;
	}
	char* dest = static_cast<char*>(rect.pBits);
	memcpy(dest, tex.data(), tex.size());
	hr = (*texture)->UnlockRect(0);

	return hr;
#endif
}

// Framework Functions

bool Setup()
{
	// Create the vertex buffer.

	Device->CreateVertexBuffer(
		6 * sizeof(Vertex), // size in bytes
		D3DUSAGE_WRITEONLY, // flags
		Vertex::FVF,        // vertex format
		D3DPOOL_MANAGED,    // managed memory pool
		&Quad,              // return create vertex buffer
		0);                 // not used - set to 0

	// Fill the buffers with the triangle data.

	Vertex* v;
	Quad->Lock(0, 0, (void**)&v, 0);

	// quad built from two triangles, note texture coordinates:
	v[0] = Vertex(-1.0f, -1.0f, 1.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[1] = Vertex(-1.0f, 1.0f, 1.25f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[2] = Vertex(1.0f, 1.0f, 1.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	v[3] = Vertex(-1.0f, -1.0f, 1.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[4] = Vertex(1.0f, 1.0f, 1.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[5] = Vertex(1.0f, -1.0f, 1.25f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	Quad->Unlock();

	//
	// Create the texture and set filters.
	//

	CreateTextureFromFile(
		Device,
		"textures/cursor.dds",
		&Tex);

	Device->SetTexture(0, Tex);

	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

	//
	// Don't use lighting for this sample.
	//
	Device->SetRenderState(D3DRS_LIGHTING, false);

	// Set the projection matrix.

	D3DMATRIX proj;
	MatrixPerspectiveFovLH(
		&proj,                        // result
		M_PI * 0.5f,                  // 90 - degrees
		(float)Width / (float)Height, // aspect ratio
		1.0f,                         // near plane
		1000.0f);                     // far plane
	Device->SetTransform(D3DTS_PROJECTION, &proj);

	return true;
}

void Cleanup()
{
	d3d::Release<IDirect3DVertexBuffer9*>(Quad);
	d3d::Release<IDirect3DTexture9*>(Tex);
}

void ShowPrimitive()
{
	if (Device)
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
		Device->BeginScene();

		Device->SetStreamSource(0, Quad, 0, sizeof(Vertex));
		Device->SetFVF(Vertex::FVF);

		// Draw one triangle.
		Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
	}
}

// init ... The init function, it calls the SDL init function.
int initSDL() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		return -1;
	}

	return 0;
}

// createWindowContext ... Creating the window for later use in rendering and stuff.
SDL_Window* createWindowContext(std::string title) {
	//Declaring the variable the return later.
	SDL_Window* Window = NULL;

	uint32_t flags;
#if defined(_WIN32) || defined(USE_NINE)
	flags = SDL_WINDOW_OPENGL;
#else // for DXVK Native
	flags = SDL_WINDOW_VULKAN;
#endif

	//Creating the window and passing that reference to the previously declared variable.
	Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height, flags);

	//Returning the newly creted Window context.
	return Window;
}

// main ... The main function, right now it just calls the initialization of SDL.
int main(int argc, char* argv[]) {
	//Calling the SDL init stuff.
	initSDL();

	//Creating the context for SDL2.
	SDL_Window* Window = createWindowContext("Hello Texture!");

	if (!d3d::InitD3D(Window,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "InitD3D() - FAILED", nullptr);
		return 0;
	}

	if (!Setup())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Setup() - FAILED", nullptr);
		return 0;
	}

	bool running = true;
	while (running)
	{
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			if ((SDL_QUIT == ev.type) ||
				(SDL_KEYDOWN == ev.type && SDL_SCANCODE_ESCAPE == ev.key.keysym.scancode))
			{
				running = false;
				break;
			}
		}
		ShowPrimitive();
	}

	//Cleaning up everything.
	Cleanup();
	Device->Release();
	SDL_Quit();

	return 0;
}
