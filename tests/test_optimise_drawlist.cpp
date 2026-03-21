#include "test_common.h"

UTEST( optimise_drawlist, different_colours_not_merged )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );

	float red[ 4 ] = { 1.0f, 0.0f, 0.0f, 1.0f };
	float green[ 4 ] = { 0.0f, 1.0f, 0.0f, 1.0f };

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"B" ) );
	vefc_test::flush( ctx );

	ve_fontcache_set_colour( &ctx.cache, red );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );

	ve_fontcache_set_colour( &ctx.cache, green );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"B" ) );

	auto* drawlist_before = vefc_test::current_drawlist( ctx, false );
	int target_drawcalls_before = vefc_test::count_pass( *drawlist_before, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET );

	// Optimise the drawlist (should NOT merge because colours differ)
	ve_fontcache_optimise_drawlist( &ctx.cache );

	auto* drawlist_after = vefc_test::current_drawlist( ctx, false );
	int target_drawcalls_after = vefc_test::count_pass( *drawlist_after, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET );

	EXPECT_GE( target_drawcalls_after, 2 );
	EXPECT_EQ( target_drawcalls_before, target_drawcalls_after );
}

UTEST( optimise_drawlist, same_colour_consecutive_draws_merged )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );

	float red[ 4 ] = { 1.0f, 0.0f, 0.0f, 1.0f };

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"B" ) );
	vefc_test::flush( ctx );

	ve_fontcache_set_colour( &ctx.cache, red );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"B" ) );

	auto* drawlist_before = vefc_test::current_drawlist( ctx, false );
	int target_drawcalls_before = vefc_test::count_pass( *drawlist_before, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET );

	// Optimise the drawlist (should merge consecutive same-colour draws)
	ve_fontcache_optimise_drawlist( &ctx.cache );

	auto* drawlist_after = vefc_test::current_drawlist( ctx, false );
	int target_drawcalls_after = vefc_test::count_pass( *drawlist_after, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET );

	EXPECT_LT( target_drawcalls_after, target_drawcalls_before );
}

UTEST( optimise_drawlist, get_cursor_pos_api )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Hello" ) );
	vefc_test::flush( ctx );

	// Use the public API function, not ctx.cache.cursor_pos directly
	ve_fontcache_vec2 cursor = ve_fontcache_get_cursor_pos( &ctx.cache );

	EXPECT_GT( cursor.x, 0.0f );
}
