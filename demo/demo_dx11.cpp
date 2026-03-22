/*
-- Vertex Engine GPU Font Cache Demo --

Copyright 2020 Xi Chen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <cassert>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

#ifdef VE_FONTCACHE_HARFBUZZ
#include <hb.h>
#endif // VE_FONTCACHE_HARFBUZZ

#define VE_FONTCACHE_IMPL
// #define VE_FONTCACHE_DEBUGPRINT
#include "../ve_fontcache.h"
#include "../ve_fontcache_backend_test.h"

static ve_fontcache cache;

struct demo_window_size
{
unsigned int width = 0;
unsigned int height = 0;
};

struct dx11_texture_target
{
ID3D11Texture2D* texture = nullptr;
ID3D11RenderTargetView* rtv = nullptr;
ID3D11ShaderResourceView* srv = nullptr;
};

struct dx11_cpu_atlas_page
{
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* srv = nullptr;
};

struct dx11_vertex
{
	float x;
	float y;
	float u;
	float v;
};

struct alignas( 16 ) dx11_blit_atlas_cb
{
uint32_t region = 0;
float padding[ 3 ] = {};
float source_texture_size[ 2 ] = { static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH ), static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ) };
float padding2[ 2 ] = {};
float source_rect[ 4 ] = {};
float dest_rect[ 4 ] = {};
};

struct alignas( 16 ) dx11_draw_text_cb
{
uint32_t downsample = 0;
float padding[ 3 ] = {};
float colour[ 4 ] = { 1.0f, 1.0f, 1.0f, 1.0f };
float source_texture_size[ 2 ] = { 4096.0f, 2048.0f };
float padding2[ 2 ] = {};
};

static demo_window_size window_size;
static int mouse_scroll = 0;
extern bool demo_autoscroll;
static bool g_should_close = false;
static bool g_mouse_left_down = false;
static LONG g_mouse_x = 0;
static LONG g_mouse_y = 0;

static HWND g_hwnd = nullptr;
static IDXGISwapChain* g_swapchain = nullptr;
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static ID3D11Texture2D* g_backbuffer_texture = nullptr;
static ID3D11RenderTargetView* g_backbuffer_rtv_linear = nullptr;
static ID3D11RenderTargetView* g_backbuffer_rtv_srgb = nullptr;
static dx11_texture_target g_glyph_buffer;
static dx11_texture_target g_atlas;
static std::vector< dx11_cpu_atlas_page > g_cpu_atlas_pages;
static ID3D11VertexShader* g_vs_shared = nullptr;
static ID3D11VertexShader* g_vs_blit_atlas = nullptr;
static ID3D11VertexShader* g_vs_draw_text = nullptr;
static ID3D11PixelShader* g_ps_render_glyph = nullptr;
static ID3D11PixelShader* g_ps_blit_atlas = nullptr;
static ID3D11PixelShader* g_ps_draw_text = nullptr;
static ID3D11InputLayout* g_input_layout_shared = nullptr;
static ID3D11InputLayout* g_input_layout_blit_atlas = nullptr;
static ID3D11InputLayout* g_input_layout_draw_text = nullptr;
static ID3D11BlendState* g_blend_xor = nullptr;
static ID3D11BlendState* g_blend_alpha = nullptr;
static ID3D11SamplerState* g_point_sampler = nullptr;
static ID3D11RasterizerState* g_rasterizer_state = nullptr;
static ID3D11Buffer* g_cb_blit_atlas = nullptr;
static ID3D11Buffer* g_cb_draw_text = nullptr;
static bool g_dx11_test_mode = false;
static void DX_CHECK_IMPL( HRESULT hr, int line )
{
if ( FAILED( hr ) ) {
std::printf( "DX error 0x%08X at line %d\n", static_cast< unsigned int >( hr ), line );
assert( !"stop" );
}
}
#define DX_CHECK( call ) DX_CHECK_IMPL( ( call ), __LINE__ )

template < typename T >
static void dx11_release( T*& ptr )
{
if ( ptr ) {
ptr->Release();
ptr = nullptr;
}
}

static std::filesystem::path current_executable_directory()
{
std::array< char, MAX_PATH > path {};
DWORD length = GetModuleFileNameA( nullptr, path.data(), static_cast< DWORD >( path.size() ) );
return std::filesystem::path( std::string( path.data(), length ) ).parent_path();
}

static std::string resolve_demo_asset_path( const char* relative_path )
{
const std::filesystem::path relative( relative_path );
const std::filesystem::path cwd = std::filesystem::current_path();
const std::filesystem::path exe_dir = current_executable_directory();
const std::array< std::filesystem::path, 6 > candidates = {
cwd / relative,
cwd / "demo" / relative,
cwd / ".." / "demo" / relative,
exe_dir / relative,
exe_dir / ".." / "demo" / relative,
exe_dir / ".." / ".." / "demo" / relative,
};

for ( const std::filesystem::path& candidate : candidates ) {
if ( std::filesystem::exists( candidate ) ) {
return candidate.lexically_normal().string();
}
}

return relative.string();
}

static ve_font_id load_demo_font( ve_fontcache* target_cache, const char* relative_path, std::vector< uint8_t >& buffer, float size_px )
{
std::string resolved_path = resolve_demo_asset_path( relative_path );
return ve_fontcache_loadfile( target_cache, resolved_path.c_str(), buffer, size_px );
}

static const char* g_vs_source_shared = R"(
struct VSInput {
	float4 vertex : POSITION;
};
struct VSOutput {
float4 pos : SV_POSITION;
float2 uv : TEXCOORD0;
};
VSOutput main( VSInput input ) {
	VSOutput output;
	output.uv = input.vertex.zw;
	output.pos = float4( input.vertex.xy, 0.0f, 1.0f );
	return output;
}
)";

static const char* g_ps_source_render_glyph = R"(
float4 main() : SV_TARGET {
return float4( 1.0f, 1.0f, 1.0f, 1.0f );
}
)";

static const char* g_vs_source_blit_atlas = R"(
cbuffer BlitAtlasCB : register( b0 ) {
uint region;
float3 padding;
float2 source_texture_size;
float2 padding2;
float4 source_rect;
float4 dest_rect;
};
struct VSInput {
float4 vertex : POSITION;
};
struct VSOutput {
float4 pos : SV_POSITION;
float2 uv : TEXCOORD0;
};
VSOutput main( VSInput input ) {
VSOutput output;
output.uv = float2( input.vertex.z, 1.0f - input.vertex.w );
output.pos = float4( input.vertex.xy, 0.0f, 1.0f );
return output;
}
)";

static const char* g_ps_source_blit_atlas = R"(
cbuffer BlitAtlasCB : register( b0 ) {
uint region;
float3 padding;
float2 source_texture_size;
float2 padding2;
float4 source_rect;
float4 dest_rect;
};
Texture2D src_texture : register( t0 );
SamplerState point_sampler : register( s0 );
struct PSInput {
float4 pos : SV_POSITION;
float2 uv : TEXCOORD0;
};

float downsample( float2 uv, float2 texel ) {
	return src_texture.SampleLevel( point_sampler, uv + float2( 0.0f, 0.0f ) * texel, 0.0f ).x * 0.25f
		+ src_texture.SampleLevel( point_sampler, uv + float2( 0.0f, 1.0f ) * texel, 0.0f ).x * 0.25f
		+ src_texture.SampleLevel( point_sampler, uv + float2( 1.0f, 0.0f ) * texel, 0.0f ).x * 0.25f
		+ src_texture.SampleLevel( point_sampler, uv + float2( 1.0f, 1.0f ) * texel, 0.0f ).x * 0.25f;
}

float4 main( PSInput input ) : SV_TARGET {
float2 uv = input.uv;
const float2 texel = float2( 1.0f, 1.0f ) / source_texture_size;
if ( region == 0u || region == 1u || region == 2u ) {
float v =
	downsample( uv + float2( -1.5f, -1.5f ) * texel, texel ) * 0.25f +
	downsample( uv + float2(  0.5f, -1.5f ) * texel, texel ) * 0.25f +
	downsample( uv + float2( -1.5f,  0.5f ) * texel, texel ) * 0.25f +
	downsample( uv + float2(  0.5f,  0.5f ) * texel, texel ) * 0.25f;
return float4( 1.0f, 1.0f, 1.0f, v );
}
return float4( 0.0f, 0.0f, 0.0f, 1.0f );
}
)";

static const char* g_vs_source_draw_text = R"(
struct VSInput {
float4 vertex : POSITION;
};
struct VSOutput {
float4 pos : SV_POSITION;
float2 uv : TEXCOORD0;
};
VSOutput main( VSInput input ) {
VSOutput output;
// Y-flip vtex to convert from library (GL-convention, y=0 at bottom) to
// DX11 texture convention (y=0 at top). No cbuffer needed.
output.uv = float2( input.vertex.z, 1.0f - input.vertex.w );
output.pos = float4( input.vertex.xy * 2.0f - 1.0f, 0.0f, 1.0f );
return output;
}
)";

static const char* g_ps_source_draw_text = R"(
cbuffer DrawTextCB : register( b0 ) {
uint downsample;
float3 padding;
float4 colour;
float2 source_texture_size;
float2 padding2;
};
Texture2D src_texture : register( t0 );
SamplerState point_sampler : register( s0 );
struct PSInput {
float4 pos : SV_POSITION;
float2 uv : TEXCOORD0;
};

float4 main( PSInput input ) : SV_TARGET {
float2 uv = input.uv;
float v;
if ( downsample == 1u ) {
const float2 texel = float2( 1.0f, 1.0f ) / source_texture_size;
v = src_texture.SampleLevel( point_sampler, uv + float2( -0.5f, -0.5f ) * texel, 0.0f ).x * 0.25f
  + src_texture.SampleLevel( point_sampler, uv + float2( -0.5f,  0.5f ) * texel, 0.0f ).x * 0.25f
  + src_texture.SampleLevel( point_sampler, uv + float2(  0.5f, -0.5f ) * texel, 0.0f ).x * 0.25f
  + src_texture.SampleLevel( point_sampler, uv + float2(  0.5f,  0.5f ) * texel, 0.0f ).x * 0.25f;
} else {
v = src_texture.SampleLevel( point_sampler, uv, 0.0f ).x;
}
return float4( colour.xyz, colour.w * v );
}
)";

static ID3DBlob* dx11_compile_shader_blob( const char* source, const char* target, const char* source_name )
{
UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
ID3DBlob* shader_blob = nullptr;
ID3DBlob* error_blob = nullptr;
HRESULT hr = D3DCompile(
source,
std::strlen( source ),
source_name,
nullptr,
nullptr,
"main",
target,
flags,
0,
&shader_blob,
&error_blob );
if ( error_blob ) {
std::printf( "%s", static_cast< const char* >( error_blob->GetBufferPointer() ) );
error_blob->Release();
}
DX_CHECK( hr );
return shader_blob;
}

static ID3D11VertexShader* dx11_compile_vertex_shader( const char* source, const char* source_name, ID3DBlob** bytecode_out )
{
ID3DBlob* shader_blob = dx11_compile_shader_blob( source, "vs_5_0", source_name );
ID3D11VertexShader* shader = nullptr;
DX_CHECK( g_device->CreateVertexShader( shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, &shader ) );
*bytecode_out = shader_blob;
return shader;
}

static ID3D11PixelShader* dx11_compile_pixel_shader( const char* source, const char* source_name )
{
ID3DBlob* shader_blob = dx11_compile_shader_blob( source, "ps_5_0", source_name );
ID3D11PixelShader* shader = nullptr;
DX_CHECK( g_device->CreatePixelShader( shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr, &shader ) );
shader_blob->Release();
return shader;
}

static ID3D11Buffer* dx11_create_dynamic_buffer( UINT size, UINT bind_flags )
{
D3D11_BUFFER_DESC desc = {};
desc.ByteWidth = size;
desc.Usage = D3D11_USAGE_DYNAMIC;
desc.BindFlags = bind_flags;
desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
ID3D11Buffer* buffer = nullptr;
DX_CHECK( g_device->CreateBuffer( &desc, nullptr, &buffer ) );
return buffer;
}

static void dx11_update_dynamic_buffer( ID3D11Buffer* buffer, const void* data, size_t size )
{
D3D11_MAPPED_SUBRESOURCE mapped = {};
DX_CHECK( g_context->Map( buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) );
std::memcpy( mapped.pData, data, size );
g_context->Unmap( buffer, 0 );
}

static void dx11_create_texture_target( UINT width, UINT height, DXGI_FORMAT format, dx11_texture_target& target )
{
D3D11_TEXTURE2D_DESC desc = {};
desc.Width = width;
desc.Height = height;
desc.MipLevels = 1;
desc.ArraySize = 1;
desc.Format = format;
desc.SampleDesc.Count = 1;
desc.Usage = D3D11_USAGE_DEFAULT;
desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
DX_CHECK( g_device->CreateTexture2D( &desc, nullptr, &target.texture ) );
DX_CHECK( g_device->CreateRenderTargetView( target.texture, nullptr, &target.rtv ) );
DX_CHECK( g_device->CreateShaderResourceView( target.texture, nullptr, &target.srv ) );
}

static void dx11_release_texture_target( dx11_texture_target& target )
{
dx11_release( target.srv );
dx11_release( target.rtv );
dx11_release( target.texture );
}

static void dx11_ensure_cpu_atlas_page( size_t atlas_page )
{
if ( g_cpu_atlas_pages.size() <= atlas_page ) {
g_cpu_atlas_pages.resize( atlas_page + 1 );
}
if ( g_cpu_atlas_pages[ atlas_page ].texture ) {
return;
}

D3D11_TEXTURE2D_DESC desc = {};
desc.Width = VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE;
desc.Height = VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE;
desc.MipLevels = 1;
desc.ArraySize = 1;
desc.Format = DXGI_FORMAT_R8_UNORM;
desc.SampleDesc.Count = 1;
desc.Usage = D3D11_USAGE_DEFAULT;
desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
DX_CHECK( g_device->CreateTexture2D( &desc, nullptr, &g_cpu_atlas_pages[ atlas_page ].texture ) );
DX_CHECK( g_device->CreateShaderResourceView( g_cpu_atlas_pages[ atlas_page ].texture, nullptr, &g_cpu_atlas_pages[ atlas_page ].srv ) );
}

static void dx11_release_cpu_atlas_pages()
{
for ( dx11_cpu_atlas_page& page : g_cpu_atlas_pages ) {
dx11_release( page.srv );
dx11_release( page.texture );
}
g_cpu_atlas_pages.clear();
}

static void dx11_unbind_ps_srv0()
{
ID3D11ShaderResourceView* null_srv = nullptr;
g_context->PSSetShaderResources( 0, 1, &null_srv );
}

static void dx11_clear_render_target( ID3D11RenderTargetView* rtv )
{
const float clear_colour[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
g_context->ClearRenderTargetView( rtv, clear_colour );
}

static ID3D11RenderTargetView* dx11_current_backbuffer_rtv()
{
return g_dx11_test_mode ? g_backbuffer_rtv_linear : g_backbuffer_rtv_srgb;
}

static void dx11_set_render_target( ID3D11RenderTargetView* rtv, UINT width, UINT height )
{
dx11_unbind_ps_srv0();
g_context->OMSetRenderTargets( 1, &rtv, nullptr );
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
vp.Width = static_cast< float >( width );
vp.Height = static_cast< float >( height );
vp.MinDepth = 0.0f;
vp.MaxDepth = 1.0f;
g_context->RSSetViewports( 1, &vp );
D3D11_RECT scissor = { 0, 0, static_cast< LONG >( width ), static_cast< LONG >( height ) };
g_context->RSSetScissorRects( 1, &scissor );
}

static void dx11_compile_buffers(
ID3D11Buffer** dest_vb,
ID3D11Buffer** dest_ib,
const ve_fontcache_vertex* verts,
int nverts,
const uint32_t* indices,
int nindices )
{
	*dest_vb = nullptr;
	*dest_ib = nullptr;

	if ( nverts > 0 ) {
	std::vector< dx11_vertex > dx11_verts( static_cast< size_t >( nverts ) );
	for ( int i = 0; i < nverts; i++ ) {
		dx11_verts[ static_cast< size_t >( i ) ].x = verts[ i ].x;
		dx11_verts[ static_cast< size_t >( i ) ].y = verts[ i ].y;
		dx11_verts[ static_cast< size_t >( i ) ].u = verts[ i ].u;
		dx11_verts[ static_cast< size_t >( i ) ].v = verts[ i ].v;
	}
	D3D11_BUFFER_DESC vb_desc = {};
	vb_desc.ByteWidth = static_cast< UINT >( nverts * static_cast< int >( sizeof( dx11_vertex ) ) );
	vb_desc.Usage = D3D11_USAGE_DEFAULT;
	vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vb_data = {};
	vb_data.pSysMem = dx11_verts.data();
	DX_CHECK( g_device->CreateBuffer( &vb_desc, &vb_data, dest_vb ) );
	}

if ( nindices > 0 ) {
D3D11_BUFFER_DESC ib_desc = {};
ib_desc.ByteWidth = static_cast< UINT >( nindices * static_cast< int >( sizeof( uint32_t ) ) );
ib_desc.Usage = D3D11_USAGE_DEFAULT;
ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
D3D11_SUBRESOURCE_DATA ib_data = {};
ib_data.pSysMem = indices;
DX_CHECK( g_device->CreateBuffer( &ib_desc, &ib_data, dest_ib ) );
}
}

static UINT dx11_bottom_left_to_top_left_y( UINT texture_height, int y, int h )
{
assert( y >= 0 && h >= 0 );
assert( static_cast< UINT >( y + h ) <= texture_height );
return texture_height - static_cast< UINT >( y + h );
}

static bool dx11_readback_texture_region( ID3D11Texture2D* source_texture, int x, int y, int w, int h, uint8_t* out_pixels )
{
if ( !source_texture || !out_pixels || x < 0 || y < 0 || w <= 0 || h <= 0 ) {
return false;
}

D3D11_TEXTURE2D_DESC source_desc = {};
source_texture->GetDesc( &source_desc );
if ( static_cast< UINT >( x + w ) > source_desc.Width || static_cast< UINT >( y + h ) > source_desc.Height ) {
return false;
}

D3D11_TEXTURE2D_DESC staging_desc = source_desc;
staging_desc.Width = static_cast< UINT >( w );
staging_desc.Height = static_cast< UINT >( h );
staging_desc.MipLevels = 1;
staging_desc.ArraySize = 1;
staging_desc.Usage = D3D11_USAGE_STAGING;
staging_desc.BindFlags = 0;
staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
staging_desc.MiscFlags = 0;
ID3D11Texture2D* staging = nullptr;
DX_CHECK( g_device->CreateTexture2D( &staging_desc, nullptr, &staging ) );

const UINT top = dx11_bottom_left_to_top_left_y( source_desc.Height, y, h );
D3D11_BOX box = {};
box.left = static_cast< UINT >( x );
box.top = top;
box.right = static_cast< UINT >( x + w );
box.bottom = top + static_cast< UINT >( h );
box.front = 0;
box.back = 1;
g_context->CopySubresourceRegion( staging, 0, 0, 0, 0, source_texture, 0, &box );

D3D11_MAPPED_SUBRESOURCE mapped = {};
DX_CHECK( g_context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) );
if ( source_desc.Format == DXGI_FORMAT_R8_UNORM ) {
for ( int row = 0; row < h; row++ ) {
const uint8_t* src = static_cast< const uint8_t* >( mapped.pData ) + row * mapped.RowPitch;
std::memcpy( out_pixels + static_cast< size_t >( h - 1 - row ) * w, src, static_cast< size_t >( w ) );
}
} else if ( source_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM ) {
for ( int row = 0; row < h; row++ ) {
const uint8_t* src = static_cast< const uint8_t* >( mapped.pData ) + row * mapped.RowPitch;
uint8_t* dst = out_pixels + static_cast< size_t >( h - 1 - row ) * w;
for ( int col = 0; col < w; col++ ) {
dst[ col ] = src[ col * 4 ];
}
}
} else {
g_context->Unmap( staging, 0 );
staging->Release();
return false;
}
g_context->Unmap( staging, 0 );
staging->Release();
	return true;
}

static void dx11_resample_greyscale_box( const uint8_t* src_pixels, int src_w, int src_h, uint8_t* dst_pixels, int dst_w, int dst_h )
{
	for ( int dst_y = 0; dst_y < dst_h; dst_y++ ) {
		const int src_y0 = ( dst_y * src_h ) / dst_h;
		const int src_y1 = std::max( src_y0 + 1, ( ( dst_y + 1 ) * src_h ) / dst_h );
		for ( int dst_x = 0; dst_x < dst_w; dst_x++ ) {
			const int src_x0 = ( dst_x * src_w ) / dst_w;
			const int src_x1 = std::max( src_x0 + 1, ( ( dst_x + 1 ) * src_w ) / dst_w );
			uint32_t sum = 0;
			uint32_t count = 0;
			for ( int src_y = src_y0; src_y < src_y1; src_y++ ) {
				for ( int src_x = src_x0; src_x < src_x1; src_x++ ) {
					sum += src_pixels[ static_cast< size_t >( src_y ) * src_w + src_x ];
					count++;
				}
			}
			dst_pixels[ static_cast< size_t >( dst_y ) * dst_w + dst_x ] = static_cast< uint8_t >( count ? sum / count : 0 );
		}
	}
}

static void dx11_update_texture_region_from_greyscale( ID3D11Texture2D* texture, UINT texture_height, int x, int y, int w, int h, const uint8_t* pixels )
{
	std::vector< uint8_t > rgba_pixels( static_cast< size_t >( w ) * static_cast< size_t >( h ) * 4 );
	for ( int row = 0; row < h; row++ ) {
		for ( int col = 0; col < w; col++ ) {
			const uint8_t value = pixels[ static_cast< size_t >( h - 1 - row ) * w + col ];
			const size_t dst_index = ( static_cast< size_t >( row ) * w + col ) * 4;
			rgba_pixels[ dst_index + 0 ] = value;
			rgba_pixels[ dst_index + 1 ] = value;
			rgba_pixels[ dst_index + 2 ] = value;
			rgba_pixels[ dst_index + 3 ] = value;
		}
	}

	const UINT top = dx11_bottom_left_to_top_left_y( texture_height, y, h );
	D3D11_BOX box = {};
	box.left = static_cast< UINT >( x );
	box.top = top;
	box.right = static_cast< UINT >( x + w );
	box.bottom = top + static_cast< UINT >( h );
	box.front = 0;
	box.back = 1;
	g_context->UpdateSubresource( texture, 0, &box, rgba_pixels.data(), static_cast< UINT >( w * 4 ), 0 );
}

static void dx11_release_backbuffer()
{
dx11_release( g_backbuffer_rtv_srgb );
dx11_release( g_backbuffer_rtv_linear );
dx11_release( g_backbuffer_texture );
}

static void dx11_create_backbuffer()
{
DX_CHECK( g_swapchain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast< void** >( &g_backbuffer_texture ) ) );
DX_CHECK( g_device->CreateRenderTargetView( g_backbuffer_texture, nullptr, &g_backbuffer_rtv_linear ) );

D3D11_RENDER_TARGET_VIEW_DESC srgb_desc = {};
srgb_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
srgb_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
srgb_desc.Texture2D.MipSlice = 0;
DX_CHECK( g_device->CreateRenderTargetView( g_backbuffer_texture, &srgb_desc, &g_backbuffer_rtv_srgb ) );
}

static void dx11_resize_backbuffer( UINT width, UINT height )
{
if ( width == 0 || height == 0 || !g_swapchain ) {
return;
}

window_size.width = width;
window_size.height = height;
g_context->OMSetRenderTargets( 0, nullptr, nullptr );
dx11_release_backbuffer();
DX_CHECK( g_swapchain->ResizeBuffers( 0, width, height, DXGI_FORMAT_UNKNOWN, 0 ) );
dx11_create_backbuffer();
}

static LRESULT CALLBACK dx11_window_proc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
switch ( message ) {
case WM_CLOSE:
DestroyWindow( hwnd );
return 0;
	case WM_DESTROY:
		g_should_close = true;
		g_hwnd = nullptr;
		PostQuitMessage( 0 );
		return 0;
case WM_SIZE:
window_size.width = static_cast< unsigned int >( LOWORD( lparam ) );
window_size.height = static_cast< unsigned int >( HIWORD( lparam ) );
if ( g_swapchain && window_size.width > 0 && window_size.height > 0 ) {
dx11_resize_backbuffer( window_size.width, window_size.height );
}
return 0;
case WM_MOUSEWHEEL:
mouse_scroll += GET_WHEEL_DELTA_WPARAM( wparam ) < 0 ? 1 : -1;
demo_autoscroll = false;
return 0;
case WM_LBUTTONDOWN:
g_mouse_left_down = true;
SetCapture( hwnd );
g_mouse_x = static_cast< short >( LOWORD( lparam ) );
g_mouse_y = static_cast< short >( HIWORD( lparam ) );
return 0;
case WM_LBUTTONUP:
g_mouse_left_down = false;
ReleaseCapture();
g_mouse_x = static_cast< short >( LOWORD( lparam ) );
g_mouse_y = static_cast< short >( HIWORD( lparam ) );
return 0;
case WM_MOUSEMOVE:
g_mouse_x = static_cast< short >( LOWORD( lparam ) );
g_mouse_y = static_cast< short >( HIWORD( lparam ) );
return 0;
}
return DefWindowProcA( hwnd, message, wparam, lparam );
}

static void dx11_create_window( int width, int height, bool start_hidden )
{
WNDCLASSA wc = {};
wc.lpfnWndProc = dx11_window_proc;
wc.hInstance = GetModuleHandleA( nullptr );
wc.lpszClassName = "VEFontCacheDX11Window";
wc.hCursor = LoadCursor( nullptr, IDC_ARROW );
RegisterClassA( &wc );

RECT rect = { 0, 0, width, height };
AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );
g_hwnd = CreateWindowExA(
0,
wc.lpszClassName,
"VEFontCache DX11",
WS_OVERLAPPEDWINDOW,
CW_USEDEFAULT,
CW_USEDEFAULT,
rect.right - rect.left,
rect.bottom - rect.top,
nullptr,
nullptr,
wc.hInstance,
nullptr );
assert( g_hwnd != nullptr );
ShowWindow( g_hwnd, start_hidden ? SW_HIDE : SW_SHOWDEFAULT );
UpdateWindow( g_hwnd );
}

static void dx11_create_device_and_swapchain( int width, int height )
{
DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
swapchain_desc.BufferCount = 1;
swapchain_desc.BufferDesc.Width = static_cast< UINT >( width );
swapchain_desc.BufferDesc.Height = static_cast< UINT >( height );
swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
swapchain_desc.OutputWindow = g_hwnd;
swapchain_desc.SampleDesc.Count = 1;
swapchain_desc.Windowed = TRUE;
swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

const D3D_FEATURE_LEVEL requested_levels[] = {
D3D_FEATURE_LEVEL_11_0,
D3D_FEATURE_LEVEL_10_1,
D3D_FEATURE_LEVEL_10_0,
};
D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
HRESULT hr = D3D11CreateDeviceAndSwapChain(
nullptr,
D3D_DRIVER_TYPE_HARDWARE,
nullptr,
0,
requested_levels,
static_cast< UINT >( std::size( requested_levels ) ),
D3D11_SDK_VERSION,
&swapchain_desc,
&g_swapchain,
&g_device,
&feature_level,
&g_context );
if ( FAILED( hr ) ) {
DX_CHECK( D3D11CreateDeviceAndSwapChain(
nullptr,
D3D_DRIVER_TYPE_WARP,
nullptr,
0,
requested_levels,
static_cast< UINT >( std::size( requested_levels ) ),
D3D11_SDK_VERSION,
&swapchain_desc,
&g_swapchain,
&g_device,
&feature_level,
&g_context ) );
}
window_size.width = static_cast< unsigned int >( width );
window_size.height = static_cast< unsigned int >( height );
dx11_create_backbuffer();
}

static void dx11_create_backend_resources()
{
ID3DBlob* vs_shared_blob = nullptr;
ID3DBlob* vs_blit_atlas_blob = nullptr;
ID3DBlob* vs_draw_text_blob = nullptr;
g_vs_shared = dx11_compile_vertex_shader( g_vs_source_shared, "vs_shared", &vs_shared_blob );
g_vs_blit_atlas = dx11_compile_vertex_shader( g_vs_source_blit_atlas, "vs_blit_atlas", &vs_blit_atlas_blob );
g_vs_draw_text = dx11_compile_vertex_shader( g_vs_source_draw_text, "vs_draw_text", &vs_draw_text_blob );

const D3D11_INPUT_ELEMENT_DESC layout[] = {
{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
DX_CHECK( g_device->CreateInputLayout( layout, static_cast< UINT >( std::size( layout ) ), vs_shared_blob->GetBufferPointer(), vs_shared_blob->GetBufferSize(), &g_input_layout_shared ) );
DX_CHECK( g_device->CreateInputLayout( layout, static_cast< UINT >( std::size( layout ) ), vs_blit_atlas_blob->GetBufferPointer(), vs_blit_atlas_blob->GetBufferSize(), &g_input_layout_blit_atlas ) );
DX_CHECK( g_device->CreateInputLayout( layout, static_cast< UINT >( std::size( layout ) ), vs_draw_text_blob->GetBufferPointer(), vs_draw_text_blob->GetBufferSize(), &g_input_layout_draw_text ) );
vs_shared_blob->Release();
vs_blit_atlas_blob->Release();
vs_draw_text_blob->Release();

g_ps_render_glyph = dx11_compile_pixel_shader( g_ps_source_render_glyph, "ps_render_glyph" );
g_ps_blit_atlas = dx11_compile_pixel_shader( g_ps_source_blit_atlas, "ps_blit_atlas" );
g_ps_draw_text = dx11_compile_pixel_shader( g_ps_source_draw_text, "ps_draw_text" );

D3D11_BLEND_DESC blend_xor = {};
blend_xor.RenderTarget[ 0 ].BlendEnable = TRUE;
blend_xor.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
blend_xor.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
blend_xor.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;
blend_xor.RenderTarget[ 0 ].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
blend_xor.RenderTarget[ 0 ].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
blend_xor.RenderTarget[ 0 ].BlendOpAlpha = D3D11_BLEND_OP_ADD;
blend_xor.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
DX_CHECK( g_device->CreateBlendState( &blend_xor, &g_blend_xor ) );

D3D11_BLEND_DESC blend_alpha = {};
blend_alpha.RenderTarget[ 0 ].BlendEnable = TRUE;
blend_alpha.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_SRC_ALPHA;
blend_alpha.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
blend_alpha.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;
blend_alpha.RenderTarget[ 0 ].SrcBlendAlpha = D3D11_BLEND_ONE;
blend_alpha.RenderTarget[ 0 ].DestBlendAlpha = D3D11_BLEND_ZERO;
blend_alpha.RenderTarget[ 0 ].BlendOpAlpha = D3D11_BLEND_OP_ADD;
blend_alpha.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
DX_CHECK( g_device->CreateBlendState( &blend_alpha, &g_blend_alpha ) );

D3D11_SAMPLER_DESC sampler_desc = {};
sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
sampler_desc.MinLOD = 0.0f;
sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
DX_CHECK( g_device->CreateSamplerState( &sampler_desc, &g_point_sampler ) );

D3D11_RASTERIZER_DESC raster_desc = {};
raster_desc.FillMode = D3D11_FILL_SOLID;
raster_desc.CullMode = D3D11_CULL_NONE;
raster_desc.ScissorEnable = TRUE;
raster_desc.DepthClipEnable = TRUE;
DX_CHECK( g_device->CreateRasterizerState( &raster_desc, &g_rasterizer_state ) );

g_cb_blit_atlas = dx11_create_dynamic_buffer( sizeof( dx11_blit_atlas_cb ), D3D11_BIND_CONSTANT_BUFFER );
g_cb_draw_text = dx11_create_dynamic_buffer( sizeof( dx11_draw_text_cb ), D3D11_BIND_CONSTANT_BUFFER );

	dx11_create_texture_target( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_glyph_buffer );
	dx11_create_texture_target( VE_FONTCACHE_ATLAS_WIDTH, VE_FONTCACHE_ATLAS_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_atlas );
}

static void dx11_destroy()
{
dx11_release_cpu_atlas_pages();
dx11_release_texture_target( g_atlas );
dx11_release_texture_target( g_glyph_buffer );
dx11_release( g_cb_draw_text );
dx11_release( g_cb_blit_atlas );
dx11_release( g_rasterizer_state );
dx11_release( g_point_sampler );
dx11_release( g_blend_alpha );
dx11_release( g_blend_xor );
dx11_release( g_input_layout_draw_text );
dx11_release( g_input_layout_blit_atlas );
dx11_release( g_input_layout_shared );
dx11_release( g_ps_draw_text );
dx11_release( g_ps_blit_atlas );
dx11_release( g_ps_render_glyph );
dx11_release( g_vs_draw_text );
dx11_release( g_vs_blit_atlas );
dx11_release( g_vs_shared );
dx11_release_backbuffer();
if ( g_context ) {
g_context->ClearState();
}
dx11_release( g_swapchain );
dx11_release( g_context );
dx11_release( g_device );
if ( g_hwnd ) {
DestroyWindow( g_hwnd );
g_hwnd = nullptr;
}
}

static void dx11_initialise( int width, int height, bool start_hidden )
{
dx11_create_window( width, height, start_hidden );
dx11_create_device_and_swapchain( width, height );
dx11_create_backend_resources();
}

static void fontcache_drawcmd()
{
ve_fontcache_optimise_drawlist( &cache );
ve_fontcache_drawlist* drawlist = ve_fontcache_get_drawlist( &cache );

ID3D11Buffer* vbo = nullptr;
ID3D11Buffer* ibo = nullptr;
dx11_compile_buffers(
&vbo,
&ibo,
drawlist->vertices.empty() ? nullptr : drawlist->vertices.data(),
static_cast< int >( drawlist->vertices.size() ),
drawlist->indices.empty() ? nullptr : drawlist->indices.data(),
static_cast< int >( drawlist->indices.size() ) );

const UINT stride = sizeof( dx11_vertex );
const UINT offset = 0;
g_context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
g_context->IASetVertexBuffers( 0, 1, &vbo, &stride, &offset );
g_context->IASetIndexBuffer( ibo, DXGI_FORMAT_R32_UINT, 0 );
g_context->RSSetState( g_rasterizer_state );
g_context->PSSetSamplers( 0, 1, &g_point_sampler );

const float blend_factor[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
for ( ve_fontcache_draw& dcall : drawlist->dcalls ) {
		if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ) {
			dx11_set_render_target( g_glyph_buffer.rtv, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
			g_context->OMSetBlendState( g_blend_xor, blend_factor, 0xFFFFFFFFu );
			g_context->IASetInputLayout( g_input_layout_shared );
			g_context->VSSetShader( g_vs_shared, nullptr, 0 );
			g_context->PSSetShader( g_ps_render_glyph, nullptr, 0 );
} else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS ) {
dx11_set_render_target( g_atlas.rtv, VE_FONTCACHE_ATLAS_WIDTH, VE_FONTCACHE_ATLAS_HEIGHT );
g_context->OMSetBlendState( g_blend_alpha, blend_factor, 0xFFFFFFFFu );
g_context->IASetInputLayout( g_input_layout_blit_atlas );
g_context->VSSetShader( g_vs_blit_atlas, nullptr, 0 );
g_context->PSSetShader( g_ps_blit_atlas, nullptr, 0 );
	dx11_blit_atlas_cb cb = {};
	cb.region = dcall.region;
	cb.source_texture_size[ 0 ] = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH );
	cb.source_texture_size[ 1 ] = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
if ( dcall.region != static_cast< uint32_t >( -1 ) ) {
float min_x = std::numeric_limits< float >::max();
float min_y = std::numeric_limits< float >::max();
float max_x = std::numeric_limits< float >::lowest();
float max_y = std::numeric_limits< float >::lowest();
float min_u = std::numeric_limits< float >::max();
float min_v = std::numeric_limits< float >::max();
float max_u = std::numeric_limits< float >::lowest();
float max_v = std::numeric_limits< float >::lowest();
for ( uint32_t index = dcall.start_index; index < dcall.end_index; index++ ) {
const ve_fontcache_vertex& vertex = drawlist->vertices[ drawlist->indices[ index ] ];
min_x = std::min( min_x, vertex.x );
min_y = std::min( min_y, vertex.y );
max_x = std::max( max_x, vertex.x );
max_y = std::max( max_y, vertex.y );
min_u = std::min( min_u, vertex.u );
min_v = std::min( min_v, vertex.v );
max_u = std::max( max_u, vertex.u );
max_v = std::max( max_v, vertex.v );
}
const float source_width = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH );
const float source_height = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
const float src_x = std::floor( min_u * source_width );
const float src_y_bottom = std::floor( min_v * source_height );
const float src_w = std::max( 1.0f, std::round( ( max_u - min_u ) * source_width ) );
const float src_h = std::max( 1.0f, std::round( ( max_v - min_v ) * source_height ) );
cb.source_rect[ 0 ] = src_x;
cb.source_rect[ 1 ] = source_height - src_y_bottom - src_h;
cb.source_rect[ 2 ] = src_w;
cb.source_rect[ 3 ] = src_h;
const float dest_x = ( min_x + 1.0f ) * 0.5f * VE_FONTCACHE_ATLAS_WIDTH;
const float dest_y_bottom = ( min_y + 1.0f ) * 0.5f * VE_FONTCACHE_ATLAS_HEIGHT;
const float dest_w = std::max( 1.0f, std::round( ( max_x - min_x ) * 0.5f * VE_FONTCACHE_ATLAS_WIDTH ) );
const float dest_h = std::max( 1.0f, std::round( ( max_y - min_y ) * 0.5f * VE_FONTCACHE_ATLAS_HEIGHT ) );
cb.dest_rect[ 0 ] = dest_x;
cb.dest_rect[ 1 ] = VE_FONTCACHE_ATLAS_HEIGHT - dest_y_bottom - dest_h;
cb.dest_rect[ 2 ] = dest_w;
cb.dest_rect[ 3 ] = dest_h;
}
dx11_update_dynamic_buffer( g_cb_blit_atlas, &cb, sizeof( cb ) );
g_context->VSSetConstantBuffers( 0, 1, &g_cb_blit_atlas );
g_context->PSSetConstantBuffers( 0, 1, &g_cb_blit_atlas );
g_context->PSSetShaderResources( 0, 1, &g_glyph_buffer.srv );
} else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET || dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED || dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) {
dx11_set_render_target( dx11_current_backbuffer_rtv(), window_size.width, window_size.height );
g_context->OMSetBlendState( g_blend_alpha, blend_factor, 0xFFFFFFFFu );
g_context->IASetInputLayout( g_input_layout_draw_text );
g_context->VSSetShader( g_vs_draw_text, nullptr, 0 );
g_context->PSSetShader( g_ps_draw_text, nullptr, 0 );
	dx11_draw_text_cb cb = {};
	cb.downsample = dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ? 1u : 0u;
	cb.colour[ 0 ] = dcall.colour[ 0 ];
	cb.colour[ 1 ] = dcall.colour[ 1 ];
	cb.colour[ 2 ] = dcall.colour[ 2 ];
	cb.colour[ 3 ] = dcall.colour[ 3 ];
if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ) {
cb.source_texture_size[ 0 ] = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH );
cb.source_texture_size[ 1 ] = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
} else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) {
cb.source_texture_size[ 0 ] = static_cast< float >( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE );
cb.source_texture_size[ 1 ] = static_cast< float >( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE );
} else {
cb.source_texture_size[ 0 ] = static_cast< float >( VE_FONTCACHE_ATLAS_WIDTH );
cb.source_texture_size[ 1 ] = static_cast< float >( VE_FONTCACHE_ATLAS_HEIGHT );
}
dx11_update_dynamic_buffer( g_cb_draw_text, &cb, sizeof( cb ) );
g_context->VSSetConstantBuffers( 0, 1, &g_cb_draw_text );
g_context->PSSetConstantBuffers( 0, 1, &g_cb_draw_text );
if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) {
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
assert( dcall.atlas_page < g_cpu_atlas_pages.size() && g_cpu_atlas_pages[ dcall.atlas_page ].srv != nullptr );
g_context->PSSetShaderResources( 0, 1, &g_cpu_atlas_pages[ dcall.atlas_page ].srv );
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
} else {
ID3D11ShaderResourceView* srv = dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ? g_glyph_buffer.srv : g_atlas.srv;
g_context->PSSetShaderResources( 0, 1, &srv );
}
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
} else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE ) {
dx11_ensure_cpu_atlas_page( dcall.atlas_page );
continue;
} else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD ) {
dx11_ensure_cpu_atlas_page( dcall.atlas_page );
dx11_unbind_ps_srv0();
const dx11_cpu_atlas_page& page = g_cpu_atlas_pages[ dcall.atlas_page ];
D3D11_TEXTURE2D_DESC page_desc = {};
page.texture->GetDesc( &page_desc );
D3D11_BOX box = {};
box.left = dcall.upload_region_x;
box.top = dx11_bottom_left_to_top_left_y( page_desc.Height, static_cast< int >( dcall.upload_region_y ), static_cast< int >( dcall.upload_region_h ) );
box.right = dcall.upload_region_x + dcall.upload_region_w;
box.bottom = box.top + dcall.upload_region_h;
box.front = 0;
box.back = 1;
const uint8_t* texels = &drawlist->texels[ dcall.texel_offset ];
g_context->UpdateSubresource( page.texture, 0, &box, texels, dcall.upload_region_w, 0 );
continue;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
} else {
continue;
}

		const UINT draw_count = dcall.end_index - dcall.start_index;
		if ( dcall.clear_before_draw ) {
			ID3D11RenderTargetView* current_rtv = nullptr;
			if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ) {
				current_rtv = g_glyph_buffer.rtv;
} else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS ) {
current_rtv = g_atlas.rtv;
} else {
current_rtv = dx11_current_backbuffer_rtv();
}
dx11_clear_render_target( current_rtv );
}

		if ( draw_count == 0 ) {
			continue;
		}
g_context->DrawIndexed( draw_count, dcall.start_index, 0 );
}

dx11_unbind_ps_srv0();
dx11_release( ibo );
dx11_release( vbo );
ve_fontcache_flush_drawlist( &cache );
}// ----------------------------------- Demo ----------------------------------

bool demo_autoscroll = true;

ve_font_id logo_font;
ve_font_id title_font;
ve_font_id print_font;
ve_font_id mono_font;
ve_font_id small_font;

ve_font_id demo_sans_font;
ve_font_id demo_serif_font;
ve_font_id demo_script_font;
ve_font_id demo_mono_font;

ve_font_id demo_chinese_font;
ve_font_id demo_japanese_font;
ve_font_id demo_korean_font;
ve_font_id demo_thai_font;
ve_font_id demo_arabic_font;
ve_font_id demo_hebrew_font;

ve_font_id demo_raincode_font;
ve_font_id demo_grid2_font;
ve_font_id demo_grid3_font;

#if 0
void test_font( ve_font_id id )
{
	std::string s = 
		/*u8"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor\n"
		u8"incididunt ut labore et dolore magna aliqua. Est ullamcorper eget nulla facilisi\n"
		u8"etiam dignissim diam quis enim. Convallis convallis tellus id interdum. Risus\n"
		u8"viverra adipiscing at in. Venenatis a condimentum vitae sapien pellentesque\n"
		u8"habitant morbi. Vitae et leo duis ut. Dignissim enim sit amet venenatis. Lacus\n"
		u8"viverra vitae congue eu consequat ac felis donec. Habitant morbi tristique\n"
		u8"senectus et netus. Scelerisque fermentum dui faucibus in ornare quam viverra\n"
		u8"orci sagittis. Porttitor lacus luctus accumsan tortor posuere ac. Tortor at\n"
		u8"auctor urna nunc id cursus metus. Massa id neque aliquam vestibulum morbi\n"
		u8"blandit cursus risus. A lacus vestibulum sed arcu non odio euismod lacinia at.\n"
		u8"Porttitor leo a diam sollicitudin tempor id eu nisl. Convallis aenean et tortor\n"
		u8"at risus viverra adipiscing at in. Dolor purus non enim praesent elementum\n"
		u8"facilisis. Hendrerit gravida rutrum quisque non tellus.\n"
		u8"\n"*/
		u8"Hello世界! Ça va! Això 文字列 ist řetězec by librería de software \"VEFONT\" キャッシュ! そうですね☺ 数\n"
		u8"左右中大小月日年早木林山川土空田天生花草虫犬人名女男子目耳口手足見\n"
		u8"毛頭顔首心時曜朝昼夜分週春夏秋冬今新古間方北南東西遠近前後内外場地\n"
		u8"国園谷野原里市京風雪雲池海岩星室戸家寺通門道話言答声聞語読書記紙画\n"
		u8"絵図工教晴思考知才理算作元食肉馬牛魚鳥羽鳴麦米茶色黄黒来行帰歩走止\n"
		u8"活店買売午汽弓回会組船明社切電毎合当台楽公引科歌刀番用何12345€£¥¢ ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
		u8"abcdefghijklmnopqrstuvwxyz\n"
		u8"1234567890\n"
		u8"!#$%^&*()_+-=`~|\n"
		u8"Erat nam at lectus urna duis. Quam elementum pulvinar etiam non quam lacus\n"
		u8"suspendisse faucibus. Vulputate odio ut enim blandit. Arcu cursus vitae congue\n"
		u8"mauris rhoncus. Sapien eget mi proin sed libero enim. Nulla facilisi nullam\n"
		u8"vehicula ipsum. Ante metus dictum at tempor commodo ullamcorper a lacus\n"
		u8"vestibulum. Mauris in aliquam sem fringilla ut morbi tincidunt augue interdum.\n"
		u8"Et molestie ac feugiat sed. Lacus sed viverra tellus in hac habitasse platea\n"
		u8"dictumst.\n";

	ve_fontcache_flush_glyph_buffer_to_atlas( &cache );
	ve_fontcache_draw_text( &cache, id, s, 0.1f, 0.8f, 1.0f / window_size.width,  1.0f / window_size.height );
}

void test_font2( ve_font_id id )
{
	ve_fontcache_flush_glyph_buffer_to_atlas( &cache );
	std::string s = u8"Hello世界! Ça va! Això 文字列 ist řetězec by librería de software \"VEFONT\" キャッシュ! そうですね☺。";
	for( int i= 0; i < 2; i++ ) {
		ve_fontcache_draw_text( &cache, id, s, 0.12f, 0.05f + i * 0.033f, 1.0f / window_size.width,  1.0f / window_size.height );
	}
}
#endif

static void load_demo_fonts()
{
	static std::vector< uint8_t > buffer, buffer2, buffer3, buffer4, buffer5, buffer6,
		buffer7, buffer8, buffer9, buffer10, buffer11, buffer12, buffer13, buffer14;

	logo_font = load_demo_font( &cache, "fonts/SawarabiMincho-Regular.ttf", buffer, 330.0f );
	title_font = load_demo_font( &cache, "fonts/OpenSans-Regular.ttf", buffer2, 42.0f );
	print_font = load_demo_font( &cache, "fonts/NotoSansJP-Light.otf", buffer3, 19.0f );
	mono_font = load_demo_font( &cache, "fonts/UbuntuMono-Regular.ttf", buffer4, 21.0f );
	small_font = load_demo_font( &cache, "fonts/Roboto-Regular.ttf", buffer14, 10.0f );

	demo_sans_font = load_demo_font( &cache, "fonts/OpenSans-Regular.ttf", buffer2, 18.0f );
	demo_serif_font = load_demo_font( &cache, "fonts/Bitter-Regular.ttf", buffer5, 18.0f );
	demo_script_font = load_demo_font( &cache, "fonts/DancingScript-Regular.ttf", buffer6, 22.0f );
	demo_mono_font = load_demo_font( &cache, "fonts/NovaMono-Regular.ttf", buffer7, 18.0f );

	demo_chinese_font = load_demo_font( &cache, "fonts/NotoSerifSC-Regular.otf", buffer8, 24.0f );
	demo_japanese_font = load_demo_font( &cache, "fonts/SawarabiMincho-Regular.ttf", buffer, 24.0f );
	demo_korean_font = load_demo_font( &cache, "fonts/NanumPenScript-Regular.ttf", buffer9, 36.0f );
	demo_thai_font = load_demo_font( &cache, "fonts/Krub-Regular.ttf", buffer10, 24.0f );
	demo_arabic_font = load_demo_font( &cache, "fonts/Tajawal-Regular.ttf", buffer11, 24.0f );
	demo_hebrew_font = load_demo_font( &cache, "fonts/DavidLibre-Regular.ttf", buffer12, 22.0f );

	demo_raincode_font = load_demo_font( &cache, "fonts/NotoSansJP-Regular.otf", buffer13, 20.0f );
	demo_grid2_font = load_demo_font( &cache, "fonts/NotoSerifSC-Regular.otf", buffer8, 54.0f );
	demo_grid3_font = load_demo_font( &cache, "fonts/Bitter-Regular.ttf", buffer5, 44.0f );
}

void init_demo()
{
	ve_fontcache_init( &cache );
	ve_fontcache_configure_snap( &cache, window_size.width, window_size.height );
	load_demo_fonts();
}

void render_demo( float dT )
{
	ve_fontcache_configure_snap( &cache, window_size.width, window_size.height );
	static float current_scroll = 0.1f;

	if ( current_scroll < 1.5f ) {
		std::u8string intro = 
			u8"Ça va! Everything here is rendered using VE Font Cache, a single header-only library designed for game engines.\n"
			u8"It aims to:\n"
			u8"           •    Be fast and simple to integrate.\n"
			u8"           •    Take advantage of modern GPU power.\n"
			u8"           •    Be backend agnostic and easy to port to any API such as Vulkan, DirectX, OpenGL.\n"
			u8"           •    Load TTF & OTF file formats directly.\n"
			u8"           •    Use only runtime cache with no offline calculation.\n"
			u8"           •    Render glyphs at reasonable quality at a wide range of hb_font sizes.\n"
			u8"           •    Support a good amount of internationalisation. そうですね!\n"
			u8"           •    Support cached text shaping with HarfBuzz with simple Latin-style fallback.\n"
			u8"           •    Load and unload fonts at any time.\n"
			;

		ve_fontcache_draw_text( &cache, logo_font, u8"ゑ", 0.4f, current_scroll + 0.0f, 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, title_font, u8"VEFontCache Demo", 0.2f, current_scroll - 0.1f, 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, intro, 0.2f, current_scroll - 0.14f, 1.0f / window_size.width,  1.0f / window_size.height );
	}

	float section_start = 0.42f; float section_end = 2.32f;
	if ( current_scroll > section_start && current_scroll < section_end )
	{
		std::u8string how_it_works = 
			u8"Glyphs are GPU rasterised with 16x supersampling. This method is a simplification of \"Easy Scalable Text Rendering on the GPU\",\n"
			u8"by Evan Wallace, making use of XOR blending. Bézier curves are handled via brute force triangle tessellation; even 6 triangles per\n"
			u8"curve only generates < 300 triangles, which is nothing for modern GPUs! This avoids complex frag shader for reasonable quality.\n"
			u8"\n"
			u8"Texture atlas caching uses naïve grid placement; this wastes a lot of space but ensures interchangeable cache slots allowing for\n"
			u8"straight up LRU ( Least Recently Used ) caching scheme to be employed.\n"
			u8"The hb_font atlas is a single 4k x 2k R8 texture divided into 4 regions:"
			;
		std::u8string caching_strategy = 
			u8"                         2k\n"
			u8"                         --------------------\n"
			u8"                         |         |        |\n"
			u8"                         |    A    |        |\n"
			u8"                         |         |        | 2\n"
			u8"                         |---------|    C   | k  \n"
			u8"                         |         |        |\n"
			u8"                      1k |    B    |        |\n"
			u8"                         |         |        |\n"
			u8"                         --------------------\n"
			u8"                         |                  |\n"
			u8"                         |                  |\n"
			u8"                         |                  | 2\n"
			u8"                         |        D         | k  \n"
			u8"                         |                  |\n"
			u8"                         |                  |\n"
			u8"                         |                  |\n"
			u8"                         --------------------\n"
			u8"                    \n"
			u8"                         Region A = 32x32 caches, 1024 glyphs\n"
			u8"                         Region B = 32x64 caches, 512 glyphs\n"
			u8"                         Region C = 64x64 caches, 512 glyphs\n"
			u8"                         Region D = 128x128 caches, 256 glyphs\n"
			;
		std::u8string how_it_works2 = 
			u8"Region A is designed for small glyphs, Region B is for tall glyphs, Region C is for large glyphs, and Region D for huge glyphs.\n"
			u8"Glyphs are first rendered to an intermediate 2k x 512px R8 texture. This allows for minimum 4 Region D glyphs supersampled at\n"
			u8"4 x 4 = 16x supersampling, and 8 Region C glyphs similarly. A simple 16-tap box downsample shader is then used to blit from this\n"
			u8"intermediate texture to the final atlas location.\n"
			;
		ve_fontcache_draw_text( &cache, title_font, u8"How it works", 0.2f, current_scroll - ( section_start + 0.06f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, how_it_works, 0.2f, current_scroll - ( section_start + 0.1f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, mono_font, caching_strategy, 0.28f, current_scroll - ( section_start + 0.32f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, how_it_works2, 0.2f, current_scroll - ( section_start + 0.82f ), 1.0f / window_size.width,  1.0f / window_size.height );
	}

	section_start = 1.2f; section_end = 3.2f;
	if ( current_scroll > section_start && current_scroll < section_end )
	{
		std::u8string font_family_test =
			u8"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor\n"
			u8"incididunt ut labore et dolore magna aliqua. Est ullamcorper eget nulla facilisi\n"
			u8"etiam dignissim diam quis enim. Convallis convallis tellus id interdum.";
		ve_fontcache_draw_text( &cache, title_font, u8"Showcase", 0.2f, current_scroll - ( section_start + 0.2f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"This is a showcase demonstrating different hb_font categories and languages.", 0.2f, current_scroll - ( section_start + 0.24f ), 1.0f / window_size.width,  1.0f / window_size.height );
	
		ve_fontcache_draw_text( &cache, print_font, u8"Sans serif", 0.2f, current_scroll - ( section_start + 0.28f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_sans_font, font_family_test, 0.3f, current_scroll - ( section_start + 0.28f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Serif", 0.2f, current_scroll - ( section_start + 0.36f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_serif_font, font_family_test, 0.3f, current_scroll - ( section_start + 0.36f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Script", 0.2f, current_scroll - ( section_start + 0.44f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_script_font, font_family_test, 0.3f, current_scroll - ( section_start + 0.44f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Monospace", 0.2f, current_scroll - ( section_start + 0.52f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_mono_font, font_family_test, 0.3f, current_scroll - ( section_start + 0.52f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Small", 0.2f, current_scroll - ( section_start + 0.60f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, small_font, font_family_test, 0.3f, current_scroll - ( section_start + 0.60f ), 1.0f / window_size.width,  1.0f / window_size.height );

		ve_fontcache_draw_text( &cache, print_font, u8"Greek", 0.2f, current_scroll - ( section_start + 0.72f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_sans_font, u8"Ήταν απλώς θέμα χρόνου.", 0.3f, current_scroll - ( section_start + 0.72f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Vietnamnese", 0.2f, current_scroll - ( section_start + 0.76f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_sans_font, u8"Bầu trời trong xanh thăm thẳm, không một gợn mây.", 0.3f, current_scroll - ( section_start + 0.76f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Thai", 0.2f, current_scroll - ( section_start + 0.80f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_thai_font, u8"การเดินทางขากลับคงจะเหงา", 0.3f, current_scroll - ( section_start + 0.80f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Chinese", 0.2f, current_scroll - ( section_start + 0.84f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_chinese_font, u8"床前明月光 疑是地上霜 举头望明月 低头思故乡", 0.3f, current_scroll - ( section_start + 0.84f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Japanese", 0.2f, current_scroll - ( section_start + 0.88f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_japanese_font, u8"ぎょしょうとナレズシの研究 モンスーン・アジアの食事文化", 0.3f, current_scroll - ( section_start + 0.88f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Korean", 0.2f, current_scroll - ( section_start + 0.92f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_korean_font, u8"그들의 장비와 기구는 모두 살아 있다.", 0.3f, current_scroll - ( section_start + 0.92f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Arabic", 0.2f, current_scroll - ( section_start + 0.96f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_arabic_font, u8"حب السماء لا تمطر غير الأحلام. This one needs HarfBuzz to work!", 0.3f, current_scroll - ( section_start + 0.96f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, print_font, u8"Hebrew", 0.2f, current_scroll - ( section_start + 1.0f ), 1.0f / window_size.width,  1.0f / window_size.height );
		ve_fontcache_draw_text( &cache, demo_hebrew_font, u8"אז הגיע הלילה של כוכב השביט הראשון. This one needs HarfBuzz to work!", 0.3f, current_scroll - ( section_start + 1.0f ), 1.0f / window_size.width,  1.0f / window_size.height );
	}

	section_start = 2.1f; section_end = section_start + 2.23f;
	if ( current_scroll > section_start && current_scroll < section_end )
	{
		const int GRID_W = 80, GRID_H = 50, NUM_RAINDROPS = GRID_W / 3;
		
		static bool init_grid = false;
		static int grid[ GRID_W * GRID_H ];
		static float grid_age[ GRID_W * GRID_H ];
		static int raindropsX[ NUM_RAINDROPS ];
		static int raindropsY[ NUM_RAINDROPS ];
		static float code_colour[4];
		static std::array< std::u8string, 72 > codes = {
			u8" ", u8"0", u8"1", u8"2", u8"3", u8"4", u8"5", u8"6", u8"7", u8"8", u8"9", u8"Z", u8"T", u8"H", u8"E", u8"｜", u8"¦", u8"日",
			u8"ﾊ", u8"ﾐ", u8"ﾋ", u8"ｰ", u8"ｳ", u8"ｼ", u8"ﾅ", u8"ﾓ", u8"ﾆ", u8"ｻ", u8"ﾜ", u8"ﾂ", u8"ｵ", u8"ﾘ", u8"ｱ", u8"ﾎ", u8"ﾃ", u8"ﾏ",
			u8"ｹ", u8"ﾒ", u8"ｴ", u8"ｶ", u8"ｷ", u8"ﾑ", u8"ﾕ", u8"ﾗ", u8"ｾ", u8"ﾈ", u8"ｽ", u8"ﾂ", u8"ﾀ", u8"ﾇ", u8"ﾍ", u8":", u8"・", u8".",
			u8"\"", u8"=", u8"*", u8"+", u8"-", u8"<", u8">", u8"ç", u8"ﾘ", u8"ｸ", u8"ｺ", u8"ﾁ", u8"ﾔ", u8"ﾙ", u8"ﾝ", u8"C", u8"O", u8"D"
		};
		
		if ( !init_grid ) {
			for ( int i = 0; i < NUM_RAINDROPS; i++ ) raindropsY[i] = GRID_H;
			init_grid = true;
		}

		static float fixed_timestep_passed = 0.0f;
		fixed_timestep_passed += dT;
		while ( fixed_timestep_passed > ( 1.0f / 20.0f ) ) {
			// Step grid.
			for ( int i = 0; i < GRID_W * GRID_H; i++ ) {
				grid_age[i] += dT;
			}

			// Step raindrops.
			for( int i = 0; i < NUM_RAINDROPS; i++ ) {
				raindropsY[i]++;
				if ( raindropsY[i] < 0 ) continue;
				if ( raindropsY[i] >= GRID_H ) {
					raindropsY[i] = -5 - ( rand() % 40 );
					raindropsX[i] = rand() % GRID_W;
					continue;
				}
				grid[ raindropsY[i] * GRID_W + raindropsX[i] ] = rand() % codes.size();
				grid_age[ raindropsY[i] * GRID_W + raindropsX[i] ] = 0.0f;
			}
			fixed_timestep_passed -= ( 1.0f / 20.0f );
		}

		// Draw grid.
		ve_fontcache_draw_text( &cache, title_font, u8"Raincode demo", 0.2f, current_scroll - ( section_start + 0.2f ), 1.0f / window_size.width,  1.0f / window_size.height, false );
		for ( int y = 0; y < GRID_H; y++ ) {
			for ( int x = 0; x < GRID_W; x++ ) {
				float posx = 0.2f + x * 0.007f, posy = current_scroll - ( section_start + 0.24f + y * 0.018f );
				float age = grid_age[ y * GRID_W + x ];
				code_colour[0] = 1.0f; code_colour[1] = 1.0f; code_colour[2] = 1.0f; code_colour[3] = 1.0f;
				if ( age > 0.0f ) {
					code_colour[0] = 0.2f; code_colour[1] = 0.3f; code_colour[2] = 0.4f;
					code_colour[3] = 1.0f - age;
					if ( code_colour[3] < 0.0f ) continue;
				}
				ve_fontcache_set_colour( &cache, code_colour );
				ve_fontcache_draw_text( &cache, demo_raincode_font, codes[ grid[ y * GRID_W + x ] ], posx, posy, 1.0f / window_size.width,  1.0f / window_size.height, false );
			}
		}
		
		code_colour[0] = code_colour[1] = code_colour[2] = code_colour[3] = 1.0f;
		ve_fontcache_set_colour( &cache, code_colour );
	}

	section_start = 3.3f; section_end = 5.1f;
	if ( current_scroll > section_start && current_scroll < section_end )
	{
		const int GRID_W = 30, GRID_H = 15, GRID2_W = 8, GRID2_H = 2, GRID3_W = 16, GRID3_H = 4;
		static int grid[ GRID_W * GRID_H ];
		static int grid2[ GRID2_W * GRID2_H ];
		static int grid3[ GRID3_W * GRID3_H ];
		
		static int rotate_current = 0;
		static float fixed_timestep_passed = 0.0f;
		fixed_timestep_passed += dT;
		while ( fixed_timestep_passed > ( 1.0f / 20.0f ) ) {
			rotate_current = ( rotate_current + 1 ) % 4;
			int rotate_idx = 0;
			for( auto& g : grid ) {
				// CJK Unified Ideographs
				if ( ( rotate_idx++ % 4 ) != rotate_current ) continue;
				g = 0x4E00 + rand() % ( 0x9FFF - 0x4E00 );
			}
			for( auto& g : grid2 ) {
				g = 0x4E00 + rand() % ( 0x9FFF - 0x4E00 );
			}
			for( auto& g : grid3 ) {
				g = rand() % 128;
			}
			fixed_timestep_passed -= ( 1.0f / 20.0f );
		}

		auto codepoint_to_utf8 = []( char *c, int chr ) {
			if (0 == chr) {
				return;
			} else if (0 == ((int32_t)0xffffff80 & chr)) {
				c[0] = (char)chr;
			} else if (0 == ((int32_t)0xfffff800 & chr)) {
				c[0] = 0xc0 | (char)(chr >> 6);
				c[1] = 0x80 | (char)(chr & 0x3f);
			} else if (0 == ((int32_t)0xffff0000 & chr)) {
				c[0] = 0xe0 | (char)(chr >> 12);
				c[1] = 0x80 | (char)((chr >> 6) & 0x3f);
				c[2] = 0x80 | (char)(chr & 0x3f);
			} else {
				c[0] = 0xf0 | (char)(chr >> 18);
				c[1] = 0x80 | (char)((chr >> 12) & 0x3f);
				c[2] = 0x80 | (char)((chr >> 6) & 0x3f);
				c[3] = 0x80 | (char)(chr & 0x3f);
			}
		};

		// Draw grid.
		ve_fontcache_draw_text( &cache, title_font, u8"Cache pressure test", 0.2f, current_scroll - ( section_start + 0.2f ), 1.0f / window_size.width,  1.0f / window_size.height, false );
		for ( int y = 0; y < GRID_H; y++ ) {
			for ( int x = 0; x < GRID_W; x++ ) {
				float posx = 0.2f + x * 0.02f, posy = current_scroll - ( section_start + 0.24f + y * 0.025f );
				char c[5] = {'\0', '\0', '\0', '\0', '\0'};
				codepoint_to_utf8( c, grid[ y * GRID_W + x ] );
				ve_fontcache_draw_text( &cache, demo_chinese_font, (const char8_t*) c, posx, posy, 1.0f / window_size.width,  1.0f / window_size.height, false );
			}
		}
		for ( int y = 0; y < GRID2_H; y++ ) {
			for ( int x = 0; x < GRID2_W; x++ ) {
				float posx = 0.2f + x * 0.03f, posy = current_scroll - ( section_start + 0.66f + y * 0.052f );
				char c[5] = {'\0', '\0', '\0', '\0', '\0'};
				codepoint_to_utf8( c, grid2[ y * GRID2_W + x ] );
				ve_fontcache_draw_text( &cache, demo_grid2_font, (const char8_t*) c, posx, posy, 1.0f / window_size.width,  1.0f / window_size.height, false );
			}
		}
		for ( int y = 0; y < GRID3_H; y++ ) {
			for ( int x = 0; x < GRID3_W; x++ ) {
				float posx = 0.45f + x * 0.02f, posy = current_scroll - ( section_start + 0.64f + y * 0.034f );
				char c[5] = {'\0', '\0', '\0', '\0', '\0'};
				codepoint_to_utf8( c, grid3[ y * GRID3_W + x ] );
				ve_fontcache_draw_text( &cache, demo_grid3_font, (const char8_t*) c, posx, posy, 1.0f / window_size.width,  1.0f / window_size.height, false );
			}
		}
	}

	// Smooth scrolling!
	// printf("%f\n", current_scroll);
	static float mouse_down_pos = -1.0f, mouse_down_scroll = -1.0f, mouse_prev_pos, scroll_velocity = 0.0f;
	if ( g_mouse_left_down ) {
		if ( mouse_down_pos < 0.0f ) {
			mouse_down_pos = mouse_prev_pos = ( float ) g_mouse_y;
			mouse_down_scroll = current_scroll;
		}
		demo_autoscroll = false;
		current_scroll = mouse_down_scroll + ( mouse_down_pos - g_mouse_y ) / window_size.height;

		float new_scroll_velocity = ( mouse_prev_pos - g_mouse_y ) / window_size.height;
		scroll_velocity = scroll_velocity * 0.2f + new_scroll_velocity * 0.8f;
		mouse_prev_pos = ( float ) g_mouse_y;
	} else {
		scroll_velocity += mouse_scroll * 0.05f;
		mouse_down_pos = -1.0f;
		float substep_dT = dT / 4.0f;
		for( int i = 0; i < 4; i++ ) {
			scroll_velocity *= exp( -3.5f * substep_dT );
			current_scroll += scroll_velocity * substep_dT * 18.0f;
		}
		if ( demo_autoscroll ) current_scroll += 0.01f * dT;
		mouse_scroll = 0;
	}
}

void test_plist()
{
	ve_fontcache_poollist plist;
	ve_fontcache_poollist_init( plist, 8 );

	for( int repeat = 0; repeat < 128; repeat++ ) {
		ve_fontcache_poollist_push_front( plist, 31337 ); assert( plist.size == 1 );
		ve_fontcache_poollist_push_front( plist, 31338 ); assert( plist.size == 2 );
		ve_fontcache_poollist_push_front( plist, 31339 ); assert( plist.size == 3 );
		auto v = ve_fontcache_poollist_pop_back( plist ); assert( v == 31337 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 31338 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 31339 );
		assert( plist.size == 0 );
	
		ve_fontcache_poollist_push_front( plist, 1337 ); assert( plist.size == 1 );
		ve_fontcache_poollist_push_front( plist, 1338 ); assert( plist.size == 2 );
		ve_fontcache_poollist_push_front( plist, 1339 ); assert( plist.size == 3 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 1337 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 1338 );
		ve_fontcache_poollist_push_front( plist, 1339 ); assert( plist.size == 2 );
		ve_fontcache_poollist_push_front( plist, 1339 ); assert( plist.size == 3 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 1339 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 1339 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 1339 );
		assert( plist.size == 0 );

		ve_fontcache_poollist_push_front( plist, 10 );
		ve_fontcache_poollist_push_front( plist, 11 );
		ve_fontcache_poollist_push_front( plist, 12 );
		ve_fontcache_poollist_push_front( plist, 13 );
		auto itr = plist.front;

		ve_fontcache_poollist_push_front( plist, 14 );
		ve_fontcache_poollist_push_front( plist, 15 );
		ve_fontcache_poollist_push_front( plist, 16 );
		ve_fontcache_poollist_push_front( plist, 17 );
		assert( plist.size == 8 );

		ve_fontcache_poollist_erase( plist, itr ); assert( plist.size == 7 );
		ve_fontcache_poollist_erase( plist, plist.front ); assert( plist.size == 6 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 10 ); assert( plist.size == 5 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 11 ); assert( plist.size == 4 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 12 ); assert( plist.size == 3 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 14 ); assert( plist.size == 2 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 15 ); assert( plist.size == 1 );
		v = ve_fontcache_poollist_pop_back( plist ); assert( v == 16 ); assert( plist.size == 0 );
	}
}

static bool has_flag( int argc, char** argv, const char* flag )
{
	for ( int i = 1; i < argc; i++ ) {
		if ( std::strcmp( argv[ i ], flag ) == 0 ) {
			return true;
		}
	}

	return false;
}
static void clear_framebuffer_colour( ID3D11RenderTargetView* rtv )
{
dx11_clear_render_target( rtv );
}

static void clear_backend_test_surfaces( bool clear_cpu_atlas_pages = true )
{
clear_framebuffer_colour( g_glyph_buffer.rtv );
clear_framebuffer_colour( g_atlas.rtv );
clear_framebuffer_colour( dx11_current_backbuffer_rtv() );
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( clear_cpu_atlas_pages && !g_cpu_atlas_pages.empty() ) {
		static std::vector< uint8_t > zeros( static_cast< size_t >( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE ) * VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE, 0 );
		for ( const dx11_cpu_atlas_page& page : g_cpu_atlas_pages ) {
			if ( !page.texture ) {
				continue;
			}
			g_context->UpdateSubresource(
				page.texture,
				0,
				nullptr,
				zeros.data(),
				VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
				0 );
		}
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
g_context->Flush();
}

static void backend_test_reset_surfaces()
{
	clear_backend_test_surfaces( cache.use_freetype ? false : true );
	g_context->Flush();
}

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
static void dx11_update_r8_texture_region_from_greyscale( ID3D11Texture2D* texture, UINT texture_height, int x, int y, int w, int h, const uint8_t* pixels )
{
	std::vector< uint8_t > r8_pixels( static_cast< size_t >( w ) * static_cast< size_t >( h ) );
	for ( int row = 0; row < h; row++ ) {
		std::memcpy(
			r8_pixels.data() + static_cast< size_t >( row ) * w,
			pixels + static_cast< size_t >( h - 1 - row ) * w,
			static_cast< size_t >( w ) );
	}

	const UINT top = dx11_bottom_left_to_top_left_y( texture_height, y, h );
	D3D11_BOX box = {};
	box.left = static_cast< UINT >( x );
	box.top = top;
	box.right = static_cast< UINT >( x + w );
	box.bottom = top + static_cast< UINT >( h );
	box.front = 0;
	box.back = 1;
	g_context->UpdateSubresource( texture, 0, &box, r8_pixels.data(), static_cast< UINT >( w ), 0 );
}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

static bool backend_test_write_surface( const char* name, int x, int y, int w, int h, const uint8_t* pixels )
{
	if ( !name || !pixels || x < 0 || y < 0 || w <= 0 || h <= 0 ) {
		return false;
	}

	if ( std::strcmp( name, "glyph_buffer" ) == 0 ) {
		if ( x + w > VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH || y + h > VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ) {
			return false;
		}
		dx11_update_texture_region_from_greyscale(
			g_glyph_buffer.texture,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT,
			x,
			y,
			w,
			h,
			pixels );
		g_context->Flush();
		return true;
	}

	if ( std::strcmp( name, "atlas" ) == 0 ) {
		if ( x + w > VE_FONTCACHE_ATLAS_WIDTH || y + h > VE_FONTCACHE_ATLAS_HEIGHT ) {
			return false;
		}
		dx11_update_texture_region_from_greyscale(
			g_atlas.texture,
			VE_FONTCACHE_ATLAS_HEIGHT,
			x,
			y,
			w,
			h,
			pixels );
		g_context->Flush();
		return true;
	}

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( std::strcmp( name, "cpu_atlas_page_0" ) == 0 ) {
		if ( x + w > VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE || y + h > VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE ) {
			return false;
		}
		dx11_ensure_cpu_atlas_page( 0 );
		dx11_update_r8_texture_region_from_greyscale(
			g_cpu_atlas_pages[ 0 ].texture,
			VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
			x,
			y,
			w,
			h,
			pixels );
		g_context->Flush();
		return true;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	return false;
}

static bool backend_test_readback( const char* name, int x, int y, int w, int h, uint8_t* out_pixels )
{
if ( std::strcmp( name, "glyph_buffer" ) == 0 ) {
return dx11_readback_texture_region( g_glyph_buffer.texture, x, y, w, h, out_pixels );
}
if ( std::strcmp( name, "atlas" ) == 0 ) {
return dx11_readback_texture_region( g_atlas.texture, x, y, w, h, out_pixels );
}
if ( std::strcmp( name, "target" ) == 0 || std::strcmp( name, "target_linear" ) == 0 || std::strcmp( name, "presented" ) == 0 ) {
return dx11_readback_texture_region( g_backbuffer_texture, x, y, w, h, out_pixels );
}
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
if ( std::strcmp( name, "cpu_atlas_page_0" ) == 0 && !g_cpu_atlas_pages.empty() && g_cpu_atlas_pages[ 0 ].texture ) {
return dx11_readback_texture_region( g_cpu_atlas_pages[ 0 ].texture, x, y, w, h, out_pixels );
}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
return false;
}

static void backend_test_execute_pipeline()
{
clear_framebuffer_colour( dx11_current_backbuffer_rtv() );
fontcache_drawcmd();
g_context->Flush();
}

static void backend_test_execute_present()
{
	g_context->Flush();
}

static void backend_test_execute_frame()
{
	clear_backend_test_surfaces( cache.use_freetype ? false : true );
	ve_fontcache_flush_drawlist( &cache );
	ve_fontcache_configure_snap( &cache, window_size.width, window_size.height );

	const float sx = 1.0f / window_size.width;
	const float sy = 1.0f / window_size.height;
	const ve_font_id logo_frame_font = logo_font >= 0 ? logo_font : print_font;
	const ve_font_id title_frame_font = title_font >= 0 ? title_font : print_font;
	const ve_font_id cjk_frame_font = demo_chinese_font >= 0 ? demo_chinese_font : print_font;
	const ve_font_id arabic_frame_font = demo_arabic_font >= 0 ? demo_arabic_font : print_font;
	const ve_font_id hebrew_frame_font = demo_hebrew_font >= 0 ? demo_hebrew_font : print_font;
	const ve_font_id rain_frame_font = demo_raincode_font >= 0 ? demo_raincode_font : print_font;
	const ve_font_id grid_frame_font = demo_grid2_font >= 0 ? demo_grid2_font : print_font;
	ve_fontcache_draw_text( &cache, logo_frame_font, u8"ゑ", 0.08f, 0.84f, sx, sy, false );
	ve_fontcache_draw_text( &cache, title_frame_font, u8"VEFontCache Demo", 0.18f, 0.84f, sx, sy, false );
	ve_fontcache_draw_text(
		&cache,
		print_font,
		u8"Backend conformance frame using real demo fonts and strings.",
		0.08f,
		0.78f,
		sx,
		sy,
		false );
	ve_fontcache_draw_text( &cache, cjk_frame_font, u8"床前明月光 疑是地上霜", 0.58f, 0.78f, sx, sy, false );
	ve_fontcache_draw_text( &cache, arabic_frame_font, u8"حب السماء لا تمطر غير الأحلام", 0.08f, 0.42f, sx, sy, false );
	ve_fontcache_draw_text( &cache, hebrew_frame_font, u8"אז הגיע הלילה של כוכב השביט הראשון", 0.08f, 0.32f, sx, sy, false );
	ve_fontcache_draw_text( &cache, rain_frame_font, u8"CODE CODE CODE 0123456789", 0.62f, 0.42f, sx, sy, false );
	ve_fontcache_draw_text( &cache, grid_frame_font, u8"漢字キャッシュ圧力", 0.62f, 0.26f, sx, sy, false );
	backend_test_execute_pipeline();
	backend_test_execute_present();
}

static int run_backend_test_mode()
{
	int total_passed = 0;
	int total_failed = 0;
	int total_skipped = 0;

	const auto run_mode = [&]( const char* mode_name, bool use_freetype ) {
		auto pick_first_available = []( std::initializer_list< ve_font_id > ids ) {
			for ( ve_font_id id : ids ) {
				if ( id >= 0 ) {
					return id;
				}
			}
			return static_cast< ve_font_id >( -1 );
		};

		cache = ve_fontcache();
		ve_fontcache_init( &cache, use_freetype );
		ve_fontcache_configure_snap( &cache, window_size.width, window_size.height );
		load_demo_fonts();

		std::vector< uint8_t > huge_buffer;
		std::vector< uint8_t > fallback_print_buffer;
		std::vector< uint8_t > fallback_title_buffer;
		std::vector< uint8_t > fallback_small_buffer;
		std::vector< uint8_t > fallback_logo_buffer;
		std::vector< std::vector< uint8_t > > reload_buffers;
		const auto apply_font_fallbacks = [&]() {
			if ( print_font < 0 ) {
				print_font = load_demo_font( &cache, "fonts/OpenSans-Regular.ttf", fallback_print_buffer, 19.0f );
			}
			if ( title_font < 0 ) {
				title_font = load_demo_font( &cache, "fonts/OpenSans-Regular.ttf", fallback_title_buffer, 42.0f );
			}
			if ( small_font < 0 ) {
				small_font = load_demo_font( &cache, "fonts/Roboto-Regular.ttf", fallback_small_buffer, 10.0f );
				if ( small_font < 0 ) {
					small_font = print_font;
				}
			}
			if ( logo_font < 0 ) {
				logo_font = load_demo_font( &cache, "fonts/OpenSans-Regular.ttf", fallback_logo_buffer, 72.0f );
			}
		};
		const auto normalize_font_ids = [&]( ve_font_id& huge_font ) {
			print_font = pick_first_available( { print_font, title_font, small_font, mono_font, demo_serif_font, demo_mono_font, demo_grid3_font } );
			title_font = pick_first_available( { title_font, print_font, mono_font, demo_serif_font } );
			small_font = pick_first_available( { small_font, print_font, title_font } );
			logo_font = pick_first_available( { logo_font, title_font, print_font } );
			if ( huge_font < 0 ) {
				huge_font = print_font;
			}
		};
		if ( !use_freetype ) {
			apply_font_fallbacks();
		}
		ve_font_id huge_test_font = load_demo_font( &cache, "fonts/NotoSansJP-Light.otf", huge_buffer, 200.0f );
		if ( huge_test_font < 0 && !use_freetype ) {
			huge_test_font = load_demo_font( &cache, "fonts/OpenSans-Regular.ttf", huge_buffer, 200.0f );
		}
		normalize_font_ids( huge_test_font );

		bool fonts_ready =
			print_font >= 0
			&& huge_test_font >= 0;
		if ( !fonts_ready ) {
			std::printf( "VEFontCache backend tests [%s] failed to load one or more demo fonts.\n", mode_name );
			ve_fontcache_shutdown( &cache );
			return false;
		}

		clear_backend_test_surfaces( use_freetype ? false : true );

		ve_fontcache_backend_test_options options;
		options.cache = &cache;
		options.font = print_font;
		options.secondary_font = title_font >= 0 ? title_font : print_font;
		options.small_font = small_font >= 0 ? small_font : print_font;
		options.latin_font = demo_grid3_font >= 0 ? demo_grid3_font : options.secondary_font;
		options.cjk_font = demo_grid2_font >= 0 ? demo_grid2_font : print_font;
		options.huge_font = huge_test_font;
		options.arabic_font = use_freetype ? demo_arabic_font : -1;
		options.hebrew_font = use_freetype ? demo_hebrew_font : -1;
		options.capabilities.has_present_surface = true;
		options.capabilities.has_target_linear_surface = true;
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
		options.capabilities.has_cpu_atlas_surface = true;
		options.capabilities.supports_freetype_mode = true;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
#ifdef VE_FONTCACHE_HARFBUZZ
		options.capabilities.supports_harfbuzz_mode = true;
#endif // VE_FONTCACHE_HARFBUZZ
		options.execute_pipeline = backend_test_execute_pipeline;
		options.execute_present = backend_test_execute_present;
		options.execute_frame = backend_test_execute_frame;
		options.readback_surface = backend_test_readback;
		options.reset_surfaces = backend_test_reset_surfaces;
		options.write_surface = backend_test_write_surface;
		options.reload_font = [ &reload_buffers ]() -> ve_font_id {
			reload_buffers.emplace_back();
			return load_demo_font(
				&cache,
				cache.use_freetype ? "fonts/NotoSansJP-Light.otf" : "fonts/OpenSans-Regular.ttf",
				reload_buffers.back(),
				19.0f );
		};
		options.prepare_real_text = [&, use_freetype]() {
			ve_fontcache_shutdown( &cache );
			cache = ve_fontcache();
			ve_fontcache_init( &cache, use_freetype );
			ve_fontcache_configure_snap( &cache, window_size.width, window_size.height );
			load_demo_fonts();
			if ( !use_freetype ) {
				apply_font_fallbacks();
			}
			ve_font_id refreshed_huge_font = -1;
			normalize_font_ids( refreshed_huge_font );
			clear_backend_test_surfaces( true );
		};

		ve_fontcache_backend_test_result result = ve_fontcache_backend_test_run( options );
		std::printf(
			"VEFontCache backend tests [%s]: %d passed, %d failed, %d skipped\n",
			mode_name,
			result.passed,
			result.failed,
			result.skipped );
		for ( const std::string& failure : result.failures ) {
			std::printf( "FAIL[%s]: %s\n", mode_name, failure.c_str() );
		}
		for ( const std::string& skipped : result.skipped_tests ) {
			std::printf( "SKIP[%s]: %s\n", mode_name, skipped.c_str() );
		}

		total_passed += result.passed;
		total_failed += result.failed;
		total_skipped += result.skipped;
		ve_fontcache_shutdown( &cache );
		return result.failed == 0;
	};

	bool ok = run_mode( "stb", false );
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	ok = run_mode( "freetype", true ) && ok;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	std::printf(
		"VEFontCache backend tests [all modes]: %d passed, %d failed, %d skipped\n",
		total_passed,
		total_failed,
		total_skipped );
	return ok ? 0 : 1;
}

int main( int argc, char** argv )
{
g_dx11_test_mode = has_flag( argc, argv, "--test" );
dx11_initialise( 1980, 1080, g_dx11_test_mode );

#ifndef VE_FONTCACHE_DEBUGPRINT
const int numGlyphs = 1024;
{
auto start = std::chrono::high_resolution_clock::now();
for ( int i = 0; i < numGlyphs; i++ ) {
// Benchmark placeholder kept aligned with the OpenGL demo.
}
auto finish = std::chrono::high_resolution_clock::now();
std::chrono::duration< double, std::milli > elapsed = finish - start;
std::printf( "ve_fontcache_cache_glyph() benchmark: total %lf ms for %d glyphs, per-glyph %lf ms\n", elapsed.count(), numGlyphs, elapsed.count() / numGlyphs );
}
#endif // VE_FONTCACHE_DEBUGPRINT

if ( g_dx11_test_mode ) {
int exit_code = run_backend_test_mode();
dx11_destroy();
return exit_code;
}

init_demo();
while ( !g_should_close ) {
MSG msg = {};
while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) ) {
TranslateMessage( &msg );
DispatchMessage( &msg );
if ( msg.message == WM_QUIT ) {
g_should_close = true;
}
}
if ( g_should_close ) {
break;
}
if ( window_size.width == 0 || window_size.height == 0 ) {
Sleep( 16 );
continue;
}

const float clear_colour[ 4 ] = {
0.18f * 0.18f,
0.204f * 0.204f,
0.251f * 0.251f,
1.0f,
};
g_context->ClearRenderTargetView( dx11_current_backbuffer_rtv(), clear_colour );

render_demo( 1.0f / 60.0f );
fontcache_drawcmd();

HRESULT present_hr = g_swapchain->Present( 1, 0 );
if ( FAILED( present_hr ) && present_hr != DXGI_STATUS_OCCLUDED ) {
DX_CHECK_IMPL( present_hr, __LINE__ );
}
}

ve_fontcache_shutdown( &cache );
dx11_destroy();
return 0;
}
