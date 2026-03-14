#pragma once

#include <climits>
#include <cmath>

#include "ve_fontcache.h"

struct ve_fontcache_backend_test_result
{
	int passed = 0;
	int failed = 0;
	const char* last_failure = nullptr;
};

inline void ve_fontcache_backend_test_expect( ve_fontcache_backend_test_result& result, bool cond, const char* msg )
{
	if ( cond ) {
		result.passed++;
		return;
	}

	result.failed++;
	result.last_failure = msg;
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

inline ve_fontcache_backend_test_result ve_fontcache_backend_test_run( ve_fontcache* cache, ve_font_id font )
{
	ve_fontcache_backend_test_result result;
	const std::u8string text = u8"Hello, World! 日本語";

	ve_fontcache_flush_drawlist( cache );
	ve_fontcache_backend_test_expect( result,
		ve_fontcache_draw_text( cache, font, text, 0.1f, 0.5f, 1.0f / 1920.0f, 1.0f / 1080.0f ),
		"draw_text should succeed" );
	ve_fontcache_optimise_drawlist( cache );

	ve_fontcache_drawlist* drawlist = ve_fontcache_get_drawlist( cache );
	ve_fontcache_backend_test_expect( result, !drawlist->dcalls.empty(), "drawlist should not be empty" );

	int last_glyph_idx = -1;
	int first_atlas_idx = INT_MAX;
	bool has_target = false;

	for ( size_t i = 0; i < drawlist->indices.size(); i++ ) {
		ve_fontcache_backend_test_expect( result, drawlist->indices[ i ] < drawlist->vertices.size(), "index out of vertex range" );
	}

	for ( size_t i = 0; i < drawlist->vertices.size(); i++ ) {
		const ve_fontcache_vertex& v = drawlist->vertices[ i ];
		ve_fontcache_backend_test_expect( result, std::isfinite( v.x ), "vertex x should be finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( v.y ), "vertex y should be finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( v.u ), "vertex u should be finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( v.v ), "vertex v should be finite" );
	}

	for ( size_t i = 0; i < drawlist->dcalls.size(); i++ ) {
		const ve_fontcache_draw& draw = drawlist->dcalls[ i ];
		ve_fontcache_backend_test_expect( result, ve_fontcache_backend_test_is_known_pass( draw.pass ), "unknown draw pass" );
		ve_fontcache_backend_test_expect( result, draw.end_index >= draw.start_index, "draw index range should be monotonic" );
		ve_fontcache_backend_test_expect( result, draw.end_index <= drawlist->indices.size(), "draw index range should stay in bounds" );

		if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH && draw.end_index > draw.start_index ) {
			last_glyph_idx = static_cast< int >( i );
		}

		if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS && first_atlas_idx == INT_MAX ) {
			first_atlas_idx = static_cast< int >( i );
		}

		if ( ve_fontcache_backend_test_is_target_pass( draw.pass ) ) {
			has_target = true;
			for ( uint32_t idx = draw.start_index; idx < draw.end_index; idx++ ) {
				const ve_fontcache_vertex& v = drawlist->vertices[ drawlist->indices[ idx ] ];
				ve_fontcache_backend_test_expect( result, v.u >= -0.01f && v.u <= 1.01f, "target u should stay near [0, 1]" );
				ve_fontcache_backend_test_expect( result, v.v >= -0.01f && v.v <= 1.01f, "target v should stay near [0, 1]" );
			}
		}
	}

	ve_fontcache_backend_test_expect( result, has_target, "drawlist should contain a target pass" );
	if ( last_glyph_idx != -1 && first_atlas_idx != INT_MAX ) {
		ve_fontcache_backend_test_expect( result, last_glyph_idx < first_atlas_idx, "glyph pass should precede atlas pass" );
	}

	ve_fontcache_flush_drawlist( cache );
	ve_fontcache_draw_text( cache, font, text, 0.1f, 0.5f, 1.0f / 1920.0f, 1.0f / 1080.0f );
	ve_fontcache_optimise_drawlist( cache );
	drawlist = ve_fontcache_get_drawlist( cache );

	int glyph_pass_count = 0;
	for ( const ve_fontcache_draw& draw : drawlist->dcalls ) {
		if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH && draw.end_index > draw.start_index ) {
			glyph_pass_count++;
		}
	}
	ve_fontcache_backend_test_expect( result, glyph_pass_count == 0, "cached redraw should not emit glyph passes" );
	ve_fontcache_flush_drawlist( cache );

	return result;
}
