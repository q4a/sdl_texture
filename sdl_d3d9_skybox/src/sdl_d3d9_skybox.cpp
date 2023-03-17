
#include "d3d_utility.h"
#include "cube.h"
#include "skybox.h"
#include "vertex.h"

#include <string.h>
#include <SDL2/SDL.h>

// Globals

IDirect3DDevice9* Device = 0;
const int Width = 640;
const int Height = 480;

Cube*              Box = 0;
SkyBox*            Sky = 0;
IDirect3DTexture9* Tex = 0;

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

#ifdef _WIN32
#include <d3dx9.h>
#define FLT_PI D3DX_PI
#else
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define FLT_PI glm::pi<float>()

inline D3DMATRIX Matrix4GlmToD3d(const glm::mat4& mat)
{
	D3DMATRIX matrix = {
		mat[0][0], mat[0][1], mat[0][2], mat[0][3],
		mat[1][0], mat[1][1], mat[1][2], mat[1][3],
		mat[2][0], mat[2][1], mat[2][2], mat[2][3],
		mat[3][0], mat[3][1], mat[3][2], mat[3][3]
	};
	return matrix;
}
#endif

// camera
static float cameraAngle  = (3.0f * FLT_PI) / 2.0f;
static float cameraHeight = 2.0f;

// Framework Functions

bool CreateSkyBox()
{
	if (Device)
	{
		Sky = new SkyBox(Device);
		if (!Sky->InitSkyBox(300))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SkyBox init failed", nullptr);
		}
#ifdef UseCubeTexture
		Sky->SetTexture("textures/earth-cubemap.dds", 0);
#else
		Sky->SetTexture("textures/skybox/right.jpg",  0);
		Sky->SetTexture("textures/skybox/left.jpg",   1);
		Sky->SetTexture("textures/skybox/top.jpg",    2);
		Sky->SetTexture("textures/skybox/bottom.jpg", 3);
		Sky->SetTexture("textures/skybox/front.jpg",  4);
		Sky->SetTexture("textures/skybox/back.jpg",   5);
#endif

		return true;
	}
	else
	{
		return false;
	}
}

bool Setup()
{
	// Create the cube and skybox.

	Box = new Cube(Device);
	CreateSkyBox();

	// Set a directional light.

	D3DLIGHT9 light;
	memset(&light, 0, sizeof(light));
	light.Type      = D3DLIGHT_DIRECTIONAL;
	light.Ambient   = D3DCOLORVALUE(0.8f, 0.8f, 0.8f, 1.0f);
	light.Diffuse   = D3DCOLORVALUE(1.0f, 1.0f, 1.0f, 1.0f);
	light.Specular  = D3DCOLORVALUE(0.2f, 0.2f, 0.2f, 1.0f);
	light.Direction = D3DVECTOR(1.0f, -1.0f, 0.0f);
	Device->SetLight(0, &light);
	Device->LightEnable(0, true);

	Device->SetRenderState(D3DRS_NORMALIZENORMALS, true);
	Device->SetRenderState(D3DRS_SPECULARENABLE, true);

	// Create texture.

	d3d::CreateTextureFromFile(
		Device,
		"textures/cursor.dds",
		&Tex);

	// Set Texture Filter States.

	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

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
	d3d::Delete<Cube*>(Box);
	d3d::Delete<SkyBox*>(Sky);
	d3d::Release<IDirect3DTexture9*>(Tex);
}

void ShowPrimitive()
{
	if (Device)
	{
		// Update the scene: update camera position.

#ifdef _WIN32
		D3DXVECTOR3 pos( cosf(cameraAngle) * 3.0f, cameraHeight, sinf(cameraAngle) * 3.0f );
		D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
		D3DXMATRIX V;
		D3DXMatrixLookAtLH(&V, &pos, &target, &up);
#else
		glm::vec3 position(cosf(cameraAngle) * 3.0f, cameraHeight, sinf(cameraAngle) * 3.0f);
		glm::vec3 target(0.0f, 0.0f, 0.0f);
		glm::vec3 up(0.0f, 1.0f, 0.0f);
		glm::mat4 glmV = glm::lookAtLH(position, target, up);
		D3DMATRIX V = Matrix4GlmToD3d(glmV);
#endif

		Device->SetTransform(D3DTS_VIEW, &V);

		// Draw the scene:

		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
		Device->BeginScene();

		Device->SetMaterial(&d3d::WHITE_MTRL);
		Device->SetTexture(0, Tex);

		Box->draw(0, 0, 0);
		Sky->Render();

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

	float currentFrame;
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	SDL_Event ev;
	const uint8_t* keystate;
	bool running = true;
	while (running)
	{
		keystate = SDL_GetKeyboardState(nullptr);
		while (SDL_PollEvent(&ev))
		{
			if ((ev.type == SDL_QUIT) ||
				(ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE))
			{
				running = false;
				break;
			}
		}
		if (keystate[SDL_SCANCODE_W])
			cameraHeight += 5.0f * deltaTime;
		if (keystate[SDL_SCANCODE_S])
			cameraHeight -= 5.0f * deltaTime;
		if (keystate[SDL_SCANCODE_A])
			cameraAngle -= 2.0f * deltaTime;
		if (keystate[SDL_SCANCODE_D])
			cameraAngle += 2.0f * deltaTime;

		currentFrame = SDL_GetTicks() / 1000.0f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		ShowPrimitive();
	}

	//Cleaning up everything.
	Cleanup();
	Device->Release();
	SDL_Quit();

	return 0;
}
