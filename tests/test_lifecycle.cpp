#include "test_common.h"

UTEST( lifecycle, draw_invalid_font_id_returns_false_and_preserves_state )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	float colour[ 4 ] = { 1.0f, 0.5f, 0.25f, 0.75f };
	ve_fontcache_set_colour( &ctx.cache, colour );
	ve_fontcache_configure_snap( &ctx.cache, 1920, 1080 );
	ctx.cache.cursor_pos = { 100.0f, 200.0f };

	ve_fontcache_drawlist* dl_before = vefc_test::current_drawlist( ctx );
	size_t verts_before = dl_before->vertices.size();

	bool result = ve_fontcache_draw_text( &ctx.cache, -1, u8"hello", 0.0f, 0.0f, vefc_test::kScaleX, vefc_test::kScaleY, true );
	EXPECT_FALSE( result );

	ve_fontcache_drawlist* dl_after = vefc_test::current_drawlist( ctx );
	EXPECT_EQ( verts_before, dl_after->vertices.size() );

	ve_fontcache_vec2 cursor = ve_fontcache_get_cursor_pos( &ctx.cache );
	EXPECT_EQ( 100.0f, cursor.x );
	EXPECT_EQ( 200.0f, cursor.y );
}

UTEST( lifecycle, measure_invalid_font_id_returns_zero_and_preserves_state )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ctx.cache.cursor_pos = { 999.0f, 888.0f };

	ve_fontcache_vec2 result = ve_fontcache_measure_text( &ctx.cache, -1, u8"hello", vefc_test::kScaleX, vefc_test::kScaleY, true );
	EXPECT_EQ( 0.0f, result.x );
	EXPECT_EQ( 0.0f, result.y );

	ve_fontcache_vec2 cursor = ve_fontcache_get_cursor_pos( &ctx.cache );
	EXPECT_EQ( 999.0f, cursor.x );
	EXPECT_EQ( 888.0f, cursor.y );
}

UTEST( lifecycle, unload_then_draw_fails_cleanly )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ve_fontcache_unload( &ctx.cache, font );

	bool result = ve_fontcache_draw_text( &ctx.cache, font, u8"hello", 0.0f, 0.0f, vefc_test::kScaleX, vefc_test::kScaleY, true );
	EXPECT_FALSE( result );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	EXPECT_TRUE( dl->dcalls.empty() );
}

UTEST( lifecycle, unload_then_measure_returns_zero )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ve_fontcache_unload( &ctx.cache, font );

	ve_fontcache_vec2 result = ve_fontcache_measure_text( &ctx.cache, font, u8"hello", vefc_test::kScaleX, vefc_test::kScaleY, true );
	EXPECT_EQ( 0.0f, result.x );
	EXPECT_EQ( 0.0f, result.y );
}

UTEST( lifecycle, load_invalid_truncated_buffer_consumes_entry_slot )
{
	vefc_test::context ctx;
	ve_font_id font_before = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font_before, 0 );

	// Must be >= 6 bytes so stbtt_InitFont can safely read the num_tables
	// field at data+4 without a heap-buffer-overflow. With all bytes zero,
	// num_tables=0 so no further reads occur and the function fails cleanly.
	std::vector< uint8_t > bad_buffer( 16, 0 );
	ve_font_id bad_font = ve_fontcache_load( &ctx.cache, bad_buffer.data(), bad_buffer.size(), 24.0f );
	 ASSERT_EQ( -1, bad_font );

	 ASSERT_GE( ctx.cache.entry.size(), 2u );

	ve_font_id font_after = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font_after, 0 );
	 ASSERT_NE( font_after, font_before );
}

UTEST( lifecycle, load_zero_length_non_null_buffer_fails_cleanly )
{
	vefc_test::context ctx;
	std::vector< uint8_t > zero_buffer( 1024, 0 );
	ve_font_id font = ve_fontcache_load( &ctx.cache, zero_buffer.data(), 0, 24.0f );
	EXPECT_EQ( -1, font );
}

UTEST( lifecycle, shutdown_reinit_keeps_entries_but_marks_unused )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	 ASSERT_GE( font, 0 );

	ve_fontcache_shutdown( &ctx.cache );
	ve_fontcache_init( &ctx.cache, vefc_test::default_use_freetype() );

	 ASSERT_GE( ctx.cache.entry.size(), 1u );
	 ASSERT_FALSE( ctx.cache.entry[ 0 ].used );
}

UTEST( lifecycle, reset_transient_clears_drawlists_and_temp_codepoint )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ctx.cache.temp_codepoint_seen[ 0x41 ] = true;
	ctx.cache.temp_codepoint_seen[ 0x42 ] = true;
	ctx.cache.temp_codepoint_seen[ 0x43 ] = true;
	ctx.cache.atlas.glyph_update_batch_x = 99;

	ve_fontcache_draw_text( &ctx.cache, font, u8"test", 0.0f, 0.0f, vefc_test::kScaleX, vefc_test::kScaleY, true );
	ve_fontcache_drawlist* dl = ve_fontcache_get_drawlist( &ctx.cache );
	ASSERT_FALSE( dl->dcalls.empty() );

	ve_fontcache_reset_transient_test_state( &ctx.cache );

	ve_fontcache_drawlist* dl_after = ve_fontcache_get_drawlist( &ctx.cache );
	EXPECT_TRUE( dl_after->dcalls.empty() );
	EXPECT_TRUE( dl_after->vertices.empty() );
	EXPECT_TRUE( ctx.cache.temp_codepoint_seen.empty() );

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	EXPECT_TRUE( ctx.cache.atlasCPU.drawlist.dcalls.empty() );
	EXPECT_TRUE( ctx.cache.atlasCPU.drawlist.vertices.empty() );
#endif
	EXPECT_TRUE( ctx.cache.atlas.glyph_update_batch_drawlist.dcalls.empty() );
	EXPECT_TRUE( ctx.cache.atlas.glyph_update_batch_clear_drawlist.dcalls.empty() );
	EXPECT_EQ( 0u, ctx.cache.atlas.glyph_update_batch_x );
}

UTEST( lifecycle, load_font_after_shutdown_reinit_works )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	ve_fontcache_shutdown( &ctx.cache );
	ve_fontcache_init( &ctx.cache, vefc_test::default_use_freetype() );

	ve_font_id font2 = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font2, 0 );

	bool ok = ve_fontcache_draw_text( &ctx.cache, font2, u8"hello", 0.0f, 0.0f, vefc_test::kScaleX, vefc_test::kScaleY, true );
	 ASSERT_TRUE( ok );
	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	ASSERT_FALSE( dl->dcalls.empty() );
}

UTEST( lifecycle, reset_transient_preserves_non_transient_state )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"warmup" );
	 ASSERT_TRUE( ok );
	vefc_test::flush( ctx );

	size_t shape_cache_count_before = ctx.cache.shape_cache.state.cache.size();
	 ASSERT_GT( shape_cache_count_before, 0u );

	ctx.cache.cursor_pos = { 111.0f, 222.0f };

	float colour[ 4 ] = { 0.1f, 0.2f, 0.3f, 0.4f };
	ve_fontcache_set_colour( &ctx.cache, colour );
	ve_fontcache_configure_snap( &ctx.cache, 1920, 1080 );

	ve_fontcache_vertex dummy_vert = { 1.0f, 2.0f, 0.5f, 0.5f };
	ctx.cache.drawlist.vertices.push_back( dummy_vert );
	ctx.cache.drawlist.indices.push_back( 0 );
	ctx.cache.drawlist.indices.push_back( 0 );
	ctx.cache.drawlist.indices.push_back( 0 );
	ve_fontcache_draw dummy_draw = {};
	dummy_draw.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET;
	dummy_draw.start_index = 0;
	dummy_draw.end_index = 3;
	ctx.cache.drawlist.dcalls.push_back( dummy_draw );

	ctx.cache.atlas.glyph_update_batch_x = 99;
	ctx.cache.atlas.glyph_update_batch_drawlist.vertices.push_back( dummy_vert );
	ctx.cache.atlas.glyph_update_batch_drawlist.dcalls.push_back( dummy_draw );
	ctx.cache.temp_codepoint_seen[ 99999 ] = true;

	 ASSERT_FALSE( ctx.cache.drawlist.dcalls.empty() );
	 ASSERT_FALSE( ctx.cache.atlas.glyph_update_batch_drawlist.dcalls.empty() );
	 ASSERT_NE( 0u, ctx.cache.atlas.glyph_update_batch_x );
	 ASSERT_FALSE( ctx.cache.temp_codepoint_seen.empty() );

	ve_fontcache_reset_transient_test_state( &ctx.cache );

	 ASSERT_TRUE( ctx.cache.drawlist.dcalls.empty() );
	 ASSERT_TRUE( ctx.cache.drawlist.vertices.empty() );
	 ASSERT_TRUE( ctx.cache.drawlist.indices.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_drawlist.dcalls.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_drawlist.vertices.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_drawlist.indices.empty() );
	 ASSERT_TRUE( ctx.cache.atlas.glyph_update_batch_clear_drawlist.dcalls.empty() );
	 ASSERT_EQ( 0u, ctx.cache.atlas.glyph_update_batch_x );
	 ASSERT_TRUE( ctx.cache.temp_codepoint_seen.empty() );

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	 ASSERT_TRUE( ctx.cache.atlasCPU.drawlist.dcalls.empty() );
	 ASSERT_TRUE( ctx.cache.atlasCPU.drawlist.vertices.empty() );
#endif

	 ASSERT_TRUE( ve_fontcache_is_valid_font_id( &ctx.cache, font ) );
	 ASSERT_EQ( shape_cache_count_before, ctx.cache.shape_cache.state.cache.size() );
	 ASSERT_EQ( 111.0f, ctx.cache.cursor_pos.x );
	 ASSERT_EQ( 222.0f, ctx.cache.cursor_pos.y );
}
