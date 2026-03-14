#include "test_common.h"

UTEST( utf8, ascii_single_char )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ve_fontcache_enable_advanced_text_shaping( &ctx.cache, false );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );

	EXPECT_EQ( 1u, ctx.cache.shape_cache.storage[ 0 ].glyphs.size() );
}

UTEST( utf8, two_byte_sequence )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kNotoSansJP );

	ASSERT_GE( font, 0 );
	ve_fontcache_enable_advanced_text_shaping( &ctx.cache, false );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"é" ) );

	EXPECT_EQ( 1u, ctx.cache.shape_cache.storage[ 0 ].glyphs.size() );
}

UTEST( utf8, three_byte_sequence )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kNotoSansJP );

	ASSERT_GE( font, 0 );
	ve_fontcache_enable_advanced_text_shaping( &ctx.cache, false );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"日" ) );

	EXPECT_EQ( 1u, ctx.cache.shape_cache.storage[ 0 ].glyphs.size() );
}

UTEST( utf8, four_byte_sequence )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kNotoSansJP );
	std::u8string codepoint;

	ASSERT_GE( font, 0 );
	vefc_test::append_utf8( codepoint, 0x10000 );
	ve_fontcache_enable_advanced_text_shaping( &ctx.cache, false );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, codepoint ) );

	EXPECT_EQ( 1u, ctx.cache.shape_cache.storage[ 0 ].glyphs.size() );
}

UTEST( utf8, mixed_string_advances_cursor )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kNotoSansJP );

	ASSERT_GE( font, 0 );
	ve_fontcache_enable_advanced_text_shaping( &ctx.cache, false );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Hello世界!" ) );

	EXPECT_GT( ctx.cache.cursor_pos.x, 0.0f );
	EXPECT_GE( ctx.cache.shape_cache.storage[ 0 ].glyphs.size(), 8u );
}

UTEST( utf8, newline_is_handled )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ve_fontcache_enable_advanced_text_shaping( &ctx.cache, false );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Line1\nLine2" ) );

	EXPECT_TRUE( vefc_test::all_indices_in_range( *vefc_test::current_drawlist( ctx ) ) );
}
