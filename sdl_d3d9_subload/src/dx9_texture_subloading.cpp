//-----------------------------------------------------------------------------
// Control Keys: F1 - Toggle subloading
//-----------------------------------------------------------------------------

#include <string>

#include <d3d9.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#ifdef _WIN32
//#define UseD3DX9
#endif

#ifdef UseD3DX9
#include <d3dx9.h>
#else
#include <gli/gli.hpp>
#define LoadSurfaceFromSurfaceV2
#endif

// D3DX9 functions

// from wine-8.2

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

D3DMATRIX* MatrixTranslation(D3DMATRIX* pout, float x, float y, float z)
{
    MatrixIdentity(pout);
    pout->m[3][0] = x;
    pout->m[3][1] = y;
    pout->m[3][2] = z;
    return pout;
}

#ifdef LoadSurfaceFromSurfaceV1

static inline bool WinSetRect(LPRECT rect, int left, int top, int right, int bottom)
{
    if (!rect) return false;
    rect->left = left;
    rect->right = right;
    rect->top = top;
    rect->bottom = bottom;
    return true;
}

HRESULT LoadSurfaceFromSurface(IDirect3DSurface9* dst_surface,
                               const PALETTEENTRY* dst_palette, const RECT* dst_rect, IDirect3DSurface9* src_surface,
                               const PALETTEENTRY* src_palette, const RECT* src_rect, DWORD filter, D3DCOLOR color_key)
{
    D3DSURFACE_DESC src_desc;
    D3DLOCKED_RECT lock;
    HRESULT hr;

    if (!dst_surface || !src_surface)
        return D3DERR_INVALIDCALL;

    src_surface->GetDesc(&src_desc);
    if (!src_rect)
    {
        RECT s;
        WinSetRect(&s, 0, 0, src_desc.Width, src_desc.Height);
        src_rect = &s;
    }

    if (!dst_rect)
    {
        D3DSURFACE_DESC dst_desc;
        dst_surface->GetDesc(&dst_desc);
        RECT s;
        WinSetRect(&s, 0, 0, dst_desc.Width, dst_desc.Height);
        dst_rect = &s;
    }

    if (FAILED(src_surface->LockRect(&lock, nullptr, D3DLOCK_READONLY)))
        return D3DERR_INVALIDCALL;

    hr = LoadSurfaceFromMemory(dst_surface, dst_palette, dst_rect, lock.pBits,
                                   src_desc.Format, lock.Pitch, src_palette, src_rect, filter, color_key);

    if (FAILED(src_surface->UnlockRect()))
        return D3DERR_INVALIDCALL;

    return hr;
}
#endif

#ifdef LoadSurfaceFromSurfaceV2

/************************************************************
 * helper functions for D3DXLoadSurfaceFromSurface
 */

// wine-8.2/include/d3dx9.h

#define D3DX_DEFAULT         ((UINT)-1)

// wine-8.2/include/d3dx9tex.h

#define D3DX_FILTER_NONE                 0x00000001
#define D3DX_FILTER_POINT                0x00000002
#define D3DX_FILTER_LINEAR               0x00000003
#define D3DX_FILTER_TRIANGLE             0x00000004
#define D3DX_FILTER_BOX                  0x00000005
#define D3DX_FILTER_MIRROR_U             0x00010000
#define D3DX_FILTER_MIRROR_V             0x00020000
#define D3DX_FILTER_MIRROR_W             0x00040000
#define D3DX_FILTER_MIRROR               0x00070000
#define D3DX_FILTER_DITHER               0x00080000

// wine-8.2/include/winuser.h

static inline BOOL WinSetRect(LPRECT rect, INT left, INT top, INT right, INT bottom)
{
    if (!rect) return FALSE;
    rect->left   = left;
    rect->right  = right;
    rect->top    = top;
    rect->bottom = bottom;
    return TRUE;
}

// wine-8.2/dlls/d3dx9_36/d3dx9_private.h

struct vec4
{
    float x, y, z, w;
};

struct volume
{
    UINT width;
    UINT height;
    UINT depth;
};

/* for internal use */
enum format_type {
    FORMAT_ARGB,   /* unsigned */
    FORMAT_ARGBF16,/* float 16 */
    FORMAT_ARGBF,  /* float */
    FORMAT_DXT,
    FORMAT_INDEX,
    FORMAT_UNKNOWN
};

struct pixel_format_desc {
    D3DFORMAT format;
    BYTE bits[4];
    BYTE shift[4];
    UINT bytes_per_pixel;
    UINT block_width;
    UINT block_height;
    UINT block_byte_count;
    enum format_type type;
    void (*from_rgba)(const struct vec4 *src, struct vec4 *dst);
    void (*to_rgba)(const struct vec4 *src, struct vec4 *dst, const PALETTEENTRY *palette);
};

static inline BOOL is_conversion_from_supported(const struct pixel_format_desc *format)
{
    if (format->type == FORMAT_ARGB || format->type == FORMAT_ARGBF16
        || format->type == FORMAT_ARGBF || format->type == FORMAT_DXT)
        return TRUE;
    return !!format->to_rgba;
}

static inline BOOL is_conversion_to_supported(const struct pixel_format_desc *format)
{
    if (format->type == FORMAT_ARGB || format->type == FORMAT_ARGBF16
        || format->type == FORMAT_ARGBF || format->type == FORMAT_DXT)
        return TRUE;
    return !!format->from_rgba;
}

// wine-8.2/dlls/d3dx9_36/util.c

static void la_from_rgba(const struct vec4 *rgba, struct vec4 *la)
{
    la->x = rgba->x * 0.2125f + rgba->y * 0.7154f + rgba->z * 0.0721f;
    la->w = rgba->w;
}

static void la_to_rgba(const struct vec4 *la, struct vec4 *rgba, const PALETTEENTRY *palette)
{
    rgba->x = la->x;
    rgba->y = la->x;
    rgba->z = la->x;
    rgba->w = la->w;
}

static void index_to_rgba(const struct vec4 *index, struct vec4 *rgba, const PALETTEENTRY *palette)
{
    ULONG idx = (ULONG)(index->x * 255.0f + 0.5f);

    rgba->x = palette[idx].peRed / 255.0f;
    rgba->y = palette[idx].peGreen / 255.0f;
    rgba->z = palette[idx].peBlue / 255.0f;
    rgba->w = palette[idx].peFlags / 255.0f; /* peFlags is the alpha component in DX8 and higher */
}

/************************************************************
 * pixel format table providing info about number of bytes per pixel,
 * number of bits per channel and format type.
 *
 * Call get_format_info to request information about a specific format.
 */
static const struct pixel_format_desc formats[] =
    {
        /* format              bpc               shifts             bpp blocks   type            from_rgba     to_rgba */
        {D3DFMT_R8G8B8,        { 0,  8,  8,  8}, { 0, 16,  8,  0},  3, 1, 1,  3, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8R8G8B8,      { 8,  8,  8,  8}, {24, 16,  8,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_X8R8G8B8,      { 0,  8,  8,  8}, { 0, 16,  8,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8B8G8R8,      { 8,  8,  8,  8}, {24,  0,  8, 16},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_X8B8G8R8,      { 0,  8,  8,  8}, { 0,  0,  8, 16},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_R5G6B5,        { 0,  5,  6,  5}, { 0, 11,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_X1R5G5B5,      { 0,  5,  5,  5}, { 0, 10,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A1R5G5B5,      { 1,  5,  5,  5}, {15, 10,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_R3G3B2,        { 0,  3,  3,  2}, { 0,  5,  2,  0},  1, 1, 1,  1, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8R3G3B2,      { 8,  3,  3,  2}, { 8,  5,  2,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A4R4G4B4,      { 4,  4,  4,  4}, {12,  8,  4,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_X4R4G4B4,      { 0,  4,  4,  4}, { 0,  8,  4,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A2R10G10B10,   { 2, 10, 10, 10}, {30, 20, 10,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A2B10G10R10,   { 2, 10, 10, 10}, {30,  0, 10, 20},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A16B16G16R16,  {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_G16R16,        { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8,            { 8,  0,  0,  0}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8L8,          { 8,  8,  0,  0}, { 8,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
        {D3DFMT_A4L4,          { 4,  4,  0,  0}, { 4,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
        {D3DFMT_L8,            { 0,  8,  0,  0}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
        {D3DFMT_L16,           { 0, 16,  0,  0}, { 0,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
        {D3DFMT_DXT1,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4,  8, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_DXT2,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_DXT3,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_DXT4,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_DXT5,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_R16F,          { 0, 16,  0,  0}, { 0,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGBF16, NULL,         NULL      },
        {D3DFMT_G16R16F,       { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, FORMAT_ARGBF16, NULL,         NULL      },
        {D3DFMT_A16B16G16R16F, {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, FORMAT_ARGBF16, NULL,         NULL      },
        {D3DFMT_R32F,          { 0, 32,  0,  0}, { 0,  0,  0,  0},  4, 1, 1,  4, FORMAT_ARGBF,   NULL,         NULL      },
        {D3DFMT_G32R32F,       { 0, 32, 32,  0}, { 0,  0, 32,  0},  8, 1, 1,  8, FORMAT_ARGBF,   NULL,         NULL      },
        {D3DFMT_A32B32G32R32F, {32, 32, 32, 32}, {96,  0, 32, 64}, 16, 1, 1, 16, FORMAT_ARGBF,   NULL,         NULL      },
        {D3DFMT_P8,            { 8,  8,  8,  8}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_INDEX,   NULL,         index_to_rgba},
        /* marks last element */
        {D3DFMT_UNKNOWN,       { 0,  0,  0,  0}, { 0,  0,  0,  0},  0, 1, 1,  0, FORMAT_UNKNOWN, NULL,         NULL      },
        };

/************************************************************
 * get_format_info
 *
 * Returns information about the specified format.
 * If the format is unsupported, it's filled with the D3DFMT_UNKNOWN desc.
 *
 * PARAMS
 *   format [I] format whose description is queried
 *
 */
const struct pixel_format_desc *get_format_info(D3DFORMAT format)
{
    unsigned int i = 0;
    while(formats[i].format != format && formats[i].format != D3DFMT_UNKNOWN) i++;
//    if (formats[i].format == D3DFMT_UNKNOWN)
//        FIXME("Unknown format %#x (as FOURCC %s).\n", format, debugstr_an((const char *)&format, 4));
    return &formats[i];
}

// wine-8.2/dlls/d3dx9_36/surface.c

HRESULT lock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect, D3DLOCKED_RECT *lock,
                     IDirect3DSurface9 **temp_surface, BOOL write)
{
    unsigned int width, height;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    DWORD lock_flag;
    HRESULT hr;

    lock_flag = write ? 0 : D3DLOCK_READONLY;
    *temp_surface = NULL;
    if (FAILED(hr = IDirect3DSurface9_LockRect(surface, lock, surface_rect, lock_flag)))
    {
        IDirect3DSurface9_GetDevice(surface, &device);
        IDirect3DSurface9_GetDesc(surface, &desc);

        if (surface_rect)
        {
            width = surface_rect->right - surface_rect->left;
            height = surface_rect->bottom - surface_rect->top;
        }
        else
        {
            width = desc.Width;
            height = desc.Height;
        }

        hr = write ? IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height,
                                                                  desc.Format, D3DPOOL_SYSTEMMEM, temp_surface, NULL)
                   : IDirect3DDevice9_CreateRenderTarget(device, width, height,
                                                         desc.Format, D3DMULTISAMPLE_NONE, 0, TRUE, temp_surface, NULL);
        if (FAILED(hr))
        {
//            WARN("Failed to create temporary surface, surface %p, format %#x, "
//                 "usage %#lx, pool %#x, write %#x, width %u, height %u.\n",
//                 surface, desc.Format, desc.Usage, desc.Pool, write, width, height);
            IDirect3DDevice9_Release(device);
            return hr;
        }

        if (write || SUCCEEDED(hr = IDirect3DDevice9_StretchRect(device, surface, surface_rect,
                                                                 *temp_surface, NULL, D3DTEXF_NONE)))
            hr = IDirect3DSurface9_LockRect(*temp_surface, lock, NULL, lock_flag);

        IDirect3DDevice9_Release(device);
        if (FAILED(hr))
        {
//            WARN("Failed to lock surface %p, write %#x, usage %#lx, pool %#x.\n",
//                 surface, write, desc.Usage, desc.Pool);
            IDirect3DSurface9_Release(*temp_surface);
            *temp_surface = NULL;
            return hr;
        }
//        TRACE("Created temporary surface %p.\n", surface);
    }
    return hr;
}

HRESULT unlock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect,
                       IDirect3DSurface9 *temp_surface, BOOL update)
{
    IDirect3DDevice9 *device;
    POINT surface_point;
    HRESULT hr;

    if (!temp_surface)
    {
        hr = IDirect3DSurface9_UnlockRect(surface);
        return hr;
    }

    hr = IDirect3DSurface9_UnlockRect(temp_surface);
    if (update)
    {
        if (surface_rect)
        {
            surface_point.x = surface_rect->left;
            surface_point.y = surface_rect->top;
        }
        else
        {
            surface_point.x = 0;
            surface_point.y = 0;
        }
        IDirect3DSurface9_GetDevice(surface, &device);
//        if (FAILED(hr = IDirect3DDevice9_UpdateSurface(device, temp_surface, NULL, surface, &surface_point)))
//            WARN("Updating surface failed, hr %#lx, surface %p, temp_surface %p.\n",
//                 hr, surface, temp_surface);
        IDirect3DDevice9_Release(device);
    }
    IDirect3DSurface9_Release(temp_surface);
    return hr;
}

/************************************************************
 * helper functions for D3DXLoadSurfaceFromMemory
 */
struct argb_conversion_info
{
    const struct pixel_format_desc *srcformat;
    const struct pixel_format_desc *destformat;
    DWORD srcshift[4], destshift[4];
    DWORD srcmask[4], destmask[4];
    BOOL process_channel[4];
    DWORD channelmask;
};

static void init_argb_conversion_info(const struct pixel_format_desc *srcformat, const struct pixel_format_desc *destformat, struct argb_conversion_info *info)
{
    UINT i;
    ZeroMemory(info->process_channel, 4 * sizeof(BOOL));
    info->channelmask = 0;

    info->srcformat  =  srcformat;
    info->destformat = destformat;

    for(i = 0;i < 4;i++) {
        /* srcshift is used to extract the _relevant_ components */
        info->srcshift[i]  =  srcformat->shift[i] + std::max( srcformat->bits[i] - destformat->bits[i], 0);

        /* destshift is used to move the components to the correct position */
        info->destshift[i] = destformat->shift[i] + std::max(destformat->bits[i] -  srcformat->bits[i], 0);

        info->srcmask[i]  = ((1 <<  srcformat->bits[i]) - 1) <<  srcformat->shift[i];
        info->destmask[i] = ((1 << destformat->bits[i]) - 1) << destformat->shift[i];

        /* channelmask specifies bits which aren't used in the source format but in the destination one */
        if(destformat->bits[i]) {
            if(srcformat->bits[i]) info->process_channel[i] = TRUE;
            else info->channelmask |= info->destmask[i];
        }
    }
}

/************************************************************
 * get_relevant_argb_components
 *
 * Extracts the relevant components from the source color and
 * drops the less significant bits if they aren't used by the destination format.
 */
static void get_relevant_argb_components(const struct argb_conversion_info *info, const BYTE *col, DWORD *out)
{
    unsigned int i, j;
    unsigned int component, mask;

    for (i = 0; i < 4; ++i)
    {
        if (!info->process_channel[i])
            continue;

        component = 0;
        mask = info->srcmask[i];
        for (j = 0; j < 4 && mask; ++j)
        {
            if (info->srcshift[i] < j * 8)
                component |= (col[j] & mask) << (j * 8 - info->srcshift[i]);
            else
                component |= (col[j] & mask) >> (info->srcshift[i] - j * 8);
            mask >>= 8;
        }
        out[i] = component;
    }
}

/************************************************************
 * make_argb_color
 *
 * Recombines the output of get_relevant_argb_components and converts
 * it to the destination format.
 */
static DWORD make_argb_color(const struct argb_conversion_info *info, const DWORD *in)
{
    UINT i;
    DWORD val = 0;

    for(i = 0;i < 4;i++) {
        if(info->process_channel[i]) {
            /* necessary to make sure that e.g. an X4R4G4B4 white maps to an R8G8B8 white instead of 0xf0f0f0 */
            signed int shift;
            for(shift = info->destshift[i]; shift > info->destformat->shift[i]; shift -= info->srcformat->bits[i]) val |= in[i] << shift;
            val |= (in[i] >> (info->destformat->shift[i] - shift)) << info->destformat->shift[i];
        }
    }
    val |= info->channelmask;   /* new channels are set to their maximal value */
    return val;
}

/* It doesn't work for components bigger than 32 bits (or somewhat smaller but unaligned). */
void format_to_vec4(const struct pixel_format_desc *format, const BYTE *src, struct vec4 *dst)
{
    DWORD mask, tmp;
    unsigned int c;

    for (c = 0; c < 4; ++c)
    {
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        float *dst_component = (float *)dst + component_offsets[c];

        if (format->bits[c])
        {
            mask = ~0u >> (32 - format->bits[c]);

            memcpy(&tmp, src + format->shift[c] / 8,
                   std::min(sizeof(DWORD), static_cast<ulong>((format->shift[c] % 8 + format->bits[c] + 7) / 8)));

            if (format->type == FORMAT_ARGBF16)
            {
//                WARN("Unimplemented for FORMAT_ARGBF16");
//                *dst_component = float_16_to_32(tmp);
            }
            else if (format->type == FORMAT_ARGBF)
                *dst_component = *(float *)&tmp;
            else
                *dst_component = (float)((tmp >> format->shift[c] % 8) & mask) / mask;
        }
        else
            *dst_component = 1.0f;
    }
}

/* It doesn't work for components bigger than 32 bits. */
static void format_from_vec4(const struct pixel_format_desc *format, const struct vec4 *src, BYTE *dst)
{
    DWORD v, mask32;
    unsigned int c, i;

    memset(dst, 0, format->bytes_per_pixel);

    for (c = 0; c < 4; ++c)
    {
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        const float src_component = *((const float *)src + component_offsets[c]);

        if (!format->bits[c])
            continue;

        mask32 = ~0u >> (32 - format->bits[c]);

        if (format->type == FORMAT_ARGBF16)
        {
//            WARN("Unimplemented for FORMAT_ARGBF16");
//            v = float_32_to_16(src_component);
        }
        else if (format->type == FORMAT_ARGBF)
            v = *(DWORD *)&src_component;
        else
            v = (DWORD)(src_component * ((1 << format->bits[c]) - 1) + 0.5f);

        for (i = format->shift[c] / 8 * 8; i < format->shift[c] + format->bits[c]; i += 8)
        {
            BYTE mask, byte;

            if (format->shift[c] > i)
            {
                mask = mask32 << (format->shift[c] - i);
                byte = (v << (format->shift[c] - i)) & mask;
            }
            else
            {
                mask = mask32 >> (i - format->shift[c]);
                byte = (v >> (i - format->shift[c])) & mask;
            }
            dst[i / 8] |= byte;
        }
    }
}

/************************************************************
 * copy_pixels
 *
 * Copies the source buffer to the destination buffer.
 * Works for any pixel format.
 * The source and the destination must be block-aligned.
 */
void copy_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
                 BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, const struct volume *size,
                 const struct pixel_format_desc *format)
{
    UINT row, slice;
    BYTE *dst_addr;
    const BYTE *src_addr;
    UINT row_block_count = (size->width + format->block_width - 1) / format->block_width;
    UINT row_count = (size->height + format->block_height - 1) / format->block_height;

    for (slice = 0; slice < size->depth; slice++)
    {
        src_addr = src + slice * src_slice_pitch;
        dst_addr = dst + slice * dst_slice_pitch;

        for (row = 0; row < row_count; row++)
        {
            memcpy(dst_addr, src_addr, row_block_count * format->block_byte_count);
            src_addr += src_row_pitch;
            dst_addr += dst_row_pitch;
        }
    }
}

/************************************************************
 * convert_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion and color keying.
 * Pixels outsize the source rect are blacked out.
 */
void convert_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
                         const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
                         const struct volume *dst_size, const struct pixel_format_desc *dst_format, D3DCOLOR color_key,
                         const PALETTEENTRY *palette)
{
    struct argb_conversion_info conv_info, ck_conv_info;
    const struct pixel_format_desc *ck_format = NULL;
    DWORD channels[4];
    UINT min_width, min_height, min_depth;
    UINT x, y, z;

//    TRACE("src %p, src_row_pitch %u, src_slice_pitch %u, src_size %p, src_format %p, dst %p, "
//          "dst_row_pitch %u, dst_slice_pitch %u, dst_size %p, dst_format %p, color_key 0x%08lx, palette %p.\n",
//          src, src_row_pitch, src_slice_pitch, src_size, src_format, dst, dst_row_pitch, dst_slice_pitch, dst_size,
//          dst_format, color_key, palette);

    ZeroMemory(channels, sizeof(channels));
    init_argb_conversion_info(src_format, dst_format, &conv_info);

    min_width = std::min(src_size->width, dst_size->width);
    min_height = std::min(src_size->height, dst_size->height);
    min_depth = std::min(src_size->depth, dst_size->depth);

    if (color_key)
    {
        /* Color keys are always represented in D3DFMT_A8R8G8B8 format. */
        ck_format = get_format_info(D3DFMT_A8R8G8B8);
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);
    }

    for (z = 0; z < min_depth; z++) {
        const BYTE *src_slice_ptr = src + z * src_slice_pitch;
        BYTE *dst_slice_ptr = dst + z * dst_slice_pitch;

        for (y = 0; y < min_height; y++) {
            const BYTE *src_ptr = src_slice_ptr + y * src_row_pitch;
            BYTE *dst_ptr = dst_slice_ptr + y * dst_row_pitch;

            for (x = 0; x < min_width; x++) {
                if (!src_format->to_rgba && !dst_format->from_rgba
                    && src_format->type == dst_format->type
                    && src_format->bytes_per_pixel <= 4 && dst_format->bytes_per_pixel <= 4)
                {
                    DWORD val;

                    get_relevant_argb_components(&conv_info, src_ptr, channels);
                    val = make_argb_color(&conv_info, channels);

                    if (color_key)
                    {
                        DWORD ck_pixel;

                        get_relevant_argb_components(&ck_conv_info, src_ptr, channels);
                        ck_pixel = make_argb_color(&ck_conv_info, channels);
                        if (ck_pixel == color_key)
                            val &= ~conv_info.destmask[0];
                    }
                    memcpy(dst_ptr, &val, dst_format->bytes_per_pixel);
                }
                else
                {
                    struct vec4 color, tmp;

                    format_to_vec4(src_format, src_ptr, &color);
                    if (src_format->to_rgba)
                        src_format->to_rgba(&color, &tmp, palette);
                    else
                        tmp = color;

                    if (ck_format)
                    {
                        DWORD ck_pixel;

                        format_from_vec4(ck_format, &tmp, (BYTE *)&ck_pixel);
                        if (ck_pixel == color_key)
                            tmp.w = 0.0f;
                    }

                    if (dst_format->from_rgba)
                        dst_format->from_rgba(&tmp, &color);
                    else
                        color = tmp;

                    format_from_vec4(dst_format, &color, dst_ptr);
                }

                src_ptr += src_format->bytes_per_pixel;
                dst_ptr += dst_format->bytes_per_pixel;
            }

            if (src_size->width < dst_size->width) /* black out remaining pixels */
                memset(dst_ptr, 0, dst_format->bytes_per_pixel * (dst_size->width - src_size->width));
        }

        if (src_size->height < dst_size->height) /* black out remaining pixels */
            memset(dst + src_size->height * dst_row_pitch, 0, dst_row_pitch * (dst_size->height - src_size->height));
    }
    if (src_size->depth < dst_size->depth) /* black out remaining pixels */
        memset(dst + src_size->depth * dst_slice_pitch, 0, dst_slice_pitch * (dst_size->depth - src_size->depth));
}

/************************************************************
 * point_filter_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion, color keying and stretching
 * using a point filter.
 */
void point_filter_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
                              const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
                              const struct volume *dst_size, const struct pixel_format_desc *dst_format, D3DCOLOR color_key,
                              const PALETTEENTRY *palette)
{
    struct argb_conversion_info conv_info, ck_conv_info;
    const struct pixel_format_desc *ck_format = NULL;
    DWORD channels[4];
    UINT x, y, z;

//    TRACE("src %p, src_row_pitch %u, src_slice_pitch %u, src_size %p, src_format %p, dst %p, "
//          "dst_row_pitch %u, dst_slice_pitch %u, dst_size %p, dst_format %p, color_key 0x%08lx, palette %p.\n",
//          src, src_row_pitch, src_slice_pitch, src_size, src_format, dst, dst_row_pitch, dst_slice_pitch, dst_size,
//          dst_format, color_key, palette);

    ZeroMemory(channels, sizeof(channels));
    init_argb_conversion_info(src_format, dst_format, &conv_info);

    if (color_key)
    {
        /* Color keys are always represented in D3DFMT_A8R8G8B8 format. */
        ck_format = get_format_info(D3DFMT_A8R8G8B8);
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);
    }

    for (z = 0; z < dst_size->depth; z++)
    {
        BYTE *dst_slice_ptr = dst + z * dst_slice_pitch;
        const BYTE *src_slice_ptr = src + src_slice_pitch * (z * src_size->depth / dst_size->depth);

        for (y = 0; y < dst_size->height; y++)
        {
            BYTE *dst_ptr = dst_slice_ptr + y * dst_row_pitch;
            const BYTE *src_row_ptr = src_slice_ptr + src_row_pitch * (y * src_size->height / dst_size->height);

            for (x = 0; x < dst_size->width; x++)
            {
                const BYTE *src_ptr = src_row_ptr + (x * src_size->width / dst_size->width) * src_format->bytes_per_pixel;

                if (!src_format->to_rgba && !dst_format->from_rgba
                    && src_format->type == dst_format->type
                    && src_format->bytes_per_pixel <= 4 && dst_format->bytes_per_pixel <= 4)
                {
                    DWORD val;

                    get_relevant_argb_components(&conv_info, src_ptr, channels);
                    val = make_argb_color(&conv_info, channels);

                    if (color_key)
                    {
                        DWORD ck_pixel;

                        get_relevant_argb_components(&ck_conv_info, src_ptr, channels);
                        ck_pixel = make_argb_color(&ck_conv_info, channels);
                        if (ck_pixel == color_key)
                            val &= ~conv_info.destmask[0];
                    }
                    memcpy(dst_ptr, &val, dst_format->bytes_per_pixel);
                }
                else
                {
                    struct vec4 color, tmp;

                    format_to_vec4(src_format, src_ptr, &color);
                    if (src_format->to_rgba)
                        src_format->to_rgba(&color, &tmp, palette);
                    else
                        tmp = color;

                    if (ck_format)
                    {
                        DWORD ck_pixel;

                        format_from_vec4(ck_format, &tmp, (BYTE *)&ck_pixel);
                        if (ck_pixel == color_key)
                            tmp.w = 0.0f;
                    }

                    if (dst_format->from_rgba)
                        dst_format->from_rgba(&tmp, &color);
                    else
                        color = tmp;

                    format_from_vec4(dst_format, &color, dst_ptr);
                }

                dst_ptr += dst_format->bytes_per_pixel;
            }
        }
    }
}

/************************************************************
 * D3DXLoadSurfaceFromMemory
 *
 * Loads data from a given memory chunk into a surface,
 * applying any of the specified filters.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcMemory   [I] pointer to the source data
 *   SrcFormat    [I] format of the source pixel data
 *   SrcPitch     [I] number of bytes in a row
 *   pSrcPalette  [I] palette used in the source image
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load the pixel data into our surface or
 *                    if pSrcMemory is NULL but the other parameters are valid
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, SrcPitch or pSrcRect is NULL or
 *                                if SrcFormat is an invalid format (other than D3DFMT_UNKNOWN) or
 *                                if DestRect is invalid
 *            D3DXERR_INVALIDDATA, if we fail to lock pDestSurface
 *            E_FAIL, if SrcFormat is D3DFMT_UNKNOWN or the dimensions of pSrcRect are invalid
 *
 * NOTES
 *   pSrcRect specifies the dimensions of the source data;
 *   negative values for pSrcRect are allowed as we're only looking at the width and height anyway.
 *
 */
HRESULT LoadSurfaceFromMemory(IDirect3DSurface9 *dst_surface,
                                         const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
                                         D3DFORMAT src_format, UINT src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
                                         DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *srcformatdesc, *destformatdesc;
    struct volume src_size, dst_size, dst_size_aligned;
    RECT dst_rect_temp, dst_rect_aligned;
    IDirect3DSurface9 *surface;
    D3DSURFACE_DESC surfdesc;
    D3DLOCKED_RECT lockrect;
    HRESULT hr;

//    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_memory %p, src_format %#x, "
//          "src_pitch %u, src_palette %p, src_rect %s, filter %#lx, color_key 0x%08lx.\n",
//          dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_memory, src_format,
//          src_pitch, src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_memory || !src_rect)
    {
//        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }
    if (src_format == D3DFMT_UNKNOWN
        || src_rect->left >= src_rect->right
        || src_rect->top >= src_rect->bottom)
    {
//        WARN("Invalid src_format or src_rect.\n");
        return E_FAIL;
    }

    srcformatdesc = get_format_info(src_format);
    if (srcformatdesc->type == FORMAT_UNKNOWN)
    {
//        FIXME("Unsupported format %#x.\n", src_format);
        return E_NOTIMPL;
    }

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;

    IDirect3DSurface9_GetDesc(dst_surface, &surfdesc);
    destformatdesc = get_format_info(surfdesc.Format);
    if (!dst_rect)
    {
        dst_rect = &dst_rect_temp;
        dst_rect_temp.left = 0;
        dst_rect_temp.top = 0;
        dst_rect_temp.right = surfdesc.Width;
        dst_rect_temp.bottom = surfdesc.Height;
    }
    else
    {
        if (dst_rect->left > dst_rect->right || dst_rect->right > surfdesc.Width
            || dst_rect->top > dst_rect->bottom || dst_rect->bottom > surfdesc.Height
            || dst_rect->left < 0 || dst_rect->top < 0)
        {
//            WARN("Invalid dst_rect specified.\n");
            return D3DERR_INVALIDCALL;
        }
        if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
        {
//            WARN("Empty dst_rect specified.\n");
            return D3D_OK;
        }
    }

    dst_rect_aligned = *dst_rect;
    if (dst_rect_aligned.left & (destformatdesc->block_width - 1))
        dst_rect_aligned.left = dst_rect_aligned.left & ~(destformatdesc->block_width - 1);
    if (dst_rect_aligned.top & (destformatdesc->block_height - 1))
        dst_rect_aligned.top = dst_rect_aligned.top & ~(destformatdesc->block_height - 1);
    if (dst_rect_aligned.right & (destformatdesc->block_width - 1) && dst_rect_aligned.right != surfdesc.Width)
        dst_rect_aligned.right = std::min((dst_rect_aligned.right + destformatdesc->block_width - 1)
                                         & ~(destformatdesc->block_width - 1), surfdesc.Width);
    if (dst_rect_aligned.bottom & (destformatdesc->block_height - 1) && dst_rect_aligned.bottom != surfdesc.Height)
        dst_rect_aligned.bottom = std::min((dst_rect_aligned.bottom + destformatdesc->block_height - 1)
                                          & ~(destformatdesc->block_height - 1), surfdesc.Height);

    dst_size.width = dst_rect->right - dst_rect->left;
    dst_size.height = dst_rect->bottom - dst_rect->top;
    dst_size.depth = 1;
    dst_size_aligned.width = dst_rect_aligned.right - dst_rect_aligned.left;
    dst_size_aligned.height = dst_rect_aligned.bottom - dst_rect_aligned.top;
    dst_size_aligned.depth = 1;

    if (filter == D3DX_DEFAULT)
        filter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER;

    if (FAILED(hr = lock_surface(dst_surface, &dst_rect_aligned, &lockrect, &surface, TRUE)))
        return hr;

    src_memory = (BYTE *)src_memory + src_rect->top / srcformatdesc->block_height * src_pitch
                 + src_rect->left / srcformatdesc->block_width * srcformatdesc->block_byte_count;

    if (src_format == surfdesc.Format
        && dst_size.width == src_size.width
        && dst_size.height == src_size.height
        && color_key == 0
        && !(src_rect->left & (srcformatdesc->block_width - 1))
        && !(src_rect->top & (srcformatdesc->block_height - 1))
        && !(dst_rect->left & (destformatdesc->block_width - 1))
        && !(dst_rect->top & (destformatdesc->block_height - 1)))
    {
//        TRACE("Simple copy.\n");
        copy_pixels(static_cast<const BYTE*>(src_memory), src_pitch, 0, static_cast<BYTE*>(lockrect.pBits), lockrect.Pitch, 0,
                    &src_size, srcformatdesc);
    }
    else /* Stretching or format conversion. */
    {
        const struct pixel_format_desc *dst_format;
        BYTE *dst_uncompressed = NULL;
        unsigned int dst_pitch;
        BYTE *dst_mem;

        if (!is_conversion_from_supported(srcformatdesc)
            || !is_conversion_to_supported(destformatdesc))
        {
//            FIXME("Unsupported format conversion %#x -> %#x.\n", src_format, surfdesc.Format);
            unlock_surface(dst_surface, &dst_rect_aligned, surface, FALSE);
            return E_NOTIMPL;
        }

        if (srcformatdesc->type == FORMAT_DXT || destformatdesc->type == FORMAT_DXT)
        {
//            WARN("Src/Dst FORMAT_DXT unsupported\n");
            return E_NOTIMPL;
        }
        else
        {
            dst_mem = static_cast<BYTE*>(lockrect.pBits);
            dst_pitch = lockrect.Pitch;
            dst_format = destformatdesc;
        }

        if ((filter & 0xf) == D3DX_FILTER_NONE)
        {
            convert_argb_pixels(static_cast<const BYTE*>(src_memory), src_pitch, 0, &src_size, srcformatdesc,
                                dst_mem, dst_pitch, 0, &dst_size, dst_format, color_key, src_palette);
        }
        else /* if ((filter & 0xf) == D3DX_FILTER_POINT) */
        {
//            if ((filter & 0xf) != D3DX_FILTER_POINT)
//                FIXME("Unhandled filter %#lx.\n", filter);

            /* Always apply a point filter until D3DX_FILTER_LINEAR,
             * D3DX_FILTER_TRIANGLE and D3DX_FILTER_BOX are implemented. */
            point_filter_argb_pixels(static_cast<const BYTE*>(src_memory), src_pitch, 0, &src_size, srcformatdesc,
                                     dst_mem, dst_pitch, 0, &dst_size, dst_format, color_key, src_palette);
        }
    }

    return unlock_surface(dst_surface, &dst_rect_aligned, surface, TRUE);
}

/************************************************************
 * D3DXLoadSurfaceFromSurface
 *
 * Copies the contents from one surface to another, performing any required
 * format conversion, resizing or filtering.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the destination surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcSurface  [I] pointer to the source surface
 *   pSrcPalette  [I] palette used for the source surface
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on resizing
 *   Colorkey     [I] any ARGB value or 0 to disable color-keying
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcSurface is NULL
 *            D3DXERR_INVALIDDATA, if one of the surfaces is not lockable
 *
 */
HRESULT LoadSurfaceFromSurface(IDirect3DSurface9 *dst_surface,
                                          const PALETTEENTRY *dst_palette, const RECT *dst_rect, IDirect3DSurface9 *src_surface,
                                          const PALETTEENTRY *src_palette, const RECT *src_rect, DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *src_format_desc, *dst_format_desc;
    D3DSURFACE_DESC src_desc, dst_desc;
    struct volume src_size, dst_size;
    IDirect3DSurface9 *temp_surface;
    D3DTEXTUREFILTERTYPE d3d_filter;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lock;
    RECT dst_rect_temp;
    HRESULT hr;
    RECT s;

//    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_surface %p, "
//          "src_palette %p, src_rect %s, filter %#lx, color_key 0x%08lx.\n",
//          dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_surface,
//          src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_surface)
        return D3DERR_INVALIDCALL;

    IDirect3DSurface9_GetDesc(src_surface, &src_desc);
    src_format_desc = get_format_info(src_desc.Format);
    if (!src_rect)
    {
        WinSetRect(&s, 0, 0, src_desc.Width, src_desc.Height);
        src_rect = &s;
    }
    else if (src_rect->left == src_rect->right || src_rect->top == src_rect->bottom)
    {
//        WARN("Empty src_rect specified.\n");
        return filter == D3DX_FILTER_NONE ? D3D_OK : E_FAIL;
    }
    else if (src_rect->left > src_rect->right || src_rect->right > src_desc.Width
             || src_rect->left < 0 || src_rect->left > src_desc.Width
             || src_rect->top > src_rect->bottom || src_rect->bottom > src_desc.Height
             || src_rect->top < 0 || src_rect->top > src_desc.Height)
    {
//        WARN("Invalid src_rect specified.\n");
        return D3DERR_INVALIDCALL;
    }

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;

    IDirect3DSurface9_GetDesc(dst_surface, &dst_desc);
    dst_format_desc = get_format_info(dst_desc.Format);
    if (!dst_rect)
    {
        WinSetRect(&dst_rect_temp, 0, 0, dst_desc.Width, dst_desc.Height);
        dst_rect = &dst_rect_temp;
    }
    else if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
    {
//        WARN("Empty dst_rect specified.\n");
        return filter == D3DX_FILTER_NONE ? D3D_OK : E_FAIL;
    }
    else if (dst_rect->left > dst_rect->right || dst_rect->right > dst_desc.Width
             || dst_rect->left < 0 || dst_rect->left > dst_desc.Width
             || dst_rect->top > dst_rect->bottom || dst_rect->bottom > dst_desc.Height
             || dst_rect->top < 0 || dst_rect->top > dst_desc.Height)
    {
//        WARN("Invalid dst_rect specified.\n");
        return D3DERR_INVALIDCALL;
    }

    dst_size.width = dst_rect->right - dst_rect->left;
    dst_size.height = dst_rect->bottom - dst_rect->top;
    dst_size.depth = 1;

    if (!dst_palette && !src_palette && !color_key)
    {
        if (src_desc.Format == dst_desc.Format
            && dst_size.width == src_size.width
            && dst_size.height == src_size.height
            && color_key == 0
            && !(src_rect->left & (src_format_desc->block_width - 1))
            && !(src_rect->top & (src_format_desc->block_height - 1))
            && !(dst_rect->left & (dst_format_desc->block_width - 1))
            && !(dst_rect->top & (dst_format_desc->block_height - 1)))
        {
            d3d_filter = D3DTEXF_NONE;
        }
        else
        {
            switch (filter)
            {
            case D3DX_FILTER_NONE:
                d3d_filter = D3DTEXF_NONE;
                break;

            case D3DX_FILTER_POINT:
                d3d_filter = D3DTEXF_POINT;
                break;

            case D3DX_FILTER_LINEAR:
                d3d_filter = D3DTEXF_LINEAR;
                break;

            default:
                d3d_filter = D3DTEXF_FORCE_DWORD;
                break;
            }
        }

        if (d3d_filter != D3DTEXF_FORCE_DWORD)
        {
            IDirect3DSurface9_GetDevice(src_surface, &device);
            hr = IDirect3DDevice9_StretchRect(device, src_surface, src_rect, dst_surface, dst_rect, d3d_filter);
            IDirect3DDevice9_Release(device);
            if (SUCCEEDED(hr))
                return D3D_OK;
        }
    }

    if (FAILED(lock_surface(src_surface, NULL, &lock, &temp_surface, FALSE)))
        return D3DERR_INVALIDCALL; // D3DXERR_INVALIDDATA;

    hr = LoadSurfaceFromMemory(dst_surface, dst_palette, dst_rect, lock.pBits,
                                   src_desc.Format, lock.Pitch, src_palette, src_rect, filter, color_key);

    if (FAILED(unlock_surface(src_surface, NULL, temp_surface, FALSE)))
        return D3DERR_INVALIDCALL; // D3DXERR_INVALIDDATA;

    return hr;
}

#endif

// custom D3DX9 functions

HRESULT CreateTextureFromFile(
    IDirect3DDevice9* device,
    const char* srcfile,
    IDirect3DTexture9** texture)
{
#ifdef UseD3DX9
    return D3DXCreateTextureFromFile(device, srcfile, texture);
#else

    gli::texture tex = gli::load(srcfile);
    const auto dimensions = tex.extent();
    gli::dx DX;
    D3DFORMAT fmt = static_cast<D3DFORMAT>(DX.translate(tex.format()).D3DFormat);

    HRESULT hr = device->CreateTexture(dimensions.x, dimensions.y, 1, 0, fmt, D3DPOOL_MANAGED, texture, nullptr);
    if (FAILED(hr))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "LockRect failed", nullptr);
        return hr;
    }

    D3DLOCKED_RECT rect;
    hr = (*texture)->LockRect(0, &rect, 0, D3DLOCK_DISCARD);
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
bool Setup(int Width, int Height);
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
    if (!Setup(Width, Height))
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

bool Setup(int Width, int Height)
{
   LoadTexture();

    g_pd3dDevice->CreateVertexBuffer(4 * sizeof(Vertex), D3DUSAGE_WRITEONLY,
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT,
        &g_pVertexBuffer, nullptr);
    void* pVertices = nullptr;

    g_pVertexBuffer->Lock(0, sizeof(g_quadVertices), (void**)&pVertices, 0);
    memcpy(pVertices, g_quadVertices, sizeof(g_quadVertices));
    g_pVertexBuffer->Unlock();

#ifdef UseD3DX9
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(45.0f),
        (float)Width / (float)Height, 0.1f, 100.0f);
#else
    D3DMATRIX matProj;
    MatrixPerspectiveFovLH(&matProj, M_PI / 4.0f,
        (float)Width / (float)Height, 0.1f, 100.0f);
#endif
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    return true;
}

void LoadTextureV1()
{
    CreateTextureFromFile(g_pd3dDevice, "textures/chess4.dds", &g_pTexture);
}

using u32 = std::uint32_t;
typedef IDirect3DTexture9 ID3DTexture2D;

#define R_CHK(expr)\
do\
    {\
        HRESULT hr = expr;\
        if (FAILED(hr))\
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "R_CHK - FAILED", nullptr);\
    } while (false)

#define R_ASSERT(expr)\
do\
    {\
        if (!(expr))\
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "R_CHK - FAILED", nullptr);\
    } while (false)

void Reduce(int& w, int& h, int& l, int& skip)
{
    while ((l > 1) && skip)
    {
        w /= 2;
        h /= 2;
        l -= 1;

        skip--;
    }
    if (w < 1)
        w = 1;
    if (h < 1)
        h = 1;
}

ID3DTexture2D* TW_LoadTextureFromTexture(
    ID3DTexture2D* t_from, D3DFORMAT& t_dest_fmt, int levels_2_skip, u32& w, u32& h)
{
    // Calculate levels & dimensions
    ID3DTexture2D* t_dest = nullptr;
    D3DSURFACE_DESC t_from_desc0;
    R_CHK(t_from->GetLevelDesc(0, &t_from_desc0));
    int levels_exist = t_from->GetLevelCount();
    int top_width = t_from_desc0.Width;
    int top_height = t_from_desc0.Height;
    Reduce(top_width, top_height, levels_exist, levels_2_skip);

// Create HW-surface
    R_ASSERT(t_dest_fmt == t_from_desc0.Format);
    R_CHK(g_pd3dDevice->CreateTexture(top_width, top_height, levels_exist, 0, t_dest_fmt,
                                    (1 ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED),
                                    &t_dest, nullptr));

    // Copy surfaces & destroy temporary
    ID3DTexture2D* T_src = t_from;
    ID3DTexture2D* T_dst = t_dest;

    int L_src = T_src->GetLevelCount() - 1;
    int L_dst = T_dst->GetLevelCount() - 1;
    for (; L_dst >= 0; L_src--, L_dst--)
    {
        // Get surfaces
        IDirect3DSurface9 *S_src, *S_dst;
        R_CHK(T_src->GetSurfaceLevel(L_src, &S_src));
        R_CHK(T_dst->GetSurfaceLevel(L_dst, &S_dst));

        // Copy
        R_CHK(LoadSurfaceFromSurface(S_dst, NULL, NULL, S_src, NULL, NULL, D3DX_FILTER_NONE, 0));

        // Release surfaces
        //_RELEASE(S_src);
        //_RELEASE(S_dst);
    }

    // OK
    w = top_width;
    h = top_height;
    return t_dest;
}

void LoadTextureV2()
{
    HRESULT result;
    gli::texture texture;
    gli::storage_linear::extent_type dimensions;
    D3DLOCKED_RECT lockRect;
    gli::dx DX;

    u32 dwWidth, dwHeight;
    int img_loaded_lod = 0;
    D3DFORMAT fmt;

    ID3DTexture2D* T_sysmem;

    texture = gli::load("textures/chess4.dds");
    dimensions = texture.extent();
    fmt = static_cast<D3DFORMAT>(DX.translate(texture.format()).D3DFormat);
    result = g_pd3dDevice->CreateTexture(dimensions.x, dimensions.y, 0, 0, fmt,
                                       D3DPOOL_SYSTEMMEM, &T_sysmem, nullptr);
    result = T_sysmem->LockRect( 0, &lockRect, 0, D3DLOCK_DISCARD );
    if (!FAILED(result))
    {
        char* dest = static_cast<char*>(lockRect.pBits);
        memcpy(dest, texture.data(), texture.size());
        result = T_sysmem->UnlockRect(0);
    }
    if (FAILED(result))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "T_sysmem->UnlockRect(0) - FAILED", nullptr);
    }
    img_loaded_lod = 1;
    g_pTexture = TW_LoadTextureFromTexture(T_sysmem, fmt, img_loaded_lod, dwWidth, dwHeight);
}

void LoadTexture()
{
#ifdef LoadTextureV1
    LoadTextureV1();
#else
    LoadTextureV2();
#endif

    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
}

void LoadSubTexture()
{
    return;
    LPDIRECT3DTEXTURE9 pSubTexture  = nullptr;
    LPDIRECT3DSURFACE9 pDestSurface = nullptr;
    LPDIRECT3DSURFACE9 pSrcSurface  = nullptr;

    CreateTextureFromFile(g_pd3dDevice, "textures/cursor.dds", &pSubTexture);

    g_pTexture->GetSurfaceLevel( 0, &pDestSurface );
    pSubTexture->GetSurfaceLevel( 0, &pSrcSurface );

    RECT srcRect[]  = {  0,  0, 64, 64 };
    RECT destRect[] = { 32, 32, 96, 96 };

#ifdef UseD3DX9
    D3DXLoadSurfaceFromSurface( pDestSurface, nullptr, destRect,
                                pSrcSurface,  nullptr, srcRect,
                                D3DX_DEFAULT, 0);
#else
    LoadSurfaceFromSurface(pDestSurface, nullptr, destRect,
                               pSrcSurface,  nullptr, srcRect,
                               D3DX_DEFAULT, 0);
#endif

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

#ifdef UseD3DX9
    D3DXMATRIX matWorld;
    D3DXMatrixTranslation( &matWorld, 0.0f, 0.0f, 4.0f );
#else
    D3DMATRIX matWorld;
    MatrixTranslation(&matWorld, 0.0f, 0.0f, 4.0f);
#endif
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

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
