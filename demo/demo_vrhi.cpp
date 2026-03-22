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

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <unistd.h>
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <mach-o/dyld.h>
#endif
#include <GLFW/glfw3native.h>

#include <vrhi.h>

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

struct vrhi_texture_target
{
    vhTexture texture = VRHI_INVALID_HANDLE;
    nvrhi::Format format = nvrhi::Format::UNKNOWN;
    glm::ivec2 size = glm::ivec2( 0 );
    const char* name = nullptr;
};

struct vrhi_cpu_atlas_page
{
    vhTexture texture = VRHI_INVALID_HANDLE;
    nvrhi::Format format = nvrhi::Format::R8_UNORM;
    int size = VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE;
};

static demo_window_size window_size;
static int mouse_scroll = 0;
extern bool demo_autoscroll;
static bool g_mouse_left_down = false;
static double g_mouse_x = 0.0;
static double g_mouse_y = 0.0;
static GLFWwindow* g_window = nullptr;
static bool g_vrhi_initialised = false;
static bool g_use_offscreen_target = false;

static vrhi_texture_target g_glyph_buffer;
static vrhi_texture_target g_atlas;
static vrhi_texture_target g_test_target;
static vrhi_texture_target g_test_presented_target;
static vrhi_texture_target g_present_target;
static std::vector< vrhi_cpu_atlas_page > g_cpu_atlas_pages;

static vhBuffer g_dynamic_vertex_buffer = VRHI_INVALID_HANDLE;
static vhBuffer g_dynamic_index_buffer = VRHI_INVALID_HANDLE;

static vhShader g_vs_shared = VRHI_INVALID_HANDLE;
static vhShader g_vs_target = VRHI_INVALID_HANDLE;
static vhShader g_vs_present = VRHI_INVALID_HANDLE;
static vhShader g_ps_render_glyph = VRHI_INVALID_HANDLE;
static vhShader g_ps_blit_atlas = VRHI_INVALID_HANDLE;
static vhShader g_ps_draw_text = VRHI_INVALID_HANDLE;
static vhShader g_ps_present = VRHI_INVALID_HANDLE;

static vhProgram g_program_glyph;
static vhProgram g_program_atlas;
static vhProgram g_program_target;
static vhProgram g_program_present;

static vhState g_state_clear_template;
static vhState g_state_glyph_template;
static vhState g_state_atlas_template;
static vhState g_state_target_template;
static vhState g_state_present_template;

static constexpr vhStateId VEFC_VRHI_STATE_CLEAR_GLYPH = 1;
static constexpr vhStateId VEFC_VRHI_STATE_CLEAR_ATLAS = 2;
static constexpr vhStateId VEFC_VRHI_STATE_CLEAR_TARGET = 3;
static constexpr vhStateId VEFC_VRHI_STATE_GLYPH = 4;
static constexpr vhStateId VEFC_VRHI_STATE_ATLAS = 5;
static constexpr vhStateId VEFC_VRHI_STATE_TARGET = 6;
static constexpr vhStateId VEFC_VRHI_STATE_CLEAR_BACKBUFFER = 7;
static constexpr vhStateId VEFC_VRHI_STATE_PRESENT = 8;

static const char* g_fontcache_vertex_layout = "float2 ATTR0 float2 ATTR1";
static const char* g_uniform_misc0_name = "u_misc0";
static const char* g_uniform_colour_name = "u_colour";

static const char* g_vs_source_shared = R"(
struct VSInput
{
    float2 position : ATTR0;
    float2 uv : ATTR1;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput main( VSInput input )
{
    VSOutput output;
    output.position = float4( input.position.xy, 0.0f, 1.0f );
    output.uv = input.uv;
    return output;
}
)";

static const char* g_ps_source_render_glyph = R"(
float4 main() : SV_Target0
{
    return float4( 1.0f, 1.0f, 1.0f, 1.0f );
}
)";

static const char* g_ps_source_blit_atlas = R"(
Texture2D g_sourceTexture : register( t0, VRHI_STAGE_SPACE );
SamplerState g_pointSampler : register( s0, VRHI_STAGE_SPACE );
cbuffer globalParams : register( b2, VRHI_STAGE_SPACE )
{
    float4 u_misc0;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float sample_source( float2 uv )
{
    return g_sourceTexture.SampleLevel( g_pointSampler, float2( uv.x, 1.0f - uv.y ), 0.0f ).x;
}

float downsample( float2 uv, float2 texel )
{
    return sample_source( uv + float2( 0.0f, 0.0f ) * texel ) * 0.25f
        + sample_source( uv + float2( 0.0f, 1.0f ) * texel ) * 0.25f
        + sample_source( uv + float2( 1.0f, 0.0f ) * texel ) * 0.25f
        + sample_source( uv + float2( 1.0f, 1.0f ) * texel ) * 0.25f;
}

float4 main( PSInput input ) : SV_Target0
{
    const uint region = ( uint ) round( u_misc0.x );
    const float2 sourceTextureSize = float2( max( u_misc0.z, 1.0f ), max( u_misc0.w, 1.0f ) );
    const float2 texel = 1.0f / sourceTextureSize;
    if ( region == 0u || region == 1u || region == 2u ) {
        const float v =
            downsample( input.uv + float2( -1.5f, -1.5f ) * texel, texel ) * 0.25f +
            downsample( input.uv + float2(  0.5f, -1.5f ) * texel, texel ) * 0.25f +
            downsample( input.uv + float2( -1.5f,  0.5f ) * texel, texel ) * 0.25f +
            downsample( input.uv + float2(  0.5f,  0.5f ) * texel, texel ) * 0.25f;
        return float4( 1.0f, 1.0f, 1.0f, v );
    }

    return float4( 0.0f, 0.0f, 0.0f, 1.0f );
}
)";

static const char* g_vs_source_target = R"(
struct VSInput
{
    float2 position : ATTR0;
    float2 uv : ATTR1;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput main( VSInput input )
{
    VSOutput output;
    output.position = float4( input.position.xy * 2.0f - 1.0f, 0.0f, 1.0f );
    output.uv = input.uv;
    return output;
}
)";

static const char* g_ps_source_draw_text = R"(
Texture2D g_sourceTexture : register( t0, VRHI_STAGE_SPACE );
SamplerState g_pointSampler : register( s0, VRHI_STAGE_SPACE );
cbuffer globalParams : register( b2, VRHI_STAGE_SPACE )
{
    float4 u_misc0;
    float4 u_colour;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float sample_source( float2 uv )
{
    return g_sourceTexture.SampleLevel( g_pointSampler, float2( uv.x, 1.0f - uv.y ), 0.0f ).x;
}

float4 main( PSInput input ) : SV_Target0
{
    const uint downsample = ( uint ) round( u_misc0.x );
    const float2 sourceTextureSize = float2( max( u_misc0.z, 1.0f ), max( u_misc0.w, 1.0f ) );
    const float2 texel = 1.0f / sourceTextureSize;

    float v = sample_source( input.uv );
    if ( downsample == 1u ) {
        v = sample_source( input.uv + float2( -0.5f, -0.5f ) * texel ) * 0.25f
          + sample_source( input.uv + float2( -0.5f,  0.5f ) * texel ) * 0.25f
          + sample_source( input.uv + float2(  0.5f, -0.5f ) * texel ) * 0.25f
          + sample_source( input.uv + float2(  0.5f,  0.5f ) * texel ) * 0.25f;
    }

    return float4( u_colour.xyz, u_colour.w * v );
}
)";

static const char* g_vs_source_present = R"(
struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput main( uint vertex_id : SV_VulkanVertexID )
{
    const float2 positions[ 3 ] = {
        float2( -1.0f, -1.0f ),
        float2(  3.0f, -1.0f ),
        float2( -1.0f,  3.0f ),
    };
    const float2 uvs[ 3 ] = {
        float2( 0.0f, 0.0f ),
        float2( 2.0f, 0.0f ),
        float2( 0.0f, 2.0f ),
    };

    VSOutput output;
    output.position = float4( positions[ vertex_id ], 0.0f, 1.0f );
    output.uv = uvs[ vertex_id ];
    return output;
}
)";

static const char* g_ps_source_present = R"(
Texture2D g_sourceTexture : register( t0, VRHI_STAGE_SPACE );
SamplerState g_pointSampler : register( s0, VRHI_STAGE_SPACE );

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float linear_to_srgb_channel( float value )
{
    value = saturate( value );
    if ( value <= 0.0031308f ) {
        return 12.92f * value;
    }
    return 1.055f * pow( value, 1.0f / 2.4f ) - 0.055f;
}

float3 linear_to_srgb( float3 value )
{
    return float3(
        linear_to_srgb_channel( value.r ),
        linear_to_srgb_channel( value.g ),
        linear_to_srgb_channel( value.b ) );
}

float4 main( PSInput input ) : SV_Target0
{
    const float2 sample_uv = float2( input.uv.x, 1.0f - input.uv.y );
    const float4 linear = g_sourceTexture.SampleLevel( g_pointSampler, sample_uv, 0.0f );
    return float4( linear_to_srgb( linear.rgb ), linear.a );
}
)";

static void vrhi_fail( const char* message )
{
    std::printf( "%s\n", message );
    assert( !"stop" );
    std::abort();
}

static bool vrhi_is_valid( uint32_t handle )
{
    return handle != VRHI_INVALID_HANDLE;
}

static std::filesystem::path current_executable_directory()
{
#if defined(_WIN32)
    std::array< char, 4096 > path = {};
    DWORD length = GetModuleFileNameA( nullptr, path.data(), static_cast< DWORD >( path.size() ) );
    return std::filesystem::path( std::string( path.data(), length ) ).parent_path();
#elif defined(__linux__)
    std::array< char, 4096 > path = {};
    ssize_t length = readlink( "/proc/self/exe", path.data(), path.size() - 1 );
    if ( length <= 0 ) {
        return std::filesystem::current_path();
    }
    path[ static_cast< size_t >( length ) ] = '\0';
    return std::filesystem::path( std::string( path.data(), static_cast< size_t >( length ) ) ).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath( nullptr, &size );
    std::vector< char > path( size + 1, '\0' );
    if ( _NSGetExecutablePath( path.data(), &size ) != 0 ) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path( std::string( path.data() ) ).parent_path();
#else
    return std::filesystem::current_path();
#endif
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

template < typename T >
static vhMem* vrhi_copy_bytes( const std::vector< T >& data )
{
    const size_t size_bytes = data.size() * sizeof( T );
    vhMem* mem = vhAllocMem( size_bytes );
    if ( mem && size_bytes > 0 ) {
        std::memcpy( mem->data(), data.data(), size_bytes );
    }
    return mem;
}

static std::string vrhi_make_shader_temp_dir()
{
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "vefontcache-vrhi-shaders";
    std::filesystem::create_directories( dir );
    return dir.lexically_normal().string();
}

static int vrhi_bottom_left_to_top_left_y( int texture_height, int y, int h )
{
    assert( y >= 0 && h >= 0 );
    assert( y + h <= texture_height );
    return texture_height - y - h;
}

static glm::ivec2 vrhi_window_dimensions()
{
    return glm::ivec2( static_cast< int >( window_size.width ), static_cast< int >( window_size.height ) );
}

static void vrhi_set_surface_rect( vhState& state, glm::ivec2 size )
{
    state.SetViewRect( glm::vec4( 0.0f, 0.0f, static_cast< float >( size.x ), static_cast< float >( size.y ) ) );
    state.SetViewScissor( glm::vec4( 0.0f, 0.0f, static_cast< float >( size.x ), static_cast< float >( size.y ) ) );
}

static void vrhi_destroy_texture_target( vrhi_texture_target& target )
{
    if ( vrhi_is_valid( target.texture ) ) {
        vhDestroyTexture( target.texture );
    }
    target.texture = VRHI_INVALID_HANDLE;
    target.format = nvrhi::Format::UNKNOWN;
    target.size = glm::ivec2( 0 );
    target.name = nullptr;
}

static void vrhi_create_texture_target( vrhi_texture_target& target, const char* name, int width, int height, nvrhi::Format format )
{
    vrhi_destroy_texture_target( target );
    target.name = name;
    target.size = glm::ivec2( width, height );
    target.format = format;
    target.texture = vhCreateTexture2D(
        vhAllocTexture(),
        name,
        target.size,
        1,
        format,
        VRHI_TEXTURE_RT | VRHI_TEXTURE_BLIT_DST | VRHI_SAMPLER_POINT | VRHI_SAMPLER_UVW_CLAMP,
        nullptr );
}

static void vrhi_destroy_cpu_atlas_pages()
{
    for ( vrhi_cpu_atlas_page& page : g_cpu_atlas_pages ) {
        if ( vrhi_is_valid( page.texture ) ) {
            vhDestroyTexture( page.texture );
        }
        page.texture = VRHI_INVALID_HANDLE;
    }
    g_cpu_atlas_pages.clear();
}

static void vrhi_ensure_cpu_atlas_page( size_t atlas_page )
{
    if ( g_cpu_atlas_pages.size() <= atlas_page ) {
        g_cpu_atlas_pages.resize( atlas_page + 1 );
    }
    vrhi_cpu_atlas_page& page = g_cpu_atlas_pages[ atlas_page ];
    if ( vrhi_is_valid( page.texture ) ) {
        return;
    }

    page.texture = vhCreateTexture2D(
        vhAllocTexture(),
        "vefc_vrhi_cpu_atlas_page",
        glm::ivec2( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE, VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE ),
        1,
        page.format,
        VRHI_TEXTURE_BLIT_DST | VRHI_SAMPLER_POINT | VRHI_SAMPLER_UVW_CLAMP,
        nullptr );
}

static void vrhi_compile_shader( vhShader& shader, const char* name, const char* source, uint64_t stage )
{
    std::vector< uint32_t > spirv;
    std::string error;
    uint64_t flags = stage | VRHI_SHADER_SM_6_0;
#ifndef NDEBUG
    flags |= VRHI_SHADER_DEBUG;
#endif
    if ( !vhCompileShader( name, source, flags, spirv, "main", {}, {}, &error ) ) {
        std::printf( "Failed to compile %s:\n%s\n", name, error.c_str() );
        vrhi_fail( "VRHI shader compilation failed." );
    }
    shader = vhCreateShader( vhAllocShader(), name, flags, spirv, "main" );
}

static void vrhi_destroy_backend_resources()
{
    vrhi_destroy_cpu_atlas_pages();
    vrhi_destroy_texture_target( g_test_presented_target );
    vrhi_destroy_texture_target( g_present_target );
    vrhi_destroy_texture_target( g_test_target );
    vrhi_destroy_texture_target( g_atlas );
    vrhi_destroy_texture_target( g_glyph_buffer );

    if ( vrhi_is_valid( g_dynamic_index_buffer ) ) {
        vhDestroyBuffer( g_dynamic_index_buffer );
        g_dynamic_index_buffer = VRHI_INVALID_HANDLE;
    }
    if ( vrhi_is_valid( g_dynamic_vertex_buffer ) ) {
        vhDestroyBuffer( g_dynamic_vertex_buffer );
        g_dynamic_vertex_buffer = VRHI_INVALID_HANDLE;
    }

    if ( vrhi_is_valid( g_ps_draw_text ) ) {
        vhDestroyShader( g_ps_draw_text );
        g_ps_draw_text = VRHI_INVALID_HANDLE;
    }
    if ( vrhi_is_valid( g_ps_present ) ) {
        vhDestroyShader( g_ps_present );
        g_ps_present = VRHI_INVALID_HANDLE;
    }
    if ( vrhi_is_valid( g_ps_blit_atlas ) ) {
        vhDestroyShader( g_ps_blit_atlas );
        g_ps_blit_atlas = VRHI_INVALID_HANDLE;
    }
    if ( vrhi_is_valid( g_ps_render_glyph ) ) {
        vhDestroyShader( g_ps_render_glyph );
        g_ps_render_glyph = VRHI_INVALID_HANDLE;
    }
    if ( vrhi_is_valid( g_vs_target ) ) {
        vhDestroyShader( g_vs_target );
        g_vs_target = VRHI_INVALID_HANDLE;
    }
    if ( vrhi_is_valid( g_vs_present ) ) {
        vhDestroyShader( g_vs_present );
        g_vs_present = VRHI_INVALID_HANDLE;
    }
    if ( vrhi_is_valid( g_vs_shared ) ) {
        vhDestroyShader( g_vs_shared );
        g_vs_shared = VRHI_INVALID_HANDLE;
    }

    g_program_target.clear();
    g_program_present.clear();
    g_program_atlas.clear();
    g_program_glyph.clear();
    g_state_clear_template = {};
    g_state_glyph_template = {};
    g_state_atlas_template = {};
    g_state_target_template = {};
    g_state_present_template = {};
}

static void vrhi_build_state_templates()
{
    const uint64_t colour_write = VRHI_STATE_WRITE_RGB | VRHI_STATE_WRITE_A;
    const uint64_t glyph_blend =
        VRHI_STATE_BLEND_FUNC_SEPARATE(
            VRHI_STATE_BLEND_INV_DST_COLOUR,
            VRHI_STATE_BLEND_INV_SRC_COLOUR,
            VRHI_STATE_BLEND_INV_DST_ALPHA,
            VRHI_STATE_BLEND_INV_SRC_ALPHA ) |
        VRHI_STATE_BLEND_EQUATION( VRHI_STATE_BLEND_EQUATION_ADD );
    const uint64_t alpha_blend =
        VRHI_STATE_BLEND_FUNC_SEPARATE(
            VRHI_STATE_BLEND_SRC_ALPHA,
            VRHI_STATE_BLEND_INV_SRC_ALPHA,
            VRHI_STATE_BLEND_ONE,
            VRHI_STATE_BLEND_ZERO ) |
        VRHI_STATE_BLEND_EQUATION( VRHI_STATE_BLEND_EQUATION_ADD );
    const uint64_t base_flags = colour_write | VRHI_STATE_CULL_NONE | VRHI_STATE_PT_TRIANGLES;
    const uint64_t point_clamp_sampler = VRHI_SAMPLER_POINT | VRHI_SAMPLER_UVW_CLAMP;

    g_state_clear_template = {};
    g_state_clear_template.SetStateFlags( colour_write | VRHI_STATE_CULL_NONE | VRHI_STATE_PT_TRIANGLES );

    g_state_glyph_template = {};
    g_state_glyph_template
        .SetProgram( g_program_glyph )
        .SetStateFlags( base_flags | glyph_blend );

    g_state_atlas_template = {};
    g_state_atlas_template
        .SetProgram( g_program_atlas )
        .SetStateFlags( base_flags | alpha_blend )
        .SetSampler( 0, point_clamp_sampler, 0 );

    g_state_target_template = {};
    g_state_target_template
        .SetProgram( g_program_target )
        .SetStateFlags( base_flags | alpha_blend )
        .SetSampler( 0, point_clamp_sampler, 0 );

    g_state_present_template = {};
    g_state_present_template
        .SetProgram( g_program_present )
        .SetStateFlags( base_flags )
        .SetSampler( 0, point_clamp_sampler, 0 );
}

static void vrhi_create_backend_resources()
{
    vrhi_destroy_backend_resources();

    vrhi_create_texture_target(
        g_glyph_buffer,
        "vefc_vrhi_glyph_buffer",
        VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
        VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT,
        nvrhi::Format::RGBA8_UNORM );
    vrhi_create_texture_target(
        g_atlas,
        "vefc_vrhi_atlas",
        VE_FONTCACHE_ATLAS_WIDTH,
        VE_FONTCACHE_ATLAS_HEIGHT,
        nvrhi::Format::RGBA8_UNORM );
    if ( g_use_offscreen_target ) {
        vrhi_create_texture_target(
            g_test_target,
            "vefc_vrhi_test_target",
            static_cast< int >( window_size.width ),
            static_cast< int >( window_size.height ),
            nvrhi::Format::RGBA8_UNORM );
        vrhi_create_texture_target(
            g_test_presented_target,
            "vefc_vrhi_test_presented_target",
            static_cast< int >( window_size.width ),
            static_cast< int >( window_size.height ),
            nvrhi::Format::RGBA8_UNORM );
    } else {
        vrhi_create_texture_target(
            g_present_target,
            "vefc_vrhi_present_target",
            static_cast< int >( window_size.width ),
            static_cast< int >( window_size.height ),
            nvrhi::Format::RGBA8_UNORM );
    }

    g_dynamic_vertex_buffer = vhCreateVertexBuffer(
        vhAllocBuffer(),
        "vefc_vrhi_dynamic_vertex_buffer",
        nullptr,
        g_fontcache_vertex_layout,
        1,
        VRHI_BUFFER_ALLOW_RESIZE );
    g_dynamic_index_buffer = vhCreateIndexBuffer(
        vhAllocBuffer(),
        "vefc_vrhi_dynamic_index_buffer",
        nullptr,
        1,
        VRHI_BUFFER_ALLOW_RESIZE | VRHI_BUFFER_INDEX32 );

    vrhi_compile_shader( g_vs_shared, "vefc_vrhi_vs_shared", g_vs_source_shared, VRHI_SHADER_STAGE_VERTEX );
    vrhi_compile_shader( g_vs_target, "vefc_vrhi_vs_target", g_vs_source_target, VRHI_SHADER_STAGE_VERTEX );
    vrhi_compile_shader( g_vs_present, "vefc_vrhi_vs_present", g_vs_source_present, VRHI_SHADER_STAGE_VERTEX );
    vrhi_compile_shader( g_ps_render_glyph, "vefc_vrhi_ps_render_glyph", g_ps_source_render_glyph, VRHI_SHADER_STAGE_PIXEL );
    vrhi_compile_shader( g_ps_blit_atlas, "vefc_vrhi_ps_blit_atlas", g_ps_source_blit_atlas, VRHI_SHADER_STAGE_PIXEL );
    vrhi_compile_shader( g_ps_draw_text, "vefc_vrhi_ps_draw_text", g_ps_source_draw_text, VRHI_SHADER_STAGE_PIXEL );
    vrhi_compile_shader( g_ps_present, "vefc_vrhi_ps_present", g_ps_source_present, VRHI_SHADER_STAGE_PIXEL );

    g_program_glyph = vhCreateGfxProgram( g_vs_shared, g_ps_render_glyph );
    g_program_atlas = vhCreateGfxProgram( g_vs_shared, g_ps_blit_atlas );
    g_program_target = vhCreateGfxProgram( g_vs_target, g_ps_draw_text );
    g_program_present = vhCreateGfxProgram( g_vs_present, g_ps_present );

    vrhi_build_state_templates();
    vhFinish();
}

static void vrhi_reset_init_data( int width, int height, bool headless )
{
    g_vhInit = vhInitData{};
    g_vhInit.resolution = glm::ivec2( width, height );
#ifndef NDEBUG
    g_vhInit.debug = true;
#else
    g_vhInit.debug = false;
#endif
    g_vhInit.vsync = !headless;
    g_vhInit.headless = headless;
    g_vhInit.nullMode = false;
    g_vhInit.errorOnSkippedDraw = true;
    g_vhInit.shaderCompileTempDir = vrhi_make_shader_temp_dir();
    std::filesystem::create_directories( g_vhInit.shaderCompileTempDir );
}

static void vrhi_initialise_headless( int width, int height )
{
    window_size.width = static_cast< unsigned int >( width );
    window_size.height = static_cast< unsigned int >( height );
    g_use_offscreen_target = true;
    vrhi_reset_init_data( width, height, true );
    vhInit();
    g_vrhi_initialised = true;
    vrhi_create_backend_resources();
}

static void vrhi_initialise_windowed( GLFWwindow* window, int width, int height )
{
    g_use_offscreen_target = false;
    vrhi_reset_init_data( width, height, false );
#if defined(_WIN32)
    g_vhInit.windowHandle = glfwGetWin32Window( window );
#elif defined(__linux__)
    g_vhInit.windowHandle = reinterpret_cast< void* >( glfwGetX11Window( window ) );
    g_vhInit.displayHandle = glfwGetX11Display();
#elif defined(__APPLE__)
    g_vhInit.windowHandle = glfwGetCocoaView( window );
    g_vhInit.macOSWindowIsNSView = true;
#endif
    vhInit();
    g_vrhi_initialised = true;
    vrhi_create_backend_resources();
}

static void vrhi_shutdown_runtime()
{
    if ( !g_vrhi_initialised ) {
        return;
    }
    vhFinish();
    vrhi_destroy_backend_resources();
    vhFinish();
    vhShutdown();
    g_vrhi_initialised = false;
    g_use_offscreen_target = false;
}

static bool vrhi_prepare_target_surface( vhState& state )
{
    if ( g_use_offscreen_target ) {
        state.SetColourAttachment( 0, g_test_target.texture, 0, 0, g_test_target.format );
        vrhi_set_surface_rect( state, g_test_target.size );
        return true;
    }

    if ( !vrhi_is_valid( g_present_target.texture ) ) {
        return false;
    }

    state.SetColourAttachment( 0, g_present_target.texture, 0, 0, g_present_target.format );
    vrhi_set_surface_rect( state, g_present_target.size );
    return true;
}

static bool vrhi_prepare_swapchain_surface( vhState& state )
{
    vhTexture backbuffer = vhGetBackbuffer();
    if ( !vrhi_is_valid( backbuffer ) ) {
        return false;
    }

    state.SetColourAttachment( 0, backbuffer );
    vrhi_set_surface_rect( state, vrhi_window_dimensions() );
    return true;
}

static void vrhi_prepare_fixed_surface( vhState& state, const vrhi_texture_target& target )
{
    state.SetColourAttachment( 0, target.texture, 0, 0, target.format );
    vrhi_set_surface_rect( state, target.size );
}

static void vrhi_clear_bound_surface( vhStateId state_id, vhState state, const glm::vec4& clear_colour )
{
    state.SetClearColor( clear_colour );
    state.DirtyAll();
    vhSetState( state_id, state );
    vhClear( state_id, VRHI_CLEAR_COLOR );
}

static void vrhi_clear_texture_target( vhStateId state_id, const vrhi_texture_target& target, const glm::vec4& clear_colour = glm::vec4( 0.0f ) )
{
    vhState state = g_state_clear_template;
    vrhi_prepare_fixed_surface( state, target );
    vrhi_clear_bound_surface( state_id, state, clear_colour );
}

static bool vrhi_clear_current_target( const glm::vec4& clear_colour )
{
    vhState state = g_state_clear_template;
    if ( !vrhi_prepare_target_surface( state ) ) {
        return false;
    }
    vrhi_clear_bound_surface( VEFC_VRHI_STATE_CLEAR_TARGET, state, clear_colour );
    return true;
}

static void vrhi_recreate_present_target()
{
    if ( g_use_offscreen_target || !g_vrhi_initialised || window_size.width == 0 || window_size.height == 0 ) {
        return;
    }

    vrhi_create_texture_target(
        g_present_target,
        "vefc_vrhi_present_target",
        static_cast< int >( window_size.width ),
        static_cast< int >( window_size.height ),
        nvrhi::Format::RGBA8_UNORM );
}

static bool vrhi_present_texture_to_surface( vhTexture source_texture, const vrhi_texture_target& destination )
{
    if ( !vrhi_is_valid( source_texture ) || !vrhi_is_valid( destination.texture ) ) {
        return false;
    }

    vhState clear_state = g_state_clear_template;
    clear_state.SetColourAttachment( 0, destination.texture, 0, 0, destination.format );
    vrhi_set_surface_rect( clear_state, destination.size );
    vrhi_clear_bound_surface( VEFC_VRHI_STATE_CLEAR_TARGET, clear_state, glm::vec4( 0.0f ) );

    vhState state = g_state_present_template;
    state
        .SetColourAttachment( 0, destination.texture, 0, 0, destination.format )
        .SetTexture( 0, source_texture, 0 )
        .ClearVertexBindings();
    vrhi_set_surface_rect( state, destination.size );
    state.DirtyAll();
    vhSetState( VEFC_VRHI_STATE_PRESENT, state );
    vhDraw( VEFC_VRHI_STATE_PRESENT, 3, 1, 0, 0 );
    return true;
}

static bool vrhi_present_current_target()
{
    if ( g_use_offscreen_target || !vrhi_is_valid( g_present_target.texture ) ) {
        return false;
    }

    // VRHI's public init path does not expose a swapchain sRGB selection knob,
    // so interactive rendering goes through a linear offscreen target first.
    vhState clear_state = g_state_clear_template;
    if ( !vrhi_prepare_swapchain_surface( clear_state ) ) {
        return false;
    }
    vrhi_clear_bound_surface( VEFC_VRHI_STATE_CLEAR_BACKBUFFER, clear_state, glm::vec4( 0.0f ) );

    vhState state = g_state_present_template;
    if ( !vrhi_prepare_swapchain_surface( state ) ) {
        return false;
    }

    state
        .SetTexture( 0, g_present_target.texture, 0 )
        .ClearVertexBindings()
        .DirtyAll();
    vhSetState( VEFC_VRHI_STATE_PRESENT, state );
    vhDraw( VEFC_VRHI_STATE_PRESENT, 3, 1, 0, 0 );
    return true;
}

static std::vector< uint8_t > vrhi_greyscale_bottom_left_to_top_left_r8( const uint8_t* pixels, int w, int h )
{
    std::vector< uint8_t > out( static_cast< size_t >( w ) * static_cast< size_t >( h ) );
    for ( int row = 0; row < h; row++ ) {
        const uint8_t* src = pixels + static_cast< size_t >( h - 1 - row ) * w;
        std::memcpy( out.data() + static_cast< size_t >( row ) * w, src, static_cast< size_t >( w ) );
    }
    return out;
}

static std::vector< uint8_t > vrhi_greyscale_bottom_left_to_top_left_rgba( const uint8_t* pixels, int w, int h )
{
    std::vector< uint8_t > out( static_cast< size_t >( w ) * static_cast< size_t >( h ) * 4 );
    for ( int row = 0; row < h; row++ ) {
        const uint8_t* src = pixels + static_cast< size_t >( h - 1 - row ) * w;
        uint8_t* dst = out.data() + static_cast< size_t >( row ) * w * 4;
        for ( int col = 0; col < w; col++ ) {
            const uint8_t value = src[ col ];
            dst[ col * 4 + 0 ] = value;
            dst[ col * 4 + 1 ] = value;
            dst[ col * 4 + 2 ] = value;
            dst[ col * 4 + 3 ] = value;
        }
    }
    return out;
}

static void vrhi_upload_rect_via_blit( vhTexture dst, nvrhi::Format format, int dst_x, int dst_y_top_left, int w, int h, const std::vector< uint8_t >& pixels )
{
    vhTexture upload = vhCreateTexture2D(
        vhAllocTexture(),
        "vefc_vrhi_upload_texture",
        glm::ivec2( w, h ),
        1,
        format,
        VRHI_TEXTURE_NONE,
        vhAllocMem( pixels ) );
    vhBlitTexture( dst, upload, 0, 0, 0, 0, glm::ivec3( dst_x, dst_y_top_left, 0 ), glm::ivec3( 0 ), glm::ivec3( w, h, 1 ) );
    vhDestroyTexture( upload );
}

static void vrhi_upload_greyscale_rect_to_rgba_target( const vrhi_texture_target& target, int x, int y, int w, int h, const uint8_t* pixels )
{
    const int dst_y_top_left = vrhi_bottom_left_to_top_left_y( target.size.y, y, h );
    std::vector< uint8_t > rgba_pixels = vrhi_greyscale_bottom_left_to_top_left_rgba( pixels, w, h );
    vrhi_upload_rect_via_blit( target.texture, target.format, x, dst_y_top_left, w, h, rgba_pixels );
}

static void vrhi_upload_greyscale_rect_to_r8_texture( vhTexture texture, int texture_height, int x, int y, int w, int h, const uint8_t* pixels )
{
    const int dst_y_top_left = vrhi_bottom_left_to_top_left_y( texture_height, y, h );
    std::vector< uint8_t > r8_pixels = vrhi_greyscale_bottom_left_to_top_left_r8( pixels, w, h );
    vrhi_upload_rect_via_blit( texture, nvrhi::Format::R8_UNORM, x, dst_y_top_left, w, h, r8_pixels );
}

static bool vrhi_readback_texture_region( vhTexture texture, nvrhi::Format format, int texture_width, int texture_height, int x, int y, int w, int h, uint8_t* out_pixels )
{
    if ( !vrhi_is_valid( texture ) || !out_pixels || x < 0 || y < 0 || w <= 0 || h <= 0 ) {
        return false;
    }
    if ( x + w > texture_width || y + h > texture_height ) {
        return false;
    }

    vhMem pixels;
    vhReadTextureSlow( texture, 0, 0, &pixels );
    vhFinish();

    const int bytes_per_pixel = format == nvrhi::Format::R8_UNORM ? 1 : 4;
    const size_t required = static_cast< size_t >( texture_width ) * static_cast< size_t >( texture_height ) * bytes_per_pixel;
    if ( pixels.size() < required ) {
        return false;
    }

    const int src_y_top_left = vrhi_bottom_left_to_top_left_y( texture_height, y, h );
    for ( int row = 0; row < h; row++ ) {
        const uint8_t* src = pixels.data() + ( static_cast< size_t >( src_y_top_left + row ) * texture_width + x ) * bytes_per_pixel;
        uint8_t* dst = out_pixels + static_cast< size_t >( h - 1 - row ) * w;
        if ( bytes_per_pixel == 1 ) {
            std::memcpy( dst, src, static_cast< size_t >( w ) );
        } else {
            for ( int col = 0; col < w; col++ ) {
                dst[ col ] = src[ col * 4 ];
            }
        }
    }

    return true;
}

static void vrhi_update_dynamic_buffers( const ve_fontcache_drawlist& drawlist )
{
    if ( !drawlist.vertices.empty() ) {
        vhUpdateVertexBuffer(
            g_dynamic_vertex_buffer,
            vrhi_copy_bytes( drawlist.vertices ),
            0,
            static_cast< uint64_t >( drawlist.vertices.size() ) );
    }
    if ( !drawlist.indices.empty() ) {
        vhUpdateIndexBuffer(
            g_dynamic_index_buffer,
            vrhi_copy_bytes( drawlist.indices ),
            0,
            static_cast< uint64_t >( drawlist.indices.size() ) );
    }
}

static void fontcache_drawcmd()
{
    ve_fontcache_optimise_drawlist( &cache );
    ve_fontcache_drawlist* drawlist = ve_fontcache_get_drawlist( &cache );
    vrhi_update_dynamic_buffers( *drawlist );

    for ( ve_fontcache_draw& dcall : drawlist->dcalls ) {
        vhState state;
        vhStateId state_id = 0;
        bool known_pass = true;

        if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ) {
            state = g_state_glyph_template;
            state_id = VEFC_VRHI_STATE_GLYPH;
            vrhi_prepare_fixed_surface( state, g_glyph_buffer );
        } else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS ) {
            state = g_state_atlas_template;
            state_id = VEFC_VRHI_STATE_ATLAS;
            vrhi_prepare_fixed_surface( state, g_atlas );
            const glm::vec4 misc0(
                static_cast< float >( dcall.region ),
                0.0f,
                static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH ),
                static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ) );
            state.SetUniform( 0, g_uniform_misc0_name, &misc0, 1 );
            state.SetTexture( 0, g_glyph_buffer.texture, 0 );
        } else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET || dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED || dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) {
            state = g_state_target_template;
            state_id = VEFC_VRHI_STATE_TARGET;
            if ( !vrhi_prepare_target_surface( state ) ) {
                continue;
            }

            float source_width = static_cast< float >( VE_FONTCACHE_ATLAS_WIDTH );
            float source_height = static_cast< float >( VE_FONTCACHE_ATLAS_HEIGHT );
            float downsample = 0.0f;
            if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ) {
                downsample = 1.0f;
                source_width = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH );
                source_height = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
                state.SetTexture( 0, g_glyph_buffer.texture, 0 );
            } else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) {
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
                if ( dcall.atlas_page >= g_cpu_atlas_pages.size() || !vrhi_is_valid( g_cpu_atlas_pages[ dcall.atlas_page ].texture ) ) {
                    continue;
                }
                source_width = static_cast< float >( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE );
                source_height = static_cast< float >( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE );
                state.SetTexture( 0, g_cpu_atlas_pages[ dcall.atlas_page ].texture, 0 );
#else
                continue;
#endif
            } else {
                state.SetTexture( 0, g_atlas.texture, 0 );
            }

            const glm::vec4 misc0( downsample, 0.0f, source_width, source_height );
            const glm::vec4 colour( dcall.colour[ 0 ], dcall.colour[ 1 ], dcall.colour[ 2 ], dcall.colour[ 3 ] );
            state.SetUniform( 0, g_uniform_misc0_name, &misc0, 1 );
            state.SetUniform( 1, g_uniform_colour_name, &colour, 1 );
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
        } else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE ) {
            vrhi_ensure_cpu_atlas_page( dcall.atlas_page );
            continue;
        } else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD ) {
            vrhi_ensure_cpu_atlas_page( dcall.atlas_page );
            const uint8_t* texels = &drawlist->texels[ dcall.texel_offset ];
            vrhi_upload_greyscale_rect_to_r8_texture(
                g_cpu_atlas_pages[ dcall.atlas_page ].texture,
                VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
                static_cast< int >( dcall.upload_region_x ),
                static_cast< int >( dcall.upload_region_y ),
                static_cast< int >( dcall.upload_region_w ),
                static_cast< int >( dcall.upload_region_h ),
                texels );
            continue;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
        } else {
            known_pass = false;
        }

        if ( !known_pass ) {
            continue;
        }

        if ( dcall.clear_before_draw ) {
            vhState clear_state = state;
            clear_state.DirtyAll();
            vhSetState( state_id, clear_state );
            vhClear( state_id, VRHI_CLEAR_COLOR );
        }

        const uint32_t draw_count = dcall.end_index - dcall.start_index;
        if ( draw_count == 0 ) {
            continue;
        }
        if ( drawlist->vertices.empty() || drawlist->indices.empty() ) {
            continue;
        }

        state
            .SetVertexBuffer( g_dynamic_vertex_buffer, 0, 0, 0, static_cast< uint32_t >( drawlist->vertices.size() ) )
            .SetIndexBuffer( g_dynamic_index_buffer, 0, 0, static_cast< uint32_t >( drawlist->indices.size() ) )
            .DirtyAll();
        vhSetState( state_id, state );
        vhDrawIndexed( state_id, draw_count, 1, dcall.start_index, 0, 0 );
    }

    ve_fontcache_flush_drawlist( &cache );
}

static void framebuffer_size_callback( GLFWwindow*, int width, int height )
{
    if ( width < 0 || height < 0 ) {
        return;
    }
    window_size.width = static_cast< unsigned int >( width );
    window_size.height = static_cast< unsigned int >( height );
    if ( g_vrhi_initialised && width > 0 && height > 0 ) {
        vhResize( width, height );
        vhResizeCleanup();
        vrhi_recreate_present_target();
    }
}

static void scroll_callback( GLFWwindow*, double, double yoffset )
{
    mouse_scroll += yoffset < 0.0 ? 1 : -1;
    demo_autoscroll = false;
}

static void cursor_pos_callback( GLFWwindow*, double xpos, double ypos )
{
    g_mouse_x = xpos;
    g_mouse_y = ypos;
}

static void mouse_button_callback( GLFWwindow* window, int button, int action, int )
{
    if ( button != GLFW_MOUSE_BUTTON_LEFT ) {
        return;
    }
    g_mouse_left_down = action == GLFW_PRESS;
    if ( g_mouse_left_down ) {
        glfwGetCursorPos( window, &g_mouse_x, &g_mouse_y );
    }
}

// ----------------------------------- Demo ----------------------------------

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

static ve_font_id pick_first_available_demo_font( std::initializer_list< ve_font_id > ids )
{
    for ( ve_font_id id : ids ) {
        if ( id >= 0 ) {
            return id;
        }
    }
    return static_cast< ve_font_id >( -1 );
}

static void normalize_demo_font_ids( ve_font_id* huge_font = nullptr )
{
    print_font = pick_first_available_demo_font( { print_font, title_font, small_font, mono_font, demo_serif_font, demo_mono_font, demo_grid3_font } );
    title_font = pick_first_available_demo_font( { title_font, print_font, mono_font, demo_serif_font } );
    mono_font = pick_first_available_demo_font( { mono_font, print_font, title_font } );
    small_font = pick_first_available_demo_font( { small_font, print_font, title_font, mono_font } );
    logo_font = pick_first_available_demo_font( { logo_font, title_font, print_font } );
    demo_sans_font = pick_first_available_demo_font( { demo_sans_font, print_font, title_font } );
    demo_serif_font = pick_first_available_demo_font( { demo_serif_font, print_font, title_font } );
    demo_script_font = pick_first_available_demo_font( { demo_script_font, demo_serif_font, demo_sans_font, print_font } );
    demo_mono_font = pick_first_available_demo_font( { demo_mono_font, mono_font, print_font } );
    demo_chinese_font = pick_first_available_demo_font( { demo_chinese_font, demo_japanese_font, demo_grid2_font, print_font } );
    demo_japanese_font = pick_first_available_demo_font( { demo_japanese_font, demo_chinese_font, demo_grid2_font, print_font } );
    demo_korean_font = pick_first_available_demo_font( { demo_korean_font, demo_chinese_font, demo_japanese_font, print_font } );
    demo_thai_font = pick_first_available_demo_font( { demo_thai_font, demo_sans_font, print_font } );
    demo_arabic_font = pick_first_available_demo_font( { demo_arabic_font, print_font } );
    demo_hebrew_font = pick_first_available_demo_font( { demo_hebrew_font, print_font } );
    demo_raincode_font = pick_first_available_demo_font( { demo_raincode_font, demo_mono_font, print_font } );
    demo_grid2_font = pick_first_available_demo_font( { demo_grid2_font, demo_chinese_font, demo_japanese_font, print_font } );
    demo_grid3_font = pick_first_available_demo_font( { demo_grid3_font, demo_serif_font, print_font } );
    if ( huge_font != nullptr && *huge_font < 0 ) {
        *huge_font = print_font;
    }
}

void init_demo()
{
    ve_fontcache_init( &cache );
    ve_fontcache_configure_snap( &cache, window_size.width, window_size.height );
    load_demo_fonts();
    normalize_demo_font_ids();
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

static void clear_backend_test_surfaces( bool clear_cpu_atlas_pages = true )
{
    vrhi_clear_texture_target( VEFC_VRHI_STATE_CLEAR_GLYPH, g_glyph_buffer );
    vrhi_clear_texture_target( VEFC_VRHI_STATE_CLEAR_ATLAS, g_atlas );
    if ( g_use_offscreen_target ) {
        vrhi_clear_texture_target( VEFC_VRHI_STATE_CLEAR_TARGET, g_test_target );
        vrhi_clear_texture_target( VEFC_VRHI_STATE_CLEAR_TARGET, g_test_presented_target );
    }
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
    if ( clear_cpu_atlas_pages && !g_cpu_atlas_pages.empty() ) {
        static std::vector< uint8_t > zeros(
            static_cast< size_t >( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE ) * VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
            0 );
        for ( const vrhi_cpu_atlas_page& page : g_cpu_atlas_pages ) {
            if ( !vrhi_is_valid( page.texture ) ) {
                continue;
            }
            vrhi_upload_greyscale_rect_to_r8_texture(
                page.texture,
                VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
                0,
                0,
                VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
                VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
                zeros.data() );
        }
    }
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
    vhFinish();
}

static void backend_test_reset_surfaces()
{
    clear_backend_test_surfaces( cache.use_freetype ? false : true );
}

static bool backend_test_write_surface( const char* name, int x, int y, int w, int h, const uint8_t* pixels )
{
    if ( !name || !pixels || x < 0 || y < 0 || w <= 0 || h <= 0 ) {
        return false;
    }

    if ( std::strcmp( name, "glyph_buffer" ) == 0 ) {
        if ( x + w > g_glyph_buffer.size.x || y + h > g_glyph_buffer.size.y ) {
            return false;
        }
        vrhi_upload_greyscale_rect_to_rgba_target( g_glyph_buffer, x, y, w, h, pixels );
        vhFinish();
        return true;
    }

    if ( std::strcmp( name, "atlas" ) == 0 ) {
        if ( x + w > g_atlas.size.x || y + h > g_atlas.size.y ) {
            return false;
        }
        vrhi_upload_greyscale_rect_to_rgba_target( g_atlas, x, y, w, h, pixels );
        vhFinish();
        return true;
    }

    if ( std::strcmp( name, "target" ) == 0 && g_use_offscreen_target ) {
        if ( x + w > g_test_target.size.x || y + h > g_test_target.size.y ) {
            return false;
        }
        vrhi_upload_greyscale_rect_to_rgba_target( g_test_target, x, y, w, h, pixels );
        vhFinish();
        return true;
    }

    if ( std::strcmp( name, "target_linear" ) == 0 && g_use_offscreen_target ) {
        if ( x + w > g_test_target.size.x || y + h > g_test_target.size.y ) {
            return false;
        }
        vrhi_upload_greyscale_rect_to_rgba_target( g_test_target, x, y, w, h, pixels );
        vhFinish();
        return true;
    }

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
    if ( std::strcmp( name, "cpu_atlas_page_0" ) == 0 ) {
        if ( x + w > VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE || y + h > VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE ) {
            return false;
        }
        vrhi_ensure_cpu_atlas_page( 0 );
        vrhi_upload_greyscale_rect_to_r8_texture(
            g_cpu_atlas_pages[ 0 ].texture,
            VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
            x,
            y,
            w,
            h,
            pixels );
        vhFinish();
        return true;
    }
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

    return false;
}

static bool backend_test_readback( const char* name, int x, int y, int w, int h, uint8_t* out_pixels )
{
    if ( std::strcmp( name, "glyph_buffer" ) == 0 ) {
        return vrhi_readback_texture_region( g_glyph_buffer.texture, g_glyph_buffer.format, g_glyph_buffer.size.x, g_glyph_buffer.size.y, x, y, w, h, out_pixels );
    }
    if ( std::strcmp( name, "atlas" ) == 0 ) {
        return vrhi_readback_texture_region( g_atlas.texture, g_atlas.format, g_atlas.size.x, g_atlas.size.y, x, y, w, h, out_pixels );
    }
    if ( std::strcmp( name, "target" ) == 0 && g_use_offscreen_target ) {
        return vrhi_readback_texture_region( g_test_target.texture, g_test_target.format, g_test_target.size.x, g_test_target.size.y, x, y, w, h, out_pixels );
    }
    if ( std::strcmp( name, "target_linear" ) == 0 && g_use_offscreen_target ) {
        return vrhi_readback_texture_region( g_test_target.texture, g_test_target.format, g_test_target.size.x, g_test_target.size.y, x, y, w, h, out_pixels );
    }
    if ( std::strcmp( name, "presented" ) == 0 && g_use_offscreen_target ) {
        return vrhi_readback_texture_region(
            g_test_presented_target.texture,
            g_test_presented_target.format,
            g_test_presented_target.size.x,
            g_test_presented_target.size.y,
            x,
            y,
            w,
            h,
            out_pixels );
    }
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
    if ( std::strcmp( name, "cpu_atlas_page_0" ) == 0 && !g_cpu_atlas_pages.empty() && vrhi_is_valid( g_cpu_atlas_pages[ 0 ].texture ) ) {
        return vrhi_readback_texture_region(
            g_cpu_atlas_pages[ 0 ].texture,
            g_cpu_atlas_pages[ 0 ].format,
            VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
            VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE,
            x,
            y,
            w,
            h,
            out_pixels );
    }
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
    return false;
}

static void backend_test_execute_pipeline()
{
    vrhi_clear_texture_target( VEFC_VRHI_STATE_CLEAR_TARGET, g_test_target );
    fontcache_drawcmd();
    vhFinish();
}

static void backend_test_execute_present()
{
    vrhi_present_texture_to_surface( g_test_target.texture, g_test_presented_target );
    vhFinish();
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
    constexpr int test_width = 1980;
    constexpr int test_height = 1080;
    vrhi_initialise_headless( test_width, test_height );
    int total_passed = 0;
    int total_failed = 0;
    int total_skipped = 0;

    const auto run_mode = [&]( const char* mode_name, bool use_freetype ) {
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
        if ( !use_freetype ) {
            apply_font_fallbacks();
        }
        ve_font_id huge_test_font = load_demo_font( &cache, "fonts/NotoSansJP-Light.otf", huge_buffer, 200.0f );
        if ( huge_test_font < 0 && !use_freetype ) {
            huge_test_font = load_demo_font( &cache, "fonts/OpenSans-Regular.ttf", huge_buffer, 200.0f );
        }
        normalize_demo_font_ids( &huge_test_font );

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
            ve_font_id refreshed_huge_font = load_demo_font( &cache, "fonts/NotoSansJP-Light.otf", huge_buffer, 200.0f );
            if ( refreshed_huge_font < 0 && !use_freetype ) {
                refreshed_huge_font = load_demo_font( &cache, "fonts/OpenSans-Regular.ttf", huge_buffer, 200.0f );
            }
            normalize_demo_font_ids( &refreshed_huge_font );
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
    vrhi_shutdown_runtime();
    return ok ? 0 : 1;
}

int main( int argc, char** argv )
{
#ifndef VE_FONTCACHE_DEBUGPRINT
    const int numGlyphs = 1024;
    {
        auto start = std::chrono::high_resolution_clock::now();
        for ( int i = 0; i < numGlyphs; i++ ) {
            // Benchmark placeholder kept aligned with the OpenGL and DX11 demos.
        }
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration< double, std::milli > elapsed = finish - start;
        std::printf( "ve_fontcache_cache_glyph() benchmark: total %lf ms for %d glyphs, per-glyph %lf ms\n", elapsed.count(), numGlyphs, elapsed.count() / numGlyphs );
    }
#endif // VE_FONTCACHE_DEBUGPRINT

    if ( has_flag( argc, argv, "--test" ) ) {
        // Keep backend tests fully headless by exiting before GLFW creates a window.
        return run_backend_test_mode();
    }

    if ( !glfwInit() ) {
        std::printf( "Failed to initialise GLFW.\n" );
        return 1;
    }

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

    constexpr int default_width = 1980;
    constexpr int default_height = 1080;
    g_window = glfwCreateWindow( default_width, default_height, "VEFontCache VRHI", nullptr, nullptr );
    if ( !g_window ) {
        std::printf( "Failed to create a GLFW window.\n" );
        glfwTerminate();
        return 1;
    }

    glfwSetFramebufferSizeCallback( g_window, framebuffer_size_callback );
    glfwSetScrollCallback( g_window, scroll_callback );
    glfwSetCursorPosCallback( g_window, cursor_pos_callback );
    glfwSetMouseButtonCallback( g_window, mouse_button_callback );

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize( g_window, &framebuffer_width, &framebuffer_height );
    if ( framebuffer_width <= 0 || framebuffer_height <= 0 ) {
        framebuffer_width = default_width;
        framebuffer_height = default_height;
    }
    window_size.width = static_cast< unsigned int >( framebuffer_width );
    window_size.height = static_cast< unsigned int >( framebuffer_height );

    vrhi_initialise_windowed( g_window, framebuffer_width, framebuffer_height );
    init_demo();

    while ( !glfwWindowShouldClose( g_window ) ) {
        glfwPollEvents();
        if ( window_size.width == 0 || window_size.height == 0 ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
            continue;
        }

        const glm::vec4 clear_colour(
            0.18f * 0.18f,
            0.204f * 0.204f,
            0.251f * 0.251f,
            1.0f );
        if ( !vrhi_clear_current_target( clear_colour ) ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            continue;
        }

        render_demo( 1.0f / 60.0f );
        fontcache_drawcmd();
        if ( !vrhi_present_current_target() ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            continue;
        }
        ( void ) vhFrame();
    }

    ve_fontcache_shutdown( &cache );
    vrhi_shutdown_runtime();
    glfwDestroyWindow( g_window );
    g_window = nullptr;
    glfwTerminate();
    return 0;
}
