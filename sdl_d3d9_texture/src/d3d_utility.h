//////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: d3dUtility.h
//
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0
//
// Desc: Provides utility functions for simplifying common tasks.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

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

	template<class T> void Release(T t)
	{
		if( t )
		{
			t->Release();
			t = 0;
		}
	}
}

#endif // __d3d_utility__
