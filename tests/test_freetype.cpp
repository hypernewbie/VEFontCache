#include "test_common.h"

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION

UTEST( freetype, init_use_freetype_true )
{
	vefc_test::context ctx( true );
	EXPECT_TRUE( ctx.cache.use_freetype );
}

UTEST( freetype, init_false_disables_freetype )
{
	vefc_test::context ctx( false );
	EXPECT_FALSE( ctx.cache.use_freetype );
}

UTEST( freetype, load_font_creates_fontface )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	EXPECT_TRUE( ctx.cache.entry[ font ].fontface != nullptr );
}

UTEST( freetype, draw_text_creates_cpu_atlas_data )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );

	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );
	EXPECT_FALSE( ctx.cache.atlasCPU.pages.empty() );
	EXPECT_TRUE( vefc_test::has_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) );
	EXPECT_FALSE( ctx.cache.atlasCPU.pages[ 0 ]->cache.empty() );
}

UTEST( freetype, unload_clears_fontface )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( ctx.cache.entry[ font ].fontface != nullptr );
	ve_fontcache_unload( &ctx.cache, font );
	EXPECT_TRUE( ctx.cache.entry[ font ].fontface == nullptr );
}

UTEST( freetype, second_draw_is_cpu_cached )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	vefc_test::current_drawlist( ctx );

	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );

	EXPECT_EQ( 0, vefc_test::count_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ) );
	EXPECT_EQ( 0, vefc_test::count_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD ) );
	EXPECT_TRUE(
		vefc_test::has_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED )
		|| vefc_test::has_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET ) );
}

UTEST( freetype, cpu_cached_draw_uses_consistent_page_for_same_glyph )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"X" ) );
	ve_fontcache_drawlist* dl1 = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl1->dcalls.empty() );

	uint32_t page_first = dl1->dcalls[ 0 ].atlas_page;

	vefc_test::flush( ctx );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"X" ) );
	ve_fontcache_drawlist* dl2 = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl2->dcalls.empty() );

	uint32_t page_second = dl2->dcalls[ 0 ].atlas_page;
	 ASSERT_EQ( page_first, page_second );
}

UTEST( freetype, multipage_growth_under_many_unique_glyphs )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	std::u8string many;
	for ( uint8_t i = 0; i < 100; i++ ) {
		vefc_test::append_utf8( many, 0x41 + i );
	}

	bool ok = vefc_test::draw_text( ctx, font, many );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	size_t pages_after = ctx.cache.atlasCPU.pages.size();
	 ASSERT_GE( pages_after, 1u );
}

UTEST( freetype, unload_removes_font_entries_from_cpu_pages )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"A" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	size_t pages_before = ctx.cache.atlasCPU.pages.size();
	 ASSERT_GE( pages_before, 1u );

	ve_fontcache_unload( &ctx.cache, font );

	size_t pages_after = ctx.cache.atlasCPU.pages.size();
	 ASSERT_EQ( pages_before, pages_after );
}

UTEST( freetype, set_font_size_re_rasterises_cached_glyphs )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"Size" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl1 = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl1->dcalls.empty() );

	vefc_test::flush( ctx );

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	ok = vefc_test::draw_text( ctx, font, u8"Size" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl2 = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl2->dcalls.empty() );
}

UTEST( freetype, measure_does_not_create_pages_even_after_previous_draws )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"ABC" );
	 ASSERT_TRUE( ok );

	vefc_test::current_drawlist( ctx );
	vefc_test::flush( ctx );

	size_t pages_after_draw = ctx.cache.atlasCPU.pages.size();
	 ASSERT_GE( pages_after_draw, 1u );

	ve_fontcache_measure_text( &ctx.cache, font, u8"XYZ", vefc_test::kScaleX, vefc_test::kScaleY, true );

	 ASSERT_EQ( pages_after_draw, ctx.cache.atlasCPU.pages.size() );
}

UTEST( freetype, reset_transient_test_state_does_not_destroy_cpu_page_cache )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"Reset" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	size_t pages_before = ctx.cache.atlasCPU.pages.size();
	 ASSERT_GE( pages_before, 1u );

	ve_fontcache_reset_transient_test_state( &ctx.cache );

	size_t pages_after = ctx.cache.atlasCPU.pages.size();
	 ASSERT_EQ( pages_before, pages_after );
}

UTEST( freetype, reload_font_forces_new_cpu_upload_work )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"A" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );

	ve_fontcache_unload( &ctx.cache, font );

	ve_font_id font2 = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font2, 0 );

	ok = vefc_test::draw_text( ctx, font2, u8"A" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl2 = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl2->dcalls.empty() );
}

#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
