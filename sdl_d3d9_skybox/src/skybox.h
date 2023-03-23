/*
 * skybox class, responsible for the creation of skyboxes
 */

#ifndef __skyboxH__
#define __skyboxH__

#include <d3d9.h>

//#undef UseCubeTexture

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
#ifdef UseCubeTexture
    IDirect3DCubeTexture9* _cubetexture;
#else
    IDirect3DTexture9*      _texture[6];
#endif
};
#endif // __skyboxH__
