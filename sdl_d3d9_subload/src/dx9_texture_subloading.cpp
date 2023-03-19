//-----------------------------------------------------------------------------
// Control Keys: F1 - Toggle subloading
//-----------------------------------------------------------------------------

#include <string>

#include <d3dx9.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
LPDIRECT3DDEVICE9       g_pd3dDevice    = nullptr;
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = nullptr;
LPDIRECT3DTEXTURE9      g_pTexture      = nullptr;

#define D3DFVF_CUSTOMVERTEX ( D3DFVF_XYZ | D3DFVF_TEX1 )

struct Vertex
{
    float x, y, z;
    float tu, tv;
};

Vertex g_quadVertices[] =
{
    {-1.0f, 1.0f, 0.0f,  0.0f,0.0f },
    { 1.0f, 1.0f, 0.0f,  1.0f,0.0f },
    {-1.0f,-1.0f, 0.0f,  0.0f,1.0f },
    { 1.0f,-1.0f, 0.0f,  1.0f,1.0f }
};

bool g_bAlterTexture = true;
bool g_bDoSubload    = true;

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
SDL_Window* CreateWindowContext(std::string title, int Width, int Height);
bool InitD3D(SDL_Window* Window,
    int width, int height,
    bool windowed,
    D3DDEVTYPE deviceType,
    IDirect3DDevice9** device
);
bool Setup();
void LoadTexture();
void LoadSubTexture();
void Cleanup();
void ShowPrimitive();

int main(int argc, char* argv[])
{
    const int Width = 640;
    const int Height = 480;

    //Calling the SDL init stuff.
    SDL_Init(SDL_INIT_EVERYTHING);

    //Creating the context for SDL2.
    SDL_Window* Window = CreateWindowContext("Hello Texture!", Width, Height);

    if (!InitD3D(Window, Width, Height, true, D3DDEVTYPE_HAL, &g_pd3dDevice))
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
            if (SDL_KEYDOWN == ev.type && SDL_SCANCODE_F1 == ev.key.keysym.scancode)
            {
                g_bDoSubload = !g_bDoSubload;
                g_bAlterTexture = true;
            }
        }
        ShowPrimitive();
    }

    //Cleaning up everything.
    Cleanup();
    SDL_Quit();

    return 0;
}

SDL_Window* CreateWindowContext(std::string title, int Width, int Height)
{
    //Declaring the variable the return later.
    SDL_Window* Window = nullptr;

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

void* OSHandle(SDL_Window* Window)
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

bool InitD3D(
    SDL_Window* Window,
    int width, int height,
    bool windowed,
    D3DDEVTYPE deviceType,
    IDirect3DDevice9** device)
{
    // Init D3D:

    HRESULT hr = 0;
    HWND hwnd = static_cast<HWND>(OSHandle(Window));

    // Step 1: Create the IDirect3D9 object.

    IDirect3D9* d3d9 = 0;
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

    if (!d3d9)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Direct3DCreate9() - FAILED", nullptr);
        return false;
    }

    // Step 2: Get display mode.

    D3DDISPLAYMODE d3ddm;
    d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

    // Step 3: Fill out the D3DPRESENT_PARAMETERS structure.

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    d3dpp.Windowed = windowed;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = true;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    // Step 4: Create the device.

    hr = d3d9->CreateDevice(
        D3DADAPTER_DEFAULT, // primary adapter
        D3DDEVTYPE_HAL,     // device type
        hwnd,               // window associated with device
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,// vertex processing
        &d3dpp,             // present parameters
        device);            // return created device

    if (FAILED(hr))
    {
        d3d9->Release(); // done with d3d9 object
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "CreateDevice() - FAILED", nullptr);
        return false;
    }

    d3d9->Release(); // done with d3d9 object

    return true;
}

bool Setup()
{
   LoadTexture();

    g_pd3dDevice->CreateVertexBuffer(4 * sizeof(Vertex), D3DUSAGE_WRITEONLY,
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT,
        &g_pVertexBuffer, nullptr);
    void* pVertices = nullptr;

    g_pVertexBuffer->Lock(0, sizeof(g_quadVertices), (void**)&pVertices, 0);
    memcpy(pVertices, g_quadVertices, sizeof(g_quadVertices));
    g_pVertexBuffer->Unlock();

    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(45.0f),
        640.0f / 480.0f, 0.1f, 100.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    return true;
}

void LoadTexture()
{
    D3DXCreateTextureFromFile(g_pd3dDevice, "textures/chess4.dds", &g_pTexture);

    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
}

void LoadSubTexture()
{
    LPDIRECT3DTEXTURE9 pSubTexture  = nullptr;
    LPDIRECT3DSURFACE9 pDestSurface = nullptr;
    LPDIRECT3DSURFACE9 pSrcSurface  = nullptr;

    D3DXCreateTextureFromFile( g_pd3dDevice, "textures/cursor.dds", &pSubTexture );

    g_pTexture->GetSurfaceLevel( 0, &pDestSurface );
    pSubTexture->GetSurfaceLevel( 0, &pSrcSurface );

    RECT srcRect[]  = {  0,  0, 64, 64 };
    RECT destRect[] = { 32, 32, 96, 96 };

    D3DXLoadSurfaceFromSurface( pDestSurface, nullptr, destRect,
                                pSrcSurface,  nullptr, srcRect,
                                D3DX_DEFAULT, 0);

    pSrcSurface->Release();
    pDestSurface->Release();
    pSubTexture->Release();
}

void Cleanup()
{
    if(g_pTexture != nullptr)
        g_pTexture->Release();

    if(g_pVertexBuffer != nullptr)
        g_pVertexBuffer->Release();

    if(g_pd3dDevice != nullptr)
        g_pd3dDevice->Release();
}

void ShowPrimitive()
{
    g_pd3dDevice->Clear( 0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_COLORVALUE(0.0f,0.0f,0.0f,1.0f), 1.0f, 0 );

    D3DXMATRIX matWorld;
    D3DXMatrixTranslation( &matWorld, 0.0f, 0.0f, 4.0f );
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    if(g_bAlterTexture == true)
    {
        if(g_bDoSubload == true)
            LoadSubTexture();
        else
            LoadTexture();

        g_bAlterTexture = false;
    }

    g_pd3dDevice->BeginScene();

    g_pd3dDevice->SetTexture( 0, g_pTexture );
    g_pd3dDevice->SetStreamSource( 0, g_pVertexBuffer, 0, sizeof(Vertex) );
    g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
    g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    g_pd3dDevice->EndScene();
    g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
}
