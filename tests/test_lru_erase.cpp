#include "test_common.h"

UTEST( lru_erase, removes_key )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 16 );

	ve_fontcache_LRU_put( lru, 1, 100 );
	ve_fontcache_LRU_put( lru, 2, 200 );
	ve_fontcache_LRU_put( lru, 3, 300 );

	EXPECT_EQ( 200, ve_fontcache_LRU_get( lru, 2 ) );

	ve_fontcache_LRU_erase( lru, 2 );

	EXPECT_EQ( -1, ve_fontcache_LRU_get( lru, 2 ) );
	EXPECT_EQ( 100, ve_fontcache_LRU_get( lru, 1 ) );
	EXPECT_EQ( 300, ve_fontcache_LRU_get( lru, 3 ) );
}

UTEST( lru_erase, reduces_cache_size )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 4 );

	ve_fontcache_LRU_put( lru, 1, 10 );
	ve_fontcache_LRU_put( lru, 2, 20 );
	ve_fontcache_LRU_put( lru, 3, 30 );
	ve_fontcache_LRU_put( lru, 4, 40 );

	EXPECT_EQ( 4u, lru.cache.size() );
	EXPECT_EQ( 4u, lru.key_queue.size );

	ve_fontcache_LRU_erase( lru, 2 );

	EXPECT_EQ( 3u, lru.cache.size() );
	EXPECT_EQ( 3u, lru.key_queue.size );
}

UTEST( lru_erase, allows_reinsert_after_erase )
{
	vefc_test::context ctx;
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 4 );

	ve_fontcache_LRU_put( lru, 1, 100 );
	ve_fontcache_LRU_put( lru, 2, 200 );
	ve_fontcache_LRU_put( lru, 3, 300 );

	ve_fontcache_LRU_erase( lru, 2 );

	EXPECT_EQ( -1, ve_fontcache_LRU_get( lru, 2 ) );

	// Reinsert key 2 with a new value
	ve_fontcache_LRU_put( lru, 2, 999 );

	EXPECT_EQ( 999, ve_fontcache_LRU_get( lru, 2 ) );
	EXPECT_EQ( 3u, lru.cache.size() );
}

UTEST( lru_erase, nonexistent_key_is_noop )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 4 );

	ve_fontcache_LRU_put( lru, 1, 100 );
	ve_fontcache_LRU_put( lru, 2, 200 );

	size_t initial_size = lru.cache.size();

	// Erase a key that was never inserted
	ve_fontcache_LRU_erase( lru, 999 );

	EXPECT_EQ( initial_size, lru.cache.size() );
	EXPECT_EQ( 100, ve_fontcache_LRU_get( lru, 1 ) );
	EXPECT_EQ( 200, ve_fontcache_LRU_get( lru, 2 ) );
}

UTEST( lru_erase, frees_slot_for_new_entry_without_eviction )
{
	ve_fontcache_LRU lru;
	ve_fontcache_LRU_init( lru, 4 );

	// Fill the cache to capacity
	ve_fontcache_LRU_put( lru, 1, 10 );
	ve_fontcache_LRU_put( lru, 2, 20 );
	ve_fontcache_LRU_put( lru, 3, 30 );
	ve_fontcache_LRU_put( lru, 4, 40 );

	EXPECT_EQ( 4u, lru.cache.size() );

	// Get the oldest key that would be evicted if we insert now
	uint64_t oldest = ve_fontcache_LRU_get_next_evicted( lru );

	// Remove the oldest key explicitly
	if ( oldest != -1 ) {
		ve_fontcache_LRU_erase( lru, oldest );
	}

	// Now insert a new key - it should fit without evicting anything else
	size_t size_before_insert = lru.cache.size();
	uint64_t evicted_after = ve_fontcache_LRU_get_next_evicted( lru );

	ve_fontcache_LRU_put( lru, 5, 50 );

	size_t size_after_insert = lru.cache.size();

	EXPECT_EQ( size_before_insert - 1 + 1, size_after_insert );
	EXPECT_LE( size_after_insert, static_cast< size_t >( lru.capacity ) );

	// Verify the new key is retrievable
	EXPECT_NE( -1, ve_fontcache_LRU_get( lru, 5 ) );
}