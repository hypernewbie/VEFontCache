#include "test_common.h"

UTEST( shape_cache, init_state )
{
	vefc_test::context ctx;

	EXPECT_EQ( 0u, ctx.cache.shape_cache.next_cache_idx );
	EXPECT_EQ( static_cast< size_t >( VE_FONTCACHE_SHAPECACHE_SIZE ), ctx.cache.shape_cache.storage.size() );
	EXPECT_EQ( 0u, ctx.cache.shape_cache.state.cache.size() );
}

UTEST( shape_cache, repeated_string_hits_cache )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"cache me" ) );
	ve_fontcache_vec2 first_cursor = ctx.cache.cursor_pos;
	EXPECT_EQ( 1u, ctx.cache.shape_cache.state.cache.size() );
	EXPECT_EQ( 1u, ctx.cache.shape_cache.next_cache_idx );

	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"cache me" ) );
	EXPECT_EQ( 1u, ctx.cache.shape_cache.state.cache.size() );
	EXPECT_EQ( 1u, ctx.cache.shape_cache.next_cache_idx );
	EXPECT_EQ( first_cursor.x, ctx.cache.cursor_pos.x );
}

UTEST( shape_cache, disable_bypasses_cache )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	for ( int i = 0; i < 8; i++ ) {
		std::string label = "nocache-" + std::to_string( i );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, vefc_test::to_u8string( label ), 0.0f, 0.0f, false ) );
		vefc_test::flush( ctx );
	}

	EXPECT_EQ( 0u, ctx.cache.shape_cache.state.cache.size() );
	EXPECT_EQ( 0u, ctx.cache.shape_cache.next_cache_idx );
}

UTEST( shape_cache, handles_max_and_over_length )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, vefc_test::make_ascii_string( VE_FONTCACHE_SHAPECACHE_MAX_LENGTH ) ) );
	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, vefc_test::make_ascii_string( VE_FONTCACHE_SHAPECACHE_MAX_LENGTH + 32 ) ) );

	EXPECT_LE( ctx.cache.shape_cache.state.cache.size(), static_cast< size_t >( VE_FONTCACHE_SHAPECACHE_SIZE ) );
}

UTEST( shape_cache, lru_eviction_stays_bounded )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	for ( int i = 0; i < 300; i++ ) {
		std::string label = "shape-" + std::to_string( i );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, vefc_test::to_u8string( label ) ) );
		vefc_test::flush( ctx );
	}

	EXPECT_LE( ctx.cache.shape_cache.state.cache.size(), static_cast< size_t >( VE_FONTCACHE_SHAPECACHE_SIZE ) );
	EXPECT_EQ( static_cast< uint32_t >( VE_FONTCACHE_SHAPECACHE_SIZE ), ctx.cache.shape_cache.next_cache_idx );
}
