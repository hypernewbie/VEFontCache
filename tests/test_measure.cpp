#include "test_common.h"

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION

UTEST( measure, measure_matches_draw_cursor_for_ascii )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kOpenSans );
	 ASSERT_GE( font, 0 );

	std::u8string text = u8"Main Menu";
	ve_fontcache_vec2 measure = ve_fontcache_measure_text( &ctx.cache, font, text, vefc_test::kScaleX, vefc_test::kScaleY, true );

	vefc_test::draw_text( ctx, font, text );
	ve_fontcache_vec2 cursor = ve_fontcache_get_cursor_pos( &ctx.cache );

	EXPECT_EQ( measure.x, cursor.x );
}

UTEST( measure, measure_does_not_mutate_main_drawlist )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kOpenSans );
	 ASSERT_GE( font, 0 );

	ve_fontcache_drawlist* dl_before = vefc_test::current_drawlist( ctx );
	size_t verts_before = dl_before->vertices.size();
	size_t idx_before   = dl_before->indices.size();
	size_t dcalls_before = dl_before->dcalls.size();

	ve_fontcache_measure_text( &ctx.cache, font, u8"Hello world", vefc_test::kScaleX, vefc_test::kScaleY, true );

	ve_fontcache_drawlist* dl_after = vefc_test::current_drawlist( ctx );
	EXPECT_EQ( verts_before, dl_after->vertices.size() );
	EXPECT_EQ( idx_before,   dl_after->indices.size() );
	EXPECT_EQ( dcalls_before, dl_after->dcalls.size() );
}

UTEST( measure, measure_does_not_mutate_cursor )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kOpenSans );
	 ASSERT_GE( font, 0 );

	ctx.cache.cursor_pos = { 999.0f, 888.0f };
	ve_fontcache_measure_text( &ctx.cache, font, u8"Hello", vefc_test::kScaleX, vefc_test::kScaleY, true );

	EXPECT_EQ( 999.0f, ctx.cache.cursor_pos.x );
	EXPECT_EQ( 888.0f, ctx.cache.cursor_pos.y );
}

UTEST( measure, measure_does_not_create_cpu_atlas_pages )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kOpenSans );
	 ASSERT_GE( font, 0 );

	ve_fontcache_measure_text( &ctx.cache, font, u8"X", vefc_test::kScaleX, vefc_test::kScaleY, true );

	EXPECT_EQ( 0u, ctx.cache.atlasCPU.pages.size() );
	EXPECT_TRUE( ctx.cache.atlasCPU.drawlist.dcalls.empty() );
}

UTEST( measure, draw_after_measure_still_emits_cpu_atlas_setup )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kOpenSans );
	 ASSERT_GE( font, 0 );

	ve_fontcache_measure_text( &ctx.cache, font, u8"X", vefc_test::kScaleX, vefc_test::kScaleY, true );

	bool draw_ok = vefc_test::draw_text( ctx, font, u8"X" );
	 ASSERT_TRUE( draw_ok );

	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );

	EXPECT_FALSE( ctx.cache.atlasCPU.pages.empty() );
	EXPECT_TRUE( vefc_test::has_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) );
}

UTEST( measure, repeat_measure_does_not_accumulate_hidden_state )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kOpenSans );
	 ASSERT_GE( font, 0 );

	ve_fontcache_measure_text( &ctx.cache, font, u8"Repeating", vefc_test::kScaleX, vefc_test::kScaleY, true );
	ve_fontcache_measure_text( &ctx.cache, font, u8"Repeating", vefc_test::kScaleX, vefc_test::kScaleY, true );
	ve_fontcache_measure_text( &ctx.cache, font, u8"Repeating", vefc_test::kScaleX, vefc_test::kScaleY, true );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	EXPECT_EQ( 0u, ctx.cache.atlasCPU.pages.size() );
	EXPECT_TRUE( ctx.cache.atlasCPU.drawlist.dcalls.empty() );
}

UTEST( measure, measure_multiline_returns_vertical_advance )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kOpenSans );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 multi = ve_fontcache_measure_text( &ctx.cache, font, u8"A\nA", vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_GE( multi.x, 0.0f );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_GE( dl->dcalls.size(), 0u );

	(void)multi;
}

#ifdef VE_FONTCACHE_HARFBUZZ
UTEST( measure, measure_harfbuzz_text_is_side_effect_free )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kOpenSans );
	 ASSERT_GE( font, 0 );

	vefc_test::draw_text( ctx, font, u8"Hello" );

	ve_fontcache_drawlist* dl_before = vefc_test::current_drawlist( ctx );
	size_t verts_before = dl_before->vertices.size();

	ve_fontcache_measure_text( &ctx.cache, font, u8"\u0646\u0627\u0645", vefc_test::kScaleX, vefc_test::kScaleY, true );

	ve_fontcache_drawlist* dl_after = vefc_test::current_drawlist( ctx );
	EXPECT_EQ( verts_before, dl_after->vertices.size() );
}
#endif // VE_FONTCACHE_HARFBUZZ

#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
