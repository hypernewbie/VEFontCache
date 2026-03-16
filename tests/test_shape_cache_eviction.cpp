#include "test_common.h"

UTEST( shape_cache_eviction, evicted_entry_is_reshaped )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );

	constexpr int num_unique_strings = VE_FONTCACHE_SHAPECACHE_SIZE + 100;
	std::vector< std::u8string > strings;
	strings.reserve( num_unique_strings );

	// Generate unique strings to fill the cache
	for ( int i = 0; i < num_unique_strings; i++ ) {
		strings.push_back( vefc_test::make_ascii_string( static_cast< size_t >( i + 50 ) ) );
	}

	// Draw all strings, flushing between each to trigger shape cache entries
	for ( const auto& str : strings ) {
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, str ) );
		vefc_test::flush( ctx );
		EXPECT_LE( ctx.cache.shape_cache.state.cache.size(), static_cast< size_t >( VE_FONTCACHE_SHAPECACHE_SIZE ) );
	}

	// Now draw the first string again (should have been evicted)
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, strings[ 0 ] ) );
	vefc_test::flush( ctx );

	auto* drawlist = vefc_test::current_drawlist( ctx );
	EXPECT_GT( drawlist->dcalls.size(), 0u );
	EXPECT_TRUE( vefc_test::all_indices_in_range( *drawlist ) );
	EXPECT_TRUE( vefc_test::all_vertices_finite( *drawlist ) );

	// Verify cursor position is valid after reshaping
	EXPECT_NE( ctx.cache.cursor_pos.x, -1.0f );
}

UTEST( shape_cache_eviction, cursor_pos_correct_after_eviction )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );

	// Draw a string and record its cursor position
	std::u8string test_string = u8"HelloWorld";
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, test_string ) );
	ctx.cache.cursor_pos.x += ctx.cache.snap_width; // Reset cursor for clean measurement
	vefc_test::flush( ctx );

	float first_cursor_x = ctx.cache.cursor_pos.x;

	// Fill cache until eviction happens
	constexpr int num_unique_strings = VE_FONTCACHE_SHAPECACHE_SIZE * 2;
	for ( int i = 0; i < num_unique_strings; i++ ) {
		std::u8string label = vefc_test::make_ascii_string( static_cast< size_t >( i + 1000 ) );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, label ) );
		vefc_test::flush( ctx );
	}

	// Draw the same string again (should have been evicted)
	ctx.cache.cursor_pos.x += ctx.cache.snap_width; // Reset cursor
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, test_string ) );
	vefc_test::flush( ctx );

	float second_cursor_x = ctx.cache.cursor_pos.x;

	// The cursor positions should match (same layout reproduced)
	EXPECT_TRUE( std::fabs( first_cursor_x - second_cursor_x ) <= 0.1f );
}

UTEST( shape_cache_eviction, next_cache_idx_wraps )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );

	uint32_t initial_next_idx = ctx.cache.shape_cache.next_cache_idx;

	// Draw unique strings until we fill the cache
	for ( int i = 0; i < VE_FONTCACHE_SHAPECACHE_SIZE; i++ ) {
		std::u8string label = vefc_test::make_ascii_string( static_cast< size_t >( i + 500 ) );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, label ) );
		vefc_test::flush( ctx );

		EXPECT_LE( ctx.cache.shape_cache.next_cache_idx, VE_FONTCACHE_SHAPECACHE_SIZE );
	}

	// After filling the cache, next_cache_idx should be at size (it wraps after this point)
	uint32_t after_fill_idx = ctx.cache.shape_cache.next_cache_idx;
	EXPECT_EQ( static_cast< uint32_t >( VE_FONTCACHE_SHAPECACHE_SIZE ), after_fill_idx );

	// Draw more strings - verify next_cache_idx doesn't exceed size (it wraps/reuses slots)
	for ( int i = 0; i < 50; i++ ) {
		std::u8string label = vefc_test::make_ascii_string( static_cast< size_t >( i + 10000 ) );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, label ) );
		vefc_test::flush( ctx );

		EXPECT_LE( ctx.cache.shape_cache.next_cache_idx, VE_FONTCACHE_SHAPECACHE_SIZE );
	}

	// Verify final state is still within bounds
	EXPECT_LE( ctx.cache.shape_cache.state.cache.size(), static_cast< size_t >( VE_FONTCACHE_SHAPECACHE_SIZE ) );
}
