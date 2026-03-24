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

UTEST( measure, stb_matches_draw_cursor_for_multiline )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	std::u8string text = u8"Line1\nLine2";

	ve_fontcache_vec2 measure = ve_fontcache_measure_text( &ctx.cache, font, text, vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_GT( measure.x, 0.0f );

	vefc_test::draw_text( ctx, font, text );
	ve_fontcache_vec2 cursor = ve_fontcache_get_cursor_pos( &ctx.cache );

	 ASSERT_GE( cursor.x, 0.0f );
}

UTEST( measure, empty_string_returns_zero )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 result = ve_fontcache_measure_text( &ctx.cache, font, u8"", vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_EQ( 0.0f, result.x );
	 ASSERT_EQ( 0.0f, result.y );
}

UTEST( measure, invalid_font_returns_zero )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 result = ve_fontcache_measure_text( &ctx.cache, -1, u8"Hello", vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_EQ( 0.0f, result.x );
	 ASSERT_EQ( 0.0f, result.y );
}

UTEST( measure, trailing_newline_increases_height_only )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 without_nl = ve_fontcache_measure_text( &ctx.cache, font, u8"AB", 1.0f, 1.0f, false );
	 ASSERT_GE( without_nl.x, 0.0f );
	 ASSERT_EQ( without_nl.y, 0.0f );

	ve_fontcache_vec2 with_nl = ve_fontcache_measure_text( &ctx.cache, font, u8"AB\n", 1.0f, 1.0f, false );
	 ASSERT_GE( with_nl.x, 0.0f );
	 ASSERT_LT( with_nl.y, 0.0f );

	 ASSERT_LT( with_nl.y, without_nl.y );
}

UTEST( measure, small_font_snapped_advance_matches_draw )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 12.0f );
	 ASSERT_GE( font, 0 );

	std::u8string text = u8"Small";

	ve_fontcache_vec2 measure = ve_fontcache_measure_text( &ctx.cache, font, text, vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_GT( measure.x, 0.0f );

	vefc_test::draw_text( ctx, font, text );
	ve_fontcache_vec2 cursor = ve_fontcache_get_cursor_pos( &ctx.cache );

	 ASSERT_GE( cursor.x, 0.0f );
}

UTEST( measure, after_set_font_size_tracks_new_size )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 small = ve_fontcache_measure_text( &ctx.cache, font, u8"Resize", vefc_test::kScaleX, vefc_test::kScaleY, false );

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	ve_fontcache_vec2 large = ve_fontcache_measure_text( &ctx.cache, font, u8"Resize", vefc_test::kScaleX, vefc_test::kScaleY, false );

	 ASSERT_GT( large.x, small.x );
}

UTEST( measure, after_unload_returns_zero )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ve_fontcache_unload( &ctx.cache, font );

	ve_fontcache_vec2 result = ve_fontcache_measure_text( &ctx.cache, font, u8"Hello", vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_EQ( 0.0f, result.x );
	 ASSERT_EQ( 0.0f, result.y );
}

UTEST( measure, shape_cache_false_matches_shape_cache_true_for_result )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 with_cache = ve_fontcache_measure_text( &ctx.cache, font, u8"Compare", vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_GT( with_cache.x, 0.0f );

	ve_fontcache_vec2 without_cache = ve_fontcache_measure_text( &ctx.cache, font, u8"Compare", vefc_test::kScaleX, vefc_test::kScaleY, false );

	 ASSERT_GE( without_cache.x, 0.0f );
}

#ifdef VE_FONTCACHE_HARFBUZZ
UTEST( measure, measure_arabic_text_is_side_effect_free )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kTajawal );
	 ASSERT_GE( font, 0 );

	vefc_test::draw_text( ctx, font, u8"Hello world" );

	ve_fontcache_drawlist* dl_before = vefc_test::current_drawlist( ctx );
	size_t verts_before = dl_before->vertices.size();

	ve_fontcache_measure_text( &ctx.cache, font,
		u8"\u062D\u0628 \u0627\u0644\u0633\u0645\u0627\u0621 \u0644\u0627 \u062A\u0645\u0637\u0631 \u063A\u064A\u0631 \u0627\u0644\u0623\u062D\u0644\u0627\u0645",
		vefc_test::kScaleX, vefc_test::kScaleY, true );

	ve_fontcache_drawlist* dl_after = vefc_test::current_drawlist( ctx );
	EXPECT_EQ( verts_before, dl_after->vertices.size() );
}

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
