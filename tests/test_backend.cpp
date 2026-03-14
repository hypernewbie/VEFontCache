#include "test_common.h"

UTEST( backend, conformance_header_passes )
{
	vefc_test::context ctx;
	ve_font_id font = ctx.load_file( vefc_test::kNotoSansJP );

	ASSERT_GE( font, 0 );
	ve_fontcache_backend_test_result result = ve_fontcache_backend_test_run( &ctx.cache, font );
	EXPECT_EQ( 0, result.failed );
	EXPECT_TRUE( result.passed > 0 );
}
