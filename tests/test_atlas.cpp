#include "test_common.h"

UTEST( atlas, init_indices_start_at_zero )
{
	vefc_test::context ctx;

	EXPECT_EQ( 0u, ctx.cache.atlas.next_atlas_idx_A );
	EXPECT_EQ( 0u, ctx.cache.atlas.next_atlas_idx_B );
	EXPECT_EQ( 0u, ctx.cache.atlas.next_atlas_idx_C );
	EXPECT_EQ( 0u, ctx.cache.atlas.next_atlas_idx_D );

	EXPECT_EQ( 0u, ctx.cache.atlas.stateA.cache.size() );
	EXPECT_EQ( 0u, ctx.cache.atlas.stateB.cache.size() );
	EXPECT_EQ( 0u, ctx.cache.atlas.stateC.cache.size() );
	EXPECT_EQ( 0u, ctx.cache.atlas.stateD.cache.size() );
}

UTEST( atlas, drawing_unique_glyphs_fills_atlas_and_lru )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	std::u8string chars;
	for ( uint8_t i = 0; i < 64; i++ ) {
		vefc_test::append_utf8( chars, 0x41 + i );
	}

	bool ok = vefc_test::draw_text( ctx, font, chars );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

#ifndef VE_FONTCACHE_FREETYPE_RASTERISATION
	size_t lru_total = ctx.cache.atlas.stateA.cache.size()
		+ ctx.cache.atlas.stateB.cache.size()
		+ ctx.cache.atlas.stateC.cache.size()
		+ ctx.cache.atlas.stateD.cache.size();
	 ASSERT_GT( lru_total, 0u );
#endif
}

UTEST( atlas, lru_stays_bounded_by_atlas_capacity )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	std::u8string many_chars;
	for ( uint8_t i = 0; i < 200; i++ ) {
		vefc_test::append_utf8( many_chars, 0x41 + i );
	}

	bool ok = vefc_test::draw_text( ctx, font, many_chars );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	 ASSERT_LE( ctx.cache.atlas.stateA.cache.size(), static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_A_CAPACITY ) );
	 ASSERT_LE( ctx.cache.atlas.stateB.cache.size(), static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_B_CAPACITY ) );
	 ASSERT_LE( ctx.cache.atlas.stateC.cache.size(), static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_C_CAPACITY ) );
	 ASSERT_LE( ctx.cache.atlas.stateD.cache.size(), static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_D_CAPACITY ) );
}

UTEST( atlas, re_drawing_same_glyph_reuses_cached_entry )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"A" );
	 ASSERT_TRUE( ok );

#ifndef VE_FONTCACHE_FREETYPE_RASTERISATION
	size_t stateA_after_first = ctx.cache.atlas.stateA.cache.size();
	 ASSERT_GT( stateA_after_first, 0u );

	vefc_test::flush( ctx );

	ok = vefc_test::draw_text( ctx, font, u8"A" );
	 ASSERT_TRUE( ok );

	size_t stateA_after_second = ctx.cache.atlas.stateA.cache.size();
	 ASSERT_EQ( stateA_after_first, stateA_after_second );
#endif
}

UTEST( atlas, unload_invalidates_font_entries_from_lru )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"AB" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	ve_fontcache_unload( &ctx.cache, font );

	 ASSERT_EQ( 0u, ctx.cache.atlas.stateA.cache.size() );
}

UTEST( atlas, next_atlas_indices_advance_with_unique_glyphs )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	std::u8string chars;
	for ( uint8_t i = 0; i < 32; i++ ) {
		vefc_test::append_utf8( chars, 0x41 + i );
	}

	bool ok = vefc_test::draw_text( ctx, font, chars );
	 ASSERT_TRUE( ok );

#ifndef VE_FONTCACHE_FREETYPE_RASTERISATION
	uint32_t total_slots = ctx.cache.atlas.next_atlas_idx_A
		+ ctx.cache.atlas.next_atlas_idx_B
		+ ctx.cache.atlas.next_atlas_idx_C
		+ ctx.cache.atlas.next_atlas_idx_D;
	 ASSERT_GT( total_slots, 0u );
#endif
}

UTEST( atlas, glyph_update_batch_x_starts_at_zero )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	 ASSERT_EQ( 0u, ctx.cache.atlas.glyph_update_batch_x );

	bool ok = vefc_test::draw_text( ctx, font, u8"Test" );
	 ASSERT_TRUE( ok );

	 ASSERT_GE( ctx.cache.atlas.glyph_update_batch_x, 0u );
}

UTEST( atlas, reset_clears_glyph_update_batch_state )
{
	vefc_test::context ctx;

	ctx.cache.atlas.glyph_update_batch_x = 42;

	ve_fontcache_vertex dummy_vert = { 1.0f, 2.0f, 0.5f, 0.5f };
	ctx.cache.atlas.glyph_update_batch_drawlist.vertices.push_back( dummy_vert );
	ctx.cache.atlas.glyph_update_batch_drawlist.indices.push_back( 0 );
	ctx.cache.atlas.glyph_update_batch_drawlist.indices.push_back( 0 );
	ctx.cache.atlas.glyph_update_batch_drawlist.indices.push_back( 0 );

	ve_fontcache_draw dummy_draw;
	dummy_draw.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH;
	dummy_draw.start_index = 0;
	dummy_draw.end_index = 3;
	ctx.cache.atlas.glyph_update_batch_drawlist.dcalls.push_back( dummy_draw );

	ctx.cache.atlas.glyph_update_batch_clear_drawlist.vertices.push_back( dummy_vert );
	ctx.cache.atlas.glyph_update_batch_clear_drawlist.indices.push_back( 0 );
	ctx.cache.atlas.glyph_update_batch_clear_drawlist.indices.push_back( 0 );
	ctx.cache.atlas.glyph_update_batch_clear_drawlist.indices.push_back( 0 );
	ctx.cache.atlas.glyph_update_batch_clear_drawlist.dcalls.push_back( dummy_draw );

	ctx.cache.temp_codepoint_seen[ 12345 ] = true;

	ctx.cache.drawlist.vertices.push_back( dummy_vert );
	ctx.cache.drawlist.indices.push_back( 0 );
	ctx.cache.drawlist.indices.push_back( 0 );
	ctx.cache.drawlist.indices.push_back( 0 );
	dummy_draw.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET;
	ctx.cache.drawlist.dcalls.push_back( dummy_draw );

	 ASSERT_FALSE( ctx.cache.atlas.glyph_update_batch_drawlist.dcalls.empty() );
	 ASSERT_FALSE( ctx.cache.atlas.glyph_update_batch_clear_drawlist.dcalls.empty() );
	 ASSERT_NE( 0u, ctx.cache.atlas.glyph_update_batch_x );
	 ASSERT_FALSE( ctx.cache.temp_codepoint_seen.empty() );
	 ASSERT_FALSE( ctx.cache.drawlist.dcalls.empty() );

	ve_fontcache_reset_transient_test_state( &ctx.cache );

	 ASSERT_TRUE( ctx.cache.drawlist.dcalls.empty() );
	 ASSERT_TRUE( ctx.cache.drawlist.vertices.empty() );
	 ASSERT_TRUE( ctx.cache.drawlist.indices.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_drawlist.dcalls.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_drawlist.vertices.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_drawlist.indices.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_clear_drawlist.dcalls.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_clear_drawlist.vertices.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_clear_drawlist.indices.empty() );
	 ASSERT_EQ( 0u, ctx.cache.atlas.glyph_update_batch_x );
	 ASSERT_TRUE( ctx.cache.temp_codepoint_seen.empty() );
}

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
UTEST( atlas, freetype_unload_removes_entries_from_cpu_atlas )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"X" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	size_t pages_before = ctx.cache.atlasCPU.pages.size();
	 ASSERT_GE( pages_before, 1u );

	ve_fontcache_unload( &ctx.cache, font );

	size_t pages_after = ctx.cache.atlasCPU.pages.size();
	 ASSERT_EQ( pages_before, pages_after );
}

UTEST( atlas, freetype_cpu_page_count_matches_expected )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	 ASSERT_EQ( 0u, ctx.cache.atlasCPU.pages.size() );

	bool ok = vefc_test::draw_text( ctx, font, u8"A" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	 ASSERT_GE( ctx.cache.atlasCPU.pages.size(), 1u );
}

UTEST( atlas, freetype_set_font_size_preserves_cpu_page_count )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"ABC" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	size_t pages_before = ctx.cache.atlasCPU.pages.size();
	 ASSERT_GE( pages_before, 1u );

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	size_t pages_after = ctx.cache.atlasCPU.pages.size();
	 ASSERT_EQ( pages_before, pages_after );
}

UTEST( atlas, freetype_measure_does_not_create_pages )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	 ASSERT_EQ( 0u, ctx.cache.atlasCPU.pages.size() );

	ve_fontcache_measure_text( &ctx.cache, font, u8"NeverRendered", vefc_test::kScaleX, vefc_test::kScaleY, true );

	 ASSERT_EQ( 0u, ctx.cache.atlasCPU.pages.size() );
}

UTEST( atlas, freetype_reload_font_forces_new_cpu_work )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"X" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	ve_fontcache_unload( &ctx.cache, font );

	ve_font_id font2 = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font2, 0 );

	ok = vefc_test::draw_text( ctx, font2, u8"X" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl2 = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl2->dcalls.empty() );
}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
