
#include "skybox.h"
#include "vertex.h"

#include <SDL2/SDL.h>

SkyBox::SkyBox(IDirect3DDevice9* device)
{
    _device = device;
    _vb = nullptr;
    _ib = nullptr;
    memset(_texture, 0, sizeof(_texture));
}

SkyBox::~SkyBox()
{
    if (_vb) { _vb->Release(); _vb = 0; }
    if (_ib) { _ib->Release(); _ib = 0; }
}

bool SkyBox::InitSkyBox(int scale)
{
    // create vertex coordinates
    if (FAILED(_device->CreateVertexBuffer(
        20 * sizeof(TVertex),
        D3DUSAGE_WRITEONLY,
        FVF_TVERTEX,
        D3DPOOL_MANAGED,
        &_vb,
        0)))
    {
        return false;
    }

    TVertex* v = 0;
    _vb->Lock(0, 0, (void**)&v, 0);

    // positive x
    v[0] = TVertex(1.0f*scale, -1.0f*scale,  1.0f*scale, 0.0f, 1.0f);
    v[1] = TVertex(1.0f*scale,  1.0f*scale,  1.0f*scale, 0.0f, 0.0f);
    v[2] = TVertex(1.0f*scale,  1.0f*scale, -1.0f*scale, 1.0f, 0.0f);
    v[3] = TVertex(1.0f*scale, -1.0f*scale, -1.0f*scale, 1.0f, 1.0f);

    // negative x
    v[4] = TVertex(-1.0f*scale, -1.0f*scale, -1.0f*scale, 0.0f, 1.0f);
    v[5] = TVertex(-1.0f*scale,  1.0f*scale, -1.0f*scale, 0.0f, 0.0f);
    v[6] = TVertex(-1.0f*scale,  1.0f*scale,  1.0f*scale, 1.0f, 0.0f);
    v[7] = TVertex(-1.0f*scale, -1.0f*scale,  1.0f*scale, 1.0f, 1.0f);

    // positive y
    v[8] = TVertex(-1.0f*scale, 1.0f*scale,  1.0f*scale, 0.0f, 1.0f);
    v[9] = TVertex(-1.0f*scale, 1.0f*scale, -1.0f*scale, 0.0f, 0.0f);
    v[10] = TVertex(1.0f*scale, 1.0f*scale, -1.0f*scale, 1.0f, 0.0f);
    v[11] = TVertex(1.0f*scale, 1.0f*scale,  1.0f*scale, 1.0f, 1.0f);

    // positive z
    v[12] = TVertex(-1.0f*scale, -1.0f*scale, 1.0f*scale, 0.0f, 1.0f);
    v[13] = TVertex(-1.0f*scale,  1.0f*scale, 1.0f*scale, 0.0f, 0.0f);
    v[14] = TVertex( 1.0f*scale,  1.0f*scale, 1.0f*scale, 1.0f, 0.0f);
    v[15] = TVertex( 1.0f*scale, -1.0f*scale, 1.0f*scale, 1.0f, 1.0f);

    // negative z
    v[16] = TVertex( 1.0f*scale, -1.0f*scale, -1.0f*scale, 0.0f, 1.0f);
    v[17] = TVertex( 1.0f*scale,  1.0f*scale, -1.0f*scale, 0.0f, 0.0f);
    v[18] = TVertex(-1.0f*scale,  1.0f*scale, -1.0f*scale, 1.0f, 0.0f);
    v[19] = TVertex(-1.0f*scale, -1.0f*scale, -1.0f*scale, 1.0f, 1.0f);

    _vb->Unlock();

    // create index
    if (FAILED(_device->CreateIndexBuffer(
        30 * sizeof(uint16_t),
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_MANAGED,
        &_ib,
        0)))
    {
        return false;
    }

    uint16_t* indices = 0;
    _ib->Lock(0, 0, (void**)&indices, 0);

    // positive x
    indices[0] = 0;	indices[1] = 1;	indices[2] = 2;
    indices[3] = 0;	indices[4] = 2;	indices[5] = 3;

    // negative x
    indices[6] = 4;  indices[7] = 5;  indices[8] = 6;
    indices[9] = 4;  indices[10] = 6;  indices[11] = 7;

    // positive y
    indices[12] = 8;  indices[13] = 9;  indices[14] = 10;
    indices[15] = 8;  indices[16] = 10; indices[17] = 11;

    // positive z
    indices[18] = 12; indices[19] = 13; indices[20] = 14;
    indices[21] = 12; indices[22] = 14; indices[23] = 15;

    // negative z
    indices[24] = 16; indices[25] = 17; indices[26] = 18;
    indices[27] = 16; indices[28] = 18; indices[29] = 19;

    _ib->Unlock();

    return true;
}

bool SkyBox::SetTexture(const char* TextureFile, int flag)
{
    if (FAILED(D3DXCreateTextureFromFile(
        _device,
        TextureFile,
        &_texture[flag])))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "D3DXCreateTextureFromFile failed", nullptr);
        return false;
    }
    return true;
}

void SkyBox::Render()
{
    // set filter
    _device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    _device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    _device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

    // set addressing mode
    _device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    _device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // do not turn off the light
    /*uint32_t lightState;
    _device->GetRenderState(D3DRS_LIGHTING, &lightState);
    _device->SetRenderState(D3DRS_LIGHTING, false);*/

    _device->SetStreamSource(0, _vb, 0, sizeof(TVertex));
    _device->SetFVF(FVF_TVERTEX);
    _device->SetIndices(_ib); // set index cache

    // order is: Right->Left->Up->Front->Back
    for (int i = 0; i < 5; ++i)
    {
        _device->SetTexture(0, _texture[i]);
        _device->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            0,     // starting address of the index buffer that will be drawn
            i * 4, // smallest index value in the index array
            4,     // number of vertices in the index array to draw
            i * 6, // starting from the number element in the index array to draw the primitive
            2);    // number of primitives drawn
    }

    //_device->SetRenderState(D3DRS_LIGHTING, lightState);
}
