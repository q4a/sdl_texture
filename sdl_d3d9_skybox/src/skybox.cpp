
#include "skybox.h"
#include "d3d_utility.h"
#include "vertex.h"

#include <SDL2/SDL.h>

SkyBox::SkyBox(IDirect3DDevice9* device)
{
    _device = device;
    _vb = nullptr;
    _ib = nullptr;
#ifdef UseCubeTexture
    _cubetexture = nullptr;
#else
    memset(_texture, 0, sizeof(_texture));
#endif
}

SkyBox::~SkyBox()
{
    if (_vb) { _vb->Release(); _vb = 0; }
    if (_ib) { _ib->Release(); _ib = 0; }
}

bool SkyBox::InitSkyBox(int scale)
{
#ifdef UseCubeTexture
    // create vertex coordinates
    if (FAILED(_device->CreateVertexBuffer(
        24 * sizeof(VertexCube),
        D3DUSAGE_WRITEONLY,
        FVF_VERTEXCUBE,
        D3DPOOL_MANAGED,
        &_vb,
        0)))
    {
        return false;
    }

    VertexCube* v = 0;
    _vb->Lock(0, 0, (void**)&v, 0);

    // positive x - front face
    v[0] = VertexCube(1.0f*scale, -1.0f*scale,  1.0f*scale, 1.0f, -1.0f,  1.0f);
    v[1] = VertexCube(1.0f*scale,  1.0f*scale,  1.0f*scale, 1.0f,  1.0f,  1.0f);
    v[2] = VertexCube(1.0f*scale,  1.0f*scale, -1.0f*scale, 1.0f,  1.0f, -1.0f);
    v[3] = VertexCube(1.0f*scale, -1.0f*scale, -1.0f*scale, 1.0f, -1.0f, -1.0f);

    // negative x - back face
    v[4] = VertexCube(-1.0f*scale, -1.0f*scale, -1.0f*scale, -1.0f, -1.0f, -1.0f);
    v[5] = VertexCube(-1.0f*scale,  1.0f*scale, -1.0f*scale, -1.0f,  1.0f, -1.0f);
    v[6] = VertexCube(-1.0f*scale,  1.0f*scale,  1.0f*scale, -1.0f,  1.0f,  1.0f);
    v[7] = VertexCube(-1.0f*scale, -1.0f*scale,  1.0f*scale, -1.0f, -1.0f,  1.0f);

    // positive y - top face
    v[8] = VertexCube(-1.0f*scale, 1.0f*scale,  1.0f*scale, -1.0f, 1.0f,  1.0f);
    v[9] = VertexCube(-1.0f*scale, 1.0f*scale, -1.0f*scale, -1.0f, 1.0f, -1.0f);
    v[10] = VertexCube(1.0f*scale, 1.0f*scale, -1.0f*scale,  1.0f, 1.0f, -1.0f);
    v[11] = VertexCube(1.0f*scale, 1.0f*scale,  1.0f*scale,  1.0f, 1.0f,  1.0f);

    // negative y - bottom face
    v[12] = VertexCube(-1.0f*scale, -1.0f*scale, -1.0f*scale, -1.0f, -1.0f, -1.0f);
    v[13] = VertexCube(-1.0f*scale, -1.0f*scale,  1.0f*scale, -1.0f, -1.0f,  1.0f);
    v[14] = VertexCube( 1.0f*scale, -1.0f*scale,  1.0f*scale,  1.0f, -1.0f,  1.0f);
    v[15] = VertexCube( 1.0f*scale, -1.0f*scale, -1.0f*scale,  1.0f, -1.0f, -1.0f);

    // positive z - left face
    v[16] = VertexCube(-1.0f*scale, -1.0f*scale, 1.0f*scale, -1.0f, -1.0f, 1.0f);
    v[17] = VertexCube(-1.0f*scale,  1.0f*scale, 1.0f*scale, -1.0f,  1.0f, 1.0f);
    v[18] = VertexCube( 1.0f*scale,  1.0f*scale, 1.0f*scale,  1.0f,  1.0f, 1.0f);
    v[19] = VertexCube( 1.0f*scale, -1.0f*scale, 1.0f*scale,  1.0f, -1.0f, 1.0f);

    // negative z - right face
    v[20] = VertexCube( 1.0f*scale, -1.0f*scale, -1.0f*scale,  1.0f, -1.0f, -1.0f);
    v[21] = VertexCube( 1.0f*scale,  1.0f*scale, -1.0f*scale,  1.0f,  1.0f, -1.0f);
    v[22] = VertexCube(-1.0f*scale,  1.0f*scale, -1.0f*scale, -1.0f,  1.0f, -1.0f);
    v[23] = VertexCube(-1.0f*scale, -1.0f*scale, -1.0f*scale, -1.0f, -1.0f, -1.0f);

    _vb->Unlock();
#else
    // create vertex coordinates
    if (FAILED(_device->CreateVertexBuffer(
        24 * sizeof(TVertex),
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

    // positive x - front face
    v[0] = TVertex(1.0f*scale, -1.0f*scale,  1.0f*scale, 0.0f, 1.0f);
    v[1] = TVertex(1.0f*scale,  1.0f*scale,  1.0f*scale, 0.0f, 0.0f);
    v[2] = TVertex(1.0f*scale,  1.0f*scale, -1.0f*scale, 1.0f, 0.0f);
    v[3] = TVertex(1.0f*scale, -1.0f*scale, -1.0f*scale, 1.0f, 1.0f);

    // negative x - back face
    v[4] = TVertex(-1.0f*scale, -1.0f*scale, -1.0f*scale, 0.0f, 1.0f);
    v[5] = TVertex(-1.0f*scale,  1.0f*scale, -1.0f*scale, 0.0f, 0.0f);
    v[6] = TVertex(-1.0f*scale,  1.0f*scale,  1.0f*scale, 1.0f, 0.0f);
    v[7] = TVertex(-1.0f*scale, -1.0f*scale,  1.0f*scale, 1.0f, 1.0f);

    // positive y - top face
    v[8] = TVertex(-1.0f*scale, 1.0f*scale,  1.0f*scale, 0.0f, 1.0f);
    v[9] = TVertex(-1.0f*scale, 1.0f*scale, -1.0f*scale, 0.0f, 0.0f);
    v[10] = TVertex(1.0f*scale, 1.0f*scale, -1.0f*scale, 1.0f, 0.0f);
    v[11] = TVertex(1.0f*scale, 1.0f*scale,  1.0f*scale, 1.0f, 1.0f);

    // negative y - bottom face
    v[12] = TVertex(-1.0f*scale, -1.0f*scale, -1.0f*scale, 0.0f, 1.0f);
    v[13] = TVertex(-1.0f*scale, -1.0f*scale,  1.0f*scale, 0.0f, 0.0f);
    v[14] = TVertex( 1.0f*scale, -1.0f*scale,  1.0f*scale, 1.0f, 0.0f);
    v[15] = TVertex( 1.0f*scale, -1.0f*scale, -1.0f*scale, 1.0f, 1.0f);

    // positive z - left face
    v[16] = TVertex(-1.0f*scale, -1.0f*scale, 1.0f*scale, 0.0f, 1.0f);
    v[17] = TVertex(-1.0f*scale,  1.0f*scale, 1.0f*scale, 0.0f, 0.0f);
    v[18] = TVertex( 1.0f*scale,  1.0f*scale, 1.0f*scale, 1.0f, 0.0f);
    v[19] = TVertex( 1.0f*scale, -1.0f*scale, 1.0f*scale, 1.0f, 1.0f);

    // negative z - right face
    v[20] = TVertex( 1.0f*scale, -1.0f*scale, -1.0f*scale, 0.0f, 1.0f);
    v[21] = TVertex( 1.0f*scale,  1.0f*scale, -1.0f*scale, 0.0f, 0.0f);
    v[22] = TVertex(-1.0f*scale,  1.0f*scale, -1.0f*scale, 1.0f, 0.0f);
    v[23] = TVertex(-1.0f*scale, -1.0f*scale, -1.0f*scale, 1.0f, 1.0f);

    _vb->Unlock();
#endif

    // create index
    if (FAILED(_device->CreateIndexBuffer(
        36 * sizeof(uint16_t),
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_MANAGED,
        &_ib,
        0)))
    {
        return false;
    }

    uint16_t* i = 0;
    _ib->Lock(0, 0, (void**)&i, 0);

    // positive x - front face
    i[0] = 0;  i[1] = 1;   i[2] = 2;
    i[3] = 0;  i[4] = 2;   i[5] = 3;

    // negative x - back face
    i[6] = 4;  i[7] = 5;   i[8] = 6;
    i[9] = 4;  i[10] = 6;  i[11] = 7;

    // positive y - top face
    i[12] = 8;  i[13] = 9;  i[14] = 10;
    i[15] = 8;  i[16] = 10; i[17] = 11;

    // negative y - bottom face
    i[18] = 12; i[19] = 13; i[20] = 14;
    i[21] = 12; i[22] = 14; i[23] = 15;

    // positive z - left face
    i[24] = 16; i[25] = 17; i[26] = 18;
    i[27] = 16; i[28] = 18; i[29] = 19;

    // negative z - right face
    i[30] = 20; i[31] = 21; i[32] = 22;
    i[33] = 20; i[34] = 22; i[35] = 23;

    _ib->Unlock();

    return true;
}

bool SkyBox::SetTexture(const char* TextureFile, int flag)
{
#ifdef UseCubeTexture
    if (FAILED(d3d::CreateCubeTextureFromFile(
        _device,
        TextureFile,
        &_cubetexture)))
#else
    if (FAILED(d3d::CreateTextureFromFile(
        _device,
        TextureFile,
        &_texture[flag])))
#endif
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SkyBox::SetTexture failed", nullptr);
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

    _device->SetIndices(_ib);
#ifdef UseCubeTexture
    _device->SetStreamSource(0, _vb, 0, sizeof(VertexCube));
    _device->SetFVF(FVF_VERTEXCUBE);
    _device->SetTexture(0, _cubetexture);
    _device->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0,
        0,
        24,
        0,
        12);
#else
    _device->SetStreamSource(0, _vb, 0, sizeof(TVertex));
    _device->SetFVF(FVF_TVERTEX);
    // order is: Right->Left->Up->Down->Front->Back
    for (int i = 0; i < 6; ++i)
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
#endif

    //_device->SetRenderState(D3DRS_LIGHTING, lightState);
}
