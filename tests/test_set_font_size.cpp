#include "test_common.h"

UTEST( set_font_size, changes_size_scale )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );

	ASSERT_GE( font, 0 );

	auto& entry = ctx.cache.entry[ font ];
	float initial_scale = entry.size_scale;
	EXPECT_EQ( 24.0f, entry.size );

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	EXPECT_NE( initial_scale, entry.size_scale );
	EXPECT_EQ( 48.0f, entry.size );
}

UTEST( set_font_size, produces_different_drawlist_after_resize )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Hello" ) );
	vefc_test::flush( ctx );

	auto& entry = ctx.cache.entry[ font ];
	float initial_size = entry.size;

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Hello" ) );
	vefc_test::flush( ctx );

	auto* drawlist = vefc_test::current_drawlist( ctx );
	EXPECT_GT( drawlist->dcalls.size(), 0u );
	EXPECT_TRUE( vefc_test::all_indices_in_range( *drawlist ) );
	EXPECT_TRUE( vefc_test::all_vertices_finite( *drawlist ) );

	EXPECT_EQ( 48.0f, entry.size );
}

UTEST( set_font_size, negative_size_uses_pixel_height )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );

	ASSERT_GE( font, 0 );

	auto& entry = ctx.cache.entry[ font ];
	float positive_scale = entry.size_scale;

	ve_fontcache_set_font_size( &ctx.cache, font, -24.0f );

	EXPECT_GT( entry.size_scale, 0.0f );
	EXPECT_NE( positive_scale, entry.size_scale );
}

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
UTEST( set_font_size, freetype_face_pixel_size_updated )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );

	ASSERT_GE( font, 0 );

	auto& entry = ctx.cache.entry[ font ];
	ASSERT_NE( nullptr, entry.fontface );

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	EXPECT_NE( nullptr, entry.fontface );
}
#endif