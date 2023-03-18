
#include "d3d_utility.h"

#include <SDL2/SDL_syswm.h>

#ifdef _WIN32
#include <d3dx9.h>
#else
#include <unordered_map>
#include <gli/gli.hpp>

const std::unordered_map<gli::format, D3DFORMAT> gli_format_map{
	{ gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8, D3DFMT_DXT1 },
	{ gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16, D3DFMT_DXT5 },
	{ gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16, D3DFMT_DXT5 },
};
#endif

void* d3d::OSHandle(SDL_Window* Window)
{
	if (!Window)
		return nullptr;

#ifdef _WIN32
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(Window, &info);
	return info.info.win.window;
#else
	return Window;
#endif
}

bool d3d::InitD3D(
	SDL_Window* Window,
	int width, int height,
	bool windowed,
	D3DDEVTYPE deviceType,
	IDirect3DDevice9** device)
{
	// Init D3D:

	HRESULT hr = 0;
	HWND hwnd = static_cast<HWND>(d3d::OSHandle(Window));

	// Step 1: Create the IDirect3D9 object.

	IDirect3D9* d3d9 = 0;
	d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	if( !d3d9 )
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Direct3DCreate9() - FAILED", nullptr);
		return false;
	}

	// Step 2: Check for hardware vp.

	D3DCAPS9 caps;
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, deviceType, &caps);

	int vp = 0;
	if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// Step 3: Fill out the D3DPRESENT_PARAMETERS structure.

	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth            = width;
	d3dpp.BackBufferHeight           = height;
	d3dpp.BackBufferFormat           = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount            = 1;
	d3dpp.MultiSampleType            = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality         = 0;
	d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow              = hwnd;
	d3dpp.Windowed                   = windowed;
	d3dpp.EnableAutoDepthStencil     = true;
	d3dpp.AutoDepthStencilFormat     = D3DFMT_D24S8;
	d3dpp.Flags                      = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Step 4: Create the device.

	hr = d3d9->CreateDevice(
		D3DADAPTER_DEFAULT, // primary adapter
		deviceType,         // device type
		hwnd,               // window associated with device
		vp,                 // vertex processing
		&d3dpp,             // present parameters
		device);            // return created device

	if( FAILED(hr) )
	{
		// try again using a 16-bit depth buffer
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

		hr = d3d9->CreateDevice(
			D3DADAPTER_DEFAULT,
			deviceType,
			hwnd,
			vp,
			&d3dpp,
			device);

		if( FAILED(hr) )
		{
			d3d9->Release(); // done with d3d9 object
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "CreateDevice() - FAILED", nullptr);
			return false;
		}
	}

	d3d9->Release(); // done with d3d9 object

	return true;
}

HRESULT d3d::CreateTextureFromFile(
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

	hr = device->CreateTexture(dimensions.x, dimensions.y, 1, 0, gli_format_map.at(tex.format()), D3DPOOL_MANAGED, texture, nullptr);
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

HRESULT d3d::CreateCubeTextureFromFile(
	IDirect3DDevice9 *device,
	const char *srcfile,
	IDirect3DCubeTexture9 **texture)
{
#ifdef _WIN32
	return D3DXCreateCubeTextureFromFile(device, srcfile, texture);
#else

	gli::texture_cube tex = gli::texture_cube(gli::load(srcfile));
	const auto dimensions = tex.extent();
	HRESULT hr;

	hr = device->CreateCubeTexture(dimensions.x, 1, 0, gli_format_map.at(tex.format()), D3DPOOL_MANAGED, texture, nullptr);
	if (FAILED(hr))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "LockRect failed", nullptr);
		return hr;
	}

	D3DLOCKED_RECT rect;
	char* dest;
	auto maxface = tex.max_face();
	for (int i = 0; i <= maxface; ++i)
	{
		hr = (*texture)->LockRect( (D3DCUBEMAP_FACES)i, 0, &rect, 0, D3DLOCK_DISCARD );
		if (FAILED(hr))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "LockRect failed", nullptr);
			return hr;
		}
		dest = static_cast<char*>(rect.pBits);
		memcpy(dest, tex[i].data(), tex[i].size());
		hr = (*texture)->UnlockRect( (D3DCUBEMAP_FACES)i, 0 );
		if (FAILED(hr))
			break;
	}

	return hr;
#endif
}

D3DMATERIAL9 d3d::InitMtrl(D3DCOLORVALUE a, D3DCOLORVALUE d, D3DCOLORVALUE s, D3DCOLORVALUE e, float p)
{
	D3DMATERIAL9 mtrl;
	mtrl.Ambient  = a;
	mtrl.Diffuse  = d;
	mtrl.Specular = s;
	mtrl.Emissive = e;
	mtrl.Power    = p;
	return mtrl;
}
