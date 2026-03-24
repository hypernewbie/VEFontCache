#include "test_common.h"

inline int vefc_test_count_target_passes( const ve_fontcache_drawlist& drawlist )
{
	int count = 0;
	for ( const ve_fontcache_draw& draw : drawlist.dcalls ) {
		if ( vefc_test::is_target_pass( draw.pass ) && draw.end_index > draw.start_index ) {
			count++;
		}
	}
	return count;
}

inline void vefc_test_append_manual_target_draw( ve_fontcache_drawlist& dl, uint32_t start_idx, uint32_t end_idx, float r, float g, float b, float a, uint32_t region = 0 )
{
	ve_fontcache_draw d;
	d.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET;
	d.start_index = start_idx;
	d.end_index = end_idx;
	d.region = region;
	d.colour[ 0 ] = r;
	d.colour[ 1 ] = g;
	d.colour[ 2 ] = b;
	d.colour[ 3 ] = a;
	d.clear_before_draw = false;
	dl.dcalls.push_back( d );
}

inline void vefc_test_append_manual_vertex( ve_fontcache_drawlist& dl, float x, float y, float u, float v )
{
	ve_fontcache_vertex vtx;
	vtx.x = x; vtx.y = y; vtx.u = u; vtx.v = v;
	dl.vertices.push_back( vtx );
	dl.indices.push_back( ( uint32_t ) dl.vertices.size() - 1 );
}

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
	int target_drawcalls_before = vefc_test_count_target_passes( *drawlist_before );

	ASSERT_GE( target_drawcalls_before, 2 );

	ve_fontcache_optimise_drawlist( &ctx.cache );

	auto* drawlist_after = vefc_test::current_drawlist( ctx, false );
	int target_drawcalls_after = vefc_test_count_target_passes( *drawlist_after );

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
	int target_drawcalls_before = vefc_test_count_target_passes( *drawlist_before );

	ASSERT_GE( target_drawcalls_before, 2 );

	ve_fontcache_optimise_drawlist( &ctx.cache );

	auto* drawlist_after = vefc_test::current_drawlist( ctx, false );
	int target_drawcalls_after = vefc_test_count_target_passes( *drawlist_after );

	EXPECT_LT( target_drawcalls_after, target_drawcalls_before );
}

UTEST( optimise_drawlist, does_not_merge_across_pass_type )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );
	 ASSERT_GE( font, 0 );

	bool ok = vefc_test::draw_text( ctx, font, u8"AB" );
	 ASSERT_TRUE( ok );

	ve_fontcache_drawlist* dl = vefc_test::current_drawlist( ctx );
	 ASSERT_GE( dl->dcalls.size(), 2u );

	dl->dcalls[ 0 ].pass = VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET;
	dl->dcalls[ 1 ].pass = VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD;

	size_t dcalls_before = dl->dcalls.size();
	ve_fontcache_optimise_drawlist( &ctx.cache );

	ve_fontcache_drawlist* dl2 = vefc_test::current_drawlist( ctx, false );
	size_t dcalls_after = dl2->dcalls.size();

	 ASSERT_EQ( dcalls_before, dcalls_after );
}

UTEST( optimise_drawlist, does_not_merge_across_region )
{
	vefc_test::context ctx;
	ve_fontcache_drawlist& dl = ctx.cache.drawlist;
	dl.dcalls.clear();
	dl.vertices.clear();
	dl.indices.clear();

	vefc_test_append_manual_vertex( dl, 0.0f, 0.0f, 0.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 0.0f, 1.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 0.0f, 1.0f, 0.0f, 1.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 1.0f, 1.0f, 1.0f );

	vefc_test_append_manual_vertex( dl, 0.0f, 0.0f, 0.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 0.0f, 1.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 0.0f, 1.0f, 0.0f, 1.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 1.0f, 1.0f, 1.0f );

	vefc_test_append_manual_target_draw( dl, 0, 4, 1.0f, 1.0f, 1.0f, 1.0f, 'A' );
	vefc_test_append_manual_target_draw( dl, 4, 8, 1.0f, 1.0f, 1.0f, 1.0f, 'B' );

	 ASSERT_EQ( dl.dcalls.size(), 2u );
	ve_fontcache_optimise_drawlist( &ctx.cache );

	 ASSERT_EQ( dl.dcalls.size(), 2u );
}

UTEST( optimise_drawlist, does_not_merge_across_clear_before_draw )
{
	vefc_test::context ctx;
	ve_fontcache_drawlist& dl = ctx.cache.drawlist;
	dl.dcalls.clear();
	dl.vertices.clear();
	dl.indices.clear();

	vefc_test_append_manual_vertex( dl, 0.0f, 0.0f, 0.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 0.0f, 1.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 0.0f, 1.0f, 0.0f, 1.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 1.0f, 1.0f, 1.0f );

	vefc_test_append_manual_vertex( dl, 0.0f, 0.0f, 0.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 0.0f, 1.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 0.0f, 1.0f, 0.0f, 1.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 1.0f, 1.0f, 1.0f );

	ve_fontcache_draw d0;
	d0.pass = VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET;
	d0.start_index = 0;
	d0.end_index = 4;
	d0.clear_before_draw = false;
	d0.region = 0;
	d0.colour[ 0 ] = 1.0f; d0.colour[ 1 ] = 1.0f; d0.colour[ 2 ] = 1.0f; d0.colour[ 3 ] = 1.0f;
	dl.dcalls.push_back( d0 );

	ve_fontcache_draw d1 = d0;
	d1.start_index = 4;
	d1.end_index = 8;
	d1.clear_before_draw = true;
	dl.dcalls.push_back( d1 );

	 ASSERT_EQ( dl.dcalls.size(), 2u );
	ve_fontcache_optimise_drawlist( &ctx.cache );

	 ASSERT_EQ( dl.dcalls.size(), 2u );
}

UTEST( optimise_drawlist, does_not_merge_noncontiguous_ranges )
{
	vefc_test::context ctx;
	ve_fontcache_drawlist& dl = ctx.cache.drawlist;
	dl.dcalls.clear();
	dl.vertices.clear();
	dl.indices.clear();

	vefc_test_append_manual_vertex( dl, 0.0f, 0.0f, 0.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 0.0f, 1.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 0.0f, 1.0f, 0.0f, 1.0f );
	vefc_test_append_manual_vertex( dl, 1.0f, 1.0f, 1.0f, 1.0f );

	vefc_test_append_manual_vertex( dl, 5.0f, 5.0f, 0.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 6.0f, 5.0f, 1.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 5.0f, 6.0f, 0.0f, 1.0f );
	vefc_test_append_manual_vertex( dl, 6.0f, 6.0f, 1.0f, 1.0f );

	vefc_test_append_manual_vertex( dl, 10.0f, 10.0f, 0.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 11.0f, 10.0f, 1.0f, 0.0f );
	vefc_test_append_manual_vertex( dl, 10.0f, 11.0f, 0.0f, 1.0f );
	vefc_test_append_manual_vertex( dl, 11.0f, 11.0f, 1.0f, 1.0f );

	vefc_test_append_manual_target_draw( dl, 0, 4, 1.0f, 1.0f, 1.0f, 1.0f, 0 );
	vefc_test_append_manual_target_draw( dl, 4, 8, 1.0f, 1.0f, 1.0f, 1.0f, 0 );
	vefc_test_append_manual_target_draw( dl, 8, 12, 1.0f, 1.0f, 1.0f, 1.0f, 0 );

	 ASSERT_EQ( dl.dcalls.size(), 3u );
	ve_fontcache_optimise_drawlist( &ctx.cache );

	 ASSERT_EQ( dl.dcalls.size(), 1u );
}

UTEST( optimise_drawlist, merges_three_consecutive_equivalent_draws )
{
	vefc_test::context ctx;
	ve_fontcache_drawlist& dl = ctx.cache.drawlist;
	dl.dcalls.clear();
	dl.vertices.clear();
	dl.indices.clear();

	for ( int i = 0; i < 3; i++ ) {
		vefc_test_append_manual_vertex( dl, 0.0f, 0.0f, 0.0f, 0.0f );
		vefc_test_append_manual_vertex( dl, 1.0f, 0.0f, 1.0f, 0.0f );
		vefc_test_append_manual_vertex( dl, 0.0f, 1.0f, 0.0f, 1.0f );
		vefc_test_append_manual_vertex( dl, 1.0f, 1.0f, 1.0f, 1.0f );
	}

	uint32_t base = 0;
	for ( int i = 0; i < 3; i++ ) {
		vefc_test_append_manual_target_draw( dl, base, base + 4, 1.0f, 1.0f, 1.0f, 1.0f, 0 );
		base += 4;
	}

	 ASSERT_EQ( dl.dcalls.size(), 3u );
	ve_fontcache_optimise_drawlist( &ctx.cache );

	 ASSERT_EQ( dl.dcalls.size(), 1u );
}

UTEST( optimise_drawlist, preserve_indices_vertices_and_colour_after_merge )
{
	vefc_test::context ctx;
	ve_fontcache_drawlist& dl = ctx.cache.drawlist;
	dl.dcalls.clear();
	dl.vertices.clear();
	dl.indices.clear();

	for ( int i = 0; i < 2; i++ ) {
		vefc_test_append_manual_vertex( dl, 0.0f, 0.0f, 0.0f, 0.0f );
		vefc_test_append_manual_vertex( dl, 1.0f, 0.0f, 1.0f, 0.0f );
		vefc_test_append_manual_vertex( dl, 0.0f, 1.0f, 0.0f, 1.0f );
		vefc_test_append_manual_vertex( dl, 1.0f, 1.0f, 1.0f, 1.0f );
	}

	vefc_test_append_manual_target_draw( dl, 0, 4, 1.0f, 0.5f, 0.25f, 0.75f, 0 );
	vefc_test_append_manual_target_draw( dl, 4, 8, 1.0f, 0.5f, 0.25f, 0.75f, 0 );

	size_t verts_before = dl.vertices.size();
	size_t indices_before = dl.indices.size();

	ve_fontcache_optimise_drawlist( &ctx.cache );

	 ASSERT_EQ( dl.vertices.size(), verts_before );
	 ASSERT_EQ( dl.indices.size(), indices_before );
	 ASSERT_EQ( dl.dcalls.size(), 1u );
	 ASSERT_EQ( dl.dcalls[ 0 ].start_index, 0u );
	 ASSERT_EQ( dl.dcalls[ 0 ].end_index, 8u );
}

UTEST( optimise_drawlist, empty_and_singleton_drawlists_are_noops )
{
	vefc_test::context ctx1;
	ve_fontcache_drawlist& dl_empty = ctx1.cache.drawlist;
	dl_empty.dcalls.clear();
	dl_empty.vertices.clear();
	dl_empty.indices.clear();

	ve_fontcache_optimise_drawlist( &ctx1.cache );
	 ASSERT_TRUE( dl_empty.dcalls.empty() );

	vefc_test::context ctx2;
	ve_fontcache_drawlist& dl_single = ctx2.cache.drawlist;
	dl_single.dcalls.clear();
	dl_single.vertices.clear();
	dl_single.indices.clear();

	vefc_test_append_manual_vertex( dl_single, 0.0f, 0.0f, 0.0f, 0.0f );
	vefc_test_append_manual_vertex( dl_single, 1.0f, 0.0f, 1.0f, 0.0f );
	vefc_test_append_manual_vertex( dl_single, 0.0f, 1.0f, 0.0f, 1.0f );
	vefc_test_append_manual_vertex( dl_single, 1.0f, 1.0f, 1.0f, 1.0f );
	vefc_test_append_manual_target_draw( dl_single, 0, 4, 1.0f, 1.0f, 1.0f, 1.0f, 0 );

	ve_fontcache_optimise_drawlist( &ctx2.cache );
	 ASSERT_EQ( dl_single.dcalls.size(), 1u );
}

UTEST( optimise_drawlist, get_cursor_pos_api )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kRoboto );

	ASSERT_GE( font, 0 );

	ASSERT_TRUE( vefc_test::draw_text( ctx, font, u8"Hello" ) );
	vefc_test::flush( ctx );

	ve_fontcache_vec2 cursor = ve_fontcache_get_cursor_pos( &ctx.cache );

	EXPECT_GT( cursor.x, 0.0f );
}
