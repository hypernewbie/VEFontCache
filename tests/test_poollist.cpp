#include "test_common.h"

UTEST( poollist, init_capacity )
{
	ve_fontcache_poollist plist;
	ve_fontcache_poollist_init( plist, 8 );

	EXPECT_EQ( 8u, plist.pool.size() );
	EXPECT_EQ( 8u, plist.freelist.size() );
	EXPECT_EQ( 0u, plist.size );
	EXPECT_EQ( vefc_test::kInvalidPoolItr, plist.front );
	EXPECT_EQ( vefc_test::kInvalidPoolItr, plist.back );
}

UTEST( poollist, push_front_order )
{
	ve_fontcache_poollist plist;
	ve_fontcache_poollist_init( plist, 8 );

	ve_fontcache_poollist_push_front( plist, 10 );
	ve_fontcache_poollist_push_front( plist, 11 );
	ve_fontcache_poollist_push_front( plist, 12 );

	EXPECT_EQ( 3u, plist.size );
	EXPECT_EQ( 10ULL, ve_fontcache_poollist_pop_back( plist ) );
	EXPECT_EQ( 11ULL, ve_fontcache_poollist_pop_back( plist ) );
	EXPECT_EQ( 12ULL, ve_fontcache_poollist_pop_back( plist ) );
	EXPECT_EQ( 0u, plist.size );
}

UTEST( poollist, erase_front_and_middle )
{
	ve_fontcache_poollist plist;
	ve_fontcache_poollist_init( plist, 8 );

	ve_fontcache_poollist_push_front( plist, 10 );
	ve_fontcache_poollist_push_front( plist, 11 );
	ve_fontcache_poollist_push_front( plist, 12 );
	ve_fontcache_poollist_push_front( plist, 13 );
	ve_fontcache_poollist_itr middle = plist.front;
	ve_fontcache_poollist_push_front( plist, 14 );
	ve_fontcache_poollist_push_front( plist, 15 );
	ve_fontcache_poollist_push_front( plist, 16 );
	ve_fontcache_poollist_push_front( plist, 17 );

	ve_fontcache_poollist_erase( plist, middle );
	ve_fontcache_poollist_erase( plist, plist.front );

	std::vector< uint64_t > values = vefc_test::drain_poollist( plist );
	ASSERT_EQ( 6u, values.size() );
	EXPECT_EQ( 10ULL, values[ 0 ] );
	EXPECT_EQ( 11ULL, values[ 1 ] );
	EXPECT_EQ( 12ULL, values[ 2 ] );
	EXPECT_EQ( 14ULL, values[ 3 ] );
	EXPECT_EQ( 15ULL, values[ 4 ] );
	EXPECT_EQ( 16ULL, values[ 5 ] );
}

UTEST( poollist, peek_back )
{
	ve_fontcache_poollist plist;
	ve_fontcache_poollist_init( plist, 4 );

	ve_fontcache_poollist_push_front( plist, 1 );
	ve_fontcache_poollist_push_front( plist, 2 );
	ve_fontcache_poollist_push_front( plist, 3 );

	EXPECT_EQ( 1ULL, ve_fontcache_poollist_peek_back( plist ) );
	EXPECT_EQ( 3u, plist.size );
}

UTEST( poollist, capacity_reuse )
{
	ve_fontcache_poollist plist;
	ve_fontcache_poollist_init( plist, 4 );

	for ( int value = 0; value < 4; value++ ) {
		ve_fontcache_poollist_push_front( plist, static_cast< uint64_t >( value ) );
	}
	EXPECT_EQ( 4u, plist.size );
	EXPECT_EQ( 0u, plist.freelist.size() );
	EXPECT_EQ( 4u, plist.pool.size() );

	vefc_test::drain_poollist( plist );
	EXPECT_EQ( 4u, plist.freelist.size() );
	EXPECT_EQ( 4u, plist.pool.size() );

	for ( int value = 10; value < 14; value++ ) {
		ve_fontcache_poollist_push_front( plist, static_cast< uint64_t >( value ) );
	}
	EXPECT_EQ( 4u, plist.pool.size() );
	EXPECT_EQ( 4u, plist.size );
}

UTEST( poollist, stress_128 )
{
	ve_fontcache_poollist plist;
	ve_fontcache_poollist_init( plist, 8 );

	for ( int repeat = 0; repeat < 128; repeat++ ) {
		ve_fontcache_poollist_push_front( plist, 31337 );
		ve_fontcache_poollist_push_front( plist, 31338 );
		ve_fontcache_poollist_push_front( plist, 31339 );
		EXPECT_EQ( 31337ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 31338ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 31339ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 0u, plist.size );

		ve_fontcache_poollist_push_front( plist, 1337 );
		ve_fontcache_poollist_push_front( plist, 1338 );
		ve_fontcache_poollist_push_front( plist, 1339 );
		EXPECT_EQ( 1337ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 1338ULL, ve_fontcache_poollist_pop_back( plist ) );
		ve_fontcache_poollist_push_front( plist, 1339 );
		ve_fontcache_poollist_push_front( plist, 1339 );
		EXPECT_EQ( 1339ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 1339ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 1339ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 0u, plist.size );

		ve_fontcache_poollist_push_front( plist, 10 );
		ve_fontcache_poollist_push_front( plist, 11 );
		ve_fontcache_poollist_push_front( plist, 12 );
		ve_fontcache_poollist_push_front( plist, 13 );
		ve_fontcache_poollist_itr itr = plist.front;

		ve_fontcache_poollist_push_front( plist, 14 );
		ve_fontcache_poollist_push_front( plist, 15 );
		ve_fontcache_poollist_push_front( plist, 16 );
		ve_fontcache_poollist_push_front( plist, 17 );

		ve_fontcache_poollist_erase( plist, itr );
		ve_fontcache_poollist_erase( plist, plist.front );
		EXPECT_EQ( 10ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 11ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 12ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 14ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 15ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 16ULL, ve_fontcache_poollist_pop_back( plist ) );
		EXPECT_EQ( 0u, plist.size );
	}
}
