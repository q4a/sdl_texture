//////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: vertex.h
//
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0
//
// Desc: Represents a vertex.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __vertexH__
#define __vertexH__

struct Vertex
{
	Vertex(){}
	Vertex(
		float x, float y, float z,
		float nx, float ny, float nz,
		float u, float v)
	{
		_x  = x;  _y  = y;  _z  = z;
		_nx = nx; _ny = ny; _nz = nz;
		_u  = u;  _v  = v;
	}
	float _x, _y, _z;
	float _nx, _ny, _nz;
	float _u, _v; // texture coordinates
};
#define FVF_VERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

// different vertex format
struct TVertex
{
	TVertex() {}
	TVertex(
		float x, float y, float z,
		float u, float v)
	{
		_x = x;  _y = y;  _z = z;
		_u = u;  _v = v;
	}
	float _x, _y, _z;
	float _u, _v; // texture coordinates
};
#define FVF_TVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

struct VertexCube
{
	VertexCube() {}
	VertexCube(
		float x, float y, float z,
		float nx, float ny, float nz)
	{
		_x = x;   _y = y;   _z = z;
		_nx = nx; _ny = ny; _nz = nz;
	}
	float _x, _y, _z;
	float _nx, _ny, _nz;
};
#define FVF_VERTEXCUBE (D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0))

#endif // __vertexH__
