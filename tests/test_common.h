#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#endif

#ifdef VE_FONTCACHE_HARFBUZZ
#include <hb.h>
#endif

#define VE_STBTT_NOIMPL
#include "../utf8/test/utest.h"
#include "../ve_fontcache.h"
#include "../ve_fontcache_backend_test.h"

namespace vefc_test
{
inline constexpr float kScaleX = 1.0f / 1920.0f;
inline constexpr float kScaleY = 1.0f / 1080.0f;
inline constexpr ve_fontcache_poollist_itr kInvalidPoolItr = static_cast< ve_fontcache_poollist_itr >( -1 );

inline constexpr const char* kRoboto = "fonts/Roboto-Regular.ttf";
inline constexpr const char* kOpenSans = "fonts/OpenSans-Regular.ttf";
inline constexpr const char* kNotoSansJP = "fonts/NotoSansJP-Regular.otf";
inline constexpr const char* kNotoSerifSC = "fonts/NotoSerifSC-Regular.otf";
inline constexpr const char* kTajawal = "fonts/Tajawal-Regular.ttf";
inline constexpr const char* kDavidLibre = "fonts/DavidLibre-Regular.ttf";

inline std::filesystem::path resolve_font_path( const char* relative_font_path )
{
	std::filesystem::path common_h = __FILE__;
	std::filesystem::path repo_root = common_h.parent_path().parent_path();
	return repo_root / "demo" / relative_font_path;
}

inline bool default_use_freetype()
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	return true;
#else
	return false;
#endif
}

struct context
{
	ve_fontcache cache {};
	std::vector< std::vector< uint8_t > > buffers;

	explicit context( bool use_freetype = default_use_freetype() )
	{
		ve_fontcache_init( &cache, use_freetype );
	}

	~context()
	{
		ve_fontcache_shutdown( &cache );
	}

	ve_font_id load_file( const char* relative_path, float size_px = 24.0f )
	{
		buffers.emplace_back();
		std::filesystem::path resolved = resolve_font_path( relative_path );
		return ve_fontcache_loadfile( &cache, resolved.string().c_str(), buffers.back(), size_px );
	}

	ve_font_id load_buffer_copy( size_t buffer_idx, float size_px = 24.0f )
	{
		std::vector< uint8_t >& buffer = buffers[ buffer_idx ];
		return ve_fontcache_load( &cache, buffer.data(), buffer.size(), size_px );
	}
};

inline void flush( context& ctx )
{
	ve_fontcache_flush_drawlist( &ctx.cache );
}

inline bool draw_text( context& ctx, ve_font_id font, std::u8string_view text, float posx = 0.0f, float posy = 0.0f, bool shape_cache = true )
{
	return ve_fontcache_draw_text( &ctx.cache, font, std::u8string( text ), posx, posy, kScaleX, kScaleY, shape_cache );
}

inline ve_fontcache_drawlist* current_drawlist( context& ctx, bool optimise = true )
{
	if ( optimise ) {
		ve_fontcache_optimise_drawlist( &ctx.cache );
	}
	return ve_fontcache_get_drawlist( &ctx.cache );
}

inline bool is_target_pass( uint32_t pass )
{
	return pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED;
}

inline const ve_fontcache_draw* find_first_target_draw( const ve_fontcache_drawlist& drawlist )
{
	for ( const ve_fontcache_draw& draw : drawlist.dcalls ) {
		if ( is_target_pass( draw.pass ) ) {
			return &draw;
		}
	}

	return nullptr;
}

inline int count_pass( const ve_fontcache_drawlist& drawlist, uint32_t pass )
{
	int count = 0;
	for ( const ve_fontcache_draw& draw : drawlist.dcalls ) {
		if ( draw.pass == pass && draw.end_index > draw.start_index ) {
			count++;
		}
	}

	return count;
}

inline bool has_pass( const ve_fontcache_drawlist& drawlist, uint32_t pass )
{
	return count_pass( drawlist, pass ) > 0;
}

inline bool all_indices_in_range( const ve_fontcache_drawlist& drawlist )
{
	for ( uint32_t idx : drawlist.indices ) {
		if ( idx >= drawlist.vertices.size() ) {
			return false;
		}
	}

	return true;
}

inline bool all_vertices_finite( const ve_fontcache_drawlist& drawlist )
{
	for ( const ve_fontcache_vertex& v : drawlist.vertices ) {
		if ( !std::isfinite( v.x ) || !std::isfinite( v.y ) || !std::isfinite( v.u ) || !std::isfinite( v.v ) ) {
			return false;
		}
	}

	return true;
}

inline bool target_uvs_normalised( const ve_fontcache_drawlist& drawlist )
{
	for ( const ve_fontcache_draw& draw : drawlist.dcalls ) {
		if ( !is_target_pass( draw.pass ) ) {
			continue;
		}

		for ( uint32_t idx = draw.start_index; idx < draw.end_index; idx++ ) {
			const ve_fontcache_vertex& v = drawlist.vertices[ drawlist.indices[ idx ] ];
			if ( v.u < -0.01f || v.u > 1.01f || v.v < -0.01f || v.v > 1.01f ) {
				return false;
			}
		}
	}

	return true;
}

inline bool colour_equals( const float actual[ 4 ], const std::array< float, 4 >& expected )
{
	for ( size_t i = 0; i < expected.size(); i++ ) {
		if ( std::fabs( actual[ i ] - expected[ i ] ) > 0.0001f ) {
			return false;
		}
	}

	return true;
}

inline std::vector< uint64_t > drain_poollist( ve_fontcache_poollist& plist )
{
	std::vector< uint64_t > values;
	values.reserve( plist.size );
	while ( plist.size > 0 ) {
		values.push_back( ve_fontcache_poollist_pop_back( plist ) );
	}
	return values;
}

inline std::u8string to_u8string( const std::string& text )
{
	std::u8string out;
	out.reserve( text.size() );
	for ( char ch : text ) {
		out.push_back( static_cast< char8_t >( static_cast< unsigned char >( ch ) ) );
	}
	return out;
}

inline void append_utf8( std::u8string& out, char32_t codepoint )
{
	if ( codepoint <= 0x7F ) {
		out.push_back( static_cast< char8_t >( codepoint ) );
		return;
	}

	if ( codepoint <= 0x7FF ) {
		out.push_back( static_cast< char8_t >( 0xC0 | ( codepoint >> 6 ) ) );
		out.push_back( static_cast< char8_t >( 0x80 | ( codepoint & 0x3F ) ) );
		return;
	}

	if ( codepoint <= 0xFFFF ) {
		out.push_back( static_cast< char8_t >( 0xE0 | ( codepoint >> 12 ) ) );
		out.push_back( static_cast< char8_t >( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) ) );
		out.push_back( static_cast< char8_t >( 0x80 | ( codepoint & 0x3F ) ) );
		return;
	}

	out.push_back( static_cast< char8_t >( 0xF0 | ( codepoint >> 18 ) ) );
	out.push_back( static_cast< char8_t >( 0x80 | ( ( codepoint >> 12 ) & 0x3F ) ) );
	out.push_back( static_cast< char8_t >( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) ) );
	out.push_back( static_cast< char8_t >( 0x80 | ( codepoint & 0x3F ) ) );
}

inline std::u8string make_ascii_string( size_t length )
{
	std::u8string text;
	text.reserve( length );
	for ( size_t i = 0; i < length; i++ ) {
		text.push_back( static_cast< char8_t >( 'A' + ( i % 26 ) ) );
	}
	return text;
}

inline std::u8string make_cjk_string( size_t length, char32_t start = 0x4E00 )
{
	std::u8string text;
	for ( size_t i = 0; i < length; i++ ) {
		append_utf8( text, start + static_cast< char32_t >( i ) );
	}
	return text;
}

inline bool any_non_zero( const std::vector< uint8_t >& values )
{
	return std::any_of( values.begin(), values.end(), []( uint8_t value ) { return value != 0; } );
}
} // namespace vefc_test
