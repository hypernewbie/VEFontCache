#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "ve_fontcache.h"

using ve_fontcache_backend_readback_fn =
	std::function< bool( const char* name, int x, int y, int w, int h, uint8_t* out_pixels ) >;
using ve_fontcache_backend_execute_fn = std::function< void() >;
using ve_fontcache_backend_reload_font_fn = std::function< ve_font_id() >;

struct ve_fontcache_backend_test_options
{
	ve_fontcache* cache = nullptr;
	ve_font_id font = -1;
	ve_font_id secondary_font = -1;
	ve_font_id small_font = -1;
	ve_font_id latin_font = -1;
	ve_font_id cjk_font = -1;
	ve_font_id huge_font = -1;
	ve_fontcache_backend_execute_fn execute;
	ve_fontcache_backend_readback_fn readback;
	ve_fontcache_backend_reload_font_fn reload_font;
};

struct ve_fontcache_backend_test_result
{
	int passed = 0;
	int failed = 0;
	int skipped = 0;
	std::string last_failure;
	std::vector< std::string > failures;
	std::vector< std::string > skipped_tests;
};

inline void ve_fontcache_backend_test_expect(
	ve_fontcache_backend_test_result& result,
	bool cond,
	std::string_view msg )
{
	if ( cond ) {
		result.passed++;
		return;
	}

	result.failed++;
	result.last_failure = std::string( msg );
	result.failures.emplace_back( msg );
}

inline void ve_fontcache_backend_test_skip(
	ve_fontcache_backend_test_result& result,
	std::string_view msg )
{
	result.skipped++;
	result.skipped_tests.emplace_back( msg );
}

inline bool ve_fontcache_backend_test_is_target_pass( uint32_t pass )
{
	return pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED;
}

inline bool ve_fontcache_backend_test_is_known_pass( uint32_t pass )
{
	return pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED;
}

inline bool ve_fontcache_backend_test_is_atlas_update_pass( uint32_t pass )
{
	return pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD;
}

inline int ve_fontcache_backend_test_count_pass( const ve_fontcache_drawlist& drawlist, uint32_t pass )
{
	int count = 0;
	for ( const ve_fontcache_draw& draw : drawlist.dcalls ) {
		if ( draw.pass == pass && draw.end_index > draw.start_index ) {
			count++;
		}
	}
	return count;
}

inline int ve_fontcache_backend_test_count_cache_miss_passes( const ve_fontcache& cache, const ve_fontcache_drawlist& drawlist )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( cache.use_freetype ) {
		return ve_fontcache_backend_test_count_pass( drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE )
			+ ve_fontcache_backend_test_count_pass( drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD );
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	return ve_fontcache_backend_test_count_pass( drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH );
}

inline bool ve_fontcache_backend_test_any_non_zero( const std::vector< uint8_t >& values )
{
	return std::any_of( values.begin(), values.end(), []( uint8_t value ) { return value != 0; } );
}

inline bool ve_fontcache_backend_test_has_intermediate_coverage( const std::vector< uint8_t >& values )
{
	return std::any_of( values.begin(), values.end(), []( uint8_t value ) {
		return value > 0 && value < std::numeric_limits< uint8_t >::max();
	} );
}

inline void ve_fontcache_backend_test_append_utf8( std::u8string& out, char32_t codepoint )
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

inline std::u8string ve_fontcache_backend_test_codepoint_to_utf8( char32_t codepoint )
{
	std::u8string text;
	ve_fontcache_backend_test_append_utf8( text, codepoint );
	return text;
}

inline std::u8string ve_fontcache_backend_test_make_ascii_string( size_t length )
{
	std::u8string text;
	text.reserve( length );
	for ( size_t i = 0; i < length; i++ ) {
		text.push_back( static_cast< char8_t >( 'A' + ( i % 26 ) ) );
	}
	return text;
}

inline std::u8string ve_fontcache_backend_test_make_cjk_string( size_t length, char32_t start = 0x4E00 )
{
	std::u8string text;
	for ( size_t i = 0; i < length; i++ ) {
		ve_fontcache_backend_test_append_utf8( text, start + static_cast< char32_t >( i ) );
	}
	return text;
}

inline ve_glyph ve_fontcache_backend_test_find_glyph( ve_fontcache* cache, ve_font_id font, char32_t codepoint )
{
	if ( !cache || font < 0 || font >= static_cast< ve_font_id >( cache->entry.size() ) || !cache->entry[ font ].used ) {
		return 0;
	}

	return stbtt_FindGlyphIndex( &cache->entry[ font ].info, codepoint );
}

inline bool ve_fontcache_backend_test_allowed_region( char region, std::string_view allowed_regions )
{
	return allowed_regions.find( region ) != std::string_view::npos;
}

inline bool ve_fontcache_backend_test_get_region_for_codepoint(
	ve_fontcache* cache,
	ve_font_id font,
	char32_t codepoint,
	char& region )
{
	ve_glyph glyph = ve_fontcache_backend_test_find_glyph( cache, font, codepoint );
	if ( !glyph ) {
		return false;
	}

	ve_fontcache_entry& entry = cache->entry[ font ];
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph ) ) {
		return false;
	}

	ve_fontcache_LRU* state = nullptr;
	uint32_t* next_idx = nullptr;
	float oversample_x = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X;
	float oversample_y = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y;
	region = ve_fontcache_decide_codepoint_region( cache, entry, glyph, state, next_idx, oversample_x, oversample_y );
	return region != '\0';
}

inline bool ve_fontcache_backend_test_find_codepoint_for_regions(
	ve_fontcache* cache,
	ve_font_id font,
	std::string_view allowed_regions,
	char32_t start,
	char32_t end,
	char32_t& codepoint_out,
	char& region_out )
{
	for ( char32_t codepoint = start; codepoint <= end; codepoint++ ) {
		char region = '\0';
		if ( !ve_fontcache_backend_test_get_region_for_codepoint( cache, font, codepoint, region ) ) {
			continue;
		}
		if ( !ve_fontcache_backend_test_allowed_region( region, allowed_regions ) ) {
			continue;
		}

		codepoint_out = codepoint;
		region_out = region;
		return true;
	}

	return false;
}

inline ve_fontcache_LRU* ve_fontcache_backend_test_state_for_region( ve_fontcache* cache, char region );
inline uint64_t ve_fontcache_backend_test_cache_key( ve_font_id font, ve_glyph glyph );

inline bool ve_fontcache_backend_test_is_codepoint_cached(
	ve_fontcache* cache,
	ve_font_id font,
	char32_t codepoint )
{
	char region = '\0';
	if ( !ve_fontcache_backend_test_get_region_for_codepoint( cache, font, codepoint, region ) ) {
		return false;
	}

	if ( region == 'E' ) {
		return false;
	}

	ve_glyph glyph = ve_fontcache_backend_test_find_glyph( cache, font, codepoint );
	ve_fontcache_LRU* state = ve_fontcache_backend_test_state_for_region( cache, region );
	if ( !glyph || !state ) {
		return false;
	}

	return ve_fontcache_LRU_peek( *state, ve_fontcache_backend_test_cache_key( font, glyph ) ) != -1;
}

inline bool ve_fontcache_backend_test_find_uncached_codepoint_for_regions(
	ve_fontcache* cache,
	ve_font_id font,
	std::string_view allowed_regions,
	char32_t start,
	char32_t end,
	char32_t& codepoint_out,
	char& region_out )
{
	for ( char32_t codepoint = start; codepoint <= end; codepoint++ ) {
		char region = '\0';
		if ( !ve_fontcache_backend_test_get_region_for_codepoint( cache, font, codepoint, region ) ) {
			continue;
		}
		if ( !ve_fontcache_backend_test_allowed_region( region, allowed_regions ) ) {
			continue;
		}
		if ( ve_fontcache_backend_test_is_codepoint_cached( cache, font, codepoint ) ) {
			continue;
		}

		codepoint_out = codepoint;
		region_out = region;
		return true;
	}

	return false;
}

inline std::vector< char32_t > ve_fontcache_backend_test_collect_codepoints_for_region(
	ve_fontcache* cache,
	ve_font_id font,
	char region,
	size_t wanted,
	char32_t start,
	char32_t end )
{
	std::vector< char32_t > codepoints;
	codepoints.reserve( wanted );
	for ( char32_t codepoint = start; codepoint <= end && codepoints.size() < wanted; codepoint++ ) {
		char actual_region = '\0';
		if ( !ve_fontcache_backend_test_get_region_for_codepoint( cache, font, codepoint, actual_region ) ) {
			continue;
		}
		if ( actual_region == region && !ve_fontcache_backend_test_is_codepoint_cached( cache, font, codepoint ) ) {
			codepoints.push_back( codepoint );
		}
	}

	return codepoints;
}

struct ve_fontcache_backend_test_atlas_sizes
{
	size_t region_A = 0;
	size_t region_B = 0;
	size_t region_C = 0;
	size_t region_D = 0;
};

inline ve_fontcache_backend_test_atlas_sizes ve_fontcache_backend_test_capture_atlas_sizes( const ve_fontcache& cache )
{
	ve_fontcache_backend_test_atlas_sizes sizes;
	sizes.region_A = cache.atlas.stateA.cache.size();
	sizes.region_B = cache.atlas.stateB.cache.size();
	sizes.region_C = cache.atlas.stateC.cache.size();
	sizes.region_D = cache.atlas.stateD.cache.size();
	return sizes;
}

inline size_t ve_fontcache_backend_test_region_cache_size( const ve_fontcache& cache, char region )
{
	switch ( region ) {
		case 'A': return cache.atlas.stateA.cache.size();
		case 'B': return cache.atlas.stateB.cache.size();
		case 'C': return cache.atlas.stateC.cache.size();
		case 'D': return cache.atlas.stateD.cache.size();
		default: return 0;
	}
}

inline char ve_fontcache_backend_test_growing_region(
	const ve_fontcache_backend_test_atlas_sizes& before,
	const ve_fontcache_backend_test_atlas_sizes& after )
{
	if ( after.region_A > before.region_A ) {
		return 'A';
	}
	if ( after.region_B > before.region_B ) {
		return 'B';
	}
	if ( after.region_C > before.region_C ) {
		return 'C';
	}
	if ( after.region_D > before.region_D ) {
		return 'D';
	}
	return '\0';
}

inline uint32_t* ve_fontcache_backend_test_next_index_for_region( ve_fontcache* cache, char region )
{
	switch ( region ) {
		case 'A': return &cache->atlas.next_atlas_idx_A;
		case 'B': return &cache->atlas.next_atlas_idx_B;
		case 'C': return &cache->atlas.next_atlas_idx_C;
		case 'D': return &cache->atlas.next_atlas_idx_D;
		default: return nullptr;
	}
}

inline ve_fontcache_LRU* ve_fontcache_backend_test_state_for_region( ve_fontcache* cache, char region )
{
	switch ( region ) {
		case 'A': return &cache->atlas.stateA;
		case 'B': return &cache->atlas.stateB;
		case 'C': return &cache->atlas.stateC;
		case 'D': return &cache->atlas.stateD;
		default: return nullptr;
	}
}

inline uint64_t ve_fontcache_backend_test_cache_key( ve_font_id font, ve_glyph glyph )
{
	return glyph + ( ( 0x100000000ULL * font ) & 0xFFFFFFFF00000000ULL );
}

inline bool ve_fontcache_backend_test_predict_atlas_slot(
	ve_fontcache* cache,
	ve_font_id font,
	char32_t codepoint,
	char& region_out,
	int& atlas_index_out )
{
	ve_glyph glyph = ve_fontcache_backend_test_find_glyph( cache, font, codepoint );
	if ( !glyph ) {
		return false;
	}

	ve_fontcache_entry& entry = cache->entry[ font ];
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph ) ) {
		return false;
	}

	ve_fontcache_LRU* state = nullptr;
	uint32_t* next_idx = nullptr;
	float oversample_x = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X;
	float oversample_y = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y;
	region_out = ve_fontcache_decide_codepoint_region( cache, entry, glyph, state, next_idx, oversample_x, oversample_y );
	if ( region_out == '\0' || region_out == 'E' || !state || !next_idx ) {
		return false;
	}

	uint64_t key = ve_fontcache_backend_test_cache_key( font, glyph );
	atlas_index_out = ve_fontcache_LRU_peek( *state, key );
	if ( atlas_index_out != -1 ) {
		return true;
	}

	if ( static_cast< int >( *next_idx ) < state->capacity ) {
		atlas_index_out = static_cast< int >( *next_idx );
		return true;
	}

	uint64_t next_evict = ve_fontcache_LRU_get_next_evicted( *state );
	if ( next_evict == 0xFFFFFFFFFFFFFFFFULL ) {
		return false;
	}

	atlas_index_out = ve_fontcache_LRU_peek( *state, next_evict );
	return atlas_index_out != -1;
}

inline bool ve_fontcache_backend_test_readback_texture(
	const ve_fontcache_backend_test_options& options,
	const char* name,
	int x,
	int y,
	int w,
	int h,
	std::vector< uint8_t >& out_pixels )
{
	if ( !options.readback ) {
		return false;
	}

	out_pixels.assign( static_cast< size_t >( w ) * static_cast< size_t >( h ), 0 );
	return options.readback( name, x, y, w, h, out_pixels.data() );
}

inline void ve_fontcache_backend_test_validate_drawlist(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_drawlist& drawlist,
	bool expect_target_pass )
{
	bool has_target_pass = false;
	bool seen_atlas_update = false;
	bool seen_target = false;
	int last_glyph_idx = -1;
	int first_atlas_idx = std::numeric_limits< int >::max();

	for ( uint32_t idx : drawlist.indices ) {
		ve_fontcache_backend_test_expect( result, idx < drawlist.vertices.size(), "index stayed within vertex bounds" );
	}

	for ( const ve_fontcache_vertex& vertex : drawlist.vertices ) {
		ve_fontcache_backend_test_expect( result, std::isfinite( vertex.x ), "vertex x was finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( vertex.y ), "vertex y was finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( vertex.u ), "vertex u was finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( vertex.v ), "vertex v was finite" );
	}

	for ( size_t i = 0; i < drawlist.dcalls.size(); i++ ) {
		const ve_fontcache_draw& draw = drawlist.dcalls[ i ];
		ve_fontcache_backend_test_expect( result, ve_fontcache_backend_test_is_known_pass( draw.pass ), "drawlist pass type was known" );
		ve_fontcache_backend_test_expect( result, draw.end_index >= draw.start_index, "draw index range was monotonic" );
		ve_fontcache_backend_test_expect( result, draw.end_index <= drawlist.indices.size(), "draw index range stayed in bounds" );

		if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH && draw.end_index == draw.start_index ) {
			ve_fontcache_backend_test_expect( result, draw.clear_before_draw, "empty glyph drawcall was a clear" );
		}

		if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH && draw.end_index > draw.start_index ) {
			last_glyph_idx = static_cast< int >( i );
		}

		if ( ve_fontcache_backend_test_is_atlas_update_pass( draw.pass ) ) {
			seen_atlas_update = true;
			if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS && first_atlas_idx == std::numeric_limits< int >::max() ) {
				first_atlas_idx = static_cast< int >( i );
			}
		}

		if ( ve_fontcache_backend_test_is_target_pass( draw.pass ) ) {
			has_target_pass = true;
			seen_target = true;
			for ( uint32_t idx = draw.start_index; idx < draw.end_index; idx++ ) {
				const ve_fontcache_vertex& vertex = drawlist.vertices[ drawlist.indices[ idx ] ];
				ve_fontcache_backend_test_expect(
					result,
					vertex.u >= -0.01f && vertex.u <= 1.01f,
					"target u stayed near [0, 1]" );
				ve_fontcache_backend_test_expect(
					result,
					vertex.v >= -0.01f && vertex.v <= 1.01f,
					"target v stayed near [0, 1]" );
			}
		} else if ( seen_target ) {
			ve_fontcache_backend_test_expect( result, false, "atlas updates finished before target draws" );
		}
	}

	if ( expect_target_pass ) {
		ve_fontcache_backend_test_expect( result, has_target_pass, "non-empty text produced a target draw" );
	}

	if ( last_glyph_idx != -1 && first_atlas_idx != std::numeric_limits< int >::max() ) {
		ve_fontcache_backend_test_expect( result, last_glyph_idx < first_atlas_idx, "glyph passes preceded atlas blits" );
	}

	if ( seen_atlas_update && seen_target ) {
		ve_fontcache_backend_test_expect( result, true, "atlas updates were ordered before target draws" );
	}
}

inline ve_fontcache_drawlist* ve_fontcache_backend_test_draw(
	ve_fontcache_backend_test_result& result,
	ve_fontcache* cache,
	ve_font_id font,
	const std::u8string& text,
	bool shape_cache = true,
	float posx = 0.1f,
	float posy = 0.5f,
	float scalex = 1.0f / 1920.0f,
	float scaley = 1.0f / 1080.0f )
{
	ve_fontcache_flush_drawlist( cache );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_draw_text( cache, font, text, posx, posy, scalex, scaley, shape_cache ),
		"draw_text succeeded" );
	ve_fontcache_optimise_drawlist( cache );
	return ve_fontcache_get_drawlist( cache );
}

inline void ve_fontcache_backend_test_run_structural_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	const std::u8string text = u8"Hello, World! 日本語";
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, text );
	ve_fontcache_backend_test_expect( result, !drawlist->dcalls.empty(), "drawlist was not empty" );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );
	ve_fontcache_flush_drawlist( options.cache );
}

inline void ve_fontcache_backend_test_run_caching_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "caching checks skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	const std::u8string cached_text = u8"GlyphCacheXYZ123";
	const std::u8string uncached_text = u8"FreshUVW987";
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, cached_text );
	int initial_cache_misses = ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist );
	ve_fontcache_backend_test_expect( result, initial_cache_misses > 0, "first draw emitted cache-miss work" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, cached_text );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"cached redraw reused glyph cache" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, cached_text, false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"atlas cache survived shape-cache bypass" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, uncached_text );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
		"different text triggered fresh glyph work" );

	if ( options.secondary_font >= 0 ) {
		(void) ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"A" );
		drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.secondary_font, u8"A" );
		ve_fontcache_backend_test_expect(
			result,
			ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
			"same glyph in a second font used an independent cache entry" );
	} else {
		ve_fontcache_backend_test_skip( result, "multi-font independence skipped: secondary_font not supplied" );
	}

	ve_fontcache_flush_drawlist( options.cache );
}

inline void ve_fontcache_backend_test_run_region_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "atlas region routing skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	if ( options.small_font >= 0 ) {
		char32_t codepoint = 0;
		char region = '\0';
		bool found = ve_fontcache_backend_test_find_uncached_codepoint_for_regions(
			options.cache,
			options.small_font,
			"AB",
			0x20,
			0x9FFF,
			codepoint,
			region );
		ve_fontcache_backend_test_expect( result, found, "found a small-font glyph in atlas region A or B" );
		if ( found ) {
			ve_fontcache_backend_test_atlas_sizes before = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			(void) ve_fontcache_backend_test_draw( result, options.cache, options.small_font, ve_fontcache_backend_test_codepoint_to_utf8( codepoint ) );
			ve_fontcache_backend_test_atlas_sizes after = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			char actual_region = ve_fontcache_backend_test_growing_region( before, after );
			ve_fontcache_backend_test_expect(
				result,
				actual_region == 'A' || actual_region == 'B',
				"small-font glyph routed into atlas region A or B" );
		}
	} else {
		ve_fontcache_backend_test_skip( result, "small-font routing skipped: small_font not supplied" );
	}

	if ( options.latin_font >= 0 ) {
		char32_t codepoint = 0;
		char region = '\0';
		bool found = ve_fontcache_backend_test_find_uncached_codepoint_for_regions(
			options.cache,
			options.latin_font,
			"BC",
			0x20,
			0x00FF,
			codepoint,
			region );
		ve_fontcache_backend_test_expect( result, found, "found a latin glyph in atlas region B or C" );
		if ( found ) {
			ve_fontcache_backend_test_atlas_sizes before = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			(void) ve_fontcache_backend_test_draw( result, options.cache, options.latin_font, ve_fontcache_backend_test_codepoint_to_utf8( codepoint ) );
			ve_fontcache_backend_test_atlas_sizes after = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			char actual_region = ve_fontcache_backend_test_growing_region( before, after );
			ve_fontcache_backend_test_expect(
				result,
				actual_region == 'B' || actual_region == 'C',
				"standard latin glyph routed into atlas region B or C" );
		}
	} else {
		ve_fontcache_backend_test_skip( result, "latin-font routing skipped: latin_font not supplied" );
	}

	if ( options.cjk_font >= 0 ) {
		char32_t codepoint = 0;
		char region = '\0';
		bool found = ve_fontcache_backend_test_find_uncached_codepoint_for_regions(
			options.cache,
			options.cjk_font,
			"CD",
			0x4E00,
			0x9FFF,
			codepoint,
			region );
		ve_fontcache_backend_test_expect( result, found, "found a CJK glyph in atlas region C or D" );
		if ( found ) {
			ve_fontcache_backend_test_atlas_sizes before = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			(void) ve_fontcache_backend_test_draw( result, options.cache, options.cjk_font, ve_fontcache_backend_test_codepoint_to_utf8( codepoint ) );
			ve_fontcache_backend_test_atlas_sizes after = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			char actual_region = ve_fontcache_backend_test_growing_region( before, after );
			ve_fontcache_backend_test_expect(
				result,
				actual_region == 'C' || actual_region == 'D',
				"CJK glyph routed into atlas region C or D" );
		}
	} else {
		ve_fontcache_backend_test_skip( result, "CJK routing skipped: cjk_font not supplied" );
	}

	if ( options.huge_font >= 0 ) {
		ve_fontcache_backend_test_atlas_sizes before = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
		ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.huge_font, u8"W" );
		ve_fontcache_backend_test_atlas_sizes after = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
		ve_fontcache_backend_test_expect(
			result,
			ve_fontcache_backend_test_count_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ) > 0,
			"very large glyphs used the uncached target path" );
		ve_fontcache_backend_test_expect(
			result,
			after.region_A == before.region_A
				&& after.region_B == before.region_B
				&& after.region_C == before.region_C
				&& after.region_D == before.region_D,
			"very large glyphs skipped atlas insertion" );
	} else {
		ve_fontcache_backend_test_skip( result, "huge-font routing skipped: huge_font not supplied" );
	}
}

inline void ve_fontcache_backend_test_run_lru_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "atlas LRU checks skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	ve_fontcache_LRU_init( options.cache->atlas.stateA, VE_FONTCACHE_ATLAS_REGION_A_CAPACITY );
	ve_fontcache_LRU_init( options.cache->atlas.stateB, VE_FONTCACHE_ATLAS_REGION_B_CAPACITY );
	ve_fontcache_LRU_init( options.cache->atlas.stateC, VE_FONTCACHE_ATLAS_REGION_C_CAPACITY );
	ve_fontcache_LRU_init( options.cache->atlas.stateD, VE_FONTCACHE_ATLAS_REGION_D_CAPACITY );
	options.cache->atlas.next_atlas_idx_A = 0;
	options.cache->atlas.next_atlas_idx_B = 0;
	options.cache->atlas.next_atlas_idx_C = 0;
	options.cache->atlas.next_atlas_idx_D = 0;
	ve_fontcache_flush_drawlist( options.cache );

	struct candidate
	{
		ve_font_id font = -1;
		char region = '\0';
		size_t capacity = 0;
		char32_t start = 0;
		char32_t end = 0;
	};

	const candidate candidates[] = {
		{ options.small_font, 'A', static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_A_CAPACITY ), 0x4E00, 0x9FFF },
		{ options.small_font, 'B', static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_B_CAPACITY ), 0x20, 0x9FFF },
		{ options.cjk_font, 'C', static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_C_CAPACITY ), 0x4E00, 0x9FFF },
		{ options.cjk_font, 'D', static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_D_CAPACITY ), 0x4E00, 0x9FFF },
	};

	candidate chosen {};
	std::vector< char32_t > codepoints;
	for ( const candidate& current : candidates ) {
		if ( current.font < 0 ) {
			continue;
		}

		const size_t starting_size = ve_fontcache_backend_test_region_cache_size( *options.cache, current.region );
		if ( starting_size >= current.capacity ) {
			continue;
		}

		codepoints = ve_fontcache_backend_test_collect_codepoints_for_region(
			options.cache,
			current.font,
			current.region,
			( current.capacity - starting_size ) + 1,
			current.start,
			current.end );
		if ( codepoints.size() >= ( current.capacity - starting_size ) + 1 ) {
			chosen = current;
			break;
		}
	}

	if ( chosen.font < 0 || codepoints.empty() ) {
		ve_fontcache_backend_test_skip( result, "atlas LRU checks skipped: no region supplied enough glyphs" );
		return;
	}

	const size_t starting_size = ve_fontcache_backend_test_region_cache_size( *options.cache, chosen.region );
	const size_t fill_count = chosen.capacity - starting_size;
	if ( codepoints.size() < fill_count + 1 ) {
		ve_fontcache_backend_test_skip( result, "atlas LRU checks skipped: region was too warm to overflow reliably" );
		return;
	}

	for ( size_t i = 0; i < fill_count; i++ ) {
		(void) ve_fontcache_backend_test_draw(
			result,
			options.cache,
			chosen.font,
			ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ i ] ),
			false );
	}

	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_region_cache_size( *options.cache, chosen.region ) == chosen.capacity,
		"atlas region filled to its expected capacity" );

	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		chosen.font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ fill_count - 1 ] ),
		false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"most recently touched glyph stayed cached before overflow" );

	drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		chosen.font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ fill_count ] ),
		false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
		"overflow glyph triggered a fresh cache upload" );

	drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		chosen.font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ 0 ] ),
		false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
		"least recently used glyph was evicted and regenerated" );

	drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		chosen.font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ fill_count - 1 ] ),
		false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"recent glyph stayed cached after eviction pressure" );
}

inline void ve_fontcache_backend_test_run_edge_case_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"" );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"empty string emitted no cache-miss work" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"   " );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"whitespace-only text emitted no glyph upload work" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8" " );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"single space emitted no glyph upload work" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"\n" );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, false );

	drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		options.font,
		ve_fontcache_backend_test_make_ascii_string( 10000 ) );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );

	ve_fontcache_flush_drawlist( options.cache );
	size_t previous_dcalls = 0;
	bool monotonic = true;
	for ( int i = 0; i < 100; i++ ) {
		std::u8string label = u8"frame-";
		std::string suffix = std::to_string( i );
		for ( char ch : suffix ) {
			label.push_back( static_cast< char8_t >( ch ) );
		}
		ve_fontcache_backend_test_expect(
			result,
			ve_fontcache_draw_text( options.cache, options.font, label, 0.0f, 0.0f, 1.0f / 1920.0f, 1.0f / 1080.0f, false ),
			"draw_text succeeded without drawlist flush" );
		size_t current_dcalls = ve_fontcache_get_drawlist( options.cache )->dcalls.size();
		monotonic = monotonic && current_dcalls >= previous_dcalls;
		previous_dcalls = current_dcalls;
	}
	ve_fontcache_backend_test_expect( result, monotonic, "drawlist grew monotonically without per-call flushes" );
	ve_fontcache_flush_drawlist( options.cache );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"\u4E2D\u0627\u05D0\U0001F600" );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, false );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"micro", true, 0.0f, 0.0f, 0.0001f, 0.0001f );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"huge", true, 0.0f, 0.0f, 10.0f, 10.0f );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"negative", true, -1.0f, -1.0f );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );

	ve_fontcache_configure_snap( options.cache, 0, 0 );
	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"snap-zero" );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );
	ve_fontcache_backend_test_expect(
		result,
		!ve_fontcache_draw_text( options.cache, -1, u8"invalid" ),
		"invalid font id was rejected without crashing" );
}

inline void ve_fontcache_backend_test_run_reload_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !options.reload_font ) {
		ve_fontcache_backend_test_skip( result, "font unload and reload skipped: reload_font not supplied" );
		return;
	}

	(void) ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"Reload me" );
	ve_fontcache_unload( options.cache, options.font );
	ve_font_id reloaded_font = options.reload_font();
	ve_fontcache_backend_test_expect( result, reloaded_font >= 0, "font reload succeeded" );
	if ( reloaded_font < 0 ) {
		return;
	}

	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, reloaded_font, u8"Reload me" );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
		"unload and reload invalidated cached glyph state" );
}

inline void ve_fontcache_backend_test_run_readback_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "GPU readback checks skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	if ( !options.execute || !options.readback ) {
		ve_fontcache_backend_test_skip( result, "GPU readback checks skipped: execute/readback callbacks not supplied" );
		return;
	}

	ve_font_id font = options.small_font >= 0 ? options.small_font : options.font;
	char32_t codepoint = 0;
	char region = '\0';
	if ( !ve_fontcache_backend_test_find_uncached_codepoint_for_regions( options.cache, font, "ABCD", 0x20, 0x9FFF, codepoint, region ) ) {
		ve_fontcache_backend_test_skip( result, "GPU readback checks skipped: no atlas-backed glyph was found" );
		return;
	}

	int atlas_index = -1;
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_predict_atlas_slot( options.cache, font, codepoint, region, atlas_index ),
		"predicted an atlas slot for readback testing" );
	if ( atlas_index == -1 ) {
		return;
	}

	float atlas_x = 0.0f;
	float atlas_y = 0.0f;
	float atlas_w = 0.0f;
	float atlas_h = 0.0f;
	ve_fontcache_atlas_bbox( region, atlas_index, atlas_x, atlas_y, atlas_w, atlas_h );

	const int atlas_read_x = static_cast< int >( atlas_x );
	const int atlas_read_y = static_cast< int >( atlas_y );
	const int atlas_read_w = std::max( 1, static_cast< int >( atlas_w ) );
	const int atlas_read_h = std::max( 1, static_cast< int >( atlas_h ) );

	std::vector< uint8_t > atlas_before;
	std::vector< uint8_t > atlas_after;
	std::vector< uint8_t > atlas_again;
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture( options, "atlas", atlas_read_x, atlas_read_y, atlas_read_w, atlas_read_h, atlas_before ),
		"atlas readback before draw succeeded" );

	(void) ve_fontcache_backend_test_draw(
		result,
		options.cache,
		font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoint ),
		false );
	options.execute();

	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture( options, "atlas", atlas_read_x, atlas_read_y, atlas_read_w, atlas_read_h, atlas_after ),
		"atlas readback after draw succeeded" );
	ve_fontcache_backend_test_expect(
		result,
		atlas_after != atlas_before && ve_fontcache_backend_test_any_non_zero( atlas_after ),
		"atlas pixels changed and became non-zero after caching a glyph" );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_has_intermediate_coverage( atlas_after ),
		"atlas snapshot contained anti-aliased intermediate coverage values" );

	(void) ve_fontcache_backend_test_draw(
		result,
		options.cache,
		font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoint ),
		false );
	options.execute();
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture( options, "atlas", atlas_read_x, atlas_read_y, atlas_read_w, atlas_read_h, atlas_again ),
		"atlas readback on redraw succeeded" );
	ve_fontcache_backend_test_expect(
		result,
		atlas_after == atlas_again,
		"atlas snapshot stayed stable on redraw" );

	std::vector< uint8_t > glyph_buffer;
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture(
			options,
			"glyph_buffer",
			0,
			0,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT,
			glyph_buffer ),
		"glyph-buffer readback succeeded" );
	ve_fontcache_backend_test_expect(
		result,
		!ve_fontcache_backend_test_any_non_zero( glyph_buffer ),
		"glyph buffer was cleared after backend execution" );

	std::vector< uint8_t > target_pixels;
	const int target_w = options.cache->snap_width ? static_cast< int >( options.cache->snap_width ) : 1920;
	const int target_h = options.cache->snap_height ? static_cast< int >( options.cache->snap_height ) : 1080;
	(void) ve_fontcache_backend_test_draw( result, options.cache, font, u8"Target output" );
	options.execute();
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture( options, "target", 0, 0, target_w, target_h, target_pixels ),
		"target readback succeeded" );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_any_non_zero( target_pixels ),
		"target output contained visible text pixels" );
}

inline ve_fontcache_backend_test_result ve_fontcache_backend_test_run( const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_result result;
	ve_fontcache_backend_test_expect( result, options.cache != nullptr, "backend test received a cache" );
	if ( !options.cache ) {
		return result;
	}

	ve_fontcache_backend_test_expect( result, options.font >= 0, "backend test received a primary font" );
	if ( options.font < 0 ) {
		return result;
	}

	ve_fontcache_backend_test_run_structural_checks( result, options );
	ve_fontcache_backend_test_run_caching_checks( result, options );
	ve_fontcache_backend_test_run_readback_checks( result, options );
	ve_fontcache_backend_test_run_region_checks( result, options );
	ve_fontcache_backend_test_run_lru_checks( result, options );
	ve_fontcache_backend_test_run_edge_case_checks( result, options );
	ve_fontcache_backend_test_run_reload_checks( result, options );

	return result;
}

inline ve_fontcache_backend_test_result ve_fontcache_backend_test_run(
	ve_fontcache* cache,
	ve_font_id font,
	ve_fontcache_backend_execute_fn execute = {},
	ve_fontcache_backend_readback_fn readback = {} )
{
	ve_fontcache_backend_test_options options;
	options.cache = cache;
	options.font = font;
	options.execute = execute;
	options.readback = readback;
	return ve_fontcache_backend_test_run( options );
}
