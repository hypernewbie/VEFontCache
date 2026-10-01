#include "test_common.h"

UTEST( glyph_size, glyph_and_size_do_not_cancel )
{
	ve_fontcache_entry entry = {};
	entry.font_id = 7;
	entry.size = 24.0f;
	uint64_t original = ve_fontcache_glyph_cache_key( entry, 36 );
	uint8_t* bytes = (uint8_t*) &entry.size;
	bytes[0] ^= ( 36 * 33 ) ^ ( 37 * 33 );
	ASSERT_NE( ve_fontcache_glyph_cache_key( entry, 37 ), original );
}

UTEST( glyph_size, same_size_reuses_raster )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
	ASSERT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 1 );

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, 24.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 0 );
	ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET ), 1 );
	ASSERT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 1 );
	ASSERT_EQ( ctx.cache.atlas.next_atlas_idx_A, (uint32_t) 1 );
}

UTEST( glyph_size, new_size_in_same_region_rasterizes )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
	ASSERT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 1 );
	float advance24 = ve_fontcache_get_cursor_pos( &ctx.cache ).x;

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, 30.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	EXPECT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
	EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 2 );
	EXPECT_EQ( ctx.cache.atlas.next_atlas_idx_A, (uint32_t) 2 );
	EXPECT_GT( ve_fontcache_get_cursor_pos( &ctx.cache ).x, advance24 );
}

UTEST( glyph_size, round_trip_reuses_original_slot )
{
	float sizes[] = { 24.0f, 30.0f };
	for ( int i = 0; i < 2; i++ )
	{
		vefc_test::context ctx( false );
		ve_font_id font = ctx.load_file( vefc_test::kRoboto, sizes[i] );
		ASSERT_GE( font, 0 );

		ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
		ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
		const ve_fontcache_draw* draw = vefc_test::find_first_target_draw( *dl );
		ASSERT_TRUE( draw != nullptr );
		float original_u = dl->vertices[dl->indices[draw->start_index]].u;
		ASSERT_EQ( vefc_test::count_pass( *dl, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );

		vefc_test::flush( ctx );
		ve_fontcache_set_font_size( &ctx.cache, font, sizes[1 - i] );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
		dl = vefc_test::current_drawlist( ctx );
		draw = vefc_test::find_first_target_draw( *dl );
		ASSERT_TRUE( draw != nullptr );
		EXPECT_NE( dl->vertices[dl->indices[draw->start_index]].u, original_u );
		EXPECT_EQ( vefc_test::count_pass( *dl, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
		EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 2 );

		vefc_test::flush( ctx );
		ve_fontcache_set_font_size( &ctx.cache, font, sizes[i] );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
		dl = vefc_test::current_drawlist( ctx );
		draw = vefc_test::find_first_target_draw( *dl );
		ASSERT_TRUE( draw != nullptr );
		EXPECT_EQ( dl->vertices[dl->indices[draw->start_index]].u, original_u );
		EXPECT_EQ( vefc_test::count_pass( *dl, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 0 );
		EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 2 );
		EXPECT_EQ( ctx.cache.atlas.next_atlas_idx_A, (uint32_t) 2 );
	}
}

UTEST( glyph_size, fractional_sizes_do_not_alias )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ASSERT_GE( font, 0 );

	float sizes[] = { 24.0f, 24.125f, 24.25f, 24.0f };
	for ( int i = 0; i < 4; i++ )
	{
		vefc_test::flush( ctx );
		ve_fontcache_set_font_size( &ctx.cache, font, sizes[i] );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
		EXPECT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), i < 3 ? 1 : 0 );
		EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) ( i < 3 ? i + 1 : 3 ) );
	}
}

UTEST( glyph_size, negative_size_is_distinct )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, -24.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	EXPECT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
	EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 2 );

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, 24.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	EXPECT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 0 );
	EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 2 );
}

UTEST( glyph_size, different_fonts_and_glyphs_stay_distinct )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ve_font_id other = ctx.load_buffer_copy( 0, 24.0f );
	ASSERT_GE( font, 0 );
	ASSERT_GE( other, 0 );
	ASSERT_NE( font, other );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );

	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, other, u8"A" ) );
	ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
	ASSERT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 2 );

	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"B" ) );
	ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
	ASSERT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 3 );
}

UTEST( glyph_size, resize_keeps_other_fonts_and_glyphs_cached )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ve_font_id other = ctx.load_buffer_copy( 0, 24.0f );
	ASSERT_GE( font, 0 );
	ASSERT_GE( other, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"AB" ) );
	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, other, u8"A" ) );
	ASSERT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 3 );

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, 30.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	EXPECT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
	EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 4 );

	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, other, u8"A" ) );
	EXPECT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 0 );
	EXPECT_EQ( ctx.cache.entry[other].size, 24.0f );

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, 24.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"AB" ) );
	EXPECT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 0 );
	EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 4 );
}

UTEST( glyph_size, shaping_cache_does_not_mask_size_identity )
{
	for ( int i = 0; i < 2; i++ )
	{
		vefc_test::context ctx( false );
		ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
		ASSERT_GE( font, 0 );

		ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A", 0, 0, i != 0 ) );
		ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );

		vefc_test::flush( ctx );
		ve_fontcache_set_font_size( &ctx.cache, font, 30.0f );
		ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A", 0, 0, i != 0 ) );
		EXPECT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
		EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 2 );
	}
}

UTEST( glyph_size, queued_sizes_use_distinct_slots )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx, false );
	const ve_fontcache_draw* first = vefc_test::find_first_target_draw( *dl );
	ASSERT_TRUE( first != nullptr );
	float original_u = dl->vertices[dl->indices[first->start_index]].u;

	ve_fontcache_set_font_size( &ctx.cache, font, 30.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	dl = vefc_test::current_drawlist( ctx, false );
	EXPECT_EQ( vefc_test::count_pass( *dl, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 2 );
	EXPECT_EQ( vefc_test::count_pass( *dl, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET ), 2 );
	EXPECT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 2 );
	int targets = 0;
	for ( int i = 0; i < (int) dl->dcalls.size(); i++ )
	{
		const ve_fontcache_draw& draw = dl->dcalls[i];
		if ( draw.pass != VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET ) continue;
		float u = dl->vertices[dl->indices[draw.start_index]].u;
		if ( targets == 0 )
		{
			EXPECT_EQ( u, original_u );
		}
		else
		{
			EXPECT_NE( u, original_u );
		}
		targets++;
	}
}

UTEST( glyph_size, transform_and_colour_do_not_rasterize_again )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	vefc_test::flush( ctx );
	float red[] = { 1, 0, 0, 1 };
	ve_fontcache_set_colour( &ctx.cache, red );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A", 0.25f, 0.5f ) );
	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	const ve_fontcache_draw* draw = vefc_test::find_first_target_draw( *dl );
	ASSERT_TRUE( draw != nullptr );
	ASSERT_EQ( vefc_test::count_pass( *dl, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 0 );
	ASSERT_EQ( ctx.cache.atlas.stateA.cache.size(), (size_t) 1 );
	ASSERT_EQ( draw->colour[0], 1.0f );
	ASSERT_EQ( draw->colour[1], 0.0f );
	ASSERT_EQ( ve_fontcache_get_cursor_pos( &ctx.cache ).y, 0.5f );
}

UTEST( glyph_size, many_glyphs_and_sizes_stay_distinct )
{
	vefc_test::context ctx( false );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ASSERT_GE( font, 0 );

	float sizes[] = { 17.0f, 24.0f, 30.0f, 36.0f };
	for ( int i = 0; i < 4; i++ )
	{
		ve_fontcache_set_font_size( &ctx.cache, font, sizes[i] );
		for ( int c = 33; c <= 126; c++ )
		{
			char8_t text[] = { (char8_t) c, 0 };
			vefc_test::flush( ctx );
			ASSERT_TRUE( vefc_test::draw_text( ctx, font, text, 0, 0, false ) );
			ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 1 );
		}
	}
	ASSERT_EQ( ctx.cache.atlas.stateA.cache.size() + ctx.cache.atlas.stateB.cache.size() + ctx.cache.atlas.stateC.cache.size() + ctx.cache.atlas.stateD.cache.size(), (size_t) 376 );

	for ( int i = 0; i < 4; i++ )
	{
		ve_fontcache_set_font_size( &ctx.cache, font, sizes[i] );
		for ( int c = 33; c <= 126; c++ )
		{
			char8_t text[] = { (char8_t) c, 0 };
			vefc_test::flush( ctx );
			ASSERT_TRUE( vefc_test::draw_text( ctx, font, text, 0, 0, false ) );
			ASSERT_EQ( vefc_test::count_pass( *vefc_test::current_drawlist( ctx ), VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ), 0 );
		}
	}
}

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
UTEST( glyph_size, freetype_round_trip_reuses_original_upload )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ASSERT_GT( vefc_test::current_drawlist( ctx )->texels.size(), (size_t) 0 );
	ASSERT_EQ( ctx.cache.atlasCPU.pages.size(), (size_t) 1 );
	ASSERT_EQ( ctx.cache.atlasCPU.pages[0]->cache.size(), (size_t) 1 );
	uint64_t original_key = ctx.cache.atlasCPU.pages[0]->cache.begin()->first;
	ve_fontcache_atlas_CPU_bitmap_region original = ctx.cache.atlasCPU.pages[0]->cache.begin()->second;

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, 30.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	EXPECT_GT( vefc_test::current_drawlist( ctx )->texels.size(), (size_t) 0 );
	EXPECT_EQ( ctx.cache.atlasCPU.pages.size(), (size_t) 1 );
	EXPECT_EQ( ctx.cache.atlasCPU.pages[0]->cache.size(), (size_t) 2 );

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, 24.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	EXPECT_EQ( vefc_test::current_drawlist( ctx )->texels.size(), (size_t) 0 );
	EXPECT_EQ( ctx.cache.atlasCPU.pages[0]->cache.size(), (size_t) 2 );
	ASSERT_TRUE( ctx.cache.atlasCPU.pages[0]->cache.find( original_key ) != ctx.cache.atlasCPU.pages[0]->cache.end() );
	EXPECT_EQ( ctx.cache.atlasCPU.pages[0]->cache[original_key].x, original.x );
	EXPECT_EQ( ctx.cache.atlasCPU.pages[0]->cache[original_key].y, original.y );
	EXPECT_EQ( ctx.cache.atlasCPU.pages[0]->cache[original_key].w, original.w );
	EXPECT_EQ( ctx.cache.atlasCPU.pages[0]->cache[original_key].h, original.h );
}

UTEST( glyph_size, freetype_unload_removes_every_size_of_only_that_font )
{
	vefc_test::context ctx( true );
	ve_font_id font = ctx.load_file( vefc_test::kRoboto, 24.0f );
	ve_font_id other = ctx.load_buffer_copy( 0, 24.0f );
	ASSERT_GE( font, 0 );
	ASSERT_GE( other, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, other, u8"A" ) );
	ASSERT_EQ( ctx.cache.atlasCPU.pages.size(), (size_t) 1 );
	ASSERT_EQ( ctx.cache.atlasCPU.pages[0]->cache.size(), (size_t) 2 );
	uint64_t other_key = 0;
	for ( const std::pair< const uint64_t, ve_fontcache_atlas_CPU_bitmap_region >& item : ctx.cache.atlasCPU.pages[0]->cache )
		if ( ( item.first >> 32 ) == (uint32_t) other ) other_key = item.first;
	ASSERT_TRUE( ctx.cache.atlasCPU.pages[0]->cache.find( other_key ) != ctx.cache.atlasCPU.pages[0]->cache.end() );

	vefc_test::flush( ctx );
	ve_fontcache_set_font_size( &ctx.cache, font, 30.0f );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	EXPECT_EQ( ctx.cache.atlasCPU.pages[0]->cache.size(), (size_t) 3 );
	ve_fontcache_unload( &ctx.cache, font );
	EXPECT_EQ( ctx.cache.atlasCPU.pages[0]->cache.size(), (size_t) 1 );
	EXPECT_TRUE( ctx.cache.atlasCPU.pages[0]->cache.find( other_key ) != ctx.cache.atlasCPU.pages[0]->cache.end() );

	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, other, u8"A" ) );
	EXPECT_EQ( vefc_test::current_drawlist( ctx )->texels.size(), (size_t) 0 );
}
#endif
