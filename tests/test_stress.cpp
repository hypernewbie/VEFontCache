#include "test_common.h"

UTEST( stress, many_unique_glyphs_stay_within_capacity )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kNotoSerifSC, 48.0f );

	ASSERT_GE( font, 0 );
	for ( int i = 0; i < 400; i++ ) {
		std::u8string glyph = vefc_test::make_cjk_string( 1, 0x4E00 + i );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, glyph ) );
		vefc_test::flush( ctx );
	}

	EXPECT_LE( ctx.cache.atlas.stateA.cache.size(), static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_A_CAPACITY ) );
	EXPECT_LE( ctx.cache.atlas.stateB.cache.size(), static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_B_CAPACITY ) );
	EXPECT_LE( ctx.cache.atlas.stateC.cache.size(), static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_C_CAPACITY ) );
	EXPECT_LE( ctx.cache.atlas.stateD.cache.size(), static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_D_CAPACITY ) );
}

UTEST( stress, flush_every_frame )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	std::minstd_rand rng( 321 );
	std::uniform_int_distribution< int > dist( 8, 24 );

	ASSERT_GE( font, 0 );
	for ( int frame = 0; frame < 100; frame++ ) {
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, vefc_test::make_ascii_string( dist( rng ) ) ) );
		vefc_test::current_drawlist( ctx );
		vefc_test::flush( ctx );
		EXPECT_TRUE( ve_fontcache_get_drawlist( &ctx.cache )->dcalls.empty() );
	}
}

UTEST( stress, load_unload_fonts )
{
	vefc_test::context ctx;

	for ( int i = 0; i < 32; i++ ) {
		ve_font_id font = ctx.load_file( vefc_test::kRoboto );
		ASSERT_GE( font, 0 );
		ve_fontcache_unload( &ctx.cache, font );
		EXPECT_FALSE( ctx.cache.entry[ font ].used );
	}
}

UTEST( stress, optimise_empty_drawlist )
{
	vefc_test::context ctx;
	ve_fontcache_optimise_drawlist( &ctx.cache );
	ve_fontcache_drawlist* drawlist = ve_fontcache_get_drawlist( &ctx.cache );
	EXPECT_TRUE( drawlist->vertices.empty() );
	EXPECT_TRUE( drawlist->indices.empty() );
	EXPECT_LE( drawlist->dcalls.size(), 1u );
	if ( !drawlist->dcalls.empty() ) {
		EXPECT_EQ( 0u, drawlist->dcalls.front().start_index );
		EXPECT_EQ( 0u, drawlist->dcalls.front().end_index );
	}
}

UTEST( stress, very_long_string )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, vefc_test::make_ascii_string( 4096 ) ) );

	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );
	EXPECT_FALSE( drawlist->vertices.empty() );
	EXPECT_TRUE( vefc_test::all_indices_in_range( *drawlist ) );
}

UTEST( stress, multi_font_same_frame )
{
	vefc_test::context ctx;
	const char* fonts[] = {
		vefc_test::kRoboto,
		vefc_test::kOpenSans,
		vefc_test::kNotoSansJP,
		vefc_test::kTajawal,
	};

	for ( const char* path : fonts ) {
		ve_font_id font = ctx.load_file( path );
		ASSERT_GE( font, 0 );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Frame text" ) );
	}

	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );
	EXPECT_FALSE( drawlist->dcalls.empty() );
	EXPECT_TRUE( vefc_test::all_indices_in_range( *drawlist ) );
}
