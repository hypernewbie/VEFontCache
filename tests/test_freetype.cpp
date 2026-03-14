#include "test_common.h"

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION

UTEST( freetype, init_use_freetype_true )
{
	vefc_test::context ctx( true );
	EXPECT_TRUE( ctx.cache.use_freetype );
}

UTEST( freetype, init_false_keeps_current_default )
{
	vefc_test::context ctx( false );
	EXPECT_TRUE( ctx.cache.use_freetype );
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

#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
