/*
 * skybox class, responsible for the creation of skyboxes
 */

#ifndef __skyboxH__
#define __skyboxH__

#include <d3dx9.h>

class SkyBox
{
public:
    SkyBox(IDirect3DDevice9* device);
    ~SkyBox();

    bool InitSkyBox(int scale);

    void Render();
    bool SetTexture(const char* textureFile, int flag);

private:
    IDirect3DDevice9*       _device;
    IDirect3DVertexBuffer9* _vb;
    IDirect3DIndexBuffer9*  _ib;
    IDirect3DTexture9*      _texture[5]; // ground not included
};
#endif __skyboxH__
