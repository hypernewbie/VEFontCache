#define _CRT_SECURE_NO_WARNINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cmath>
#include <string>

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#endif

#ifdef VE_FONTCACHE_HARFBUZZ
#include <hb.h>
#endif

#define VE_FONTCACHE_IMPL
#include "../ve_fontcache.h"

#define UTEST_IMPLEMENTATION
#include "../utf8/test/utest.h"

#include "test_poollist.cpp"
#include "test_lru.cpp"
#include "test_font_load.cpp"
#include "test_drawlist.cpp"
#include "test_backend.cpp"
#include "test_shape_cache.cpp"
#include "test_utf8.cpp"
#include "test_stress.cpp"
#include "test_freetype.cpp"
#include "test_set_font_size.cpp"
#include "test_backend_full.cpp"
#include "test_lru_erase.cpp"
#include "test_shape_cache_eviction.cpp"
#include "test_optimise_drawlist.cpp"

UTEST_MAIN();
