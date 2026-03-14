#include "test_common.h"

static_assert( VE_FONTCACHE_ATLAS_REGION_A_CAPACITY == 1024 );
static_assert( VE_FONTCACHE_ATLAS_REGION_B_CAPACITY == 512 );
static_assert( VE_FONTCACHE_ATLAS_REGION_C_CAPACITY == 512 );
static_assert( VE_FONTCACHE_ATLAS_REGION_D_CAPACITY == 256 );

UTEST( drawlist, atlas_indices_start_zero )
{
	vefc_test::context ctx;

	EXPECT_EQ( 0u, ctx.cache.atlas.next_atlas_idx_A );
	EXPECT_EQ( 0u, ctx.cache.atlas.next_atlas_idx_B );
	EXPECT_EQ( 0u, ctx.cache.atlas.next_atlas_idx_C );
	EXPECT_EQ( 0u, ctx.cache.atlas.next_atlas_idx_D );
}

UTEST( drawlist, draw_text_produces_valid_drawlist )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );

	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );
	EXPECT_FALSE( drawlist->dcalls.empty() );
	EXPECT_TRUE( vefc_test::all_indices_in_range( *drawlist ) );
	EXPECT_TRUE( vefc_test::all_vertices_finite( *drawlist ) );
	EXPECT_TRUE( vefc_test::target_uvs_normalised( *drawlist ) );
	EXPECT_TRUE(
		vefc_test::has_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET )
		|| vefc_test::has_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED )
		|| vefc_test::has_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) );
}

UTEST( drawlist, pass_types_are_known )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Known passes" ) );

	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx, false );
	for ( const ve_fontcache_draw& draw : drawlist->dcalls ) {
		EXPECT_TRUE(
			draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH
			|| draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS
			|| draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET
			|| draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED
			|| draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE
			|| draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD
			|| draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED );
	}
}

UTEST( drawlist, optimise_does_not_increase_drawcalls )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Hello hello hello" ) );

	size_t unoptimised = ve_fontcache_get_drawlist( &ctx.cache )->dcalls.size();
	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );
	EXPECT_LE( drawlist->dcalls.size(), unoptimised );
}

UTEST( drawlist, second_draw_uses_cached_glyphs )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	vefc_test::current_drawlist( ctx );

	vefc_test::flush( ctx );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"A" ) );
	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );

	EXPECT_EQ( 0, vefc_test::count_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ) );
}

UTEST( drawlist, colour_is_propagated )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	float colour[ 4 ] = { 0.0f, 1.0f, 0.0f, 1.0f };

	ASSERT_GE( font, 0 );
	ve_fontcache_set_colour( &ctx.cache, colour );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"X" ) );

	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );
	const ve_fontcache_draw* target = vefc_test::find_first_target_draw( *drawlist );
	ASSERT_TRUE( target != nullptr );
	EXPECT_TRUE( vefc_test::colour_equals( target->colour, { 0.0f, 1.0f, 0.0f, 1.0f } ) );
}

UTEST( drawlist, position_offset_affects_target_vertices )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Offset", 0.5f, 0.5f ) );

	ve_fontcache_drawlist* drawlist = vefc_test::current_drawlist( ctx );
	const ve_fontcache_draw* target = vefc_test::find_first_target_draw( *drawlist );
	ASSERT_TRUE( target != nullptr );

	float max_x = -std::numeric_limits< float >::infinity();
	for ( uint32_t idx = target->start_index; idx < target->end_index; idx++ ) {
		const ve_fontcache_vertex& v = drawlist->vertices[ drawlist->indices[ idx ] ];
		max_x = std::max( max_x, v.x );
	}
	EXPECT_GT( max_x, 0.5f );
}

UTEST( drawlist, flush_clears_drawlist )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Flush me" ) );
	vefc_test::current_drawlist( ctx );
	vefc_test::flush( ctx );

	ve_fontcache_drawlist* drawlist = ve_fontcache_get_drawlist( &ctx.cache );
	EXPECT_TRUE( drawlist->dcalls.empty() );
	EXPECT_TRUE( drawlist->vertices.empty() );
	EXPECT_TRUE( drawlist->indices.empty() );
}

UTEST( drawlist, cursor_x_advances )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );
	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"AB" ) );
	EXPECT_GT( ctx.cache.cursor_pos.x, 0.0f );
}
