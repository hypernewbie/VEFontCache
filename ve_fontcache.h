/*
	-- Vertex Engine GPU Font Cache --

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

/*
---------------------------------- How to plug into rendering API ----------------------------------------

1. Create these simple shaders ( GLSL example provided ):

	// vs_source_shared
	#version 330 core
	in vec2 vpos;
	in vec2 vtex;
	out vec2 uv;
	void main( void ) {
		uv = vtex;
		gl_Position = vec4( vpos.xy, 0.0, 1.0 );
	}		

	// fs_source_render_glyph
	#version 330 core
	out vec4 fragc;
	void main( void ) {
		fragc = vec4( 1.0, 1.0, 1.0, 1.0 );
	}

	// fs_source_blit_atlas
	#version 330 core
	in vec2 uv;
	out vec4 fragc;
	uniform uint region;
	uniform sampler2D src_texture;
	float downsample( vec2 uv, vec2 texsz )
	{
		float v =
			texture( src_texture, uv + vec2( 0.0f, 0.0f ) * texsz ).x * 0.25f +
			texture( src_texture, uv + vec2( 0.0f, 1.0f ) * texsz ).x * 0.25f +
			texture( src_texture, uv + vec2( 1.0f, 0.0f ) * texsz ).x * 0.25f +
			texture( src_texture, uv + vec2( 1.0f, 1.0f ) * texsz ).x * 0.25f;
		return v;
	}
	void main( void ) {
		const vec2 texsz = 1.0f / vec2( 2048 , 512 ); // VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH/HEIGHT
		if ( region == 0u || region == 1u || region == 2u ) {
			float v =
				downsample( uv + vec2( -1.5f, -1.5f ) * texsz, texsz ) * 0.25f +
				downsample( uv + vec2(  0.5f, -1.5f ) * texsz, texsz ) * 0.25f +
				downsample( uv + vec2( -1.5f,  0.5f ) * texsz, texsz ) * 0.25f +
				downsample( uv + vec2(  0.5f,  0.5f ) * texsz, texsz ) * 0.25f;
			fragc = vec4( 1, 1, 1, v );
		} else {
			fragc = vec4( 0, 0, 0, 1 );
		}
	}

	// vs_source_draw_text
	#version 330 core
	in vec2 vpos;
	in vec2 vtex;
	out vec2 uv;
	void main( void ) {
		uv = vtex;
		gl_Position = vec4( vpos.xy * 2.0f - 1.0f, 0.0, 1.0 );
	}

	// fs_source_draw_text
	#version 330 core
	in vec2 uv;
	out vec4 fragc;
	uniform sampler2D src_texture;
	uniform uint downsample;
	uniform vec4 colour;
	void main( void ) {
	float v = texture( src_texture, uv ).x;
	if ( downsample == 1u ) {
		const vec2 texsz = 1.0f / vec2( 2048 , 512 ); // VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH/HEIGHT
		v =
			texture( src_texture, uv + vec2(-0.5f,-0.5f ) * texsz ).x * 0.25f +
			texture( src_texture, uv + vec2(-0.5f, 0.5f ) * texsz ).x * 0.25f +
			texture( src_texture, uv + vec2( 0.5f,-0.5f ) * texsz ).x * 0.25f +
			texture( src_texture, uv + vec2( 0.5f, 0.5f ) * texsz ).x * 0.25f;
		}
		fragc = vec4( colour.xyz, colour.a * v );
	}

	fontcache_shader_render_glyph = compile_shader( vs_source_shared, fs_source_render_glyph );
	fontcache_shader_blit_atlas = compile_shader( vs_source_shared, fs_source_blit_atlas );
	fontcache_shader_draw_text = compile_shader( vs_source_draw_text, fs_source_draw_text );

2. Set up these 2 render target textures ( OpenGL example ):

	// First render target is 2k x 512 single-channel 8-byte red.
	glBindFramebuffer( GL_FRAMEBUFFER, fontcache_fbo[ 0 ] );
	glBindTexture( GL_TEXTURE_2D, fontcache_fbo_texture[0] );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fontcache_fbo_texture[ 0 ], 0 );

	// Second render target is 4k x 2k single-channel 8-byte red.
	glBindFramebuffer( GL_FRAMEBUFFER, fontcache_fbo[ 1 ] );
	glBindTexture( GL_TEXTURE_2D, fontcache_fbo_texture[ 1 ] );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, VE_FONTCACHE_ATLAS_WIDTH, VE_FONTCACHE_ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fontcache_fbo_texture[ 1 ], 0 );
	
3. Implement drawlist execute method ( OpenGL example ):
	
	glDisable( GL_CULL_FACE );
	glEnable( GL_BLEND );
	glBlendEquation( GL_FUNC_ADD );
	
	for ( auto& dcall : drawlist->dcalls ) {
				if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ) {
			glUseProgram( fontcache_shader_render_glyph );
			glBindFramebuffer( GL_FRAMEBUFFER, fontcache_fbo[ 0 ] );
			glBlendFunc( GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR );
			glViewport( 0, 0, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
			glScissor( 0, 0, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
			glDisable( GL_FRAMEBUFFER_SRGB );
		} else if ( dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS ) {
			glUseProgram( fontcache_shader_blit_atlas );
			glBindFramebuffer( GL_FRAMEBUFFER, fontcache_fbo[ 1 ] );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glViewport( 0, 0, VE_FONTCACHE_ATLAS_WIDTH, VE_FONTCACHE_ATLAS_HEIGHT );
			glScissor( 0, 0, VE_FONTCACHE_ATLAS_WIDTH, VE_FONTCACHE_ATLAS_HEIGHT );
			glUniform1i( glGetUniformLocation( fontcache_shader_blit_atlas, "src_texture" ), 0 );
			glUniform1ui( glGetUniformLocation( fontcache_shader_blit_atlas, "region" ), dcall.region );
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, fontcache_fbo_texture[0] );
			glDisable( GL_FRAMEBUFFER_SRGB );
		} else {
			glUseProgram( fontcache_shader_draw_text );
			glBindFramebuffer( GL_FRAMEBUFFER, 0 );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glViewport( 0, 0, window_size.width, window_size.height );
			glScissor( 0, 0, window_size.width, window_size.height );
			glUniform1i( glGetUniformLocation( fontcache_shader_draw_text, "src_texture" ), 0 );
			glUniform1ui( glGetUniformLocation( fontcache_shader_draw_text, "downsample" ), dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ? 1 : 0 );
			glUniform4fv( glGetUniformLocation( fontcache_shader_draw_text, "colour" ), 1, dcall.colour );
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, dcall.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ? fontcache_fbo_texture[0] : fontcache_fbo_texture[1] );
			glEnable( GL_FRAMEBUFFER_SRGB );
		}
		if ( dcall.clear_before_draw ) {
			glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
			glClear( GL_COLOR_BUFFER_BIT );
		}
		if ( dcall.end_index - dcall.start_index == 0 )
			continue;
		glDrawElements( GL_TRIANGLES, dcall.end_index - dcall.start_index, GL_UNSIGNED_INT, ( GLvoid* ) ( dcall.start_index * sizeof( uint32_t ) ) );
	}
*/

#include <cstdio>
#include <vector>
#include <memory>
#include <unordered_map>
#include <deque>

#ifndef VE_STBTT_NOIMPL
	#define STB_TRUETYPE_IMPLEMENTATION
#endif // VE_STBTT_NOIMPL

#ifndef VE_STBTT_INCLUDED
	#define VE_STBTT_INCLUDED
	#include "stb_truetype.h"
#endif // VE_STBTT_INCLUDED

#ifndef VE_FONTCACHE_CURVE_QUALITY
	#define VE_FONTCACHE_CURVE_QUALITY 6
#endif // VE_FONTCACHE_CURVE_QUALITY

#ifndef VE_FONTCACHE_HARFBUZZ
	#pragma message( "WARNING: Please include Harfbuzz and define VE_FONTCACHE_HARFBUZZ for production. Using default fallback dumbass non-portable unoptimised text shaper." )
	#include "utf8.h"
#endif // VE_FONTCACHE_HARFBUZZ

/*
---------------------------------- Font Atlas Caching Strategy --------------------------------------------

					    2k
					    --------------------
					    |         |        |
					    |    A    |        |
					    |         |        | 2
					    |---------|    C   | k  
					    |         |        |
					 1k |    B    |        |
					    |         |        |
					    --------------------
						|                  |
						|                  |
						|                  | 2
						|        D         | k  
						|                  |
						|                  |
						|                  |
						--------------------

						Region A = 32x32 caches, 1024 glyphs
						Region B = 32x64 caches, 512 glyphs
						Region C = 64x64 caches, 512 glyphs
						Region D = 128x128 caches, 256 glyphs
*/

#define VE_FONTCACHE_ATLAS_WIDTH 4096
#define VE_FONTCACHE_ATLAS_HEIGHT 2048
#define VE_FONTCACHE_ATLAS_GLYPH_PADDING 1

#define VE_FONTCACHE_ATLAS_REGION_A_WIDTH 32
#define VE_FONTCACHE_ATLAS_REGION_A_HEIGHT 32
#define VE_FONTCACHE_ATLAS_REGION_A_XSIZE ( VE_FONTCACHE_ATLAS_WIDTH / 4 )
#define VE_FONTCACHE_ATLAS_REGION_A_YSIZE ( VE_FONTCACHE_ATLAS_HEIGHT / 2 )
#define VE_FONTCACHE_ATLAS_REGION_A_XCAPACITY ( VE_FONTCACHE_ATLAS_REGION_A_XSIZE / VE_FONTCACHE_ATLAS_REGION_A_WIDTH )
#define VE_FONTCACHE_ATLAS_REGION_A_YCAPACITY ( VE_FONTCACHE_ATLAS_REGION_A_YSIZE / VE_FONTCACHE_ATLAS_REGION_A_HEIGHT )
#define VE_FONTCACHE_ATLAS_REGION_A_CAPACITY ( VE_FONTCACHE_ATLAS_REGION_A_XCAPACITY * VE_FONTCACHE_ATLAS_REGION_A_YCAPACITY )
#define VE_FONTCACHE_ATLAS_REGION_A_XOFFSET 0
#define VE_FONTCACHE_ATLAS_REGION_A_YOFFSET 0

#define VE_FONTCACHE_ATLAS_REGION_B_WIDTH 32
#define VE_FONTCACHE_ATLAS_REGION_B_HEIGHT 64
#define VE_FONTCACHE_ATLAS_REGION_B_XSIZE ( VE_FONTCACHE_ATLAS_WIDTH / 4 )
#define VE_FONTCACHE_ATLAS_REGION_B_YSIZE ( VE_FONTCACHE_ATLAS_HEIGHT / 2 )
#define VE_FONTCACHE_ATLAS_REGION_B_XCAPACITY ( VE_FONTCACHE_ATLAS_REGION_B_XSIZE / VE_FONTCACHE_ATLAS_REGION_B_WIDTH )
#define VE_FONTCACHE_ATLAS_REGION_B_YCAPACITY ( VE_FONTCACHE_ATLAS_REGION_B_YSIZE / VE_FONTCACHE_ATLAS_REGION_B_HEIGHT )
#define VE_FONTCACHE_ATLAS_REGION_B_CAPACITY ( VE_FONTCACHE_ATLAS_REGION_B_XCAPACITY * VE_FONTCACHE_ATLAS_REGION_B_YCAPACITY )
#define VE_FONTCACHE_ATLAS_REGION_B_XOFFSET 0
#define VE_FONTCACHE_ATLAS_REGION_B_YOFFSET VE_FONTCACHE_ATLAS_REGION_A_YSIZE

#define VE_FONTCACHE_ATLAS_REGION_C_WIDTH 64
#define VE_FONTCACHE_ATLAS_REGION_C_HEIGHT 64
#define VE_FONTCACHE_ATLAS_REGION_C_XSIZE ( VE_FONTCACHE_ATLAS_WIDTH / 4 )
#define VE_FONTCACHE_ATLAS_REGION_C_YSIZE ( VE_FONTCACHE_ATLAS_HEIGHT )
#define VE_FONTCACHE_ATLAS_REGION_C_XCAPACITY ( VE_FONTCACHE_ATLAS_REGION_C_XSIZE / VE_FONTCACHE_ATLAS_REGION_C_WIDTH )
#define VE_FONTCACHE_ATLAS_REGION_C_YCAPACITY ( VE_FONTCACHE_ATLAS_REGION_C_YSIZE / VE_FONTCACHE_ATLAS_REGION_C_HEIGHT )
#define VE_FONTCACHE_ATLAS_REGION_C_CAPACITY ( VE_FONTCACHE_ATLAS_REGION_C_XCAPACITY * VE_FONTCACHE_ATLAS_REGION_C_YCAPACITY )
#define VE_FONTCACHE_ATLAS_REGION_C_XOFFSET VE_FONTCACHE_ATLAS_REGION_A_XSIZE
#define VE_FONTCACHE_ATLAS_REGION_C_YOFFSET 0

#define VE_FONTCACHE_ATLAS_REGION_D_WIDTH 128
#define VE_FONTCACHE_ATLAS_REGION_D_HEIGHT 128
#define VE_FONTCACHE_ATLAS_REGION_D_XSIZE ( VE_FONTCACHE_ATLAS_WIDTH / 2 )
#define VE_FONTCACHE_ATLAS_REGION_D_YSIZE ( VE_FONTCACHE_ATLAS_HEIGHT )
#define VE_FONTCACHE_ATLAS_REGION_D_XCAPACITY ( VE_FONTCACHE_ATLAS_REGION_D_XSIZE / VE_FONTCACHE_ATLAS_REGION_D_WIDTH )
#define VE_FONTCACHE_ATLAS_REGION_D_YCAPACITY ( VE_FONTCACHE_ATLAS_REGION_D_YSIZE / VE_FONTCACHE_ATLAS_REGION_D_HEIGHT )
#define VE_FONTCACHE_ATLAS_REGION_D_CAPACITY ( VE_FONTCACHE_ATLAS_REGION_D_XCAPACITY * VE_FONTCACHE_ATLAS_REGION_D_YCAPACITY )
#define VE_FONTCACHE_ATLAS_REGION_D_XOFFSET ( VE_FONTCACHE_ATLAS_WIDTH / 2 )
#define VE_FONTCACHE_ATLAS_REGION_D_YOFFSET 0

static_assert( VE_FONTCACHE_ATLAS_REGION_A_CAPACITY == 1024, "VE FontCache Atlas Sanity check fail. Please update this assert if you changed atlas packing strategy." );
static_assert( VE_FONTCACHE_ATLAS_REGION_B_CAPACITY == 512, "VE FontCache Atlas Sanity check fail. Please update this assert if you changed atlas packing strategy." );
static_assert( VE_FONTCACHE_ATLAS_REGION_C_CAPACITY == 512, "VE FontCache Atlas Sanity check fail. Please update this assert if you changed atlas packing strategy." );
static_assert( VE_FONTCACHE_ATLAS_REGION_D_CAPACITY == 256, "VE FontCache Atlas Sanity check fail. Please update this assert if you changed atlas packing strategy." );

#define VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X 4
#define VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y 4
#define VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH 4
#define VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH ( VE_FONTCACHE_ATLAS_REGION_D_WIDTH * VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X * VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH )
#define VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ( VE_FONTCACHE_ATLAS_REGION_D_HEIGHT * VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y )

// Set to same value as VE_FONTCACHE_ATLAS_GLYPH_PADDING for best results!
#define VE_FONTCACHE_GLYPHDRAW_PADDING VE_FONTCACHE_ATLAS_GLYPH_PADDING 

#define VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH 1
#define VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS 2
#define VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET 3
#define VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED 4

// How many to store in text shaping cache. Shaping cache is also stored in LRU format.
#define VE_FONTCACHE_SHAPECACHE_SIZE 256

// How much to reserve for each shape cache. This adds up to a ~0.768mb cache.
#define VE_FONTCACHE_SHAPECACHE_RESERVE_LENGTH 64

// Max. text size for caching. This means the cache has ~3.072mb upper bound.
#define VE_FONTCACHE_SHAPECACHE_MAX_LENGTH 256

// --------------------------------------------------------------- Data Types ---------------------------------------------------

typedef int64_t ve_font_id;
typedef int32_t ve_codepoint;
typedef char ve_atlas_region;

struct ve_fontcache_entry
{
	ve_font_id font_id = 0;
	bool used = false;
	stbtt_fontinfo info;
	float size = 24.0f;
	float size_scale = 1.0f;
};

struct ve_fontcache_vertex
{
	float x; float y;
	float u; float v;
};

struct ve_fontcache_vec2
{
	float x; float y;
};
inline ve_fontcache_vec2 ve_fontcache_make_vec2( float x_, float y_ ) { ve_fontcache_vec2 v; v.x = x_; v.y = y_; return v; }

struct ve_fontcache_draw
{
	uint32_t pass = 0; // One of VE_FONTCACHE_FRAMEBUFFER_PASS_* values.
	uint32_t start_index = 0;
	uint32_t end_index = 0;
	bool clear_before_draw = false;
	uint32_t region = 0;
	float colour[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct ve_fontcache_drawlist
{
	std::vector< ve_fontcache_vertex > vertices;
	std::vector< uint32_t > indices;
	std::vector< ve_fontcache_draw > dcalls;
};

typedef uint32_t ve_fontcache_poollist_itr;
typedef uint64_t ve_fontcache_poollist_value;

struct ve_fontcache_poollist_item
{
	ve_fontcache_poollist_itr prev = -1;
	ve_fontcache_poollist_itr next = -1;
	ve_fontcache_poollist_value value = 0;
};

struct ve_fontcache_poollist
{
	std::vector< ve_fontcache_poollist_item > pool;
	std::vector< ve_fontcache_poollist_itr > freelist;
	ve_fontcache_poollist_itr front = -1; 
	ve_fontcache_poollist_itr back = -1;
	size_t size = 0;
	size_t capacity = 0;
};

struct ve_fontcache_LRU_link
{
	int value = 0;
	ve_fontcache_poollist_itr ptr;
};

struct ve_fontcache_LRU
{
	int capacity = 0;
	std::unordered_map< uint64_t, ve_fontcache_LRU_link > cache;
	ve_fontcache_poollist key_queue;
};

struct ve_fontcache_atlas
{
	uint32_t next_atlas_idx_A = 0;
	uint32_t next_atlas_idx_B = 0;
	uint32_t next_atlas_idx_C = 0;
	uint32_t next_atlas_idx_D = 0;

	ve_fontcache_LRU stateA;
	ve_fontcache_LRU stateB;
	ve_fontcache_LRU stateC;
	ve_fontcache_LRU stateD;

	uint32_t glyph_update_batch_x = 0;
	ve_fontcache_drawlist glyph_update_batch_clear_drawlist;
	ve_fontcache_drawlist glyph_update_batch_drawlist;
};

struct ve_fontcache_shaped_text
{
	std::vector< ve_codepoint > codepoints;
	std::vector< ve_fontcache_vec2 > pos;
};

struct ve_fontcache_shaped_text_cache
{
	std::vector< ve_fontcache_shaped_text > storage;
	ve_fontcache_LRU state;
	uint32_t next_cache_idx = 0;
};

struct ve_fontcache
{
	std::vector< ve_fontcache_entry > entry;
	std::vector< ve_fontcache_vec2 > temp_path;
	std::unordered_map< uint64_t, bool > temp_codepoint_seen;
	uint32_t snap_width = 0;
	uint32_t snap_height = 0;
	float colour[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	ve_fontcache_drawlist drawlist;
	ve_fontcache_atlas atlas;
	ve_fontcache_shaped_text_cache shape_cache;
};

// --------------------------------------------------------------------- VEFontCache Function Declarations -------------------------------------------------------------

// Call this to initialise a fontcache.
void ve_fontcache_init( ve_fontcache* cache );

// Call this to shutdown everything.
void ve_fontcache_shutdown( ve_fontcache* cache );

// Load font from in-memory buffer. Supports otf, ttf, everything STB_truetype supports.
//     Caller still owns data and must hold onto it; This buffer cannot be freed while font is still being used.
//     SBTTT will keep track of weak pointers into this memory.
//     If you're loading the same font at different size_px values, it is OK to share the same data buffer amongst them.
//
ve_font_id ve_fontcache_load( ve_fontcache* cache, const void* data, float size_px = 24.0f );

// Load font from file. Supports otf, ttf, everything STB_truetype supports.
//     Caller still owns given buffer and must hold onto it. This buffer cannot be freed while font is still being used.
//     SBTTT will keep track of weak pointers into this memory.
//     If you're loading the same font at different size_px values, it is OK to share the same buffer amongst them.
//
ve_font_id ve_fontcache_loadfile( ve_fontcache* cache, const char* filename, std::vector< uint8_t >& buffer, float size_px = 24.0f );

// Configure snapping glyphs to pixel border when font is rendered to 2D screen. May affect kerning. This may be changed at any time.
// Set both to zero to disable pixel snapping.
void ve_fontcache_configure_snap( ve_fontcache* cache, uint32_t snap_width = 0, uint32_t snap_height = 0 );

// Call this per-frame after draw list has been executed. This will clear the drawlist for next frame.
void ve_fontcache_flush_drawlist( ve_fontcache* cache );

// Main draw text function. This batches caches both shape and glyphs, and uses fallback path when not available.
//     Note that this function immediately appends everything needed to render the next to the cache->drawlist.
//     If you want to draw to multiple unrelated targets, simply draw_text, then loop through execute draw list, draw_text again, and loop through execute draw list again.
//     Suggest scalex = 1 / screen_width and scaley = 1 / screen height! scalex and scaley will need to account for aspect ratio.
//
bool ve_fontcache_draw_text( ve_fontcache* cache, ve_font_id font, const std::string& text_utf8, float posx = 0.0f, float posy = 0.0f, float scalex = 1.0f, float scaley = 1.0f );

// Merges drawcalls. Significantly improves drawcall overhead, highly recommended. Call this before looping through and executing drawlist.
void ve_fontcache_optimise_drawlist( ve_fontcache* cache );

// Retrieves current drawlist from cache.
ve_fontcache_drawlist* ve_fontcache_get_drawlist( ve_fontcache* cache );

// Set text colour of subsequent text draws.
void ve_fontcache_set_colour( ve_fontcache* cache, float c[4] );

// --------------------------------------------------------------------- Generic Data Structure Declarations -------------------------------------------------------------


// Generic pool list for alloc-free LRU implementation.
void ve_fontcache_poollist_init( ve_fontcache_poollist& plist, int capacity );
void ve_fontcache_poollist_push_front( ve_fontcache_poollist& plist, ve_fontcache_poollist_value v );
void ve_fontcache_poollist_erase( ve_fontcache_poollist& plist, ve_fontcache_poollist_itr it );
ve_fontcache_poollist_value ve_fontcache_poollist_peek_back( ve_fontcache_poollist& plist );
ve_fontcache_poollist_value ve_fontcache_poollist_pop_back( ve_fontcache_poollist& plist );

// Generic LRU ( Least-Recently-Used ) cache implementation, reused for both atlas and shape cache.
void ve_fontcache_LRU_init( ve_fontcache_LRU& LRU, int capacity );
int ve_fontcache_LRU_get( ve_fontcache_LRU& LRU, uint64_t key );
int ve_fontcache_LRU_peek( ve_fontcache_LRU& LRU, uint64_t key );
uint64_t ve_fontcache_LRU_put( ve_fontcache_LRU& LRU, uint64_t key, int val );
void ve_fontcache_LRU_refresh( ve_fontcache_LRU& LRU, uint64_t key );
uint64_t ve_fontcache_LRU_get_next_evicted( ve_fontcache_LRU& LRU );

// --------------------------------------------------------------------- VEFontCache Function Implementations -------------------------------------------------------------

#ifdef VE_FONTCACHE_IMPL

void ve_fontcache_init( ve_fontcache* cache )
{
	STBTT_assert( cache );

	// Reserve global context data.
	cache->entry.reserve( 8 );
	cache->temp_path.reserve( 256 );
	cache->temp_codepoint_seen.reserve( 256 );
	cache->drawlist.vertices.reserve( 4096 );
	cache->drawlist.indices.reserve( 8192 );
	cache->drawlist.dcalls.reserve( 512 );

	// Reserve data atlas LRU regions.
	cache->atlas.next_atlas_idx_A = 0;
	cache->atlas.next_atlas_idx_B = 0;
	cache->atlas.next_atlas_idx_C = 0;
	cache->atlas.next_atlas_idx_D = 0;
	ve_fontcache_LRU_init( cache->atlas.stateA, VE_FONTCACHE_ATLAS_REGION_A_CAPACITY );
	ve_fontcache_LRU_init( cache->atlas.stateB, VE_FONTCACHE_ATLAS_REGION_B_CAPACITY );
	ve_fontcache_LRU_init( cache->atlas.stateC, VE_FONTCACHE_ATLAS_REGION_C_CAPACITY );
	ve_fontcache_LRU_init( cache->atlas.stateD, VE_FONTCACHE_ATLAS_REGION_D_CAPACITY );

	// Reserve data for shape cache. This is pretty big!
	ve_fontcache_LRU_init( cache->shape_cache.state, VE_FONTCACHE_SHAPECACHE_SIZE );
	cache->shape_cache.storage.resize( VE_FONTCACHE_SHAPECACHE_SIZE );
	for ( int i = 0; i < VE_FONTCACHE_SHAPECACHE_SIZE; i++ ) {
		cache->shape_cache.storage[ i ].codepoints.reserve( VE_FONTCACHE_SHAPECACHE_RESERVE_LENGTH );
		cache->shape_cache.storage[ i ].pos.reserve( VE_FONTCACHE_SHAPECACHE_RESERVE_LENGTH );
	}

	// We can actually go over VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH batches due to smart packing!
	cache->atlas.glyph_update_batch_drawlist.dcalls.reserve( VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH * 2 );
	cache->atlas.glyph_update_batch_drawlist.vertices.reserve( VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH * 2 * 4 );
	cache->atlas.glyph_update_batch_drawlist.indices.reserve( VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH * 2 * 6 );
	cache->atlas.glyph_update_batch_clear_drawlist.dcalls.reserve( VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH * 2 );
	cache->atlas.glyph_update_batch_clear_drawlist.vertices.reserve( VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH * 2 * 4 );
	cache->atlas.glyph_update_batch_clear_drawlist.indices.reserve( VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH * 2 * 6 );
}

void ve_fontcache_shutdown( ve_fontcache* cache )
{
	STBTT_assert( cache );
}

ve_font_id ve_fontcache_load( ve_fontcache* cache, const void* data, float size_px )
{
	STBTT_assert( cache );
	if ( !data  ) return -1;

	// Allocate cache entry.
	int id = -1;
	for( int i = 0; i < cache->entry.size(); i++ ) {
		if ( !cache->entry[i].used ) {
			id = i;
			break;
		}
	}
	if ( id == -1 ) {
		cache->entry.push_back( ve_fontcache_entry() );
		id = cache->entry.size() - 1;
	}
	STBTT_assert( id >= 0 && id < cache->entry.size() );

	// Load font from memory.
	auto& et = cache->entry[id];
	int success = stbtt_InitFont( &et.info, ( const unsigned char* ) data, 0 );
	if ( !success ) {
		return -1;
	}
	et.font_id = id;
	et.size = size_px;
	et.size_scale = size_px < 0.0f ? stbtt_ScaleForPixelHeight( &et.info, -size_px ) : stbtt_ScaleForMappingEmToPixels( &et.info, size_px );
	et.used = true;

	return id;
}

ve_font_id ve_fontcache_loadfile( ve_fontcache* cache, const char* filename, std::vector< uint8_t >& buffer, float size_px )
{
	FILE *fp = fopen( filename, "rb" );
	if ( !fp ) return -1;

	fseek( fp, 0, SEEK_END );
	size_t sz = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	buffer.resize( sz );

	if ( fread( buffer.data(), 1, sz, fp ) != sz ) {
		buffer.clear();
		fclose( fp );
		return -1;
	}
	fclose( fp );

	ve_font_id ret = ve_fontcache_load( cache, buffer.data(), size_px );
	return ret;
}

void ve_fontcache_configure_snap( ve_fontcache* cache, uint32_t snap_width, uint32_t snap_height )
{
	STBTT_assert( cache );
	cache->snap_width = snap_width;
	cache->snap_height = snap_height;
}

ve_fontcache_drawlist* ve_fontcache_get_drawlist( ve_fontcache* cache ) 
{
	STBTT_assert( cache );
	return &cache->drawlist;
}

void ve_fontcache_clear_drawlist( ve_fontcache_drawlist& drawlist ) 
{
	drawlist.dcalls.clear();
	drawlist.indices.clear();
	drawlist.vertices.clear();
}

void ve_fontcache_merge_drawlist( ve_fontcache_drawlist& dest, const ve_fontcache_drawlist& src ) 
{
	int voffset = dest.vertices.size();
	for ( int i = 0; i < src.vertices.size(); i++ ) {
		dest.vertices.push_back( src.vertices[i] );
	}
	int ioffset = dest.indices.size();
	for ( int i = 0; i < src.indices.size(); i++ ) {
		dest.indices.push_back( src.indices[i] + voffset );
	}
	for ( int i = 0; i < src.dcalls.size(); i++ ) {
		ve_fontcache_draw dcall = src.dcalls[i];
		dcall.start_index += ioffset;
		dcall.end_index += ioffset;
		dest.dcalls.push_back( dcall );
	}
}

void ve_fontcache_flush_drawlist( ve_fontcache* cache ) 
{
	STBTT_assert( cache );
	ve_fontcache_clear_drawlist( cache->drawlist );
}


inline ve_fontcache_vec2 ve_fontcache_eval_bezier( ve_fontcache_vec2 p0, ve_fontcache_vec2 p1, ve_fontcache_vec2 p2, float t )
{
	float t2 = t * t, c0 = ( 1.0f - t ) * ( 1.0f - t ), c1 = 2.0f * ( 1.0f - t ) * t, c2 = t2;
	return ve_fontcache_make_vec2(
		c0 * p0.x + c1 * p1.x + c2 * p2.x,
		c0 * p0.y + c1 * p1.y + c2 * p2.y
	);
}

inline ve_fontcache_vec2 ve_fontcache_eval_bezier( ve_fontcache_vec2 p0, ve_fontcache_vec2 p1, ve_fontcache_vec2 p2, ve_fontcache_vec2 p3, float t )
{
	float t2 = t * t, t3 = t2 * t;
	float c0 = ( 1.0f - t ) * ( 1.0f - t ) * ( 1.0f - t ), c1 = 3.0f * ( 1.0f - t ) * ( 1.0f - t ) * t, c2 = 3.0f * ( 1.0f - t ) * t2, c3 = t3;
	return ve_fontcache_make_vec2(
		c0 * p0.x + c1 * p1.x + c2 * p2.x + c3 * p3.x,
		c0 * p0.y + c1 * p1.y + c2 * p2.y + c3 * p3.y
	);
}

// WARNING: doesn't actually append drawcall; caller is responsible for actually appending the drawcall.
void ve_fontcache_draw_filled_path( ve_fontcache_drawlist& drawlist, ve_fontcache_vec2 outside, std::vector< ve_fontcache_vec2 >& path,
	float scaleX = 1.0f, float scaleY = 1.0f, float translateX = 0.0f, float translateY = 0.0f )
{
#ifdef VE_FONTCACHE_DEBUGPRINT_VERBOSE
	printf( "outline_path: \n" );
	for ( int i = 0; i < path.size(); i++ ) {
		printf( "    %.2f %.2f\n", path[i].x * scaleX + translateX, path[i].y * scaleY +  + translateY );
	}
#endif // VE_FONTCACHE_DEBUGPRINT_VERBOSE

	int voffset = drawlist.vertices.size();
	for ( int i = 0; i < path.size(); i++ ) {
		ve_fontcache_vertex vertex;
		vertex.x = path[i].x * scaleX + translateX; vertex.y = path[i].y * scaleY +  + translateY;
		vertex.u = 0.0f; vertex.v = 0.0f;
		drawlist.vertices.push_back( vertex );
	}
	int voutside = drawlist.vertices.size();
	{
		ve_fontcache_vertex vertex;
		vertex.x = outside.x * scaleX + translateX; vertex.y = outside.y * scaleY +  + translateY;
		vertex.u = 0.0f; vertex.v = 0.0f;
		drawlist.vertices.push_back( vertex );
	}
	for ( int i = 1; i < path.size(); i++ ) {
		drawlist.indices.push_back( voutside );
		drawlist.indices.push_back( voffset + i - 1 );
		drawlist.indices.push_back( voffset + i );
	}
}

// WARNING: doesn't actually append drawcall; caller is responsible for actually appending the drawcall.
void ve_fontcache_blit_quad( ve_fontcache_drawlist& drawlist, float x0 = 0.0f, float y0 = 0.0f, float x1 = 1.0f, float y1 = 1.0f, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f )
{
	int voffset = drawlist.vertices.size();
	
	ve_fontcache_vertex vertex;
	vertex.x = x0; vertex.y = y0; vertex.u = u0; vertex.v = v0; drawlist.vertices.push_back( vertex );
	vertex.x = x0; vertex.y = y1; vertex.u = u0; vertex.v = v1; drawlist.vertices.push_back( vertex );
	vertex.x = x1; vertex.y = y0; vertex.u = u1; vertex.v = v0; drawlist.vertices.push_back( vertex );
	vertex.x = x1; vertex.y = y1; vertex.u = u1; vertex.v = v1; drawlist.vertices.push_back( vertex );

	static uint32_t quad_indices[] = {
		0, 1, 2,
		2, 1, 3
	};
	for ( int i = 0; i < 6; i++ ) {
		drawlist.indices.push_back( voffset + quad_indices[i] );
	}
}

bool ve_fontcache_cache_glyph(
	ve_fontcache* cache, ve_font_id font, ve_codepoint unicode,
	float scaleX = 1.0f, float scaleY = 1.0f, float translateX = 0.0f, float translateY = 0.0f )
{
	STBTT_assert( cache );
	STBTT_assert( font >= 0 && font < cache->entry.size() );
	ve_fontcache_entry& entry = cache->entry[ font ];
	
	int glyph_index = stbtt_FindGlyphIndex( &entry.info, ( int ) unicode );
	if ( !glyph_index ) {
		// Glyph not in current font.
		return false;
	}

	// Retrieve the shape definition from STB TrueType.
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph_index ) )
		return true;
	stbtt_vertex* shape = nullptr;
	int nverts = stbtt_GetGlyphShape( &entry.info, glyph_index, &shape );
	if ( !nverts || !shape ) {
		return false;
	}

#ifdef VE_FONTCACHE_DEBUGPRINT_VERBOSE
	printf( "shape: \n" );
	for ( int i = 0; i < nverts; i++ ) {
		if ( shape[i].type == STBTT_vmove ) {
			printf("    move_to %d %d\n", shape[i].x, shape[i].y );
		} else if ( shape[i].type == STBTT_vline ) {
			printf("    line_to %d %d\n", shape[i].x, shape[i].y );
		} else if ( shape[i].type == STBTT_vcurve ) {
			printf("    curve_to %d %d through %d %d\n", shape[i].x, shape[i].y, shape[i].cx, shape[i].cy );
		} else if ( shape[i].type == STBTT_vcubic ) {
			printf("    cubic_to %d %d through %d %d and %d %d\n", shape[i].x, shape[i].y, shape[i].cx, shape[i].cy, shape[i].cx1, shape[i].cy1 );
		}
	}
#endif // VE_FONTCACHE_DEBUGPRINT_VERBOSE

	// We need a random point that is outside our shape. We simply pick something diagonally across from top-left bound corner.
	// Note that this outside point is scaled alongside the glyph in ve_fontcache_draw_filled_path, so we don't need to handle that here.
	int bounds_x0, bounds_x1, bounds_y0, bounds_y1;
	int success = stbtt_GetGlyphBox( &entry.info, glyph_index, &bounds_x0, &bounds_y0, &bounds_x1, &bounds_y1 );
	STBTT_assert( success );
	ve_fontcache_vec2 outside = ve_fontcache_make_vec2( bounds_x0 - 21, bounds_y0 - 33 );

	// Figure out scaling so it fits within our box.
	ve_fontcache_draw draw;
	draw.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH;
	draw.start_index = cache->drawlist.indices.size();

	// Draw the path using simplified version of https://medium.com/@evanwallace/easy-scalable-text-rendering-on-the-gpu-c3f4d782c5ac.
	// Instead of involving fragment shader code we simply make use of modern GPU ability to crunch triangles and brute force curve definitions.
	//
	std::vector< ve_fontcache_vec2 >& path = cache->temp_path;
	path.clear();
	for ( int i = 0; i < nverts; i++ ) {
		stbtt_vertex& edge = shape[i];
		switch ( edge.type ) {
			case STBTT_vmove:
				if ( path.size() > 0 ) {
					ve_fontcache_draw_filled_path( cache->drawlist, outside, path, scaleX, scaleY, translateX, translateY );
				}
				path.clear();
				// Fallthrough.
			case STBTT_vline: 
				path.push_back( ve_fontcache_make_vec2( shape[i].x, shape[i].y ) );
				break;
			case STBTT_vcurve: {
				STBTT_assert( path.size() > 0 );
				ve_fontcache_vec2 p0 = path[ path.size() - 1 ];
				ve_fontcache_vec2 p1 = ve_fontcache_make_vec2( shape[i].cx, shape[i].cy );
				ve_fontcache_vec2 p2 = ve_fontcache_make_vec2( shape[i].x, shape[i].y );

				float step = 1.0f / VE_FONTCACHE_CURVE_QUALITY, t = step;
				for ( int i = 0; i < VE_FONTCACHE_CURVE_QUALITY; i++ ) {
					path.push_back( ve_fontcache_eval_bezier( p0, p1, p2, t ) );
					t += step;
				}
				break;
			}
			case STBTT_vcubic: {
				STBTT_assert( path.size() > 0 );
				ve_fontcache_vec2 p0 = path[ path.size() - 1 ];
				ve_fontcache_vec2 p1 = ve_fontcache_make_vec2( shape[i].cx, shape[i].cy );
				ve_fontcache_vec2 p2 = ve_fontcache_make_vec2( shape[i].cx1, shape[i].cy1 );
				ve_fontcache_vec2 p3 = ve_fontcache_make_vec2( shape[i].x, shape[i].y );

				float step = 1.0f / VE_FONTCACHE_CURVE_QUALITY, t = step;
				for ( int i = 0; i < VE_FONTCACHE_CURVE_QUALITY; i++ ) {
					path.push_back( ve_fontcache_eval_bezier( p0, p1, p2, p3, t ) );
					t += step;
				}
				break;
			}
			default:
				STBTT_assert( !"Unknown shape edge type." );
		}
	}
	if ( path.size() > 0 ) {
		ve_fontcache_draw_filled_path( cache->drawlist, outside, path, scaleX, scaleY, translateX, translateY );
	}

	// Append the draw call.
	draw.end_index = cache->drawlist.indices.size();
	if ( draw.end_index > draw.start_index ) {
		cache->drawlist.dcalls.push_back( draw );
	}

	stbtt_FreeShape( &entry.info, shape );
	return true;
}

static ve_atlas_region ve_fontcache_decide_codepoint_region( ve_fontcache* cache, ve_fontcache_entry& entry, int glyph_index,
	ve_fontcache_LRU*& state, uint32_t*& next_idx, float& oversample_x, float& oversample_y )
{
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph_index ) )
		return '\0';

	// Get font text metrics. These are unscaled!
	int bounds_x0, bounds_x1, bounds_y0, bounds_y1;
	int success = stbtt_GetGlyphBox( &entry.info, glyph_index, &bounds_x0, &bounds_y0, &bounds_x1, &bounds_y1 );
	int bounds_width = bounds_x1 - bounds_x0, bounds_height = bounds_y1 - bounds_y0;
	STBTT_assert( success );

	// Decide which atlas to target. This logic should work well for reasonable on-screen text sizes of around 24px.
	// For 4k+ displays, caching font at a lower pt and drawing it upscaled at a higher pt is recommended.
	// 
	ve_atlas_region region;
	float bwidth_scaled = bounds_width * entry.size_scale + 2.0f * VE_FONTCACHE_ATLAS_GLYPH_PADDING, bheight_scaled = bounds_height * entry.size_scale + 2.0f * VE_FONTCACHE_ATLAS_GLYPH_PADDING;
	if ( bwidth_scaled <= VE_FONTCACHE_ATLAS_REGION_A_WIDTH && bheight_scaled <= VE_FONTCACHE_ATLAS_REGION_A_HEIGHT ) {
		// Region A for small glyphs. These are good for things such as punctuation.
		region = 'A';
		state = &cache->atlas.stateA;
		next_idx = &cache->atlas.next_atlas_idx_A;
	} else if ( bwidth_scaled <= VE_FONTCACHE_ATLAS_REGION_A_WIDTH && bheight_scaled > VE_FONTCACHE_ATLAS_REGION_A_HEIGHT ) {
		// Region B for tall glyphs. These are good for things such as european alphabets.
		region = 'B';
		state = &cache->atlas.stateB;
		next_idx = &cache->atlas.next_atlas_idx_B;
	} else if ( bwidth_scaled <= VE_FONTCACHE_ATLAS_REGION_C_WIDTH && bheight_scaled <= VE_FONTCACHE_ATLAS_REGION_C_HEIGHT ) {
		// Region C for big glyphs. These are good for things such as asian typography.
		region = 'C';
		state = &cache->atlas.stateC;
		next_idx = &cache->atlas.next_atlas_idx_C;
	} else if ( bwidth_scaled <= VE_FONTCACHE_ATLAS_REGION_D_WIDTH && bheight_scaled <= VE_FONTCACHE_ATLAS_REGION_D_HEIGHT ) {
		// Region D for huge glyphs. These are good for things such as titles and 4k.
		region = 'D';
		state = &cache->atlas.stateD;
		next_idx = &cache->atlas.next_atlas_idx_D;
	} else if ( bwidth_scaled <= VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH && bheight_scaled <= VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ) {
		// Region 'E' for massive glyphs. These are rendered uncached and un-oversampled.
		region = 'E';
		state = nullptr;
		next_idx = nullptr;
		if ( bwidth_scaled <= VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH / 2 && bheight_scaled <= VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT / 2 ) {
			oversample_x = 2.0f; oversample_y = 2.0f;
		} else {
			oversample_x = 1.0f; oversample_y = 1.0f;
		}
		return region;
	} else {
		return '\0';
	}

	STBTT_assert( state );
	STBTT_assert( next_idx );

	return region;
}

static void ve_fontcache_flush_glyph_buffer_to_atlas( ve_fontcache* cache )
{
	// Flush drawcalls to draw list.
	ve_fontcache_merge_drawlist( cache->drawlist, cache->atlas.glyph_update_batch_clear_drawlist );
	ve_fontcache_merge_drawlist( cache->drawlist, cache->atlas.glyph_update_batch_drawlist );
	ve_fontcache_clear_drawlist( cache->atlas.glyph_update_batch_clear_drawlist );
	ve_fontcache_clear_drawlist( cache->atlas.glyph_update_batch_drawlist );

	// Clear glyph_update_FBO.
	if ( cache->atlas.glyph_update_batch_x != 0 ) {
		ve_fontcache_draw dcall;
		dcall.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH;
		dcall.start_index = 0; dcall.end_index = 0;
		dcall.clear_before_draw = true;	cache->drawlist.dcalls.push_back( dcall );
		cache->atlas.glyph_update_batch_x = 0;
	}
}

static void ve_fontcache_screenspace_xform( float& x, float& y, float& scalex, float& scaley, float width, float height )
{
	scalex /= width; scaley /= height; scalex *= 2.0f; scaley *= 2.0f;
	x *= ( 2.0f / width ); y *= ( 2.0f / height ); x -= 1.0f; y -= 1.0f; 
}

static void ve_fontcache_texspace_xform( float& x, float& y, float& scalex, float& scaley, float width, float height )
{
	x /= width; y /= height; scalex /= width; scaley /= height;
}

static void ve_fontcache_atlas_bbox( ve_atlas_region region, int local_idx, float& x, float& y, float& width, float& height )
{
	if ( region == 'A' ) {
		width = VE_FONTCACHE_ATLAS_REGION_A_WIDTH;
		height = VE_FONTCACHE_ATLAS_REGION_A_HEIGHT;
		x = ( local_idx % VE_FONTCACHE_ATLAS_REGION_A_XCAPACITY ) * VE_FONTCACHE_ATLAS_REGION_A_WIDTH;
		y = ( local_idx / VE_FONTCACHE_ATLAS_REGION_A_XCAPACITY ) * VE_FONTCACHE_ATLAS_REGION_A_HEIGHT;
		x += VE_FONTCACHE_ATLAS_REGION_A_XOFFSET; y += VE_FONTCACHE_ATLAS_REGION_A_YOFFSET;
	} else if ( region == 'B' ) {
		width = VE_FONTCACHE_ATLAS_REGION_B_WIDTH;
		height = VE_FONTCACHE_ATLAS_REGION_B_HEIGHT;
		x = ( local_idx % VE_FONTCACHE_ATLAS_REGION_B_XCAPACITY ) * VE_FONTCACHE_ATLAS_REGION_B_WIDTH;
		y = ( local_idx / VE_FONTCACHE_ATLAS_REGION_B_XCAPACITY ) * VE_FONTCACHE_ATLAS_REGION_B_HEIGHT;
		x += VE_FONTCACHE_ATLAS_REGION_B_XOFFSET; y += VE_FONTCACHE_ATLAS_REGION_B_YOFFSET;
	} else if ( region == 'C' ) {
		width = VE_FONTCACHE_ATLAS_REGION_C_WIDTH;
		height = VE_FONTCACHE_ATLAS_REGION_C_HEIGHT;
		x = ( local_idx % VE_FONTCACHE_ATLAS_REGION_C_XCAPACITY ) * VE_FONTCACHE_ATLAS_REGION_C_WIDTH;
		y = ( local_idx / VE_FONTCACHE_ATLAS_REGION_C_XCAPACITY ) * VE_FONTCACHE_ATLAS_REGION_C_HEIGHT;
		x += VE_FONTCACHE_ATLAS_REGION_C_XOFFSET; y += VE_FONTCACHE_ATLAS_REGION_C_YOFFSET;
	} else {
		STBTT_assert( region == 'D' );
		width = VE_FONTCACHE_ATLAS_REGION_D_WIDTH;
		height = VE_FONTCACHE_ATLAS_REGION_D_HEIGHT;
		x = ( local_idx % VE_FONTCACHE_ATLAS_REGION_D_XCAPACITY ) * VE_FONTCACHE_ATLAS_REGION_D_WIDTH;
		y = ( local_idx / VE_FONTCACHE_ATLAS_REGION_D_XCAPACITY ) * VE_FONTCACHE_ATLAS_REGION_D_HEIGHT;
		x += VE_FONTCACHE_ATLAS_REGION_D_XOFFSET; y += VE_FONTCACHE_ATLAS_REGION_D_YOFFSET;
	}
}

void ve_fontcache_cache_codepoint_to_atlas( ve_fontcache* cache, ve_font_id font, ve_codepoint unicode )
{
	STBTT_assert( cache );
	STBTT_assert( font >= 0 && font < cache->entry.size() );
	ve_fontcache_entry& entry = cache->entry[ font ];

	int glyph_index = stbtt_FindGlyphIndex( &entry.info, ( int ) unicode );
	if ( !glyph_index ) {
		// Glyph not in current font.
		return;
	}
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph_index ) )
		return;

	// Get font text metrics. These are unscaled!
	int bounds_x0, bounds_x1, bounds_y0, bounds_y1;
	int success = stbtt_GetGlyphBox( &entry.info, glyph_index, &bounds_x0, &bounds_y0, &bounds_x1, &bounds_y1 );
	int bounds_width = bounds_x1 - bounds_x0, bounds_height = bounds_y1 - bounds_y0;
	STBTT_assert( success );

	// Decide which atlas to target.
	ve_fontcache_LRU* state = nullptr; uint32_t* next_idx = nullptr;
	float oversample_x = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X, oversample_y = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y;
	ve_atlas_region region = ve_fontcache_decide_codepoint_region( cache, entry, glyph_index, state, next_idx, oversample_x, oversample_y );

	// E region is special case and not cached to atlas.
	if ( region == '\0' || region == 'E' ) return;

	// Grab an atlas LRU cache slot.
	uint64_t lru_code = unicode +  ( ( 0x100000000ULL * font ) & 0xFFFFFFFF00000000ULL );
	int atlas_index = ve_fontcache_LRU_get( *state, lru_code );
	if ( atlas_index == -1 ) {
		if ( *next_idx < state->capacity ) {
			uint64_t evicted = ve_fontcache_LRU_put( *state, lru_code, *next_idx );
			atlas_index = *next_idx;
			( *next_idx )++;
			STBTT_assert( evicted == lru_code );
		} else {
			uint64_t next_evict_codepoint = ve_fontcache_LRU_get_next_evicted( *state );
			STBTT_assert( next_evict_codepoint != 0xFFFFFFFFFFFFFFFFULL );
			atlas_index = ve_fontcache_LRU_peek( *state, next_evict_codepoint );
			STBTT_assert( atlas_index != -1 );
			uint64_t evicted = ve_fontcache_LRU_put( *state, lru_code, atlas_index );
			STBTT_assert( evicted == next_evict_codepoint );
		}
		STBTT_assert( ve_fontcache_LRU_get( *state, lru_code ) != -1 );
	}

#ifdef VE_FONTCACHE_DEBUGPRINT
	static int debug_total_cached = 0;
	printf( "glyph 0x%x( %c ) caching to atlas region %c at idx %d. %d total glyphs cached.\n", unicode, (char) unicode, (char) region, atlas_index, debug_total_cached++ );
#endif // VE_FONTCACHE_DEBUGPRINT

	// Draw oversized glyph to update FBO.
	float glyph_draw_scale_x = entry.size_scale * oversample_x;
	float glyph_draw_scale_y = entry.size_scale * oversample_y;
	float glyph_draw_translate_x = -bounds_x0 * glyph_draw_scale_x + VE_FONTCACHE_GLYPHDRAW_PADDING;
	float glyph_draw_translate_y = -bounds_y0 * glyph_draw_scale_y + VE_FONTCACHE_GLYPHDRAW_PADDING;

	glyph_draw_translate_x = ( int ) ( glyph_draw_translate_x + 0.9999999f );
	glyph_draw_translate_y = ( int ) ( glyph_draw_translate_y + 0.9999999f );

	// Allocate a glyph_update_FBO region.
	int gdwidth_scaled_px = ( int )( bounds_width * glyph_draw_scale_x + 1.0f ) + 2 * oversample_x * VE_FONTCACHE_GLYPHDRAW_PADDING;
	if( cache->atlas.glyph_update_batch_x + gdwidth_scaled_px >= VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH ) {
		ve_fontcache_flush_glyph_buffer_to_atlas( cache );
	}

	// Calculate the src and destination regions.

	float destx, desty, destw, desth, srcx, srcy, srcw, srch;
	ve_fontcache_atlas_bbox( region, atlas_index, destx, desty, destw, desth );
	float dest_glyph_x = destx + VE_FONTCACHE_ATLAS_GLYPH_PADDING, dest_glyph_y = desty + VE_FONTCACHE_ATLAS_GLYPH_PADDING,
		dest_glyph_w = bounds_width * entry.size_scale, dest_glyph_h = bounds_height * entry.size_scale;
	dest_glyph_x -= VE_FONTCACHE_GLYPHDRAW_PADDING; dest_glyph_y -= VE_FONTCACHE_GLYPHDRAW_PADDING;
	dest_glyph_w += 2 * VE_FONTCACHE_GLYPHDRAW_PADDING; dest_glyph_h += 2 * VE_FONTCACHE_GLYPHDRAW_PADDING;
	ve_fontcache_screenspace_xform( dest_glyph_x, dest_glyph_y, dest_glyph_w, dest_glyph_h, VE_FONTCACHE_ATLAS_WIDTH, VE_FONTCACHE_ATLAS_HEIGHT );
	ve_fontcache_screenspace_xform( destx, desty, destw, desth, VE_FONTCACHE_ATLAS_WIDTH, VE_FONTCACHE_ATLAS_HEIGHT );

	srcx = cache->atlas.glyph_update_batch_x; srcy = 0.0;
	srcw = bounds_width * glyph_draw_scale_x; srch = bounds_height * glyph_draw_scale_y;
	srcw += 2 * oversample_x * VE_FONTCACHE_GLYPHDRAW_PADDING; srch += 2 * oversample_y * VE_FONTCACHE_GLYPHDRAW_PADDING;
	ve_fontcache_texspace_xform( srcx, srcy, srcw, srch, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
	 
	// Advance glyph_update_batch_x and calcuate final glyph drawing transform.
	glyph_draw_translate_x += cache->atlas.glyph_update_batch_x;
	cache->atlas.glyph_update_batch_x += gdwidth_scaled_px;
	ve_fontcache_screenspace_xform( glyph_draw_translate_x, glyph_draw_translate_y, glyph_draw_scale_x, glyph_draw_scale_y, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );

	// Queue up clear on target region on atlas.
	ve_fontcache_draw dcall;
	dcall.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS;
	dcall.region = ( uint32_t )( -1 );
	dcall.start_index = cache->atlas.glyph_update_batch_clear_drawlist.indices.size();
	ve_fontcache_blit_quad( cache->atlas.glyph_update_batch_clear_drawlist, destx, desty, destx + destw, desty + desth, 1.0f, 1.0f, 1.0f, 1.0f );
	dcall.end_index = cache->atlas.glyph_update_batch_clear_drawlist.indices.size();
	cache->atlas.glyph_update_batch_clear_drawlist.dcalls.push_back( dcall );

	// Queue up a blit from glyph_update_FBO to the atlas.
	dcall.region = ( uint32_t )( 0 );
	dcall.start_index = cache->atlas.glyph_update_batch_drawlist.indices.size();
	ve_fontcache_blit_quad( cache->atlas.glyph_update_batch_drawlist, dest_glyph_x, dest_glyph_y, destx + dest_glyph_w, desty + dest_glyph_h, srcx, srcy, srcx + srcw, srcy + srch );
	dcall.end_index = cache->atlas.glyph_update_batch_drawlist.indices.size();
	cache->atlas.glyph_update_batch_drawlist.dcalls.push_back( dcall );

	// Render glyph to glyph_update_FBO.
	ve_fontcache_cache_glyph(
		cache, font, unicode,
		glyph_draw_scale_x, glyph_draw_scale_y,
		glyph_draw_translate_x, glyph_draw_translate_y
	);
}

void ve_fontcache_shape_text_uncached( ve_fontcache* cache, ve_font_id font, ve_fontcache_shaped_text& output, const std::string& text_utf8  )
{
	STBTT_assert( cache );
	STBTT_assert( font >= 0 && font < cache->entry.size() );

	ve_fontcache_entry& entry = cache->entry[ font ];
	output.codepoints.clear();
	output.pos.clear();

#ifdef VE_FONTCACHE_HARFBUZZ
	// Use HarfBuzz for text shaping. Yay! This is good.
#else // VE_FONTCACHE_HARFBUZZ
	// No HarfBuzz. We use our own fallback dumbass text shaping.
	// WARNING: PLEASE DO NOT RUN THIS PATH IN PROD. GOOD TEXT SHAPING IS IMPORTANT FOR INTERNATIONALISATION.
	
	utf8_int32_t codepoint, prev_codepoint = 0;
	size_t u32_length = utf8len( text_utf8.data() );
	output.codepoints.reserve( u32_length );
	output.pos.reserve( u32_length );

	float pos = 0.0f, vpos = 0.0f;
	int advance = 0, to_left_side_glyph = 0;
	int ascent = 0, descent = 0, line_gap = 0;
	stbtt_GetFontVMetrics( &entry.info, &ascent, &descent, &line_gap ); // TODO: handle this correctly!!.

	// Loop through text and shape.
	for ( void* v = utf8codepoint( text_utf8.data(), &codepoint ); codepoint; v = utf8codepoint( v, &codepoint ) ) {
		if ( prev_codepoint ) {
			int kern = stbtt_GetCodepointKernAdvance( &entry.info, prev_codepoint, codepoint );
			pos += kern * entry.size_scale;
		}
		if ( codepoint == '\n' ) {
			pos = 0.0f;
			vpos -= ( ascent - descent + line_gap ) * entry.size_scale;
			vpos = ( int ) ( vpos + 0.5f );
			prev_codepoint = 0;
			continue;
		}
		if ( std::abs( entry.size ) <= 12 ) {
			// Expand advance to closest pixel for font small sizes.
			pos = std::ceilf( pos );
		}

		output.codepoints.push_back( ( ve_codepoint ) codepoint );
		stbtt_GetCodepointHMetrics( &entry.info, codepoint, &advance, &to_left_side_glyph );
		output.pos.push_back( ve_fontcache_make_vec2( int( pos + 0.5 ), vpos ) );
		
		float adv = advance * entry.size_scale;

		pos += adv;
		prev_codepoint = codepoint;
	}
#endif // VE_FONTCACHE_HARFBUZZ
}

template < typename T >
void ve_fontcache_ELFhash64( uint64_t& hash, const T* ptr, size_t count = 1 )
{
	uint64_t x = 0;
	auto bytes = reinterpret_cast< const uint8_t* >( ptr );

	for ( int i = 0; i < sizeof( T ) * count; i++ ) {
		hash = ( hash << 4 ) + bytes[i];
		if( ( x = hash & 0xF000000000000000ULL ) != 0 ) {
			hash ^= ( x >> 24 );
		}
		hash &= ~x;
	}
}

static ve_fontcache_shaped_text& ve_fontcache_shape_text_cached( ve_fontcache* cache, ve_font_id font, const std::string& text_utf8  )
{
	uint64_t hash = 0x9f8e00d51d263c24ULL;
	ve_fontcache_ELFhash64( hash, ( const uint8_t* ) text_utf8.data(), text_utf8.size() );
	ve_fontcache_ELFhash64( hash, &font );

	ve_fontcache_LRU& state = cache->shape_cache.state;
	int shape_cache_idx = ve_fontcache_LRU_get( state, hash );
	if ( shape_cache_idx == -1 ) {
		if ( cache->shape_cache.next_cache_idx < state.capacity ) {
			shape_cache_idx = cache->shape_cache.next_cache_idx++;
			ve_fontcache_LRU_put( state, hash, shape_cache_idx );
		} else {
			uint64_t next_evict_idx = ve_fontcache_LRU_get_next_evicted( state );
			STBTT_assert( next_evict_idx != 0xFFFFFFFFFFFFFFFFULL );
			shape_cache_idx = ve_fontcache_LRU_peek( state, next_evict_idx );
			STBTT_assert( shape_cache_idx != -1 );
			ve_fontcache_LRU_put( state, hash, shape_cache_idx );
		}
	}

	ve_fontcache_shape_text_uncached( cache, font, cache->shape_cache.storage[ shape_cache_idx ], text_utf8 );
	return cache->shape_cache.storage[ shape_cache_idx ];
}

static void ve_fontcache_directly_draw_massive_glyph( ve_fontcache* cache, ve_fontcache_entry& entry, ve_codepoint unicode, int bounds_x0, int bounds_y0, int bounds_width, int bounds_height,
	float oversample_x = 1.0f, float oversample_y = 1.0f, float posx = 0.0f, float posy = 0.0f, float scalex = 1.0f, float scaley = 1.0f )
{
	// Flush out whatever was in the glyph buffer beforehand to atlas.
	ve_fontcache_flush_glyph_buffer_to_atlas( cache );

	// Draw un-antialiased glyph to update FBO.
	float glyph_draw_scale_x = entry.size_scale * oversample_x;
	float glyph_draw_scale_y = entry.size_scale * oversample_y;
	float glyph_draw_translate_x = -bounds_x0 * glyph_draw_scale_x + VE_FONTCACHE_GLYPHDRAW_PADDING;
	float glyph_draw_translate_y = -bounds_y0 * glyph_draw_scale_y + VE_FONTCACHE_GLYPHDRAW_PADDING;
	ve_fontcache_screenspace_xform( glyph_draw_translate_x, glyph_draw_translate_y, glyph_draw_scale_x, glyph_draw_scale_y, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );

	// Render glyph to glyph_update_FBO.
	ve_fontcache_cache_glyph(
		cache, entry.font_id, unicode,
		glyph_draw_scale_x, glyph_draw_scale_y,
		glyph_draw_translate_x, glyph_draw_translate_y
	);

	// Figure out the source rect.
	float glyph_x = 0.0f, glyph_y = 0.0f, glyph_w = bounds_width * entry.size_scale * oversample_x, glyph_h = bounds_height * entry.size_scale * oversample_y;
	float glyph_dest_w = bounds_width * entry.size_scale, glyph_dest_h = bounds_height * entry.size_scale;
	glyph_w += 2 * VE_FONTCACHE_ATLAS_GLYPH_PADDING; glyph_h += 2 * VE_FONTCACHE_ATLAS_GLYPH_PADDING;
	glyph_dest_w += 2 * VE_FONTCACHE_ATLAS_GLYPH_PADDING; glyph_dest_h += 2 * VE_FONTCACHE_ATLAS_GLYPH_PADDING;

	// Figure out the destination rect.
	float bounds_x0_scaled = int( bounds_x0 * entry.size_scale - 0.5f );
	float bounds_y0_scaled = int( bounds_y0 * entry.size_scale - 0.5f );
	float dest_x = posx + scalex * bounds_x0_scaled;
	float dest_y = posy + scaley * bounds_y0_scaled;
	float dest_w = scalex * glyph_dest_w, dest_h = scaley * glyph_dest_h;
	dest_x -= scalex * VE_FONTCACHE_ATLAS_GLYPH_PADDING; dest_y -= scaley * VE_FONTCACHE_ATLAS_GLYPH_PADDING;
	ve_fontcache_texspace_xform( glyph_x, glyph_y, glyph_w, glyph_h, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );

	// Add the glyph drawcall.
	ve_fontcache_draw dcall;
	dcall.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED;
	dcall.colour[0] = cache->colour[0]; dcall.colour[1] = cache->colour[1]; dcall.colour[2] = cache->colour[2]; dcall.colour[3] = cache->colour[3]; 
	dcall.start_index = cache->drawlist.indices.size();
	ve_fontcache_blit_quad( cache->drawlist, dest_x, dest_y, dest_x + dest_w, dest_y + dest_h, glyph_x, glyph_y, glyph_x + glyph_w, glyph_y + glyph_h );
	dcall.end_index = cache->drawlist.indices.size();
	cache->drawlist.dcalls.push_back( dcall );

	// Clear glyph_update_FBO.
	dcall.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH;
	dcall.start_index = 0; dcall.end_index = 0;
	dcall.clear_before_draw = true;
	cache->drawlist.dcalls.push_back( dcall );
}

static bool ve_fontcache_empty( ve_fontcache* cache, ve_fontcache_entry& entry, ve_codepoint unicode )
{
	int glyph_index = stbtt_FindGlyphIndex( &entry.info, ( int ) unicode );
	if ( !glyph_index ) {
		// Glyph not in current font.
		return true;
	}
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph_index ) )
		return true;
	return false;
}

// This function only draws codepoints that have been drawn. Returns false without drawing anything if uncached.
bool ve_fontcache_draw_cached_codepoint( ve_fontcache* cache, ve_fontcache_entry& entry, ve_codepoint unicode, float posx = 0.0f, float posy = 0.0f, float scalex = 1.0f, float scaley = 1.0f )
{
	int glyph_index = stbtt_FindGlyphIndex( &entry.info, ( int ) unicode );
	if ( !glyph_index ) {
		// Glyph not in current font.
		return true;
	}
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph_index ) )
		return true;

	// Get font text metrics. These are unscaled!
	int bounds_x0, bounds_x1, bounds_y0, bounds_y1;
	int success = stbtt_GetGlyphBox( &entry.info, glyph_index, &bounds_x0, &bounds_y0, &bounds_x1, &bounds_y1 );
	int bounds_width = bounds_x1 - bounds_x0, bounds_height = bounds_y1 - bounds_y0;
	STBTT_assert( success );

	// Decide which atlas to target.
	ve_fontcache_LRU* state = nullptr; uint32_t* next_idx = nullptr;
	float oversample_x = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X, oversample_y = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y;
	ve_atlas_region region = ve_fontcache_decide_codepoint_region( cache, entry, glyph_index, state, next_idx, oversample_x, oversample_y );

	// E region is special case and not cached to atlas.
	if ( region == 'E' ) {
		ve_fontcache_directly_draw_massive_glyph( cache, entry, unicode, bounds_x0, bounds_y0, bounds_width, bounds_height, oversample_x, oversample_y, posx, posy, scalex, scaley );
		return true;
	}

	// Is this codepoint cached?
	uint64_t lru_code = unicode +  ( ( 0x100000000ULL * entry.font_id ) & 0xFFFFFFFF00000000ULL );
	int atlas_index = ve_fontcache_LRU_get( *state, lru_code );
	if( atlas_index == -1 ) {
		return false;
	}

	// Figure out the source bounding box in atlas texture.
	float atlas_x, atlas_y, atlas_w, atlas_h;
	ve_fontcache_atlas_bbox( region, atlas_index, atlas_x, atlas_y, atlas_w, atlas_h );
	float glyph_x = atlas_x, glyph_y = atlas_y, glyph_w = bounds_width * entry.size_scale, glyph_h = bounds_height * entry.size_scale;
	glyph_w += 2 * VE_FONTCACHE_ATLAS_GLYPH_PADDING; glyph_h += 2 * VE_FONTCACHE_ATLAS_GLYPH_PADDING;

	// Figure out the destination rect.
	float bounds_x0_scaled = int( bounds_x0 * entry.size_scale - 0.5f );
	float bounds_y0_scaled = int( bounds_y0 * entry.size_scale - 0.5f );
	float dest_x = posx + scalex * bounds_x0_scaled;
	float dest_y = posy + scaley * bounds_y0_scaled;
	float dest_w = scalex * glyph_w, dest_h = scaley * glyph_h;
	dest_x -= scalex * VE_FONTCACHE_ATLAS_GLYPH_PADDING; dest_y -= scaley * VE_FONTCACHE_ATLAS_GLYPH_PADDING;
	ve_fontcache_texspace_xform( glyph_x, glyph_y, glyph_w, glyph_h, VE_FONTCACHE_ATLAS_WIDTH, VE_FONTCACHE_ATLAS_HEIGHT );

	// Add the glyph drawcall.
	ve_fontcache_draw dcall;
	dcall.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET;
	dcall.colour[0] = cache->colour[0]; dcall.colour[1] = cache->colour[1]; dcall.colour[2] = cache->colour[2]; dcall.colour[3] = cache->colour[3]; 
	dcall.start_index = cache->drawlist.indices.size();
	ve_fontcache_blit_quad( cache->drawlist, dest_x, dest_y, dest_x + dest_w, dest_y + dest_h, glyph_x, glyph_y, glyph_x + glyph_w, glyph_y + glyph_h );
	dcall.end_index = cache->drawlist.indices.size();
	cache->drawlist.dcalls.push_back( dcall );

	return true;
}

static void ve_fontcache_reset_batch_codepoint_state( ve_fontcache* cache )
{
	cache->temp_codepoint_seen.clear();
	cache->temp_codepoint_seen.reserve( 256 );
}

static bool ve_fontcache_can_batch_codepoint( ve_fontcache* cache, ve_font_id font, ve_fontcache_entry& entry, ve_codepoint unicode )
{
	STBTT_assert( cache );
	STBTT_assert( entry.font_id == font );
	
	// Decide which atlas to target.
	int glyph_index = stbtt_FindGlyphIndex( &entry.info, ( int ) unicode ); STBTT_assert( glyph_index != -1 );
	ve_fontcache_LRU* state = nullptr; uint32_t* next_idx = nullptr;
	float oversample_x = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X, oversample_y = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y;
	ve_atlas_region region = ve_fontcache_decide_codepoint_region( cache, entry, glyph_index, state, next_idx, oversample_x, oversample_y );

	// E region can't batch.
	if ( region == 'E' || region == '\0' ) return false;
	if ( cache->temp_codepoint_seen.size() > 1024 ) return false;

	// Is this glyph cached?
	uint64_t lru_code = unicode +  ( ( 0x100000000ULL * entry.font_id ) & 0xFFFFFFFF00000000ULL );
	int atlas_index = ve_fontcache_LRU_get( *state, lru_code );
	if ( atlas_index == -1 ) {
		if ( *next_idx >= state->capacity ) {
			// We will evict LRU. We must predict which LRU will get evicted, and if it's something we've seen then we need to take slowpath and flush batch.
			uint64_t next_evict_codepoint = ve_fontcache_LRU_get_next_evicted( *state );
			if ( cache->temp_codepoint_seen[ next_evict_codepoint ] ) {
				return false;
			}
		}
		ve_fontcache_cache_codepoint_to_atlas( cache, font, unicode );
	}
	
	STBTT_assert( ve_fontcache_LRU_get( *state, lru_code ) != -1 );
	cache->temp_codepoint_seen[ lru_code ] = true;

	return true;
}

static void ve_fontcache_draw_text_batch( ve_fontcache* cache, ve_fontcache_entry& entry, ve_fontcache_shaped_text& shaped,
	int batch_start_idx, int batch_end_idx, float posx, float posy, float scalex, float scaley )
{
	ve_fontcache_flush_glyph_buffer_to_atlas( cache );
	for( int j = batch_start_idx; j < batch_end_idx; j++ ) {
		ve_codepoint unicode = shaped.codepoints[j];
		float glyph_translate_x = posx + shaped.pos[j].x * scalex, glyph_translate_y = posy + shaped.pos[j].y * scaley;
		bool glyph_cached = ve_fontcache_draw_cached_codepoint( cache, entry, unicode, glyph_translate_x, glyph_translate_y, scalex, scaley );
		STBTT_assert( glyph_cached );
	}
}

bool ve_fontcache_draw_text( ve_fontcache* cache, ve_font_id font, const std::string& text_utf8, float posx, float posy, float scalex, float scaley )
{
	ve_fontcache_shaped_text& shaped = ve_fontcache_shape_text_cached( cache, font, text_utf8 );
	
	if ( cache->snap_width ) posx = ( ( int ) ( posx * cache->snap_width + 0.5f ) ) / ( float ) cache->snap_width;
	if ( cache->snap_height ) posy = ( ( int ) ( posy * cache->snap_height + 0.5f ) ) / ( float ) cache->snap_height;

	STBTT_assert( cache );
	STBTT_assert( font >= 0 && font < cache->entry.size() );
	ve_fontcache_entry& entry = cache->entry[ font ];

	int batch_start_idx = 0;
	for( int i = 0; i < shaped.codepoints.size(); i++ ) {
		ve_codepoint unicode = shaped.codepoints[i];
		if ( ve_fontcache_empty( cache, entry, unicode ) )
			continue;

		if ( ve_fontcache_can_batch_codepoint( cache, font, entry, unicode ) ) {
			continue;
		}

		ve_fontcache_draw_text_batch( cache, entry, shaped, batch_start_idx, i, posx, posy, scalex, scaley );
		ve_fontcache_reset_batch_codepoint_state( cache );
		
		ve_fontcache_cache_codepoint_to_atlas( cache, font, unicode );
		uint64_t lru_code = unicode +  ( ( 0x100000000ULL * font ) & 0xFFFFFFFF00000000ULL );
		cache->temp_codepoint_seen[ lru_code ] = true;

		batch_start_idx = i;
	}
	ve_fontcache_draw_text_batch( cache, entry, shaped, batch_start_idx, shaped.codepoints.size(), posx, posy, scalex, scaley );
	ve_fontcache_reset_batch_codepoint_state( cache );
	return true;
}

void ve_fontcache_optimise_drawlist( ve_fontcache* cache )
{
	STBTT_assert( cache );

	int write_idx = 0;
	for ( int i = 1; i < cache->drawlist.dcalls.size(); i++ ) {
		
		STBTT_assert( write_idx <= i );
		ve_fontcache_draw& draw0 = cache->drawlist.dcalls[ write_idx ];
		ve_fontcache_draw& draw1 = cache->drawlist.dcalls[ i ];
		
		bool merge = true;
		if ( draw0.pass != draw1.pass ) merge = false;
		if ( draw0.end_index != draw1.start_index ) merge = false;
		if ( draw0.region != draw1.region ) merge = false;
		if ( draw1.clear_before_draw ) merge = false;
		if ( draw0.colour[0] != draw1.colour[0] || draw0.colour[1] != draw1.colour[1] ||
			draw0.colour[2] != draw1.colour[2] || draw0.colour[3] != draw1.colour[3] ) merge = false;

		if ( merge ) {
			draw0.end_index = draw1.end_index;
			draw1.start_index = draw1.end_index = 0;
		} else {
			ve_fontcache_draw& draw2 = cache->drawlist.dcalls[ ++write_idx ];
			if ( write_idx != i ) draw2 = draw1;
		}
	}
	cache->drawlist.dcalls.resize( write_idx + 1 );
}

void ve_fontcache_set_colour( ve_fontcache* cache, float c[4] )
{
	cache->colour[0] = c[0];
	cache->colour[1] = c[1];
	cache->colour[2] = c[2];
	cache->colour[3] = c[3];
}

// ------------------------------------------------------ Generic Data Structure Implementations ------------------------------------------


void ve_fontcache_poollist_init( ve_fontcache_poollist& plist, int capacity )
{
	plist.pool.resize( capacity );
	plist.freelist.resize( capacity );
	plist.capacity = capacity;
	for ( int i = 0; i < capacity; i++ ) plist.freelist[ i ] = i;
}

void ve_fontcache_poollist_push_front( ve_fontcache_poollist& plist, ve_fontcache_poollist_value v )
{
	if ( plist.size >= plist.capacity ) return;
	assert( plist.freelist.size() > 0 );
	assert( plist.freelist.size() == plist.capacity - plist.size );

	ve_fontcache_poollist_itr idx = plist.freelist.back();
	plist.freelist.pop_back();
	plist.pool[ idx ].prev = -1;
	plist.pool[ idx ].next = plist.front;
	plist.pool[ idx ].value = v;

	if ( plist.front != -1 ) plist.pool[ plist.front ].prev = idx;
	if ( plist.back == -1 ) plist.back = idx;
	plist.front = idx;
	plist.size++;
}

void ve_fontcache_poollist_erase( ve_fontcache_poollist& plist, ve_fontcache_poollist_itr it )
{
	if ( plist.size <= 0 ) return;
	assert( it >= 0 && it < plist.capacity );
	assert( plist.freelist.size() == plist.capacity - plist.size );

	if ( plist.pool[ it ].prev != -1 )
		plist.pool[ plist.pool[ it ].prev ].next = plist.pool[ it ].next;
	if ( plist.pool[ it ].next != -1 )
		plist.pool[ plist.pool[ it ].next ].prev = plist.pool[ it ].prev;

	if ( plist.front == it ) plist.front = plist.pool[ it ].next;
	if ( plist.back == it ) plist.back = plist.pool[ it ].prev;

	plist.pool[ it ].prev = -1;
	plist.pool[ it ].next = -1;
	plist.pool[ it ].value = 0;
	plist.freelist.push_back( it );

	if ( --plist.size == 0 ) plist.back = plist.front = -1;
}

ve_fontcache_poollist_value ve_fontcache_poollist_peek_back( ve_fontcache_poollist& plist )
{
	assert( plist.back != -1 );
	return plist.pool[ plist.back ].value;
}

ve_fontcache_poollist_value ve_fontcache_poollist_pop_back( ve_fontcache_poollist& plist )
{
	if ( plist.size <= 0 ) return 0;
	assert( plist.back != -1 );
	ve_fontcache_poollist_value v = plist.pool[ plist.back ].value;
	ve_fontcache_poollist_erase( plist, plist.back );
	return v;
}

void ve_fontcache_LRU_init( ve_fontcache_LRU& LRU, int capacity )
{
	// Thanks to dfsbfs from leetcode! This code is eavily based on that with simplifications made on top.
	// ref: https://leetcode.com/problems/lru-cache/discuss/968703/c%2B%2B
	//      https://leetcode.com/submissions/detail/436667816/
	LRU.capacity = capacity;
	LRU.cache.reserve( capacity );
	ve_fontcache_poollist_init( LRU.key_queue, capacity );
}

void ve_fontcache_LRU_refresh( ve_fontcache_LRU& LRU, uint64_t key )
{
	auto it = LRU.cache.find( key );
	ve_fontcache_poollist_erase( LRU.key_queue, it->second.ptr );
	ve_fontcache_poollist_push_front( LRU.key_queue, key );
	it->second.ptr = LRU.key_queue.front;
}

int ve_fontcache_LRU_get( ve_fontcache_LRU& LRU, uint64_t key )
{
	auto it = LRU.cache.find( key );
	if ( it == LRU.cache.end() ) {
		return -1;
	}
	ve_fontcache_LRU_refresh( LRU, key );
	return it->second.value;
}

int ve_fontcache_LRU_peek( ve_fontcache_LRU& LRU, uint64_t key )
{
	auto it = LRU.cache.find( key );
	if ( it == LRU.cache.end() ) {
		return -1;
	}
	return it->second.value;
}

uint64_t ve_fontcache_LRU_put( ve_fontcache_LRU& LRU, uint64_t key, int val )
{
	auto it = LRU.cache.find( key );
	if ( it != LRU.cache.end() ) {
		ve_fontcache_LRU_refresh( LRU, key );
		it->second.value = val;
		return key;
	}

	uint64_t evict = key;
	if ( LRU.key_queue.size >= LRU.capacity ) {
		evict = ve_fontcache_poollist_pop_back( LRU.key_queue );
		LRU.cache.erase( evict );
	}

	ve_fontcache_poollist_push_front( LRU.key_queue, key );
	LRU.cache[ key ].value = val;
	LRU.cache[ key ].ptr = LRU.key_queue.front;
	return evict;
}

uint64_t ve_fontcache_LRU_get_next_evicted( ve_fontcache_LRU& LRU )
{
	if ( LRU.key_queue.size >= LRU.capacity ) {
		uint64_t evict = ve_fontcache_poollist_peek_back( LRU.key_queue );;
		return evict;
	}
	return 0xFFFFFFFFFFFFFFFFULL;
}

#endif // VE_FONTCACHE_IMPL