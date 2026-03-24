#include "test_common.h"

UTEST( backend_full, all_checks_with_full_options )
{
	vefc_test::context ctx;
	
	// Load fonts for full coverage testing
	ve_font_id noto_sans_jp = ctx.load_file( vefc_test::kNotoSansJP, 24.0f );      // Primary font (latin + CJK)
	ve_font_id roboto_24 = ctx.load_file( vefc_test::kRoboto, 24.0f );             // Secondary font at standard size
	ve_font_id roboto_8 = ctx.load_file( vefc_test::kRoboto, 8.0f );               // Small font (Region A/B)
	ve_font_id roboto_48 = ctx.load_file( vefc_test::kRoboto, 48.0f );             // Latin font (Region B/C)
	ve_font_id noto_serif_sc = ctx.load_file( vefc_test::kNotoSerifSC, 48.0f );    // CJK font (Region C/D)
	ve_font_id roboto_512 = ctx.load_file( vefc_test::kRoboto, 512.0f );           // Huge font for Region E uncached path
	
	ASSERT_GE( noto_sans_jp, 0 );
	ASSERT_GE( roboto_24, 0 );
	ASSERT_GE( roboto_8, 0 );
	ASSERT_GE( roboto_48, 0 );
	ASSERT_GE( noto_serif_sc, 0 );
	ASSERT_GE( roboto_512, 0 );

	ve_fontcache_backend_test_options options {};
	options.cache = &ctx.cache;
	options.font = noto_sans_jp;
	options.secondary_font = roboto_24;
	options.small_font = roboto_8;
	options.latin_font = roboto_48;
	options.cjk_font = noto_serif_sc;
	options.huge_font = roboto_512;
	
	// Reload function that loads NotoSansJP again at 24px into the same cache
	options.reload_font = [ &ctx ]() -> ve_font_id {
		return ctx.load_file( vefc_test::kNotoSansJP, 24.0f );
	};
	
	// execute and readback left empty; GPU suites are intentionally skipped
	
	ve_fontcache_backend_test_result result = ve_fontcache_backend_test_run( options );
	
	for ( const auto& failure : result.failures ) {
		printf( "FAILED: %s\n", failure.c_str() );
	}
	
	EXPECT_EQ( 0, result.failed );
	EXPECT_GT( result.passed, 0 );
	EXPECT_GT( result.skipped, 0 );
}
