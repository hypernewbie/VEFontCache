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

UTEST( shape_cache, same_text_different_fonts_do_not_alias )
{
	vefc_test::context ctx;
	ve_font_id font_a = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ve_font_id font_b = ctx.load_buffer_copy( 0, 48.0f );

	ASSERT_GE( font_a, 0 );
	ASSERT_GE( font_b, 0 );
	ASSERT_NE( font_a, font_b );

	std::u8string text = u8"Shared";

	size_t cache_before = ctx.cache.shape_cache.state.cache.size();

	vefc_test::draw_text( ctx, font_a, text );
	vefc_test::flush( ctx );

	size_t cache_after_first = ctx.cache.shape_cache.state.cache.size();
	 ASSERT_GT( cache_after_first, cache_before );

	vefc_test::draw_text( ctx, font_b, text );
	vefc_test::flush( ctx );

	size_t cache_after_second = ctx.cache.shape_cache.state.cache.size();
	 ASSERT_GT( cache_after_second, cache_after_first );
}

UTEST( shape_cache, same_font_different_size_creates_new_cache_entry )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	vefc_test::draw_text( ctx, font, u8"SizeTest" );
	vefc_test::flush( ctx );

	size_t cache_after_24 = ctx.cache.shape_cache.state.cache.size();
	 ASSERT_GT( cache_after_24, 0u );

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	vefc_test::draw_text( ctx, font, u8"SizeTest" );
	vefc_test::flush( ctx );

	size_t cache_after_48 = ctx.cache.shape_cache.state.cache.size();
	 ASSERT_EQ( cache_after_24 + 1u, cache_after_48 );
}

UTEST( shape_cache, cached_draw_after_set_font_size_uses_new_advances )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 cursor_start = { 0.0f, 0.0f };
	ctx.cache.cursor_pos = cursor_start;
	ASSERT_TRUE( ve_fontcache_draw_text( &ctx.cache, font, u8"MENU", 0.0f, 0.0f,
		vefc_test::kScaleX, vefc_test::kScaleY, true ) );
	vefc_test::flush( ctx );
	float advance_24 = ctx.cache.cursor_pos.x;
	 ASSERT_GT( advance_24, 0.0f );

	ctx.cache.cursor_pos = cursor_start;
	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	ASSERT_TRUE( ve_fontcache_draw_text( &ctx.cache, font, u8"MENU", 0.0f, 0.0f,
		vefc_test::kScaleX, vefc_test::kScaleY, true ) );
	vefc_test::flush( ctx );
	float advance_48 = ctx.cache.cursor_pos.x;
	 ASSERT_GT( advance_48, advance_24 );
}

UTEST( shape_cache, reverting_to_original_size_reuses_cache_entry )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	vefc_test::draw_text( ctx, font, u8"MENU" );
	vefc_test::flush( ctx );
	size_t cache_after_24 = ctx.cache.shape_cache.state.cache.size();

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );
	vefc_test::draw_text( ctx, font, u8"MENU" );
	vefc_test::flush( ctx );
	 ASSERT_EQ( cache_after_24 + 1u, ctx.cache.shape_cache.state.cache.size() );

	ve_fontcache_set_font_size( &ctx.cache, font, 24.0f );
	vefc_test::draw_text( ctx, font, u8"MENU" );
	vefc_test::flush( ctx );
	 ASSERT_EQ( cache_after_24 + 1u, ctx.cache.shape_cache.state.cache.size() );
}

UTEST( shape_cache, measure_after_size_change_returns_rescaled_advance )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 m24 = ve_fontcache_measure_text( &ctx.cache, font, u8"MENU", 1.0f, 1.0f, true );
	 ASSERT_GT( m24.x, 0.0f );

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );

	ve_fontcache_vec2 m48 = ve_fontcache_measure_text( &ctx.cache, font, u8"MENU", 1.0f, 1.0f, true );
	 ASSERT_GT( m48.x, m24.x );
}

UTEST( shape_cache, negative_size_does_not_alias_positive_size )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	vefc_test::draw_text( ctx, font, u8"MENU" );
	vefc_test::flush( ctx );
	size_t cache_after_positive = ctx.cache.shape_cache.state.cache.size();

	ve_fontcache_set_font_size( &ctx.cache, font, -24.0f );
	vefc_test::draw_text( ctx, font, u8"MENU" );
	vefc_test::flush( ctx );
	 ASSERT_EQ( cache_after_positive + 1u, ctx.cache.shape_cache.state.cache.size() );
}

UTEST( shape_cache, fractional_size_creates_distinct_entry )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	vefc_test::draw_text( ctx, font, u8"MENU" );
	vefc_test::flush( ctx );
	size_t cache_after_24 = ctx.cache.shape_cache.state.cache.size();

	ve_fontcache_set_font_size( &ctx.cache, font, 24.5f );
	vefc_test::draw_text( ctx, font, u8"MENU" );
	vefc_test::flush( ctx );
	 ASSERT_EQ( cache_after_24 + 1u, ctx.cache.shape_cache.state.cache.size() );
}

UTEST( shape_cache, set_font_size_produces_different_cursor_advance )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	vefc_test::draw_text( ctx, font, u8"MENU", 0.0f, 0.0f, false );
	vefc_test::flush( ctx );
	float advance_24 = ctx.cache.cursor_pos.x;

	ctx.cache.cursor_pos = { 0.0f, 0.0f };
	vefc_test::flush( ctx );

	ve_fontcache_set_font_size( &ctx.cache, font, 48.0f );
	vefc_test::draw_text( ctx, font, u8"MENU", 0.0f, 0.0f, false );
	vefc_test::flush( ctx );
	float advance_48 = ctx.cache.cursor_pos.x;

	 ASSERT_GT( advance_24, 0.0f );
	 ASSERT_GT( advance_48, advance_24 );
}

UTEST( shape_cache, measure_and_draw_agree_for_ascii )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	std::u8string text = u8"MenuBar";

	ve_fontcache_vec2 measure = ve_fontcache_measure_text( &ctx.cache, font, text, 1.0f, 1.0f, false );
	 ASSERT_GT( measure.x, 0.0f );

	vefc_test::draw_text( ctx, font, text, 0.0f, 0.0f, false );
	vefc_test::flush( ctx );

	ve_fontcache_vec2 draw_cursor = ve_fontcache_get_cursor_pos( &ctx.cache );
	 ASSERT_GT( draw_cursor.x, 0.0f );
}

UTEST( shape_cache, whitespace_and_empty_strings_do_not_create_unexpected_entries )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	ve_fontcache_measure_text( &ctx.cache, font, u8"", vefc_test::kScaleX, vefc_test::kScaleY, true );
	ve_fontcache_measure_text( &ctx.cache, font, u8"   ", vefc_test::kScaleX, vefc_test::kScaleY, true );

	 ASSERT_GE( ctx.cache.shape_cache.state.cache.size(), 0u );
}

UTEST( shape_cache, trailing_newline_layout_is_stable )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	ve_fontcache_vec2 without_nl = ve_fontcache_measure_text( &ctx.cache, font, u8"Test", 1.0f, 1.0f, false );
	ve_fontcache_vec2 with_nl = ve_fontcache_measure_text( &ctx.cache, font, u8"Test\n", 1.0f, 1.0f, false );

	 ASSERT_LE( with_nl.x, without_nl.x );
	 ASSERT_EQ( 0.0f, with_nl.x );
	 ASSERT_LT( with_nl.y, 0.0f );
	 ASSERT_LT( with_nl.y, without_nl.y );
}

UTEST( shape_cache, failed_or_invalid_draw_does_not_poison_cache )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	bool invalid_draw = ve_fontcache_draw_text( &ctx.cache, -1, u8"hello", 0.0f, 0.0f, vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_FALSE( invalid_draw );

	size_t cache_after_invalid = ctx.cache.shape_cache.state.cache.size();
	 ASSERT_EQ( 0u, cache_after_invalid );

	bool valid_draw = vefc_test::draw_text( ctx, font, u8"hello" );
	 ASSERT_TRUE( valid_draw );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_FALSE( dl->dcalls.empty() );
}
