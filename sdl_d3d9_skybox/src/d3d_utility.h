
#ifndef __d3d_utility__
#define __d3d_utility__

#include <d3d9.h>
#include <SDL2/SDL.h>
#include <string>

namespace d3d
{
	void* OSHandle(SDL_Window* Window);
	bool InitD3D(
		SDL_Window* Window,        // [in] SDL Window handler
		int width, int height,     // [in] Backbuffer dimensions.
		bool windowed,             // [in] Windowed (true)or full screen (false).
		D3DDEVTYPE deviceType,     // [in] HAL or REF
		IDirect3DDevice9** device);// [out]The created device.

	HRESULT CreateTextureFromFile(
		IDirect3DDevice9 *device,
		const char *srcfile,
		IDirect3DTexture9 **texture);

	template<class T> void Release(T t)
	{
		if( t )
		{
			t->Release();
			t = 0;
		}
	}

	template<class T> void Delete(T t)
	{
		if( t )
		{
			delete t;
			t = 0;
		}
	}

	const D3DCOLORVALUE      WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
	const D3DCOLORVALUE      BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Materials

	D3DMATERIAL9 InitMtrl(D3DCOLORVALUE a, D3DCOLORVALUE d, D3DCOLORVALUE s, D3DCOLORVALUE e, float p);
	const D3DMATERIAL9 WHITE_MTRL  = InitMtrl(WHITE, WHITE, WHITE, BLACK, 2.0f);
}

#endif // __d3d_utility__
