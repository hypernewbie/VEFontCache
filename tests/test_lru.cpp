#include "test_common.h"

UTEST( lru, init )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 4 );

	EXPECT_EQ( 4, lru.capacity );
	EXPECT_EQ( 0u, lru.cache.size() );
	EXPECT_EQ( 0u, lru.key_queue.size );
}

UTEST( lru, put_get )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 4 );

	EXPECT_EQ( 1ULL, ve_fontcache_LRU_put( lru, 1, 100 ) );
	EXPECT_EQ( 100, ve_fontcache_LRU_get( lru, 1 ) );
	EXPECT_EQ( 0xFFFFFFFFFFFFFFFFULL, ve_fontcache_LRU_get_next_evicted( lru ) );
}

UTEST( lru, put_get_multiple )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 5 );

	for ( int i = 0; i < 5; i++ ) {
		ve_fontcache_LRU_put( lru, static_cast< uint64_t >( i ), i * 10 );
	}

	for ( int i = 0; i < 5; i++ ) {
		EXPECT_EQ( i * 10, ve_fontcache_LRU_get( lru, static_cast< uint64_t >( i ) ) );
	}
}

UTEST( lru, eviction_returns_oldest )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 3 );

	ve_fontcache_LRU_put( lru, 1, 10 );
	ve_fontcache_LRU_put( lru, 2, 20 );
	ve_fontcache_LRU_put( lru, 3, 30 );

	EXPECT_EQ( 1ULL, ve_fontcache_LRU_put( lru, 4, 40 ) );
	EXPECT_EQ( -1, ve_fontcache_LRU_get( lru, 1 ) );
	EXPECT_EQ( 40, ve_fontcache_LRU_get( lru, 4 ) );
}

UTEST( lru, get_nonexistent )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 2 );

	EXPECT_EQ( -1, ve_fontcache_LRU_get( lru, 99 ) );
	EXPECT_EQ( -1, ve_fontcache_LRU_peek( lru, 99 ) );
}

UTEST( lru, peek_does_not_promote )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 3 );

	ve_fontcache_LRU_put( lru, 1, 10 );
	ve_fontcache_LRU_put( lru, 2, 20 );
	ve_fontcache_LRU_put( lru, 3, 30 );

	EXPECT_EQ( 10, ve_fontcache_LRU_peek( lru, 1 ) );
	EXPECT_EQ( 1ULL, ve_fontcache_LRU_put( lru, 4, 40 ) );
}

UTEST( lru, refresh_moves_key_to_front )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 3 );

	ve_fontcache_LRU_put( lru, 1, 10 );
	ve_fontcache_LRU_put( lru, 2, 20 );
	ve_fontcache_LRU_put( lru, 3, 30 );
	EXPECT_EQ( 10, ve_fontcache_LRU_get( lru, 1 ) );

	EXPECT_EQ( 2ULL, ve_fontcache_LRU_put( lru, 4, 40 ) );
	EXPECT_EQ( -1, ve_fontcache_LRU_get( lru, 2 ) );
}

UTEST( lru, put_updates_value )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 2 );

	EXPECT_EQ( 1ULL, ve_fontcache_LRU_put( lru, 1, 10 ) );
	EXPECT_EQ( 1ULL, ve_fontcache_LRU_put( lru, 1, 99 ) );
	EXPECT_EQ( 99, ve_fontcache_LRU_get( lru, 1 ) );
}

UTEST( lru, get_next_evicted )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 2 );

	EXPECT_EQ( 0xFFFFFFFFFFFFFFFFULL, ve_fontcache_LRU_get_next_evicted( lru ) );
	ve_fontcache_LRU_put( lru, 10, 1 );
	ve_fontcache_LRU_put( lru, 20, 2 );
	EXPECT_EQ( 10ULL, ve_fontcache_LRU_get_next_evicted( lru ) );
}

UTEST( lru, capacity_one )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 1 );

	EXPECT_EQ( 1ULL, ve_fontcache_LRU_put( lru, 1, 10 ) );
	EXPECT_EQ( 1ULL, ve_fontcache_LRU_put( lru, 2, 20 ) );
	EXPECT_EQ( -1, ve_fontcache_LRU_get( lru, 1 ) );
	EXPECT_EQ( 20, ve_fontcache_LRU_get( lru, 2 ) );
}

UTEST( lru, stress_256_keys )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 256 );

	std::minstd_rand rng( 12345 );
	std::uniform_int_distribution< int > dist( 0, 511 );

	for ( int i = 0; i < 1000; i++ ) {
		int key = dist( rng );
		ve_fontcache_LRU_put( lru, static_cast< uint64_t >( key ), i );
		EXPECT_LE( lru.cache.size(), static_cast< size_t >( lru.capacity ) );
		EXPECT_LE( lru.key_queue.size, static_cast< size_t >( lru.capacity ) );
	}
}
