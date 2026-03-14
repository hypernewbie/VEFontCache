#include "test_common.h"

UTEST( font_load, ttf_valid )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	EXPECT_TRUE( ctx.cache.entry[ font ].used );
	EXPECT_GT( ctx.cache.entry[ font ].size_scale, 0.0f );
}

UTEST( font_load, otf_valid )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kNotoSansJP );

	ASSERT_GE( font, 0 );
	EXPECT_TRUE( ctx.cache.entry[ font ].used );
}

UTEST( font_load, null_data )
{
	vefc_test::context ctx;
	EXPECT_EQ( -1, ve_fontcache_load( &ctx.cache, nullptr, 0, 24.0f ) );
}

UTEST( font_load, bad_file )
{
	vefc_test::context ctx;
	std::vector< uint8_t > buffer;

	EXPECT_EQ( -1, ve_fontcache_loadfile( &ctx.cache, "fonts/does-not-exist.ttf", buffer, 24.0f ) );
	EXPECT_EQ( 0u, buffer.size() );
}

UTEST( font_load, same_buffer_two_sizes )
{
	vefc_test::context ctx;
	ve_font_id font_small = ctx.load_file( vefc_test::kRoboto, 12.0f );

	ASSERT_GE( font_small, 0 );
	ve_font_id font_large = ctx.load_buffer_copy( 0, 48.0f );
	ASSERT_GE( font_large, 0 );

	EXPECT_NE( font_small, font_large );
	EXPECT_NE( ctx.cache.entry[ font_small ].size_scale, ctx.cache.entry[ font_large ].size_scale );
}

UTEST( font_load, unload_reuses_slot )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ve_fontcache_unload( &ctx.cache, font );
	EXPECT_FALSE( ctx.cache.entry[ font ].used );

	ve_font_id reloaded = ctx.load_file( vefc_test::kRoboto );
	EXPECT_EQ( font, reloaded );
	EXPECT_TRUE( ctx.cache.entry[ reloaded ].used );
}

UTEST( font_load, negative_size_uses_pixel_height )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, -19.0f );

	ASSERT_GE( font, 0 );
	EXPECT_GT( ctx.cache.entry[ font ].size_scale, 0.0f );
}

UTEST( font_load, configure_snap_and_colour )
{
	vefc_test::context ctx;
	float colour[ 4 ] = { 1.0f, 0.0f, 0.0f, 1.0f };

	ve_fontcache_configure_snap( &ctx.cache, 1920, 1080 );
	ve_fontcache_set_colour( &ctx.cache, colour );

	EXPECT_EQ( 1920u, ctx.cache.snap_width );
	EXPECT_EQ( 1080u, ctx.cache.snap_height );
	EXPECT_EQ( 1.0f, ctx.cache.colour[ 0 ] );
	EXPECT_EQ( 0.0f, ctx.cache.colour[ 1 ] );
	EXPECT_EQ( 0.0f, ctx.cache.colour[ 2 ] );
	EXPECT_EQ( 1.0f, ctx.cache.colour[ 3 ] );
}
