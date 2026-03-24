#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "ve_fontcache.h"

using ve_fontcache_backend_readback_fn =
	std::function< bool( const char* name, int x, int y, int w, int h, uint8_t* out_pixels ) >;
using ve_fontcache_backend_execute_fn = std::function< void() >;
using ve_fontcache_backend_reset_surfaces_fn = std::function< void() >;
using ve_fontcache_backend_write_surface_fn =
	std::function< bool( const char* name, int x, int y, int w, int h, const uint8_t* pixels ) >;
using ve_fontcache_backend_reload_font_fn = std::function< ve_font_id() >;
using ve_fontcache_backend_prepare_real_text_fn = std::function< void() >;
using ve_fontcache_backend_finalise_test_state_fn = std::function< void() >;

struct ve_fontcache_backend_test_capabilities
{
	bool has_present_surface = false;
	bool has_cpu_atlas_surface = false;
	bool has_target_linear_surface = true;
	bool supports_freetype_mode = false;
	bool supports_harfbuzz_mode = false;
};

enum ve_fontcache_backend_test_font_role
{
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY = 0,
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SECONDARY,
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SMALL,
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_LATIN,
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_CJK,
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HUGE,
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_ARABIC,
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HEBREW,
	VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_COUNT,
};

struct ve_fontcache_backend_test_font_spec
{
	ve_fontcache_backend_test_font_role role = VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY;
	bool optional = false;
};

struct ve_fontcache_backend_test_font_entry
{
	ve_font_id id = -1;
	bool available = false;
	std::string diagnostic;
};

struct ve_fontcache_backend_test_font_set
{
	std::array< ve_fontcache_backend_test_font_entry, static_cast< size_t >( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_COUNT ) >
		entries {};

	ve_fontcache_backend_test_font_entry& operator[]( ve_fontcache_backend_test_font_role role )
	{
		return entries[ static_cast< size_t >( role ) ];
	}

	const ve_fontcache_backend_test_font_entry& operator[]( ve_fontcache_backend_test_font_role role ) const
	{
		return entries[ static_cast< size_t >( role ) ];
	}

	bool has( ve_fontcache_backend_test_font_role role ) const
	{
		const ve_fontcache_backend_test_font_entry& entry = ( *this )[ role ];
		return entry.available && entry.id >= 0;
	}

	void set( ve_fontcache_backend_test_font_role role, ve_font_id id, std::string diagnostic = {} )
	{
		ve_fontcache_backend_test_font_entry& entry = ( *this )[ role ];
		entry.id = id;
		entry.available = id >= 0;
		entry.diagnostic = diagnostic;
	}

	void mark_unavailable( ve_fontcache_backend_test_font_role role, std::string diagnostic = {} )
	{
		ve_fontcache_backend_test_font_entry& entry = ( *this )[ role ];
		entry.id = -1;
		entry.available = false;
		entry.diagnostic = diagnostic;
	}
};

using ve_fontcache_backend_provision_fonts_fn = std::function<
	ve_fontcache_backend_test_font_set( ve_fontcache* cache, const ve_fontcache_backend_test_font_spec* roles, size_t role_count ) >;

struct ve_fontcache_backend_test_options
{
	ve_fontcache* cache = nullptr;
	// Deprecated compatibility path when provision_fonts is not supplied.
	ve_font_id font = -1;
	ve_font_id secondary_font = -1;
	ve_font_id small_font = -1;
	ve_font_id latin_font = -1;
	ve_font_id cjk_font = -1;
	ve_font_id huge_font = -1;
	ve_font_id arabic_font = -1;
	ve_font_id hebrew_font = -1;
	ve_fontcache_backend_provision_fonts_fn provision_fonts;
	ve_fontcache_backend_test_font_set font_set;
	ve_fontcache_backend_test_capabilities capabilities;
	ve_fontcache_backend_execute_fn execute_pipeline;
	ve_fontcache_backend_execute_fn execute_present;
	ve_fontcache_backend_execute_fn execute_frame;
	ve_fontcache_backend_readback_fn readback_surface;
	ve_fontcache_backend_reset_surfaces_fn reset_surfaces;
	ve_fontcache_backend_write_surface_fn write_surface;
	ve_fontcache_backend_reload_font_fn reload_font;
	ve_fontcache_backend_prepare_real_text_fn prepare_real_text;
	ve_fontcache_backend_finalise_test_state_fn finalise_test_state;
};

struct ve_fontcache_backend_test_result
{
	int passed = 0;
	int failed = 0;
	int skipped = 0;
	std::string last_failure;
	std::vector< std::string > failures;
	std::vector< std::string > skipped_tests;
};

inline ve_fontcache_backend_test_options ve_fontcache_backend_test_resolve_font_roles(
	const ve_fontcache_backend_test_options& options );

inline void ve_fontcache_backend_test_expect(
	ve_fontcache_backend_test_result& result,
	bool cond,
	std::string_view msg )
{
	if ( cond ) {
		result.passed++;
		return;
	}

	result.failed++;
	result.last_failure = std::string( msg );
	result.failures.emplace_back( msg );
}

inline void ve_fontcache_backend_test_skip(
	ve_fontcache_backend_test_result& result,
	std::string_view msg )
{
	result.skipped++;
	result.skipped_tests.emplace_back( msg );
}

inline const char* ve_fontcache_backend_test_font_role_name( ve_fontcache_backend_test_font_role role )
{
	switch ( role ) {
		case VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY: return "primary";
		case VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SECONDARY: return "secondary";
		case VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SMALL: return "small";
		case VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_LATIN: return "latin";
		case VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_CJK: return "cjk";
		case VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HUGE: return "huge";
		case VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_ARABIC: return "arabic";
		case VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HEBREW: return "hebrew";
		default: return "unknown";
	}
}

inline const std::array< ve_fontcache_backend_test_font_spec, static_cast< size_t >( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_COUNT ) >&
ve_fontcache_backend_test_default_font_specs()
{
	static const std::array< ve_fontcache_backend_test_font_spec, static_cast< size_t >( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_COUNT ) >
		specs = { {
			{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY, false },
			{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SECONDARY, false },
			{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SMALL, false },
			{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_LATIN, false },
			{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_CJK, false },
			{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HUGE, false },
			{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_ARABIC, true },
			{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HEBREW, true },
		} };
	return specs;
}

inline ve_fontcache_backend_test_font_set ve_fontcache_backend_test_legacy_font_set(
	const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_font_set font_set;
	font_set.set( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY, options.font );
	font_set.set( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SECONDARY, options.secondary_font );
	font_set.set( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SMALL, options.small_font );
	font_set.set( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_LATIN, options.latin_font );
	font_set.set( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_CJK, options.cjk_font );
	font_set.set( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HUGE, options.huge_font );
	font_set.set( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_ARABIC, options.arabic_font );
	font_set.set( VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HEBREW, options.hebrew_font );
	return font_set;
}

inline ve_fontcache_backend_test_font_set ve_fontcache_backend_test_resolve_font_set(
	const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_font_set font_set = options.provision_fonts
		? options.provision_fonts(
			options.cache,
			ve_fontcache_backend_test_default_font_specs().data(),
			ve_fontcache_backend_test_default_font_specs().size() )
		: ve_fontcache_backend_test_legacy_font_set( options );

	for ( size_t role_index = 0; role_index < font_set.entries.size(); role_index++ ) {
		ve_fontcache_backend_test_font_entry& entry = font_set.entries[ role_index ];
		const bool candidate_available = entry.available || entry.id >= 0;
		if ( !candidate_available ) {
			entry.id = -1;
			if ( entry.diagnostic.empty() ) {
				entry.diagnostic = options.provision_fonts ? "not supplied" : "legacy font id not supplied";
			}
			continue;
		}

		if ( !options.cache || !ve_fontcache_is_valid_font_id( options.cache, entry.id ) ) {
			entry.id = -1;
			entry.available = false;
			if ( entry.diagnostic.empty() ) {
				entry.diagnostic = "supplied invalid font id";
			}
			continue;
		}

		entry.available = true;
	}

	return font_set;
}

inline ve_fontcache_backend_test_options ve_fontcache_backend_test_resolve_font_roles(
	const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_options resolved = options;
	resolved.font_set = ve_fontcache_backend_test_resolve_font_set( options );
	resolved.font = resolved.font_set[ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY ].id;
	resolved.secondary_font = resolved.font_set[ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SECONDARY ].id;
	resolved.small_font = resolved.font_set[ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SMALL ].id;
	resolved.latin_font = resolved.font_set[ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_LATIN ].id;
	resolved.cjk_font = resolved.font_set[ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_CJK ].id;
	resolved.huge_font = resolved.font_set[ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HUGE ].id;
	resolved.arabic_font = resolved.font_set[ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_ARABIC ].id;
	resolved.hebrew_font = resolved.font_set[ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HEBREW ].id;
	return resolved;
}

inline bool ve_fontcache_backend_test_has_role(
	const ve_fontcache_backend_test_options& options,
	ve_fontcache_backend_test_font_role role )
{
	return options.font_set.has( role );
}

inline ve_font_id ve_fontcache_backend_test_pick_first_available_role(
	const ve_fontcache_backend_test_options& options,
	std::initializer_list< ve_fontcache_backend_test_font_role > roles )
{
	for ( ve_fontcache_backend_test_font_role role : roles ) {
		if ( options.font_set.has( role ) ) {
			return options.font_set[ role ].id;
		}
	}
	return -1;
}

inline std::string ve_fontcache_backend_test_format_role_list(
	std::initializer_list< ve_fontcache_backend_test_font_role > roles )
{
	std::ostringstream builder;
	bool first = true;
	for ( ve_fontcache_backend_test_font_role role : roles ) {
		if ( !first ) {
			builder << "/";
		}
		builder << ve_fontcache_backend_test_font_role_name( role );
		first = false;
	}
	return builder.str();
}

inline std::string ve_fontcache_backend_test_format_missing_role_details(
	const ve_fontcache_backend_test_options& options,
	std::initializer_list< ve_fontcache_backend_test_font_role > roles )
{
	std::ostringstream builder;
	bool first = true;
	for ( ve_fontcache_backend_test_font_role role : roles ) {
		if ( options.font_set.has( role ) ) {
			continue;
		}

		const std::string& diagnostic = options.font_set[ role ].diagnostic;
		if ( diagnostic.empty() ) {
			continue;
		}

		if ( first ) {
			builder << " [";
		} else {
			builder << "; ";
		}
		builder << ve_fontcache_backend_test_font_role_name( role ) << ": " << diagnostic;
		first = false;
	}

	if ( !first ) {
		builder << "]";
	}
	return builder.str();
}

inline std::string ve_fontcache_backend_test_format_role_list(
	const std::vector< ve_fontcache_backend_test_font_role >& roles )
{
	std::ostringstream builder;
	for ( size_t i = 0; i < roles.size(); i++ ) {
		if ( i > 0 ) {
			builder << "/";
		}
		builder << ve_fontcache_backend_test_font_role_name( roles[ i ] );
	}
	return builder.str();
}

inline std::string ve_fontcache_backend_test_format_missing_role_details(
	const ve_fontcache_backend_test_options& options,
	const std::vector< ve_fontcache_backend_test_font_role >& roles )
{
	std::ostringstream builder;
	bool first = true;
	for ( ve_fontcache_backend_test_font_role role : roles ) {
		if ( options.font_set.has( role ) ) {
			continue;
		}

		const std::string& diagnostic = options.font_set[ role ].diagnostic;
		if ( diagnostic.empty() ) {
			continue;
		}

		if ( first ) {
			builder << " [";
		} else {
			builder << "; ";
		}
		builder << ve_fontcache_backend_test_font_role_name( role ) << ": " << diagnostic;
		first = false;
	}

	if ( !first ) {
		builder << "]";
	}
	return builder.str();
}

inline bool ve_fontcache_backend_test_require_roles(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options,
	std::string_view suite_name,
	std::initializer_list< ve_fontcache_backend_test_font_role > roles )
{
	std::vector< ve_fontcache_backend_test_font_role > missing_roles;
	for ( ve_fontcache_backend_test_font_role role : roles ) {
		if ( !options.font_set.has( role ) ) {
			missing_roles.push_back( role );
		}
	}

	if ( missing_roles.empty() ) {
		return true;
	}

	const std::string source = options.provision_fonts ? "provision_fonts did not supply " : "legacy font ids did not supply ";
	const std::string role_text = ve_fontcache_backend_test_format_role_list( missing_roles );
	const std::string details = ve_fontcache_backend_test_format_missing_role_details( options, missing_roles );
	ve_fontcache_backend_test_skip( result, std::string( suite_name ) + " skipped: " + source + role_text + " roles" + details );
	return false;
}

inline bool ve_fontcache_backend_test_require_any_role(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options,
	std::string_view suite_name,
	std::initializer_list< ve_fontcache_backend_test_font_role > roles )
{
	for ( ve_fontcache_backend_test_font_role role : roles ) {
		if ( options.font_set.has( role ) ) {
			return true;
		}
	}

	const std::string source = options.provision_fonts ? "provision_fonts did not supply any of " : "legacy font ids did not supply any of ";
	ve_fontcache_backend_test_skip(
		result,
		std::string( suite_name ) + " skipped: " + source + ve_fontcache_backend_test_format_role_list( roles ) + " roles"
			+ ve_fontcache_backend_test_format_missing_role_details( options, roles ) );
	return false;
}

inline bool ve_fontcache_backend_test_is_target_pass( uint32_t pass )
{
	return pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED;
}

inline bool ve_fontcache_backend_test_is_known_pass( uint32_t pass )
{
	return pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED;
}

inline bool ve_fontcache_backend_test_is_atlas_update_pass( uint32_t pass )
{
	return pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE
		|| pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD;
}

inline int ve_fontcache_backend_test_count_pass( const ve_fontcache_drawlist& drawlist, uint32_t pass )
{
	int count = 0;
	for ( const ve_fontcache_draw& draw : drawlist.dcalls ) {
		if ( draw.pass == pass && draw.end_index > draw.start_index ) {
			count++;
		}
	}
	return count;
}

inline int ve_fontcache_backend_test_count_cache_miss_passes( const ve_fontcache& cache, const ve_fontcache_drawlist& drawlist )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( cache.use_freetype ) {
		return ve_fontcache_backend_test_count_pass( drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_PAGE_TEXTURE_CREATE )
			+ ve_fontcache_backend_test_count_pass( drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS_UPLOAD );
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	return ve_fontcache_backend_test_count_pass( drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH );
}

inline bool ve_fontcache_backend_test_any_non_zero( const std::vector< uint8_t >& values )
{
	return std::any_of( values.begin(), values.end(), []( uint8_t value ) { return value != 0; } );
}

inline bool ve_fontcache_backend_test_has_intermediate_coverage( const std::vector< uint8_t >& values )
{
	return std::any_of( values.begin(), values.end(), []( uint8_t value ) {
		return value > 0 && value < std::numeric_limits< uint8_t >::max();
	} );
}

inline void ve_fontcache_backend_test_append_utf8( std::u8string& out, char32_t codepoint )
{
	if ( codepoint <= 0x7F ) {
		out.push_back( static_cast< char8_t >( codepoint ) );
		return;
	}

	if ( codepoint <= 0x7FF ) {
		out.push_back( static_cast< char8_t >( 0xC0 | ( codepoint >> 6 ) ) );
		out.push_back( static_cast< char8_t >( 0x80 | ( codepoint & 0x3F ) ) );
		return;
	}

	if ( codepoint <= 0xFFFF ) {
		out.push_back( static_cast< char8_t >( 0xE0 | ( codepoint >> 12 ) ) );
		out.push_back( static_cast< char8_t >( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) ) );
		out.push_back( static_cast< char8_t >( 0x80 | ( codepoint & 0x3F ) ) );
		return;
	}

	out.push_back( static_cast< char8_t >( 0xF0 | ( codepoint >> 18 ) ) );
	out.push_back( static_cast< char8_t >( 0x80 | ( ( codepoint >> 12 ) & 0x3F ) ) );
	out.push_back( static_cast< char8_t >( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) ) );
	out.push_back( static_cast< char8_t >( 0x80 | ( codepoint & 0x3F ) ) );
}

inline std::u8string ve_fontcache_backend_test_codepoint_to_utf8( char32_t codepoint )
{
	std::u8string text;
	ve_fontcache_backend_test_append_utf8( text, codepoint );
	return text;
}

inline std::u8string ve_fontcache_backend_test_make_ascii_string( size_t length )
{
	std::u8string text;
	text.reserve( length );
	for ( size_t i = 0; i < length; i++ ) {
		text.push_back( static_cast< char8_t >( 'A' + ( i % 26 ) ) );
	}
	return text;
}

inline std::u8string ve_fontcache_backend_test_make_cjk_string( size_t length, char32_t start = 0x4E00 )
{
	std::u8string text;
	for ( size_t i = 0; i < length; i++ ) {
		ve_fontcache_backend_test_append_utf8( text, start + static_cast< char32_t >( i ) );
	}
	return text;
}

inline ve_glyph ve_fontcache_backend_test_find_glyph( ve_fontcache* cache, ve_font_id font, char32_t codepoint )
{
	if ( !cache || font < 0 || font >= static_cast< ve_font_id >( cache->entry.size() ) || !cache->entry[ font ].used ) {
		return 0;
	}

	return stbtt_FindGlyphIndex( &cache->entry[ font ].info, codepoint );
}

inline bool ve_fontcache_backend_test_allowed_region( char region, std::string_view allowed_regions )
{
	return allowed_regions.find( region ) != std::string_view::npos;
}

inline bool ve_fontcache_backend_test_get_region_for_codepoint(
	ve_fontcache* cache,
	ve_font_id font,
	char32_t codepoint,
	char& region )
{
	ve_glyph glyph = ve_fontcache_backend_test_find_glyph( cache, font, codepoint );
	if ( !glyph ) {
		return false;
	}

	ve_fontcache_entry& entry = cache->entry[ font ];
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph ) ) {
		return false;
	}

	ve_fontcache_LRU* state = nullptr;
	uint32_t* next_idx = nullptr;
	float oversample_x = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X;
	float oversample_y = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y;
	region = ve_fontcache_decide_codepoint_region( cache, entry, glyph, state, next_idx, oversample_x, oversample_y );
	return region != '\0';
}

inline bool ve_fontcache_backend_test_find_codepoint_for_regions(
	ve_fontcache* cache,
	ve_font_id font,
	std::string_view allowed_regions,
	char32_t start,
	char32_t end,
	char32_t& codepoint_out,
	char& region_out )
{
	for ( char32_t codepoint = start; codepoint <= end; codepoint++ ) {
		char region = '\0';
		if ( !ve_fontcache_backend_test_get_region_for_codepoint( cache, font, codepoint, region ) ) {
			continue;
		}
		if ( !ve_fontcache_backend_test_allowed_region( region, allowed_regions ) ) {
			continue;
		}

		codepoint_out = codepoint;
		region_out = region;
		return true;
	}

	return false;
}

inline ve_fontcache_LRU* ve_fontcache_backend_test_state_for_region( ve_fontcache* cache, char region );
inline uint64_t ve_fontcache_backend_test_cache_key( ve_font_id font, ve_glyph glyph );

inline bool ve_fontcache_backend_test_is_codepoint_cached(
	ve_fontcache* cache,
	ve_font_id font,
	char32_t codepoint )
{
	char region = '\0';
	if ( !ve_fontcache_backend_test_get_region_for_codepoint( cache, font, codepoint, region ) ) {
		return false;
	}

	if ( region == 'E' ) {
		return false;
	}

	ve_glyph glyph = ve_fontcache_backend_test_find_glyph( cache, font, codepoint );
	ve_fontcache_LRU* state = ve_fontcache_backend_test_state_for_region( cache, region );
	if ( !glyph || !state ) {
		return false;
	}

	return ve_fontcache_LRU_peek( *state, ve_fontcache_backend_test_cache_key( font, glyph ) ) != -1;
}

inline bool ve_fontcache_backend_test_find_uncached_codepoint_for_regions(
	ve_fontcache* cache,
	ve_font_id font,
	std::string_view allowed_regions,
	char32_t start,
	char32_t end,
	char32_t& codepoint_out,
	char& region_out )
{
	for ( char32_t codepoint = start; codepoint <= end; codepoint++ ) {
		char region = '\0';
		if ( !ve_fontcache_backend_test_get_region_for_codepoint( cache, font, codepoint, region ) ) {
			continue;
		}
		if ( !ve_fontcache_backend_test_allowed_region( region, allowed_regions ) ) {
			continue;
		}
		if ( ve_fontcache_backend_test_is_codepoint_cached( cache, font, codepoint ) ) {
			continue;
		}

		codepoint_out = codepoint;
		region_out = region;
		return true;
	}

	return false;
}

inline std::vector< char32_t > ve_fontcache_backend_test_collect_codepoints_for_region(
	ve_fontcache* cache,
	ve_font_id font,
	char region,
	size_t wanted,
	char32_t start,
	char32_t end )
{
	std::vector< char32_t > codepoints;
	codepoints.reserve( wanted );
	for ( char32_t codepoint = start; codepoint <= end && codepoints.size() < wanted; codepoint++ ) {
		char actual_region = '\0';
		if ( !ve_fontcache_backend_test_get_region_for_codepoint( cache, font, codepoint, actual_region ) ) {
			continue;
		}
		if ( actual_region == region && !ve_fontcache_backend_test_is_codepoint_cached( cache, font, codepoint ) ) {
			codepoints.push_back( codepoint );
		}
	}

	return codepoints;
}

struct ve_fontcache_backend_test_atlas_sizes
{
	size_t region_A = 0;
	size_t region_B = 0;
	size_t region_C = 0;
	size_t region_D = 0;
};

inline ve_fontcache_backend_test_atlas_sizes ve_fontcache_backend_test_capture_atlas_sizes( const ve_fontcache& cache )
{
	ve_fontcache_backend_test_atlas_sizes sizes;
	sizes.region_A = cache.atlas.stateA.cache.size();
	sizes.region_B = cache.atlas.stateB.cache.size();
	sizes.region_C = cache.atlas.stateC.cache.size();
	sizes.region_D = cache.atlas.stateD.cache.size();
	return sizes;
}

inline size_t ve_fontcache_backend_test_region_cache_size( const ve_fontcache& cache, char region )
{
	switch ( region ) {
		case 'A': return cache.atlas.stateA.cache.size();
		case 'B': return cache.atlas.stateB.cache.size();
		case 'C': return cache.atlas.stateC.cache.size();
		case 'D': return cache.atlas.stateD.cache.size();
		default: return 0;
	}
}

inline char ve_fontcache_backend_test_growing_region(
	const ve_fontcache_backend_test_atlas_sizes& before,
	const ve_fontcache_backend_test_atlas_sizes& after )
{
	if ( after.region_A > before.region_A ) {
		return 'A';
	}
	if ( after.region_B > before.region_B ) {
		return 'B';
	}
	if ( after.region_C > before.region_C ) {
		return 'C';
	}
	if ( after.region_D > before.region_D ) {
		return 'D';
	}
	return '\0';
}

inline uint32_t* ve_fontcache_backend_test_next_index_for_region( ve_fontcache* cache, char region )
{
	switch ( region ) {
		case 'A': return &cache->atlas.next_atlas_idx_A;
		case 'B': return &cache->atlas.next_atlas_idx_B;
		case 'C': return &cache->atlas.next_atlas_idx_C;
		case 'D': return &cache->atlas.next_atlas_idx_D;
		default: return nullptr;
	}
}

inline ve_fontcache_LRU* ve_fontcache_backend_test_state_for_region( ve_fontcache* cache, char region )
{
	switch ( region ) {
		case 'A': return &cache->atlas.stateA;
		case 'B': return &cache->atlas.stateB;
		case 'C': return &cache->atlas.stateC;
		case 'D': return &cache->atlas.stateD;
		default: return nullptr;
	}
}

inline uint64_t ve_fontcache_backend_test_cache_key( ve_font_id font, ve_glyph glyph )
{
	return glyph + ( ( 0x100000000ULL * font ) & 0xFFFFFFFF00000000ULL );
}

inline bool ve_fontcache_backend_test_predict_atlas_slot(
	ve_fontcache* cache,
	ve_font_id font,
	char32_t codepoint,
	char& region_out,
	int& atlas_index_out )
{
	ve_glyph glyph = ve_fontcache_backend_test_find_glyph( cache, font, codepoint );
	if ( !glyph ) {
		return false;
	}

	ve_fontcache_entry& entry = cache->entry[ font ];
	if ( stbtt_IsGlyphEmpty( &entry.info, glyph ) ) {
		return false;
	}

	ve_fontcache_LRU* state = nullptr;
	uint32_t* next_idx = nullptr;
	float oversample_x = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X;
	float oversample_y = VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y;
	region_out = ve_fontcache_decide_codepoint_region( cache, entry, glyph, state, next_idx, oversample_x, oversample_y );
	if ( region_out == '\0' || region_out == 'E' || !state || !next_idx ) {
		return false;
	}

	uint64_t key = ve_fontcache_backend_test_cache_key( font, glyph );
	atlas_index_out = ve_fontcache_LRU_peek( *state, key );
	if ( atlas_index_out != -1 ) {
		return true;
	}

	if ( static_cast< int >( *next_idx ) < state->capacity ) {
		atlas_index_out = static_cast< int >( *next_idx );
		return true;
	}

	uint64_t next_evict = ve_fontcache_LRU_get_next_evicted( *state );
	if ( next_evict == 0xFFFFFFFFFFFFFFFFULL ) {
		return false;
	}

	atlas_index_out = ve_fontcache_LRU_peek( *state, next_evict );
	return atlas_index_out != -1;
}

inline bool ve_fontcache_backend_test_readback_texture(
	const ve_fontcache_backend_test_options& options,
	const char* name,
	int x,
	int y,
	int w,
	int h,
	std::vector< uint8_t >& out_pixels )
{
	if ( !options.readback_surface ) {
		return false;
	}

	out_pixels.assign( static_cast< size_t >( w ) * static_cast< size_t >( h ), 0 );
	return options.readback_surface( name, x, y, w, h, out_pixels.data() );
}

struct ve_fontcache_backend_test_rect
{
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
};

struct ve_fontcache_backend_test_bbox
{
	bool valid = false;
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	int area = 0;
};

struct ve_fontcache_backend_test_center_of_mass
{
	double x = 0.0;
	double y = 0.0;
	double mass = 0.0;
};

struct ve_fontcache_backend_test_diff_stats
{
	uint64_t total_abs_error = 0;
	double mean_abs_error = 0.0;
	uint8_t max_abs_error = 0;
	int peak_x = 0;
	int peak_y = 0;
	int differing_pixels = 0;
};

struct ve_fontcache_backend_test_mirror_scores
{
	double direct = 0.0;
	double horizontal = 0.0;
	double vertical = 0.0;
	double both = 0.0;
};

struct ve_fontcache_backend_test_surface_snapshot
{
	std::vector< uint8_t > glyph_buffer;
	std::vector< uint8_t > atlas;
	std::vector< uint8_t > target_linear;
	std::vector< uint8_t > presented;
	std::vector< uint8_t > cpu_atlas_page_0;
};

inline ve_fontcache_backend_test_rect ve_fontcache_backend_test_rect_from_bbox( const ve_fontcache_backend_test_bbox& bbox )
{
	return { bbox.x, bbox.y, bbox.w, bbox.h };
}

inline bool ve_fontcache_backend_test_rect_valid( const ve_fontcache_backend_test_rect& rect )
{
	return rect.w > 0 && rect.h > 0;
}

inline ve_fontcache_backend_test_rect ve_fontcache_backend_test_intersect_rects(
	const ve_fontcache_backend_test_rect& a,
	const ve_fontcache_backend_test_rect& b )
{
	const int x0 = std::max( a.x, b.x );
	const int y0 = std::max( a.y, b.y );
	const int x1 = std::min( a.x + a.w, b.x + b.w );
	const int y1 = std::min( a.y + a.h, b.y + b.h );
	return { x0, y0, std::max( 0, x1 - x0 ), std::max( 0, y1 - y0 ) };
}

inline ve_fontcache_backend_test_rect ve_fontcache_backend_test_inset_rect( const ve_fontcache_backend_test_rect& rect, int inset )
{
	return {
		rect.x + inset,
		rect.y + inset,
		std::max( 0, rect.w - 2 * inset ),
		std::max( 0, rect.h - 2 * inset ),
	};
}

inline ve_fontcache_backend_test_rect ve_fontcache_backend_test_expand_rect( const ve_fontcache_backend_test_rect& rect, int padding )
{
	return {
		rect.x - padding,
		rect.y - padding,
		rect.w + 2 * padding,
		rect.h + 2 * padding,
	};
}

inline ve_fontcache_backend_test_rect ve_fontcache_backend_test_clamp_rect(
	const ve_fontcache_backend_test_rect& rect,
	int width,
	int height )
{
	const int x0 = std::clamp( rect.x, 0, width );
	const int y0 = std::clamp( rect.y, 0, height );
	const int x1 = std::clamp( rect.x + rect.w, 0, width );
	const int y1 = std::clamp( rect.y + rect.h, 0, height );
	return { x0, y0, std::max( 0, x1 - x0 ), std::max( 0, y1 - y0 ) };
}

inline ve_fontcache_backend_test_rect ve_fontcache_backend_test_normalized_bounds_to_rect(
	double min_x,
	double min_y,
	double max_x,
	double max_y,
	int width,
	int height )
{
	if ( !( min_x < max_x ) || !( min_y < max_y ) ) {
		return {};
	}

	const int x0 = static_cast< int >( std::floor( min_x * width ) );
	const int y0 = static_cast< int >( std::floor( min_y * height ) );
	const int x1 = static_cast< int >( std::ceil( max_x * width ) );
	const int y1 = static_cast< int >( std::ceil( max_y * height ) );
	return ve_fontcache_backend_test_clamp_rect( { x0, y0, x1 - x0, y1 - y0 }, width, height );
}

inline int ve_fontcache_backend_test_target_width( const ve_fontcache* cache )
{
	return cache && cache->snap_width ? static_cast< int >( cache->snap_width ) : 1920;
}

inline int ve_fontcache_backend_test_target_height( const ve_fontcache* cache )
{
	return cache && cache->snap_height ? static_cast< int >( cache->snap_height ) : 1080;
}

inline const char* ve_fontcache_backend_test_target_linear_surface_name()
{
	return "target_linear";
}

inline const char* ve_fontcache_backend_test_presented_surface_name()
{
	return "presented";
}

inline const char* ve_fontcache_backend_test_cpu_atlas_page_surface_name()
{
	return "cpu_atlas_page_0";
}

inline bool ve_fontcache_backend_test_surface_extent(
	const ve_fontcache_backend_test_options& options,
	std::string_view name,
	int& width_out,
	int& height_out )
{
	if ( name == "glyph_buffer" ) {
		width_out = VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH;
		height_out = VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT;
		return true;
	}
	if ( name == "atlas" ) {
		width_out = VE_FONTCACHE_ATLAS_WIDTH;
		height_out = VE_FONTCACHE_ATLAS_HEIGHT;
		return true;
	}
	if ( name == "target" || name == ve_fontcache_backend_test_target_linear_surface_name() || name == ve_fontcache_backend_test_presented_surface_name() ) {
		width_out = ve_fontcache_backend_test_target_width( options.cache );
		height_out = ve_fontcache_backend_test_target_height( options.cache );
		return true;
	}
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( name == ve_fontcache_backend_test_cpu_atlas_page_surface_name() ) {
		width_out = VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE;
		height_out = VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE;
		return true;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
	return false;
}

struct ve_fontcache_backend_test_real_glyph_sample
{
	bool valid = false;
	uint32_t pass = 0;
	ve_fontcache_backend_test_rect source_rect;
	ve_fontcache_backend_test_rect dest_rect;
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	uint32_t atlas_page = 0;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
};

struct ve_fontcache_backend_test_real_glyph_observation
{
	bool ok = false;
	bool compare_valid = false;
	bool asymmetric = false;
	bool vertical_flip = false;
	bool horizontal_flip = false;
	ve_fontcache_backend_test_rect presented_read_rect;
	ve_fontcache_backend_test_bbox source_bbox;
	ve_fontcache_backend_test_bbox presented_bbox;
	ve_fontcache_backend_test_center_of_mass source_center;
	ve_fontcache_backend_test_center_of_mass presented_center;
	ve_fontcache_backend_test_diff_stats diff;
	ve_fontcache_backend_test_mirror_scores mirror_scores;
	ve_fontcache_backend_test_mirror_scores source_symmetry_scores;
};

struct ve_fontcache_backend_test_canonical_glyph_mask
{
	bool ok = false;
	bool compare_valid = false;
	ve_font_id font = -1;
	char32_t codepoint = 0;
	int bitmap_box_x0 = 0;
	int bitmap_box_y0 = 0;
	int bitmap_box_x1 = 0;
	int bitmap_box_y1 = 0;
	int width = 0;
	int height = 0;
	ve_fontcache_backend_test_rect compare_rect;
	ve_fontcache_backend_test_bbox bbox;
	ve_fontcache_backend_test_center_of_mass center;
	std::vector< uint8_t > mask;
	std::vector< uint8_t > summary;
};

struct ve_fontcache_backend_test_presented_glyph_mask
{
	bool ok = false;
	bool compare_valid = false;
	ve_font_id font = -1;
	char32_t codepoint = 0;
	uint32_t target_pass = 0;
	ve_fontcache_backend_test_rect draw_rect;
	ve_fontcache_backend_test_rect read_rect;
	ve_fontcache_backend_test_rect compare_rect;
	ve_fontcache_backend_test_bbox bbox;
	ve_fontcache_backend_test_center_of_mass center;
	std::vector< uint8_t > mask;
	std::vector< uint8_t > summary;
};

inline std::vector< uint8_t > ve_fontcache_backend_test_make_normalized_quadrant_summary(
	const std::array< double, 4 >& quadrants );
inline std::vector< uint8_t > ve_fontcache_backend_test_make_normalized_quadrant_summary(
	const std::vector< uint8_t >& pixels,
	int width,
	int height,
	const ve_fontcache_backend_test_rect& rect );
inline ve_fontcache_backend_test_bbox ve_fontcache_backend_test_translate_bbox(
	const ve_fontcache_backend_test_bbox& bbox,
	int dx,
	int dy );
inline ve_fontcache_backend_test_center_of_mass ve_fontcache_backend_test_translate_center(
	const ve_fontcache_backend_test_center_of_mass& center,
	int dx,
	int dy );
inline ve_fontcache_backend_test_bbox ve_fontcache_backend_test_thresholded_bbox(
	const std::vector< uint8_t >& pixels,
	int width,
	int height,
	uint8_t threshold );
inline std::vector< uint8_t > ve_fontcache_backend_test_box_downsample(
	const std::vector< uint8_t >& source_pixels,
	int source_width,
	int source_height,
	int dest_width,
	int dest_height );
inline std::vector< uint8_t > ve_fontcache_backend_test_copy_rect_pixels(
	const std::vector< uint8_t >& pixels,
	int image_width,
	int image_height,
	const ve_fontcache_backend_test_rect& rect );
inline ve_fontcache_backend_test_center_of_mass ve_fontcache_backend_test_center_of_mass_grayscale(
	const std::vector< uint8_t >& pixels,
	int width,
	int height );
inline ve_fontcache_backend_test_diff_stats ve_fontcache_backend_test_expected_vs_actual_diff(
	const std::vector< uint8_t >& expected,
	const std::vector< uint8_t >& actual,
	int width,
	int height );
inline ve_fontcache_backend_test_mirror_scores ve_fontcache_backend_test_mirror_scores_for_expected(
	const std::vector< uint8_t >& expected,
	const std::vector< uint8_t >& actual,
	int width,
	int height );
inline bool ve_fontcache_backend_test_is_vertical_flip( const ve_fontcache_backend_test_mirror_scores& scores );
inline bool ve_fontcache_backend_test_is_horizontal_flip( const ve_fontcache_backend_test_mirror_scores& scores );
inline std::vector< uint8_t > ve_fontcache_backend_test_flip_vertical(
	const std::vector< uint8_t >& pixels,
	int width,
	int height );
inline ve_fontcache_drawlist* ve_fontcache_backend_test_draw(
	ve_fontcache_backend_test_result& result,
	ve_fontcache* cache,
	ve_font_id font,
	const std::u8string& text,
	bool shape_cache,
	float posx,
	float posy,
	float scalex,
	float scaley );
inline void ve_fontcache_backend_test_reset_state( const ve_fontcache_backend_test_options& options );
inline bool ve_fontcache_backend_test_execute_pipeline_and_present( const ve_fontcache_backend_test_options& options );
inline bool ve_fontcache_backend_test_rasterize_canonical_glyph(
	ve_fontcache* cache,
	ve_font_id font,
	char32_t codepoint,
	ve_fontcache_backend_test_canonical_glyph_mask& mask );
inline bool ve_fontcache_backend_test_capture_presented_glyph_mask(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options,
	ve_font_id font,
	char32_t codepoint,
	float posx,
	float posy,
	float scalex,
	float scaley,
	ve_fontcache_backend_test_presented_glyph_mask& mask );

inline bool ve_fontcache_backend_test_extract_real_glyph_sample(
	const ve_fontcache_backend_test_options& options,
	const ve_fontcache_drawlist& drawlist,
	uint32_t target_pass,
	const char* source_surface_name,
	ve_fontcache_backend_test_real_glyph_sample& sample )
{
	int source_width = 0;
	int source_height = 0;
	if ( !ve_fontcache_backend_test_surface_extent( options, source_surface_name, source_width, source_height ) ) {
		return false;
	}

	const int target_width = ve_fontcache_backend_test_target_width( options.cache );
	const int target_height = ve_fontcache_backend_test_target_height( options.cache );
	for ( const ve_fontcache_draw& draw : drawlist.dcalls ) {
		if ( draw.pass != target_pass || draw.clear_before_draw || draw.end_index <= draw.start_index ) {
			continue;
		}

		double min_x = std::numeric_limits< double >::infinity();
		double min_y = std::numeric_limits< double >::infinity();
		double max_x = -std::numeric_limits< double >::infinity();
		double max_y = -std::numeric_limits< double >::infinity();
		double min_u = std::numeric_limits< double >::infinity();
		double min_v = std::numeric_limits< double >::infinity();
		double max_u = -std::numeric_limits< double >::infinity();
		double max_v = -std::numeric_limits< double >::infinity();
		for ( uint32_t idx = draw.start_index; idx < draw.end_index; idx++ ) {
			const ve_fontcache_vertex& vertex = drawlist.vertices[ drawlist.indices[ idx ] ];
			min_x = std::min( min_x, static_cast< double >( vertex.x ) );
			min_y = std::min( min_y, static_cast< double >( vertex.y ) );
			max_x = std::max( max_x, static_cast< double >( vertex.x ) );
			max_y = std::max( max_y, static_cast< double >( vertex.y ) );
			min_u = std::min( min_u, static_cast< double >( vertex.u ) );
			min_v = std::min( min_v, static_cast< double >( vertex.v ) );
			max_u = std::max( max_u, static_cast< double >( vertex.u ) );
			max_v = std::max( max_v, static_cast< double >( vertex.v ) );
		}

		sample.pass = draw.pass;
		sample.dest_rect = ve_fontcache_backend_test_normalized_bounds_to_rect( min_x, min_y, max_x, max_y, target_width, target_height );
		sample.source_rect = ve_fontcache_backend_test_normalized_bounds_to_rect( min_u, min_v, max_u, max_v, source_width, source_height );
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
		sample.atlas_page = draw.atlas_page;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
		sample.valid = ve_fontcache_backend_test_rect_valid( sample.dest_rect )
			&& ve_fontcache_backend_test_rect_valid( sample.source_rect );
		return sample.valid;
	}

	return false;
}

inline bool ve_fontcache_backend_test_capture_real_glyph_observation(
	const ve_fontcache_backend_test_options& options,
	const char* source_surface_name,
	const ve_fontcache_backend_test_real_glyph_sample& sample,
	ve_fontcache_backend_test_real_glyph_observation& observation )
{
	observation = {};
	if ( !sample.valid ) {
		return false;
	}

	const int target_width = ve_fontcache_backend_test_target_width( options.cache );
	const int target_height = ve_fontcache_backend_test_target_height( options.cache );
	observation.presented_read_rect = ve_fontcache_backend_test_clamp_rect(
		ve_fontcache_backend_test_expand_rect( sample.dest_rect, 8 ),
		target_width,
		target_height );
	if ( !ve_fontcache_backend_test_rect_valid( observation.presented_read_rect ) ) {
		return false;
	}

	std::vector< uint8_t > source_pixels;
	std::vector< uint8_t > presented_pixels;
	bool ok = ve_fontcache_backend_test_readback_texture(
		options,
		source_surface_name,
		sample.source_rect.x,
		sample.source_rect.y,
		sample.source_rect.w,
		sample.source_rect.h,
		source_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options,
		ve_fontcache_backend_test_presented_surface_name(),
		observation.presented_read_rect.x,
		observation.presented_read_rect.y,
		observation.presented_read_rect.w,
		observation.presented_read_rect.h,
		presented_pixels );

	std::vector< uint8_t > source_mask = source_pixels;
	std::vector< uint8_t > presented_mask = presented_pixels;
	for ( uint8_t& pixel : source_mask ) {
		pixel = pixel >= 8 ? 255 : 0;
	}
	for ( uint8_t& pixel : presented_mask ) {
		pixel = pixel >= 8 ? 255 : 0;
	}

	const ve_fontcache_backend_test_bbox source_local_bbox =
		ve_fontcache_backend_test_thresholded_bbox( source_mask, sample.source_rect.w, sample.source_rect.h, 8 );
	const ve_fontcache_backend_test_bbox presented_local_bbox =
		ve_fontcache_backend_test_thresholded_bbox( presented_mask, observation.presented_read_rect.w, observation.presented_read_rect.h, 8 );
	const ve_fontcache_backend_test_rect source_local_rect = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( source_local_bbox ),
		1 );
	const ve_fontcache_backend_test_rect presented_local_rect = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( presented_local_bbox ),
		2 );
	observation.compare_valid = ve_fontcache_backend_test_rect_valid( source_local_rect )
		&& ve_fontcache_backend_test_rect_valid( presented_local_rect );

	const std::vector< uint8_t > source_summary = observation.compare_valid
		? ve_fontcache_backend_test_box_downsample(
			ve_fontcache_backend_test_copy_rect_pixels( source_mask, sample.source_rect.w, sample.source_rect.h, source_local_rect ),
			source_local_rect.w,
			source_local_rect.h,
			3,
			3 )
		: std::vector< uint8_t > {};
	const std::vector< uint8_t > presented_summary = observation.compare_valid
		? ve_fontcache_backend_test_box_downsample(
			ve_fontcache_backend_test_copy_rect_pixels(
				presented_mask,
				observation.presented_read_rect.w,
				observation.presented_read_rect.h,
				presented_local_rect ),
			presented_local_rect.w,
			presented_local_rect.h,
			3,
			3 )
		: std::vector< uint8_t > {};

	observation.source_bbox = ve_fontcache_backend_test_translate_bbox( source_local_bbox, sample.source_rect.x, sample.source_rect.y );
	observation.presented_bbox = ve_fontcache_backend_test_translate_bbox(
		presented_local_bbox,
		observation.presented_read_rect.x,
		observation.presented_read_rect.y );
	observation.source_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( source_mask, sample.source_rect.w, sample.source_rect.h ),
		sample.source_rect.x,
		sample.source_rect.y );
	observation.presented_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale(
			presented_mask,
			observation.presented_read_rect.w,
			observation.presented_read_rect.h ),
		observation.presented_read_rect.x,
		observation.presented_read_rect.y );

	if ( observation.compare_valid ) {
		observation.diff = ve_fontcache_backend_test_expected_vs_actual_diff( source_summary, presented_summary, 3, 3 );
		observation.mirror_scores = ve_fontcache_backend_test_mirror_scores_for_expected( source_summary, presented_summary, 3, 3 );
		observation.source_symmetry_scores = ve_fontcache_backend_test_mirror_scores_for_expected( source_summary, source_summary, 3, 3 );
		observation.vertical_flip = ve_fontcache_backend_test_is_vertical_flip( observation.mirror_scores );
		observation.horizontal_flip = ve_fontcache_backend_test_is_horizontal_flip( observation.mirror_scores );
		observation.asymmetric = observation.source_symmetry_scores.horizontal >= 16.0
			&& observation.source_symmetry_scores.vertical >= 16.0;
	}

	observation.ok = ok
		&& ve_fontcache_backend_test_any_non_zero( source_mask )
		&& ve_fontcache_backend_test_any_non_zero( presented_mask )
		&& observation.source_bbox.valid
		&& observation.presented_bbox.valid
		&& observation.compare_valid;
	return observation.ok;
}

inline bool ve_fontcache_backend_test_rasterize_canonical_glyph(
	ve_fontcache* cache,
	ve_font_id font,
	char32_t codepoint,
	ve_fontcache_backend_test_canonical_glyph_mask& mask )
{
	mask = {};
	mask.font = font;
	mask.codepoint = codepoint;
	if ( !cache || !ve_fontcache_is_valid_font_id( cache, font ) ) {
		return false;
	}

	ve_fontcache_entry& entry = cache->entry[ font ];
	const ve_glyph glyph = ve_fontcache_backend_test_find_glyph( cache, font, codepoint );
	if ( !glyph || stbtt_IsGlyphEmpty( &entry.info, glyph ) ) {
		return false;
	}

	int x0 = 0;
	int y0 = 0;
	int x1 = 0;
	int y1 = 0;
	stbtt_GetGlyphBitmapBoxSubpixel(
		&entry.info,
		glyph,
		entry.size_scale,
		entry.size_scale,
		0.0f,
		0.0f,
		&x0,
		&y0,
		&x1,
		&y1 );
	const int width = std::max( 0, x1 - x0 );
	const int height = std::max( 0, y1 - y0 );
	if ( width <= 0 || height <= 0 ) {
		return false;
	}

	std::vector< uint8_t > canonical_top_left( static_cast< size_t >( width ) * static_cast< size_t >( height ), 0 );
	stbtt_MakeGlyphBitmapSubpixel(
		&entry.info,
		canonical_top_left.data(),
		width,
		height,
		width,
		entry.size_scale,
		entry.size_scale,
		0.0f,
		0.0f,
		glyph );

	mask.width = width;
	mask.height = height;
	mask.bitmap_box_x0 = x0;
	mask.bitmap_box_y0 = y0;
	mask.bitmap_box_x1 = x1;
	mask.bitmap_box_y1 = y1;
	mask.mask = ve_fontcache_backend_test_flip_vertical( canonical_top_left, width, height );
	for ( uint8_t& pixel : mask.mask ) {
		pixel = pixel >= 8 ? 255 : 0;
	}

	const ve_fontcache_backend_test_bbox local_bbox =
		ve_fontcache_backend_test_thresholded_bbox( mask.mask, width, height, 8 );
	mask.bbox = local_bbox;
	mask.compare_rect = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( local_bbox ),
		1 );
	mask.compare_valid = ve_fontcache_backend_test_rect_valid( mask.compare_rect );
	mask.center = ve_fontcache_backend_test_center_of_mass_grayscale( mask.mask, width, height );
	if ( mask.compare_valid ) {
		mask.summary = ve_fontcache_backend_test_box_downsample(
			ve_fontcache_backend_test_copy_rect_pixels( mask.mask, width, height, mask.compare_rect ),
			mask.compare_rect.w,
			mask.compare_rect.h,
			3,
			3 );
	}

	mask.ok = ve_fontcache_backend_test_any_non_zero( mask.mask )
		&& mask.bbox.valid
		&& mask.compare_valid;
	return mask.ok;
}

inline bool ve_fontcache_backend_test_capture_presented_glyph_mask(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options,
	ve_font_id font,
	char32_t codepoint,
	float posx,
	float posy,
	float scalex,
	float scaley,
	ve_fontcache_backend_test_presented_glyph_mask& mask )
{
	mask = {};
	mask.font = font;
	mask.codepoint = codepoint;
	if ( !options.cache || !ve_fontcache_is_valid_font_id( options.cache, font ) ) {
		return false;
	}

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoint ),
		false,
		posx,
		posy,
		scalex,
		scaley );

	uint32_t target_pass = 0;
	for ( const ve_fontcache_draw& draw : drawlist->dcalls ) {
		if ( ve_fontcache_backend_test_is_target_pass( draw.pass )
			&& !draw.clear_before_draw
			&& draw.end_index > draw.start_index ) {
			target_pass = draw.pass;
			break;
		}
	}

	const char* source_surface_name = nullptr;
	if ( target_pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET ) {
		source_surface_name = "atlas";
	} else if ( target_pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ) {
		source_surface_name = "glyph_buffer";
	}
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	else if ( target_pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED ) {
		source_surface_name = ve_fontcache_backend_test_cpu_atlas_page_surface_name();
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	ve_fontcache_backend_test_real_glyph_sample sample;
	bool ok = target_pass != 0
		&& source_surface_name != nullptr
		&& ve_fontcache_backend_test_extract_real_glyph_sample(
			options,
			*drawlist,
			target_pass,
			source_surface_name,
			sample );
	ok = ok && ve_fontcache_backend_test_execute_pipeline_and_present( options );
	if ( !ok || !sample.valid ) {
		return false;
	}

	mask.target_pass = target_pass;
	mask.draw_rect = sample.dest_rect;
	mask.read_rect = ve_fontcache_backend_test_clamp_rect(
		ve_fontcache_backend_test_expand_rect( sample.dest_rect, 8 ),
		ve_fontcache_backend_test_target_width( options.cache ),
		ve_fontcache_backend_test_target_height( options.cache ) );
	if ( !ve_fontcache_backend_test_rect_valid( mask.read_rect ) ) {
		return false;
	}

	ok = ve_fontcache_backend_test_readback_texture(
		options,
		ve_fontcache_backend_test_presented_surface_name(),
		mask.read_rect.x,
		mask.read_rect.y,
		mask.read_rect.w,
		mask.read_rect.h,
		mask.mask );
	if ( !ok ) {
		mask.mask.clear();
		return false;
	}

	for ( uint8_t& pixel : mask.mask ) {
		pixel = pixel >= 8 ? 255 : 0;
	}

	const ve_fontcache_backend_test_bbox local_bbox =
		ve_fontcache_backend_test_thresholded_bbox( mask.mask, mask.read_rect.w, mask.read_rect.h, 8 );
	mask.bbox = ve_fontcache_backend_test_translate_bbox( local_bbox, mask.read_rect.x, mask.read_rect.y );
	mask.compare_rect = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( local_bbox ),
		2 );
	mask.compare_valid = ve_fontcache_backend_test_rect_valid( mask.compare_rect );
	mask.center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( mask.mask, mask.read_rect.w, mask.read_rect.h ),
		mask.read_rect.x,
		mask.read_rect.y );
	if ( mask.compare_valid ) {
		mask.summary = ve_fontcache_backend_test_box_downsample(
			ve_fontcache_backend_test_copy_rect_pixels( mask.mask, mask.read_rect.w, mask.read_rect.h, mask.compare_rect ),
			mask.compare_rect.w,
			mask.compare_rect.h,
			3,
			3 );
	}

	mask.ok = ve_fontcache_backend_test_any_non_zero( mask.mask )
		&& mask.bbox.valid
		&& mask.compare_valid;
	return mask.ok;
}

inline ve_fontcache_backend_test_bbox ve_fontcache_backend_test_translate_bbox(
	const ve_fontcache_backend_test_bbox& bbox,
	int dx,
	int dy )
{
	ve_fontcache_backend_test_bbox translated = bbox;
	translated.x += dx;
	translated.y += dy;
	return translated;
}

inline ve_fontcache_backend_test_center_of_mass ve_fontcache_backend_test_translate_center(
	const ve_fontcache_backend_test_center_of_mass& center,
	int dx,
	int dy )
{
	ve_fontcache_backend_test_center_of_mass translated = center;
	translated.x += dx;
	translated.y += dy;
	return translated;
}

inline std::string ve_fontcache_backend_test_format_bbox( const ve_fontcache_backend_test_bbox& bbox )
{
	if ( !bbox.valid ) {
		return "empty";
	}

	std::ostringstream oss;
	oss << "(" << bbox.x << "," << bbox.y << " " << bbox.w << "x" << bbox.h << ")";
	return oss.str();
}

inline std::string ve_fontcache_backend_test_format_center( const ve_fontcache_backend_test_center_of_mass& center )
{
	if ( center.mass <= 0.0 ) {
		return "empty";
	}

	std::ostringstream oss;
	oss << std::fixed << std::setprecision( 2 ) << "(" << center.x << "," << center.y << ")";
	return oss.str();
}

inline std::string ve_fontcache_backend_test_format_diff( const ve_fontcache_backend_test_diff_stats& diff )
{
	std::ostringstream oss;
	oss << std::fixed << std::setprecision( 2 )
		<< "mae=" << diff.mean_abs_error
		<< ", max=" << static_cast< int >( diff.max_abs_error )
		<< " at (" << diff.peak_x << "," << diff.peak_y << ")"
		<< ", differing=" << diff.differing_pixels;
	return oss.str();
}

inline std::string ve_fontcache_backend_test_format_codepoint( char32_t codepoint )
{
	std::ostringstream oss;
	oss << std::uppercase << std::hex << static_cast< uint32_t >( codepoint );
	return oss.str();
}

inline std::vector< uint8_t > ve_fontcache_backend_test_make_image( int width, int height, uint8_t fill = 0 )
{
	return std::vector< uint8_t >( static_cast< size_t >( width ) * static_cast< size_t >( height ), fill );
}

inline void ve_fontcache_backend_test_fill_rect(
	std::vector< uint8_t >& pixels,
	int image_width,
	int image_height,
	int x,
	int y,
	int width,
	int height,
	uint8_t value )
{
	const int x0 = std::max( 0, x );
	const int y0 = std::max( 0, y );
	const int x1 = std::min( image_width, x + width );
	const int y1 = std::min( image_height, y + height );
	for ( int row = y0; row < y1; row++ ) {
		for ( int col = x0; col < x1; col++ ) {
			pixels[ static_cast< size_t >( row ) * image_width + col ] = value;
		}
	}
}

inline void ve_fontcache_backend_test_fill_hollow_rect(
	std::vector< uint8_t >& pixels,
	int image_width,
	int image_height,
	int x,
	int y,
	int width,
	int height,
	int thickness,
	uint8_t value )
{
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x, y, width, thickness, value );
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x, y + height - thickness, width, thickness, value );
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x, y, thickness, height, value );
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x + width - thickness, y, thickness, height, value );
}

inline void ve_fontcache_backend_test_fill_l_shape(
	std::vector< uint8_t >& pixels,
	int image_width,
	int image_height,
	int x,
	int y,
	int width,
	int height,
	int thickness,
	uint8_t value )
{
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x, y, thickness, height, value );
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x, y, width, thickness, value );
}

inline void ve_fontcache_backend_test_fill_quadrant_pattern(
	std::vector< uint8_t >& pixels,
	int image_width,
	int image_height,
	int x,
	int y,
	int width,
	int height,
	uint8_t top_left,
	uint8_t top_right,
	uint8_t bottom_left,
	uint8_t bottom_right )
{
	const int half_width = std::max( 1, width / 2 );
	const int half_height = std::max( 1, height / 2 );
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x, y, half_width, half_height, bottom_left );
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x + half_width, y, width - half_width, half_height, bottom_right );
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x, y + half_height, half_width, height - half_height, top_left );
	ve_fontcache_backend_test_fill_rect( pixels, image_width, image_height, x + half_width, y + half_height, width - half_width, height - half_height, top_right );
}

inline std::vector< uint8_t > ve_fontcache_backend_test_flip_horizontal(
	const std::vector< uint8_t >& pixels,
	int width,
	int height )
{
	std::vector< uint8_t > flipped( pixels.size(), 0 );
	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			flipped[ static_cast< size_t >( y ) * width + x ] =
				pixels[ static_cast< size_t >( y ) * width + ( width - 1 - x ) ];
		}
	}
	return flipped;
}

inline std::vector< uint8_t > ve_fontcache_backend_test_flip_vertical(
	const std::vector< uint8_t >& pixels,
	int width,
	int height )
{
	std::vector< uint8_t > flipped( pixels.size(), 0 );
	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			flipped[ static_cast< size_t >( y ) * width + x ] =
				pixels[ static_cast< size_t >( height - 1 - y ) * width + x ];
		}
	}
	return flipped;
}

inline std::vector< uint8_t > ve_fontcache_backend_test_box_downsample(
	const std::vector< uint8_t >& source_pixels,
	int source_width,
	int source_height,
	int dest_width,
	int dest_height )
{
	std::vector< uint8_t > downsampled( static_cast< size_t >( dest_width ) * static_cast< size_t >( dest_height ), 0 );
	for ( int dest_y = 0; dest_y < dest_height; dest_y++ ) {
		const int source_y0 = ( dest_y * source_height ) / dest_height;
		const int source_y1 = std::max( source_y0 + 1, ( ( dest_y + 1 ) * source_height ) / dest_height );
		for ( int dest_x = 0; dest_x < dest_width; dest_x++ ) {
			const int source_x0 = ( dest_x * source_width ) / dest_width;
			const int source_x1 = std::max( source_x0 + 1, ( ( dest_x + 1 ) * source_width ) / dest_width );
			uint32_t sum = 0;
			uint32_t count = 0;
			for ( int source_y = source_y0; source_y < source_y1; source_y++ ) {
				for ( int source_x = source_x0; source_x < source_x1; source_x++ ) {
					sum += source_pixels[ static_cast< size_t >( source_y ) * source_width + source_x ];
					count++;
				}
			}
			downsampled[ static_cast< size_t >( dest_y ) * dest_width + dest_x ] =
				static_cast< uint8_t >( count ? ( sum / count ) : 0 );
		}
	}
	return downsampled;
}

inline std::vector< uint8_t > ve_fontcache_backend_test_copy_rect_pixels(
	const std::vector< uint8_t >& pixels,
	int image_width,
	int image_height,
	const ve_fontcache_backend_test_rect& rect )
{
	if ( !ve_fontcache_backend_test_rect_valid( rect ) ) {
		return {};
	}

	std::vector< uint8_t > cropped( static_cast< size_t >( rect.w ) * static_cast< size_t >( rect.h ), 0 );
	for ( int y = 0; y < rect.h; y++ ) {
		for ( int x = 0; x < rect.w; x++ ) {
			const int src_x = rect.x + x;
			const int src_y = rect.y + y;
			if ( src_x < 0 || src_y < 0 || src_x >= image_width || src_y >= image_height ) {
				continue;
			}
			cropped[ static_cast< size_t >( y ) * rect.w + x ] =
				pixels[ static_cast< size_t >( src_y ) * image_width + src_x ];
		}
	}
	return cropped;
}

inline ve_fontcache_backend_test_bbox ve_fontcache_backend_test_thresholded_bbox(
	const std::vector< uint8_t >& pixels,
	int width,
	int height,
	uint8_t threshold = 1 )
{
	int min_x = width;
	int min_y = height;
	int max_x = -1;
	int max_y = -1;
	int area = 0;
	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			if ( pixels[ static_cast< size_t >( y ) * width + x ] < threshold ) {
				continue;
			}
			min_x = std::min( min_x, x );
			min_y = std::min( min_y, y );
			max_x = std::max( max_x, x );
			max_y = std::max( max_y, y );
			area++;
		}
	}

	ve_fontcache_backend_test_bbox bbox;
	bbox.valid = max_x >= min_x && max_y >= min_y;
	if ( bbox.valid ) {
		bbox.x = min_x;
		bbox.y = min_y;
		bbox.w = max_x - min_x + 1;
		bbox.h = max_y - min_y + 1;
		bbox.area = area;
	}
	return bbox;
}

inline std::vector< uint32_t > ve_fontcache_backend_test_row_sum_profile(
	const std::vector< uint8_t >& pixels,
	int width,
	int height )
{
	std::vector< uint32_t > sums( static_cast< size_t >( height ), 0 );
	for ( int y = 0; y < height; y++ ) {
		uint32_t sum = 0;
		for ( int x = 0; x < width; x++ ) {
			sum += pixels[ static_cast< size_t >( y ) * width + x ];
		}
		sums[ y ] = sum;
	}
	return sums;
}

inline std::vector< uint32_t > ve_fontcache_backend_test_column_sum_profile(
	const std::vector< uint8_t >& pixels,
	int width,
	int height )
{
	std::vector< uint32_t > sums( static_cast< size_t >( width ), 0 );
	for ( int x = 0; x < width; x++ ) {
		uint32_t sum = 0;
		for ( int y = 0; y < height; y++ ) {
			sum += pixels[ static_cast< size_t >( y ) * width + x ];
		}
		sums[ x ] = sum;
	}
	return sums;
}

inline ve_fontcache_backend_test_center_of_mass ve_fontcache_backend_test_center_of_mass_grayscale(
	const std::vector< uint8_t >& pixels,
	int width,
	int height )
{
	ve_fontcache_backend_test_center_of_mass center;
	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			const double value = static_cast< double >( pixels[ static_cast< size_t >( y ) * width + x ] );
			center.mass += value;
			center.x += ( x + 0.5 ) * value;
			center.y += ( y + 0.5 ) * value;
		}
	}
	if ( center.mass > 0.0 ) {
		center.x /= center.mass;
		center.y /= center.mass;
	}
	return center;
}

inline int ve_fontcache_backend_test_connected_component_count(
	const std::vector< uint8_t >& pixels,
	int width,
	int height,
	uint8_t threshold = 1 )
{
	std::vector< uint8_t > visited( pixels.size(), 0 );
	std::vector< int > stack_x;
	std::vector< int > stack_y;
	int component_count = 0;

	for ( int start_y = 0; start_y < height; start_y++ ) {
		for ( int start_x = 0; start_x < width; start_x++ ) {
			const size_t start_index = static_cast< size_t >( start_y ) * width + start_x;
			if ( visited[ start_index ] || pixels[ start_index ] < threshold ) {
				continue;
			}

			component_count++;
			stack_x.clear();
			stack_y.clear();
			stack_x.push_back( start_x );
			stack_y.push_back( start_y );
			visited[ start_index ] = 1;

			while ( !stack_x.empty() ) {
				const int x = stack_x.back();
				const int y = stack_y.back();
				stack_x.pop_back();
				stack_y.pop_back();
				const int neighbours[ 4 ][ 2 ] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
				for ( const auto& neighbour : neighbours ) {
					const int nx = x + neighbour[ 0 ];
					const int ny = y + neighbour[ 1 ];
					if ( nx < 0 || ny < 0 || nx >= width || ny >= height ) {
						continue;
					}
					const size_t neighbour_index = static_cast< size_t >( ny ) * width + nx;
					if ( visited[ neighbour_index ] || pixels[ neighbour_index ] < threshold ) {
						continue;
					}
					visited[ neighbour_index ] = 1;
					stack_x.push_back( nx );
					stack_y.push_back( ny );
				}
			}
		}
	}

	return component_count;
}

inline int ve_fontcache_backend_test_border_leakage_count(
	const std::vector< uint8_t >& pixels,
	int width,
	int height,
	uint8_t threshold = 1,
	int border = 1 )
{
	int leaked = 0;
	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			if ( x >= border && x < width - border && y >= border && y < height - border ) {
				continue;
			}
			if ( pixels[ static_cast< size_t >( y ) * width + x ] >= threshold ) {
				leaked++;
			}
		}
	}
	return leaked;
}

inline int ve_fontcache_backend_test_outside_rect_count(
	const std::vector< uint8_t >& pixels,
	int width,
	int height,
	const ve_fontcache_backend_test_rect& rect,
	uint8_t threshold = 1 )
{
	int leaked = 0;
	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			if ( x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h ) {
				continue;
			}
			if ( pixels[ static_cast< size_t >( y ) * width + x ] >= threshold ) {
				leaked++;
			}
		}
	}
	return leaked;
}

inline ve_fontcache_backend_test_diff_stats ve_fontcache_backend_test_expected_vs_actual_diff(
	const std::vector< uint8_t >& expected,
	const std::vector< uint8_t >& actual,
	int width,
	int height )
{
	ve_fontcache_backend_test_diff_stats diff;
	if ( expected.size() != actual.size() ) {
		diff.max_abs_error = 255;
		diff.mean_abs_error = 255.0;
		diff.total_abs_error = 255ULL * static_cast< uint64_t >( std::max( expected.size(), actual.size() ) );
		diff.differing_pixels = static_cast< int >( std::max( expected.size(), actual.size() ) );
		return diff;
	}

	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			const size_t index = static_cast< size_t >( y ) * width + x;
			const uint8_t delta = static_cast< uint8_t >( std::abs( static_cast< int >( expected[ index ] ) - static_cast< int >( actual[ index ] ) ) );
			diff.total_abs_error += delta;
			if ( delta > 0 ) {
				diff.differing_pixels++;
			}
			if ( delta >= diff.max_abs_error ) {
				diff.max_abs_error = delta;
				diff.peak_x = x;
				diff.peak_y = y;
			}
		}
	}

	const size_t pixel_count = static_cast< size_t >( width ) * static_cast< size_t >( height );
	diff.mean_abs_error = pixel_count ? static_cast< double >( diff.total_abs_error ) / pixel_count : 0.0;
	return diff;
}

inline double ve_fontcache_backend_test_mean_in_rect(
	const std::vector< uint8_t >& pixels,
	int image_width,
	int image_height,
	const ve_fontcache_backend_test_rect& rect )
{
	const int x0 = std::max( 0, rect.x );
	const int y0 = std::max( 0, rect.y );
	const int x1 = std::min( image_width, rect.x + rect.w );
	const int y1 = std::min( image_height, rect.y + rect.h );
	uint64_t sum = 0;
	uint64_t count = 0;
	for ( int y = y0; y < y1; y++ ) {
		for ( int x = x0; x < x1; x++ ) {
			sum += pixels[ static_cast< size_t >( y ) * image_width + x ];
			count++;
		}
	}
	return count ? static_cast< double >( sum ) / count : 0.0;
}

inline std::array< double, 4 > ve_fontcache_backend_test_quadrant_means(
	const std::vector< uint8_t >& pixels,
	int image_width,
	int image_height,
	const ve_fontcache_backend_test_rect& rect )
{
	const int half_width = std::max( 1, rect.w / 2 );
	const int half_height = std::max( 1, rect.h / 2 );
	return {
		ve_fontcache_backend_test_mean_in_rect( pixels, image_width, image_height, { rect.x, rect.y + half_height, half_width, rect.h - half_height } ),
		ve_fontcache_backend_test_mean_in_rect( pixels, image_width, image_height, { rect.x + half_width, rect.y + half_height, rect.w - half_width, rect.h - half_height } ),
		ve_fontcache_backend_test_mean_in_rect( pixels, image_width, image_height, { rect.x, rect.y, half_width, half_height } ),
		ve_fontcache_backend_test_mean_in_rect( pixels, image_width, image_height, { rect.x + half_width, rect.y, rect.w - half_width, half_height } ),
	};
}

inline ve_fontcache_backend_test_mirror_scores ve_fontcache_backend_test_mirror_scores_for_expected(
	const std::vector< uint8_t >& expected,
	const std::vector< uint8_t >& actual,
	int width,
	int height )
{
	ve_fontcache_backend_test_mirror_scores scores;
	scores.direct = ve_fontcache_backend_test_expected_vs_actual_diff( expected, actual, width, height ).mean_abs_error;
	scores.horizontal = ve_fontcache_backend_test_expected_vs_actual_diff(
		ve_fontcache_backend_test_flip_horizontal( expected, width, height ),
		actual,
		width,
		height ).mean_abs_error;
	scores.vertical = ve_fontcache_backend_test_expected_vs_actual_diff(
		ve_fontcache_backend_test_flip_vertical( expected, width, height ),
		actual,
		width,
		height ).mean_abs_error;
	scores.both = ve_fontcache_backend_test_expected_vs_actual_diff(
		ve_fontcache_backend_test_flip_horizontal(
			ve_fontcache_backend_test_flip_vertical( expected, width, height ),
			width,
			height ),
		actual,
		width,
		height ).mean_abs_error;
	return scores;
}

inline bool ve_fontcache_backend_test_is_vertical_flip( const ve_fontcache_backend_test_mirror_scores& scores )
{
	return scores.vertical + 2.0 < scores.direct
		&& scores.vertical <= scores.horizontal + 1.0
		&& scores.vertical <= scores.both + 1.0;
}

inline bool ve_fontcache_backend_test_is_horizontal_flip( const ve_fontcache_backend_test_mirror_scores& scores )
{
	return scores.horizontal + 2.0 < scores.direct
		&& scores.horizontal <= scores.vertical + 1.0
		&& scores.horizontal <= scores.both + 1.0;
}

inline bool ve_fontcache_backend_test_is_uniform_scale_error(
	const ve_fontcache_backend_test_bbox& expected,
	const ve_fontcache_backend_test_bbox& observed )
{
	if ( !expected.valid || !observed.valid || expected.w == 0 || expected.h == 0 ) {
		return false;
	}
	const double scale_x = static_cast< double >( observed.w ) / expected.w;
	const double scale_y = static_cast< double >( observed.h ) / expected.h;
	return std::fabs( scale_x - scale_y ) <= 0.12
		&& ( std::fabs( scale_x - 1.0 ) > 0.08 || std::fabs( scale_y - 1.0 ) > 0.08 );
}

inline bool ve_fontcache_backend_test_is_aspect_ratio_deformation(
	const ve_fontcache_backend_test_bbox& expected,
	const ve_fontcache_backend_test_bbox& observed )
{
	if ( !expected.valid || !observed.valid || expected.w == 0 || expected.h == 0 ) {
		return false;
	}
	const double scale_x = static_cast< double >( observed.w ) / expected.w;
	const double scale_y = static_cast< double >( observed.h ) / expected.h;
	return std::fabs( scale_x - scale_y ) > 0.12;
}

inline bool ve_fontcache_backend_test_has_diagonal_seam(
	const std::vector< uint8_t >& pixels,
	int width,
	int height,
	const ve_fontcache_backend_test_bbox& bbox,
	uint8_t threshold = 64 )
{
	if ( !bbox.valid || bbox.w < 4 || bbox.h < 4 ) {
		return false;
	}

	int seam_failures = 0;
	const int steps = std::min( bbox.w, bbox.h );
	for ( int i = 1; i < steps - 1; i++ ) {
		const int x = bbox.x + i;
		const int y = bbox.y + i;
		if ( x < 0 || y < 0 || x >= width || y >= height ) {
			continue;
		}
		if ( pixels[ static_cast< size_t >( y ) * width + x ] < threshold ) {
			seam_failures++;
		}
	}
	return seam_failures > std::max( 1, steps / 8 );
}

inline bool ve_fontcache_backend_test_is_wrong_source_texture_bound(
	const ve_fontcache_backend_test_diff_stats& expected_diff,
	const ve_fontcache_backend_test_diff_stats& alternate_diff )
{
	return alternate_diff.mean_abs_error + 2.0 < expected_diff.mean_abs_error * 0.7;
}

inline std::string ve_fontcache_backend_test_make_failure(
	std::string_view case_id,
	const ve_fontcache_backend_test_bbox& expected_bbox,
	const ve_fontcache_backend_test_bbox& observed_bbox,
	const ve_fontcache_backend_test_center_of_mass* expected_centroid,
	const ve_fontcache_backend_test_center_of_mass* observed_centroid,
	std::string_view extra,
	std::string_view probable_cause )
{
	std::ostringstream oss;
	oss << case_id
		<< ": expected bbox=" << ve_fontcache_backend_test_format_bbox( expected_bbox )
		<< ", observed bbox=" << ve_fontcache_backend_test_format_bbox( observed_bbox );
	if ( expected_centroid && observed_centroid ) {
		oss << ", expected centroid=" << ve_fontcache_backend_test_format_center( *expected_centroid )
			<< ", observed centroid=" << ve_fontcache_backend_test_format_center( *observed_centroid );
	}
	if ( !extra.empty() ) {
		oss << ", " << extra;
	}
	oss << ", probable cause: " << probable_cause;
	return oss.str();
}

inline void ve_fontcache_backend_test_append_triangle_vertices(
	ve_fontcache_drawlist& drawlist,
	const std::array< ve_fontcache_vertex, 3 >& vertices )
{
	const int vertex_offset = static_cast< int >( drawlist.vertices.size() );
	drawlist.vertices.insert( drawlist.vertices.end(), vertices.begin(), vertices.end() );
	drawlist.indices.push_back( vertex_offset + 0 );
	drawlist.indices.push_back( vertex_offset + 1 );
	drawlist.indices.push_back( vertex_offset + 2 );
}

inline void ve_fontcache_backend_test_surface_size_for_pass( const ve_fontcache* cache, uint32_t pass, float& width_out, float& height_out )
{
	switch ( pass ) {
		case VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH:
			width_out = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH );
			height_out = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
			break;
		case VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS:
			width_out = static_cast< float >( VE_FONTCACHE_ATLAS_WIDTH );
			height_out = static_cast< float >( VE_FONTCACHE_ATLAS_HEIGHT );
			break;
		default:
			width_out = static_cast< float >( ve_fontcache_backend_test_target_width( cache ) );
			height_out = static_cast< float >( ve_fontcache_backend_test_target_height( cache ) );
			break;
	}
}

inline void ve_fontcache_backend_test_source_size_for_pass( uint32_t pass, float& width_out, float& height_out )
{
	switch ( pass ) {
		case VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS:
		case VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED:
			width_out = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH );
			height_out = static_cast< float >( VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
			break;
		case VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET:
			width_out = static_cast< float >( VE_FONTCACHE_ATLAS_WIDTH );
			height_out = static_cast< float >( VE_FONTCACHE_ATLAS_HEIGHT );
			break;
		case VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED:
			width_out = static_cast< float >( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE );
			height_out = static_cast< float >( VE_FONTCACHE_CPU_ATLAS_PAGE_SIZE );
			break;
		default:
			width_out = 1.0f;
			height_out = 1.0f;
			break;
	}
}

inline void ve_fontcache_backend_test_transform_dest_rect(
	const ve_fontcache* cache,
	uint32_t pass,
	const ve_fontcache_backend_test_rect& rect,
	float& x0,
	float& y0,
	float& x1,
	float& y1 )
{
	float rect_x = static_cast< float >( rect.x );
	float rect_y = static_cast< float >( rect.y );
	float rect_w = static_cast< float >( rect.w );
	float rect_h = static_cast< float >( rect.h );
	float width = 0.0f;
	float height = 0.0f;
	ve_fontcache_backend_test_surface_size_for_pass( cache, pass, width, height );
	if ( pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH || pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS ) {
		ve_fontcache_screenspace_xform( rect_x, rect_y, rect_w, rect_h, width, height );
	} else {
		ve_fontcache_texspace_xform( rect_x, rect_y, rect_w, rect_h, width, height );
	}
	x0 = rect_x;
	y0 = rect_y;
	x1 = rect_x + rect_w;
	y1 = rect_y + rect_h;
}

inline void ve_fontcache_backend_test_transform_source_rect(
	uint32_t pass,
	const ve_fontcache_backend_test_rect& rect,
	float& u0,
	float& v0,
	float& u1,
	float& v1 )
{
	float rect_x = static_cast< float >( rect.x );
	float rect_y = static_cast< float >( rect.y );
	float rect_w = static_cast< float >( rect.w );
	float rect_h = static_cast< float >( rect.h );
	float width = 0.0f;
	float height = 0.0f;
	ve_fontcache_backend_test_source_size_for_pass( pass, width, height );
	ve_fontcache_texspace_xform( rect_x, rect_y, rect_w, rect_h, width, height );
	u0 = rect_x;
	v0 = rect_y;
	u1 = rect_x + rect_w;
	v1 = rect_y + rect_h;
}

template< typename BuildFn >
inline void ve_fontcache_backend_test_append_draw(
	ve_fontcache* cache,
	uint32_t pass,
	uint32_t region,
	bool clear_before_draw,
	const std::array< float, 4 >& colour,
	BuildFn&& build )
{
	ve_fontcache_draw draw;
	draw.pass = pass;
	draw.region = region;
	draw.clear_before_draw = clear_before_draw;
	for ( size_t i = 0; i < colour.size(); i++ ) {
		draw.colour[ i ] = colour[ i ];
	}
	draw.start_index = static_cast< uint32_t >( cache->drawlist.indices.size() );
	build( cache->drawlist );
	draw.end_index = static_cast< uint32_t >( cache->drawlist.indices.size() );
	cache->drawlist.dcalls.push_back( draw );
}

inline void ve_fontcache_backend_test_append_quad(
	ve_fontcache* cache,
	uint32_t pass,
	const ve_fontcache_backend_test_rect& dest_rect,
	const ve_fontcache_backend_test_rect& source_rect = {},
	uint32_t region = 0,
	const std::array< float, 4 >& colour = { 1.0f, 1.0f, 1.0f, 1.0f } )
{
	float x0 = 0.0f;
	float y0 = 0.0f;
	float x1 = 0.0f;
	float y1 = 0.0f;
	ve_fontcache_backend_test_transform_dest_rect( cache, pass, dest_rect, x0, y0, x1, y1 );
	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = 0.0f;
	float v1 = 0.0f;
	if ( pass != VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH ) {
		ve_fontcache_backend_test_transform_source_rect( pass, source_rect, u0, v0, u1, v1 );
	}

	ve_fontcache_backend_test_append_draw(
		cache,
		pass,
		region,
		false,
		colour,
		[&]( ve_fontcache_drawlist& drawlist ) {
			ve_fontcache_blit_quad( drawlist, x0, y0, x1, y1, u0, v0, u1, v1 );
		} );
}

inline void ve_fontcache_backend_test_append_clear( ve_fontcache* cache, uint32_t pass )
{
	ve_fontcache_backend_test_append_draw(
		cache,
		pass,
		0,
		true,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		[]( ve_fontcache_drawlist& ) {} );
}

inline void ve_fontcache_backend_test_append_atlas_clear(
	ve_fontcache* cache,
	const ve_fontcache_backend_test_rect& dest_rect )
{
	ve_fontcache_backend_test_append_quad( cache, VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS, dest_rect, {}, static_cast< uint32_t >( -1 ) );
}

inline void ve_fontcache_backend_test_append_atlas_blit(
	ve_fontcache* cache,
	const ve_fontcache_backend_test_rect& dest_rect,
	const ve_fontcache_backend_test_rect& source_rect )
{
	ve_fontcache_backend_test_append_quad( cache, VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS, dest_rect, source_rect, 0 );
}

inline void ve_fontcache_backend_test_append_square(
	ve_fontcache* cache,
	uint32_t pass,
	int x,
	int y,
	int size,
	const ve_fontcache_backend_test_rect& source_rect = {},
	const std::array< float, 4 >& colour = { 1.0f, 1.0f, 1.0f, 1.0f } )
{
	ve_fontcache_backend_test_append_quad( cache, pass, { x, y, size, size }, source_rect, 0, colour );
}

inline void ve_fontcache_backend_test_append_hollow_square(
	ve_fontcache* cache,
	int x,
	int y,
	int size,
	int thickness )
{
	ve_fontcache_backend_test_append_square( cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, x, y, size );
	ve_fontcache_backend_test_append_square( cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, x + thickness, y + thickness, size - 2 * thickness );
}

inline void ve_fontcache_backend_test_append_l_shape(
	ve_fontcache* cache,
	int x,
	int y,
	int width,
	int height,
	int thickness )
{
	ve_fontcache_backend_test_append_quad( cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, { x, y, thickness, height } );
	ve_fontcache_backend_test_append_quad( cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, { x + thickness, y, width - thickness, thickness } );
}

inline void ve_fontcache_backend_test_append_diagonal_seam_square(
	ve_fontcache* cache,
	int x,
	int y,
	int size )
{
	float x0 = 0.0f;
	float y0 = 0.0f;
	float x1 = 0.0f;
	float y1 = 0.0f;
	ve_fontcache_backend_test_transform_dest_rect(
		cache,
		VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH,
		{ x, y, size, size },
		x0,
		y0,
		x1,
		y1 );

	ve_fontcache_backend_test_append_draw(
		cache,
		VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH,
		0,
		false,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		[&]( ve_fontcache_drawlist& drawlist ) {
			ve_fontcache_backend_test_append_triangle_vertices( drawlist, { {
				{ x0, y0, 0.0f, 0.0f },
				{ x1, y0, 0.0f, 0.0f },
				{ x0, y1, 0.0f, 0.0f },
			} } );
			ve_fontcache_backend_test_append_triangle_vertices( drawlist, { {
				{ x1, y0, 0.0f, 0.0f },
				{ x1, y1, 0.0f, 0.0f },
				{ x0, y1, 0.0f, 0.0f },
			} } );
		} );
}

inline void ve_fontcache_backend_test_append_overlapping_quads(
	ve_fontcache* cache,
	uint32_t pass,
	const ve_fontcache_backend_test_rect& first_rect,
	const ve_fontcache_backend_test_rect& second_rect,
	const ve_fontcache_backend_test_rect& first_source = {},
	const ve_fontcache_backend_test_rect& second_source = {},
	const std::array< float, 4 >& first_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	const std::array< float, 4 >& second_colour = { 1.0f, 1.0f, 1.0f, 1.0f } )
{
	ve_fontcache_backend_test_append_quad( cache, pass, first_rect, first_source, 0, first_colour );
	ve_fontcache_backend_test_append_quad( cache, pass, second_rect, second_source, 0, second_colour );
}

inline bool ve_fontcache_backend_test_write_surface_region(
	const ve_fontcache_backend_test_options& options,
	const char* name,
	int x,
	int y,
	int w,
	int h,
	const std::vector< uint8_t >& pixels )
{
	return options.write_surface
		&& static_cast< int >( pixels.size() ) == w * h
		&& options.write_surface( name, x, y, w, h, pixels.data() );
}

inline bool ve_fontcache_backend_test_readback_surface_full(
	const ve_fontcache_backend_test_options& options,
	const char* name,
	std::vector< uint8_t >& pixels )
{
	int width = 0;
	int height = 0;
	if ( !ve_fontcache_backend_test_surface_extent( options, name, width, height ) ) {
		return false;
	}
	return ve_fontcache_backend_test_readback_texture( options, name, 0, 0, width, height, pixels );
}

inline bool ve_fontcache_backend_test_capture_surface_snapshot(
	const ve_fontcache_backend_test_options& options,
	ve_fontcache_backend_test_surface_snapshot& snapshot )
{
	bool ok = ve_fontcache_backend_test_readback_surface_full( options, "glyph_buffer", snapshot.glyph_buffer )
		&& ve_fontcache_backend_test_readback_surface_full( options, "atlas", snapshot.atlas );
	if ( options.capabilities.has_target_linear_surface ) {
		ok = ok && ve_fontcache_backend_test_readback_surface_full(
			options,
			ve_fontcache_backend_test_target_linear_surface_name(),
			snapshot.target_linear );
	}
	if ( options.capabilities.has_present_surface ) {
		if ( !ve_fontcache_backend_test_readback_surface_full(
			options,
			ve_fontcache_backend_test_presented_surface_name(),
			snapshot.presented ) ) {
			snapshot.presented = snapshot.target_linear;
		}
	} else {
		snapshot.presented = snapshot.target_linear;
	}
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.capabilities.has_cpu_atlas_surface && options.cache && options.cache->use_freetype ) {
		ok = ok && ve_fontcache_backend_test_readback_surface_full(
			options,
			ve_fontcache_backend_test_cpu_atlas_page_surface_name(),
			snapshot.cpu_atlas_page_0 );
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
	return ok;
}

inline bool ve_fontcache_backend_test_execute_pipeline( const ve_fontcache_backend_test_options& options )
{
	if ( !options.execute_pipeline ) {
		return false;
	}

	options.execute_pipeline();
	return true;
}

inline bool ve_fontcache_backend_test_execute_present( const ve_fontcache_backend_test_options& options )
{
	if ( !options.execute_present ) {
		return false;
	}

	options.execute_present();
	return true;
}

inline bool ve_fontcache_backend_test_execute_frame( const ve_fontcache_backend_test_options& options )
{
	if ( !options.execute_frame ) {
		return false;
	}

	options.execute_frame();
	return true;
}

inline void ve_fontcache_backend_test_prepare_real_text( ve_fontcache_backend_test_options& options )
{
	if ( options.prepare_real_text ) {
		options.prepare_real_text();
	}
	options = ve_fontcache_backend_test_resolve_font_roles( options );
}

inline void ve_fontcache_backend_test_finalise_state( const ve_fontcache_backend_test_options& options )
{
	if ( options.finalise_test_state ) {
		options.finalise_test_state();
	}
}

enum ve_fontcache_backend_test_suite_requirements : uint32_t
{
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU = 1 << 0,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET = 1 << 1,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE = 1 << 2,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT = 1 << 3,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_FRAME = 1 << 4,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_TARGET_LINEAR = 1 << 5,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_CPU_ATLAS = 1 << 6,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_STB_MODE = 1 << 7,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_FREETYPE_MODE = 1 << 8,
	VE_FONTCACHE_BACKEND_TEST_REQUIRES_HARFBUZZ = 1 << 9,
};

inline bool ve_fontcache_backend_test_require_suite(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options,
	std::string_view suite_name,
	uint32_t requirements )
{
	if ( !options.cache ) {
		return false;
	}

	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_STB_MODE ) != 0 ) {
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
		if ( options.cache->use_freetype ) {
			return false;
		}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
	}

	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_FREETYPE_MODE ) != 0 ) {
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
		if ( !options.cache->use_freetype ) {
			return false;
		}
#else
		return false;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
	}

	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_HARFBUZZ ) != 0 && !options.capabilities.supports_harfbuzz_mode ) {
		return false;
	}

	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU ) != 0 ) {
		if ( !options.execute_pipeline || !options.readback_surface ) {
			ve_fontcache_backend_test_skip(
				result,
				std::string( suite_name ) + " skipped: execute_pipeline/readback_surface not supplied" );
			return false;
		}
	}
	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET ) != 0 && !options.reset_surfaces ) {
		ve_fontcache_backend_test_skip( result, std::string( suite_name ) + " skipped: reset_surfaces not supplied" );
		return false;
	}
	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) != 0 && !options.write_surface ) {
		ve_fontcache_backend_test_skip( result, std::string( suite_name ) + " skipped: write_surface not supplied" );
		return false;
	}
	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_TARGET_LINEAR ) != 0 && !options.capabilities.has_target_linear_surface ) {
		return false;
	}
	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT ) != 0 ) {
		if ( !options.capabilities.has_present_surface ) {
			return false;
		}
		if ( !options.execute_present ) {
			ve_fontcache_backend_test_expect(
				result,
				false,
				std::string( suite_name ) + " failed: has_present_surface=true but execute_present not supplied" );
			return false;
		}
	}
	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_FRAME ) != 0 && !options.execute_frame ) {
		ve_fontcache_backend_test_expect(
			result,
			false,
			std::string( suite_name ) + " failed: execute_frame not supplied" );
		return false;
	}
	if ( ( requirements & VE_FONTCACHE_BACKEND_TEST_REQUIRES_CPU_ATLAS ) != 0 ) {
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
		if ( !options.capabilities.has_cpu_atlas_surface ) {
			return false;
		}
#else
		return false;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
	}
	return true;
}

inline void ve_fontcache_backend_test_validate_drawlist(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_drawlist& drawlist,
	bool expect_target_pass )
{
	bool has_target_pass = false;
	bool seen_atlas_update = false;
	bool seen_target = false;
	int last_glyph_idx = -1;
	int first_atlas_idx = std::numeric_limits< int >::max();

	for ( uint32_t idx : drawlist.indices ) {
		ve_fontcache_backend_test_expect( result, idx < drawlist.vertices.size(), "index stayed within vertex bounds" );
	}

	for ( const ve_fontcache_vertex& vertex : drawlist.vertices ) {
		ve_fontcache_backend_test_expect( result, std::isfinite( vertex.x ), "vertex x was finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( vertex.y ), "vertex y was finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( vertex.u ), "vertex u was finite" );
		ve_fontcache_backend_test_expect( result, std::isfinite( vertex.v ), "vertex v was finite" );
	}

	for ( size_t i = 0; i < drawlist.dcalls.size(); i++ ) {
		const ve_fontcache_draw& draw = drawlist.dcalls[ i ];
		ve_fontcache_backend_test_expect( result, ve_fontcache_backend_test_is_known_pass( draw.pass ), "drawlist pass type was known" );
		ve_fontcache_backend_test_expect( result, draw.end_index >= draw.start_index, "draw index range was monotonic" );
		ve_fontcache_backend_test_expect( result, draw.end_index <= drawlist.indices.size(), "draw index range stayed in bounds" );

		if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH && draw.end_index == draw.start_index ) {
			ve_fontcache_backend_test_expect( result, draw.clear_before_draw, "empty glyph drawcall was a clear" );
		}

		if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH && draw.end_index > draw.start_index ) {
			last_glyph_idx = static_cast< int >( i );
		}

		if ( ve_fontcache_backend_test_is_atlas_update_pass( draw.pass ) ) {
			seen_atlas_update = true;
			if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS && first_atlas_idx == std::numeric_limits< int >::max() ) {
				first_atlas_idx = static_cast< int >( i );
			}
		}

		if ( ve_fontcache_backend_test_is_target_pass( draw.pass ) ) {
			has_target_pass = true;
			seen_target = true;
			for ( uint32_t idx = draw.start_index; idx < draw.end_index; idx++ ) {
				const ve_fontcache_vertex& vertex = drawlist.vertices[ drawlist.indices[ idx ] ];
				ve_fontcache_backend_test_expect(
					result,
					vertex.u >= -0.01f && vertex.u <= 1.01f,
					"target u stayed near [0, 1]" );
				ve_fontcache_backend_test_expect(
					result,
					vertex.v >= -0.01f && vertex.v <= 1.01f,
					"target v stayed near [0, 1]" );
			}
		} else if ( seen_target && ve_fontcache_backend_test_is_atlas_update_pass( draw.pass ) ) {
			ve_fontcache_backend_test_expect( result, false, "atlas updates finished before target draws" );
		}
	}

	if ( expect_target_pass ) {
		ve_fontcache_backend_test_expect( result, has_target_pass, "non-empty text produced a target draw" );
	}

	if ( last_glyph_idx != -1 && first_atlas_idx != std::numeric_limits< int >::max() ) {
		ve_fontcache_backend_test_expect( result, last_glyph_idx < first_atlas_idx, "glyph passes preceded atlas blits" );
	}

	if ( seen_atlas_update && seen_target ) {
		ve_fontcache_backend_test_expect( result, true, "atlas updates were ordered before target draws" );
	}
}

inline ve_fontcache_drawlist* ve_fontcache_backend_test_draw(
	ve_fontcache_backend_test_result& result,
	ve_fontcache* cache,
	ve_font_id font,
	const std::u8string& text,
	bool shape_cache = true,
	float posx = 0.1f,
	float posy = 0.5f,
	float scalex = 1.0f / 1920.0f,
	float scaley = 1.0f / 1080.0f )
{
	ve_fontcache_flush_drawlist( cache );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_draw_text( cache, font, text, posx, posy, scalex, scaley, shape_cache ),
		"draw_text succeeded" );
	ve_fontcache_optimise_drawlist( cache );
	return ve_fontcache_get_drawlist( cache );
}

inline void ve_fontcache_backend_test_run_structural_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"structural checks",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY } ) ) {
		return;
	}

	const std::u8string text = u8"Hello, World! 日本語";
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, text );
	ve_fontcache_backend_test_expect( result, !drawlist->dcalls.empty(), "drawlist was not empty" );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );
	ve_fontcache_flush_drawlist( options.cache );
}

inline void ve_fontcache_backend_test_run_caching_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"caching checks",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY } ) ) {
		return;
	}

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "caching checks skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	const std::u8string cached_text = u8"GlyphCacheXYZ123";
	const std::u8string uncached_text = u8"FreshUVW987";
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, cached_text );
	int initial_cache_misses = ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist );
	ve_fontcache_backend_test_expect( result, initial_cache_misses > 0, "first draw emitted cache-miss work" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, cached_text );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"cached redraw reused glyph cache" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, cached_text, false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"atlas cache survived shape-cache bypass" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, uncached_text );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
		"different text triggered fresh glyph work" );

	if ( ve_fontcache_backend_test_has_role( options, VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SECONDARY ) ) {
		(void) ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"A" );
		drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.secondary_font, u8"A" );
		ve_fontcache_backend_test_expect(
			result,
			ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
			"same glyph in a second font used an independent cache entry" );
	} else {
		ve_fontcache_backend_test_skip( result, "multi-font independence skipped: secondary role not supplied" );
	}

	ve_fontcache_flush_drawlist( options.cache );
}

inline void ve_fontcache_backend_test_run_region_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "atlas region routing skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	if ( options.small_font >= 0 ) {
		char32_t codepoint = 0;
		char region = '\0';
		bool found = ve_fontcache_backend_test_find_uncached_codepoint_for_regions(
			options.cache,
			options.small_font,
			"AB",
			0x20,
			0x9FFF,
			codepoint,
			region );
		ve_fontcache_backend_test_expect( result, found, "found a small-font glyph in atlas region A or B" );
		if ( found ) {
			ve_fontcache_backend_test_atlas_sizes before = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			(void) ve_fontcache_backend_test_draw( result, options.cache, options.small_font, ve_fontcache_backend_test_codepoint_to_utf8( codepoint ) );
			ve_fontcache_backend_test_atlas_sizes after = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			char actual_region = ve_fontcache_backend_test_growing_region( before, after );
			ve_fontcache_backend_test_expect(
				result,
				actual_region == 'A' || actual_region == 'B',
				"small-font glyph routed into atlas region A or B" );
		}
	} else {
		ve_fontcache_backend_test_skip( result, "small-font routing skipped: small role not supplied" );
	}

	if ( options.latin_font >= 0 ) {
		char32_t codepoint = 0;
		char region = '\0';
		bool found = ve_fontcache_backend_test_find_uncached_codepoint_for_regions(
			options.cache,
			options.latin_font,
			"BC",
			0x20,
			0x00FF,
			codepoint,
			region );
		ve_fontcache_backend_test_expect( result, found, "found a latin glyph in atlas region B or C" );
		if ( found ) {
			ve_fontcache_backend_test_atlas_sizes before = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			(void) ve_fontcache_backend_test_draw( result, options.cache, options.latin_font, ve_fontcache_backend_test_codepoint_to_utf8( codepoint ) );
			ve_fontcache_backend_test_atlas_sizes after = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			char actual_region = ve_fontcache_backend_test_growing_region( before, after );
			ve_fontcache_backend_test_expect(
				result,
				actual_region == 'B' || actual_region == 'C',
				"standard latin glyph routed into atlas region B or C" );
		}
	} else {
		ve_fontcache_backend_test_skip( result, "latin-font routing skipped: latin role not supplied" );
	}

	if ( options.cjk_font >= 0 ) {
		char32_t codepoint = 0;
		char region = '\0';
		bool found = ve_fontcache_backend_test_find_uncached_codepoint_for_regions(
			options.cache,
			options.cjk_font,
			"CD",
			0x4E00,
			0x9FFF,
			codepoint,
			region );
		ve_fontcache_backend_test_expect( result, found, "found a CJK glyph in atlas region C or D" );
		if ( found ) {
			ve_fontcache_backend_test_atlas_sizes before = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			(void) ve_fontcache_backend_test_draw( result, options.cache, options.cjk_font, ve_fontcache_backend_test_codepoint_to_utf8( codepoint ) );
			ve_fontcache_backend_test_atlas_sizes after = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
			char actual_region = ve_fontcache_backend_test_growing_region( before, after );
			ve_fontcache_backend_test_expect(
				result,
				actual_region == 'C' || actual_region == 'D',
				"CJK glyph routed into atlas region C or D" );
		}
	} else {
		ve_fontcache_backend_test_skip( result, "CJK routing skipped: cjk role not supplied" );
	}

	if ( options.huge_font >= 0 ) {
		ve_fontcache_backend_test_atlas_sizes before = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
		ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.huge_font, u8"W" );
		ve_fontcache_backend_test_atlas_sizes after = ve_fontcache_backend_test_capture_atlas_sizes( *options.cache );
		ve_fontcache_backend_test_expect(
			result,
			ve_fontcache_backend_test_count_pass( *drawlist, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED ) > 0,
			"very large glyphs used the uncached target path" );
		ve_fontcache_backend_test_expect(
			result,
			after.region_A == before.region_A
				&& after.region_B == before.region_B
				&& after.region_C == before.region_C
				&& after.region_D == before.region_D,
			"very large glyphs skipped atlas insertion" );
	} else {
		ve_fontcache_backend_test_skip( result, "huge-font routing skipped: huge role not supplied" );
	}
}

inline void ve_fontcache_backend_test_run_lru_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "atlas LRU checks skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	if ( !ve_fontcache_backend_test_require_any_role(
		result,
		options,
		"atlas LRU checks",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SMALL, VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_CJK } ) ) {
		return;
	}

	ve_fontcache_LRU_init( options.cache->atlas.stateA, VE_FONTCACHE_ATLAS_REGION_A_CAPACITY );
	ve_fontcache_LRU_init( options.cache->atlas.stateB, VE_FONTCACHE_ATLAS_REGION_B_CAPACITY );
	ve_fontcache_LRU_init( options.cache->atlas.stateC, VE_FONTCACHE_ATLAS_REGION_C_CAPACITY );
	ve_fontcache_LRU_init( options.cache->atlas.stateD, VE_FONTCACHE_ATLAS_REGION_D_CAPACITY );
	options.cache->atlas.next_atlas_idx_A = 0;
	options.cache->atlas.next_atlas_idx_B = 0;
	options.cache->atlas.next_atlas_idx_C = 0;
	options.cache->atlas.next_atlas_idx_D = 0;
	ve_fontcache_flush_drawlist( options.cache );

	struct candidate
	{
		ve_font_id font = -1;
		char region = '\0';
		size_t capacity = 0;
		char32_t start = 0;
		char32_t end = 0;
	};

	const candidate candidates[] = {
		{ options.small_font, 'A', static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_A_CAPACITY ), 0x4E00, 0x9FFF },
		{ options.small_font, 'B', static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_B_CAPACITY ), 0x20, 0x9FFF },
		{ options.cjk_font, 'C', static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_C_CAPACITY ), 0x4E00, 0x9FFF },
		{ options.cjk_font, 'D', static_cast< size_t >( VE_FONTCACHE_ATLAS_REGION_D_CAPACITY ), 0x4E00, 0x9FFF },
	};

	candidate chosen {};
	std::vector< char32_t > codepoints;
	for ( const candidate& current : candidates ) {
		if ( current.font < 0 ) {
			continue;
		}

		const size_t starting_size = ve_fontcache_backend_test_region_cache_size( *options.cache, current.region );
		if ( starting_size >= current.capacity ) {
			continue;
		}

		codepoints = ve_fontcache_backend_test_collect_codepoints_for_region(
			options.cache,
			current.font,
			current.region,
			( current.capacity - starting_size ) + 1,
			current.start,
			current.end );
		if ( codepoints.size() >= ( current.capacity - starting_size ) + 1 ) {
			chosen = current;
			break;
		}
	}

	if ( chosen.font < 0 || codepoints.empty() ) {
		ve_fontcache_backend_test_skip( result, "atlas LRU checks skipped: no region supplied enough glyphs" );
		return;
	}

	const size_t starting_size = ve_fontcache_backend_test_region_cache_size( *options.cache, chosen.region );
	const size_t fill_count = chosen.capacity - starting_size;
	if ( codepoints.size() < fill_count + 1 ) {
		ve_fontcache_backend_test_skip( result, "atlas LRU checks skipped: region was too warm to overflow reliably" );
		return;
	}

	for ( size_t i = 0; i < fill_count; i++ ) {
		(void) ve_fontcache_backend_test_draw(
			result,
			options.cache,
			chosen.font,
			ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ i ] ),
			false );
	}

	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_region_cache_size( *options.cache, chosen.region ) == chosen.capacity,
		"atlas region filled to its expected capacity" );

	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		chosen.font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ fill_count - 1 ] ),
		false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"most recently touched glyph stayed cached before overflow" );

	drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		chosen.font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ fill_count ] ),
		false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
		"overflow glyph triggered a fresh cache upload" );

	drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		chosen.font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ 0 ] ),
		false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
		"least recently used glyph was evicted and regenerated" );

	drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		chosen.font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoints[ fill_count - 1 ] ),
		false );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"recent glyph stayed cached after eviction pressure" );
}

inline void ve_fontcache_backend_test_run_edge_case_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"edge-case checks",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY } ) ) {
		return;
	}

	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"" );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"empty string emitted no cache-miss work" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"   " );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"whitespace-only text emitted no glyph upload work" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8" " );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) == 0,
		"single space emitted no glyph upload work" );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"\n" );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, false );

	drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		options.font,
		ve_fontcache_backend_test_make_ascii_string( 10000 ) );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );

	ve_fontcache_flush_drawlist( options.cache );
	size_t previous_dcalls = 0;
	bool monotonic = true;
	for ( int i = 0; i < 100; i++ ) {
		std::u8string label = u8"frame-";
		std::string suffix = std::to_string( i );
		for ( char ch : suffix ) {
			label.push_back( static_cast< char8_t >( ch ) );
		}
		ve_fontcache_backend_test_expect(
			result,
			ve_fontcache_draw_text( options.cache, options.font, label, 0.0f, 0.0f, 1.0f / 1920.0f, 1.0f / 1080.0f, false ),
			"draw_text succeeded without drawlist flush" );
		size_t current_dcalls = ve_fontcache_get_drawlist( options.cache )->dcalls.size();
		monotonic = monotonic && current_dcalls >= previous_dcalls;
		previous_dcalls = current_dcalls;
	}
	ve_fontcache_backend_test_expect( result, monotonic, "drawlist grew monotonically without per-call flushes" );
	ve_fontcache_flush_drawlist( options.cache );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"\u4E2D\u0627\u05D0\U0001F600" );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, false );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"micro", true, 0.0f, 0.0f, 0.0001f, 0.0001f );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"huge", true, 0.0f, 0.0f, 10.0f, 10.0f );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );

	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"negative", true, -1.0f, -1.0f );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );

	ve_fontcache_configure_snap( options.cache, 0, 0 );
	drawlist = ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"snap-zero" );
	ve_fontcache_backend_test_validate_drawlist( result, *drawlist, true );
	ve_fontcache_backend_test_expect(
		result,
		!ve_fontcache_is_valid_font_id( options.cache, -1 ),
		"invalid font id was rejected before draw submission" );
}

inline void ve_fontcache_backend_test_run_reload_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"font unload and reload",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY } ) ) {
		return;
	}

#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "font unload and reload skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	if ( !options.reload_font ) {
		ve_fontcache_backend_test_skip( result, "font unload and reload skipped: reload_font not supplied" );
		return;
	}

	(void) ve_fontcache_backend_test_draw( result, options.cache, options.font, u8"Reload me" );
	ve_fontcache_unload( options.cache, options.font );
	ve_font_id reloaded_font = options.reload_font();
	ve_fontcache_backend_test_expect( result, reloaded_font >= 0, "font reload succeeded" );
	if ( reloaded_font < 0 ) {
		return;
	}

	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw( result, options.cache, reloaded_font, u8"Reload me" );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_count_cache_miss_passes( *options.cache, *drawlist ) > 0,
		"unload and reload invalidated cached glyph state" );
}

inline void ve_fontcache_backend_test_run_readback_checks(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "GPU readback checks skipped in FreeType mode" );
		return;
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	if ( !options.execute_pipeline || !options.readback_surface ) {
		ve_fontcache_backend_test_skip( result, "GPU readback checks skipped: execute_pipeline/readback_surface callbacks not supplied" );
		return;
	}

	ve_font_id font = ve_fontcache_backend_test_pick_first_available_role(
		options,
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SMALL, VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY } );
	if ( font < 0 ) {
		ve_fontcache_backend_test_skip( result, "GPU readback checks skipped: primary/small roles not supplied" );
		return;
	}
	char32_t codepoint = 0;
	char region = '\0';
	if ( !ve_fontcache_backend_test_find_uncached_codepoint_for_regions( options.cache, font, "ABCD", 0x20, 0x9FFF, codepoint, region ) ) {
		ve_fontcache_backend_test_skip( result, "GPU readback checks skipped: no atlas-backed glyph was found" );
		return;
	}

	int atlas_index = -1;
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_predict_atlas_slot( options.cache, font, codepoint, region, atlas_index ),
		"predicted an atlas slot for readback testing" );
	if ( atlas_index == -1 ) {
		return;
	}

	float atlas_x = 0.0f;
	float atlas_y = 0.0f;
	float atlas_w = 0.0f;
	float atlas_h = 0.0f;
	ve_fontcache_atlas_bbox( region, atlas_index, atlas_x, atlas_y, atlas_w, atlas_h );

	const int atlas_read_x = static_cast< int >( atlas_x );
	const int atlas_read_y = static_cast< int >( atlas_y );
	const int atlas_read_w = std::max( 1, static_cast< int >( atlas_w ) );
	const int atlas_read_h = std::max( 1, static_cast< int >( atlas_h ) );

	std::vector< uint8_t > atlas_before;
	std::vector< uint8_t > atlas_after;
	std::vector< uint8_t > atlas_again;
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture( options, "atlas", atlas_read_x, atlas_read_y, atlas_read_w, atlas_read_h, atlas_before ),
		"atlas readback before draw succeeded" );

	(void) ve_fontcache_backend_test_draw(
		result,
		options.cache,
		font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoint ),
		false );
	ve_fontcache_backend_test_execute_pipeline( options );

	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture( options, "atlas", atlas_read_x, atlas_read_y, atlas_read_w, atlas_read_h, atlas_after ),
		"atlas readback after draw succeeded" );
	ve_fontcache_backend_test_expect(
		result,
		atlas_after != atlas_before && ve_fontcache_backend_test_any_non_zero( atlas_after ),
		"atlas pixels changed and became non-zero after caching a glyph" );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_has_intermediate_coverage( atlas_after ),
		"atlas snapshot contained anti-aliased intermediate coverage values" );

	(void) ve_fontcache_backend_test_draw(
		result,
		options.cache,
		font,
		ve_fontcache_backend_test_codepoint_to_utf8( codepoint ),
		false );
	ve_fontcache_backend_test_execute_pipeline( options );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture( options, "atlas", atlas_read_x, atlas_read_y, atlas_read_w, atlas_read_h, atlas_again ),
		"atlas readback on redraw succeeded" );
	ve_fontcache_backend_test_expect(
		result,
		atlas_after == atlas_again,
		"atlas snapshot stayed stable on redraw" );

	std::vector< uint8_t > glyph_buffer;
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture(
			options,
			"glyph_buffer",
			0,
			0,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT,
			glyph_buffer ),
		"glyph-buffer readback succeeded" );
	ve_fontcache_backend_test_expect(
		result,
		!ve_fontcache_backend_test_any_non_zero( glyph_buffer ),
		"glyph buffer was cleared after backend execution" );

	std::vector< uint8_t > target_pixels;
	const int target_w = options.cache->snap_width ? static_cast< int >( options.cache->snap_width ) : 1920;
	const int target_h = options.cache->snap_height ? static_cast< int >( options.cache->snap_height ) : 1080;
	(void) ve_fontcache_backend_test_draw( result, options.cache, font, u8"Target output" );
	ve_fontcache_backend_test_execute_pipeline( options );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_readback_texture( options, "target", 0, 0, target_w, target_h, target_pixels ),
		"target readback succeeded" );
	ve_fontcache_backend_test_expect(
		result,
		ve_fontcache_backend_test_any_non_zero( target_pixels ),
		"target output contained visible text pixels" );
}

inline void ve_fontcache_backend_test_reset_state( const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_flush_drawlist( options.cache );
	if ( options.reset_surfaces ) {
		options.reset_surfaces();
	}
}

inline std::vector< uint8_t > ve_fontcache_backend_test_make_normalized_quadrant_summary(
	const std::array< double, 4 >& quadrants )
{
	std::vector< uint8_t > summary( 4, 0 );
	const double min_value = *std::min_element( quadrants.begin(), quadrants.end() );
	const double max_value = *std::max_element( quadrants.begin(), quadrants.end() );
	const double scale = max_value > min_value ? 255.0 / ( max_value - min_value ) : 0.0;
	const auto normalize = [&]( double value ) {
		if ( scale == 0.0 ) {
			return static_cast< uint8_t >( 0 );
		}
		const double scaled = std::clamp( ( value - min_value ) * scale, 0.0, 255.0 );
		return static_cast< uint8_t >( std::lround( scaled ) );
	};

	// Row-major in the test image's bottom-left coordinate convention: [bl, br, tl, tr].
	summary[ 0 ] = normalize( quadrants[ 2 ] );
	summary[ 1 ] = normalize( quadrants[ 3 ] );
	summary[ 2 ] = normalize( quadrants[ 0 ] );
	summary[ 3 ] = normalize( quadrants[ 1 ] );
	return summary;
}

inline std::vector< uint8_t > ve_fontcache_backend_test_make_normalized_quadrant_summary(
	const std::vector< uint8_t >& pixels,
	int width,
	int height,
	const ve_fontcache_backend_test_rect& rect )
{
	if ( !ve_fontcache_backend_test_rect_valid( rect ) ) {
		return {};
	}

	return ve_fontcache_backend_test_make_normalized_quadrant_summary(
		ve_fontcache_backend_test_quadrant_means( pixels, width, height, rect ) );
}

inline bool ve_fontcache_backend_test_execute_pipeline_and_present( const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_execute_pipeline( options ) ) {
		return false;
	}

	if ( options.capabilities.has_present_surface && !ve_fontcache_backend_test_execute_present( options ) ) {
		return false;
	}

	return true;
}

inline uint64_t ve_fontcache_backend_test_hash_pixels( const std::vector< uint8_t >& pixels )
{
	uint64_t hash = 1469598103934665603ULL;
	for ( uint8_t value : pixels ) {
		hash ^= value;
		hash *= 1099511628211ULL;
	}
	return hash;
}

inline void ve_fontcache_backend_test_run_surface_roundtrip(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"surface_roundtrip",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET | VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) ) {
		return;
	}

	if ( !options.readback_surface ) {
		ve_fontcache_backend_test_skip( result, "surface_roundtrip skipped: readback_surface not supplied" );
		return;
	}

	const auto run_case = [&]( const char* case_id, const char* surface_name, const ve_fontcache_backend_test_rect& rect ) {
		std::vector< uint8_t > expected = ve_fontcache_backend_test_make_image( rect.w, rect.h, 0 );
		ve_fontcache_backend_test_fill_quadrant_pattern( expected, rect.w, rect.h, 0, 0, rect.w, rect.h, 250, 160, 80, 20 );
		ve_fontcache_backend_test_reset_state( options );
		if ( !ve_fontcache_backend_test_write_surface_region( options, surface_name, rect.x, rect.y, rect.w, rect.h, expected ) ) {
			ve_fontcache_backend_test_skip(
				result,
				std::string( case_id ) + " skipped: surface is not writable through write_surface" );
			return;
		}

		std::vector< uint8_t > observed;
		bool ok = ve_fontcache_backend_test_readback_texture(
			options,
			surface_name,
			rect.x,
			rect.y,
			rect.w,
			rect.h,
			observed );
		const ve_fontcache_backend_test_diff_stats diff =
			ve_fontcache_backend_test_expected_vs_actual_diff( expected, observed, rect.w, rect.h );
		const ve_fontcache_backend_test_mirror_scores mirror_scores =
			ve_fontcache_backend_test_mirror_scores_for_expected( expected, observed, rect.w, rect.h );
		const bool vertical_flip = ve_fontcache_backend_test_is_vertical_flip( mirror_scores );
		const bool horizontal_flip = ve_fontcache_backend_test_is_horizontal_flip( mirror_scores );
		std::string cause = "surface round-trip mismatch";
		if ( vertical_flip ) {
			cause = "surface round-trip introduced a V-axis inversion";
		} else if ( horizontal_flip ) {
			cause = "surface round-trip introduced a U-axis inversion";
		}

		ve_fontcache_backend_test_expect(
			result,
			ok
				&& diff.mean_abs_error <= 1.0
				&& diff.max_abs_error <= 4
				&& !vertical_flip
				&& !horizontal_flip,
			std::string( case_id ) + ": diff=" + ve_fontcache_backend_test_format_diff( diff )
				+ ", mirror_scores={direct=" + std::to_string( mirror_scores.direct )
				+ ", horizontal=" + std::to_string( mirror_scores.horizontal )
				+ ", vertical=" + std::to_string( mirror_scores.vertical )
				+ ", both=" + std::to_string( mirror_scores.both ) + "}"
				+ "; probable cause: " + cause );
	};

	run_case( "surface_roundtrip.glyph_buffer", "glyph_buffer", { 96, 80, 48, 48 } );
	run_case( "surface_roundtrip.atlas", "atlas", { 512, 384, 48, 48 } );
	if ( options.capabilities.has_target_linear_surface ) {
		run_case(
			"surface_roundtrip.target_linear",
			ve_fontcache_backend_test_target_linear_surface_name(),
			{ 120, 96, 48, 48 } );
	}
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( options.cache->use_freetype && options.capabilities.has_cpu_atlas_surface ) {
		run_case(
			"surface_roundtrip.cpu_atlas_page_0",
			ve_fontcache_backend_test_cpu_atlas_page_surface_name(),
			{ 96, 96, 48, 48 } );
	}
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
}

inline void ve_fontcache_backend_test_run_surface_contract(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"surface_contract",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET | VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) ) {
		return;
	}

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_surface_snapshot reset_snapshot;
	bool reset_ok = ve_fontcache_backend_test_capture_surface_snapshot( options, reset_snapshot );
	const bool reset_zero = reset_ok
		&& !ve_fontcache_backend_test_any_non_zero( reset_snapshot.glyph_buffer )
		&& !ve_fontcache_backend_test_any_non_zero( reset_snapshot.atlas )
		&& !ve_fontcache_backend_test_any_non_zero( reset_snapshot.target_linear );
	ve_fontcache_backend_test_bbox glyph_reset_bbox = ve_fontcache_backend_test_thresholded_bbox(
		reset_snapshot.glyph_buffer,
		VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
		VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
	ve_fontcache_backend_test_bbox atlas_reset_bbox = ve_fontcache_backend_test_thresholded_bbox(
		reset_snapshot.atlas,
		VE_FONTCACHE_ATLAS_WIDTH,
		VE_FONTCACHE_ATLAS_HEIGHT );
	ve_fontcache_backend_test_bbox target_reset_bbox = ve_fontcache_backend_test_thresholded_bbox(
		reset_snapshot.target_linear,
		ve_fontcache_backend_test_target_width( options.cache ),
		ve_fontcache_backend_test_target_height( options.cache ) );
	std::string reset_extra = "glyph_bbox=" + ve_fontcache_backend_test_format_bbox( glyph_reset_bbox )
		+ ", atlas_bbox=" + ve_fontcache_backend_test_format_bbox( atlas_reset_bbox )
		+ ", target_bbox=" + ve_fontcache_backend_test_format_bbox( target_reset_bbox );
	ve_fontcache_backend_test_expect(
		result,
		reset_zero,
		"surface_contract.reset_zero: expected glyph_buffer/atlas/target to be empty after reset; observed "
			+ reset_extra
			+ "; probable cause: clear pass not applied or applied to the wrong surface" );

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_surface_snapshot before_empty;
	ve_fontcache_backend_test_surface_snapshot after_empty;
	bool empty_ok = ve_fontcache_backend_test_capture_surface_snapshot( options, before_empty );
	ve_fontcache_backend_test_execute_pipeline( options );
	empty_ok = empty_ok && ve_fontcache_backend_test_capture_surface_snapshot( options, after_empty );
	bool empty_mutated = empty_ok
		&& ( before_empty.glyph_buffer != after_empty.glyph_buffer
			|| before_empty.atlas != after_empty.atlas
			|| before_empty.target_linear != after_empty.target_linear );
	std::string empty_extra;
	if ( empty_mutated ) {
		empty_extra = "glyph_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				before_empty.glyph_buffer,
				after_empty.glyph_buffer,
				VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
				VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ) )
			+ ", atlas_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				before_empty.atlas,
				after_empty.atlas,
				VE_FONTCACHE_ATLAS_WIDTH,
				VE_FONTCACHE_ATLAS_HEIGHT ) )
			+ ", target_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				before_empty.target_linear,
				after_empty.target_linear,
				ve_fontcache_backend_test_target_width( options.cache ),
				ve_fontcache_backend_test_target_height( options.cache ) ) );
	}
	ve_fontcache_backend_test_expect(
		result,
		empty_ok && !empty_mutated,
		"surface_contract.empty_execute: expected empty execute to preserve all surfaces, observed "
			+ empty_extra
			+ "; probable cause: stale drawlist data or unintended state leakage during execute" );

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_append_square( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, 96, 80, 64 );
	ve_fontcache_backend_test_execute_pipeline( options );
	ve_fontcache_backend_test_surface_snapshot glyph_only_snapshot;
	bool glyph_only_ok = ve_fontcache_backend_test_capture_surface_snapshot( options, glyph_only_snapshot );
	const bool glyph_only_pass = glyph_only_ok
		&& ve_fontcache_backend_test_any_non_zero( glyph_only_snapshot.glyph_buffer )
		&& !ve_fontcache_backend_test_any_non_zero( glyph_only_snapshot.atlas )
		&& !ve_fontcache_backend_test_any_non_zero( glyph_only_snapshot.target_linear );
	ve_fontcache_backend_test_expect(
		result,
		glyph_only_pass,
		"surface_contract.glyph_isolated: expected only glyph_buffer to change, observed glyph_bbox="
			+ ve_fontcache_backend_test_format_bbox( ve_fontcache_backend_test_thresholded_bbox(
				glyph_only_snapshot.glyph_buffer,
				VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
				VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ) )
			+ ", atlas_bbox="
			+ ve_fontcache_backend_test_format_bbox( ve_fontcache_backend_test_thresholded_bbox(
				glyph_only_snapshot.atlas,
				VE_FONTCACHE_ATLAS_WIDTH,
				VE_FONTCACHE_ATLAS_HEIGHT ) )
			+ ", target_bbox="
			+ ve_fontcache_backend_test_format_bbox( ve_fontcache_backend_test_thresholded_bbox(
				glyph_only_snapshot.target_linear,
				ve_fontcache_backend_test_target_width( options.cache ),
				ve_fontcache_backend_test_target_height( options.cache ) ) )
			+ "; probable cause: wrong framebuffer bound during GLYPH pass" );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect atlas_source = { 64, 96, 192, 192 };
	const ve_fontcache_backend_test_rect atlas_dest = { 320, 224, 48, 48 };
	std::vector< uint8_t > atlas_source_pattern = ve_fontcache_backend_test_make_image( atlas_source.w, atlas_source.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		atlas_source_pattern,
		atlas_source.w,
		atlas_source.h,
		0,
		0,
		atlas_source.w,
		atlas_source.h,
		240,
		160,
		80,
		20 );
	bool atlas_only_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"glyph_buffer",
		atlas_source.x,
		atlas_source.y,
		atlas_source.w,
		atlas_source.h,
		atlas_source_pattern );
	ve_fontcache_backend_test_surface_snapshot atlas_before;
	ve_fontcache_backend_test_surface_snapshot atlas_after;
	atlas_only_ok = atlas_only_ok && ve_fontcache_backend_test_capture_surface_snapshot( options, atlas_before );
	ve_fontcache_backend_test_append_atlas_clear( options.cache, atlas_dest );
	ve_fontcache_backend_test_append_atlas_blit( options.cache, atlas_dest, atlas_source );
	ve_fontcache_backend_test_execute_pipeline( options );
	atlas_only_ok = atlas_only_ok && ve_fontcache_backend_test_capture_surface_snapshot( options, atlas_after );
	const bool atlas_only_pass = atlas_only_ok
		&& atlas_before.glyph_buffer == atlas_after.glyph_buffer
		&& atlas_before.target_linear == atlas_after.target_linear
		&& atlas_before.atlas != atlas_after.atlas
		&& ve_fontcache_backend_test_any_non_zero( atlas_after.atlas );
	ve_fontcache_backend_test_expect(
		result,
		atlas_only_pass,
		"surface_contract.atlas_isolated: expected only atlas to change, observed atlas_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				atlas_before.atlas,
				atlas_after.atlas,
				VE_FONTCACHE_ATLAS_WIDTH,
				VE_FONTCACHE_ATLAS_HEIGHT ) )
			+ ", glyph_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				atlas_before.glyph_buffer,
				atlas_after.glyph_buffer,
				VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
				VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ) )
			+ ", target_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				atlas_before.target_linear,
				atlas_after.target_linear,
				ve_fontcache_backend_test_target_width( options.cache ),
				ve_fontcache_backend_test_target_height( options.cache ) ) )
			+ "; probable cause: wrong framebuffer bound during ATLAS pass" );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect target_source = { 900, 700, 64, 64 };
	const ve_fontcache_backend_test_rect target_dest = { 320, 240, 64, 64 };
	std::vector< uint8_t > target_source_pattern = ve_fontcache_backend_test_make_image( target_source.w, target_source.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		target_source_pattern,
		target_source.w,
		target_source.h,
		0,
		0,
		target_source.w,
		target_source.h,
		255,
		180,
		110,
		40 );
	bool target_only_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		target_source.x,
		target_source.y,
		target_source.w,
		target_source.h,
		target_source_pattern );
	ve_fontcache_backend_test_surface_snapshot target_before;
	ve_fontcache_backend_test_surface_snapshot target_after;
	target_only_ok = target_only_ok && ve_fontcache_backend_test_capture_surface_snapshot( options, target_before );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, target_dest, target_source );
	ve_fontcache_backend_test_execute_pipeline( options );
	target_only_ok = target_only_ok && ve_fontcache_backend_test_capture_surface_snapshot( options, target_after );
	const bool target_only_pass = target_only_ok
		&& target_before.glyph_buffer == target_after.glyph_buffer
		&& target_before.atlas == target_after.atlas
		&& target_before.target_linear != target_after.target_linear
		&& ve_fontcache_backend_test_any_non_zero( target_after.target_linear );
	ve_fontcache_backend_test_expect(
		result,
		target_only_pass,
		"surface_contract.target_linear_isolated: expected only target to change, observed target_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				target_before.target_linear,
				target_after.target_linear,
				ve_fontcache_backend_test_target_width( options.cache ),
				ve_fontcache_backend_test_target_height( options.cache ) ) )
			+ ", glyph_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				target_before.glyph_buffer,
				target_after.glyph_buffer,
				VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
				VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT ) )
			+ ", atlas_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				target_before.atlas,
				target_after.atlas,
				VE_FONTCACHE_ATLAS_WIDTH,
				VE_FONTCACHE_ATLAS_HEIGHT ) )
			+ "; probable cause: wrong framebuffer bound during TARGET pass" );
}

inline void ve_fontcache_backend_test_run_glyph_geometry(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"glyph_geometry",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect glyph_square = { 96, 80, 64, 64 };
	const ve_fontcache_backend_test_rect glyph_read = { 88, 72, 80, 80 };

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_append_square( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, glyph_square.x, glyph_square.y, glyph_square.w );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > glyph_square_pixels;
	bool square_ok = ve_fontcache_backend_test_readback_texture(
		options,
		"glyph_buffer",
		glyph_read.x,
		glyph_read.y,
		glyph_read.w,
		glyph_read.h,
		glyph_square_pixels );
	std::vector< uint8_t > expected_square = ve_fontcache_backend_test_make_image( glyph_read.w, glyph_read.h, 0 );
	ve_fontcache_backend_test_fill_rect(
		expected_square,
		glyph_read.w,
		glyph_read.h,
		glyph_square.x - glyph_read.x,
		glyph_square.y - glyph_read.y,
		glyph_square.w,
		glyph_square.h,
		255 );
	const ve_fontcache_backend_test_bbox expected_square_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( expected_square, glyph_read.w, glyph_read.h ),
		glyph_read.x,
		glyph_read.y );
	const ve_fontcache_backend_test_bbox observed_square_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( glyph_square_pixels, glyph_read.w, glyph_read.h ),
		glyph_read.x,
		glyph_read.y );
	const ve_fontcache_backend_test_center_of_mass expected_square_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( expected_square, glyph_read.w, glyph_read.h ),
		glyph_read.x,
		glyph_read.y );
	const ve_fontcache_backend_test_center_of_mass observed_square_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( glyph_square_pixels, glyph_read.w, glyph_read.h ),
		glyph_read.x,
		glyph_read.y );
	const int square_bleed = ve_fontcache_backend_test_outside_rect_count(
		glyph_square_pixels,
		glyph_read.w,
		glyph_read.h,
		{ glyph_square.x - glyph_read.x, glyph_square.y - glyph_read.y, glyph_square.w, glyph_square.h } );
	const bool square_pass = square_ok
		&& observed_square_bbox.valid
		&& std::abs( observed_square_bbox.x - expected_square_bbox.x ) <= 1
		&& std::abs( observed_square_bbox.y - expected_square_bbox.y ) <= 1
		&& std::abs( observed_square_bbox.w - expected_square_bbox.w ) <= 1
		&& std::abs( observed_square_bbox.h - expected_square_bbox.h ) <= 1
		&& observed_square_bbox.area >= ( glyph_square.w * glyph_square.h ) - 96
		&& observed_square_bbox.area <= ( glyph_square.w * glyph_square.h ) + 32
		&& std::fabs( observed_square_center.x - expected_square_center.x ) <= 0.8
		&& std::fabs( observed_square_center.y - expected_square_center.y ) <= 0.8
		&& square_bleed == 0;
	std::string square_cause = "geometry transform mismatch in GLYPH pass";
	if ( ve_fontcache_backend_test_is_uniform_scale_error( expected_square_bbox, observed_square_bbox ) ) {
		square_cause = "uniform scale error in GLYPH rasterisation path";
	} else if ( ve_fontcache_backend_test_is_aspect_ratio_deformation( expected_square_bbox, observed_square_bbox ) ) {
		square_cause = "aspect-ratio deformation in GLYPH rasterisation path";
	}
	ve_fontcache_backend_test_expect(
		result,
		square_pass,
		ve_fontcache_backend_test_make_failure(
			"glyph_geometry.square_bbox",
			expected_square_bbox,
			observed_square_bbox,
			&expected_square_center,
			&observed_square_center,
			"expected area in [4000,4128], observed area=" + std::to_string( observed_square_bbox.area )
				+ ", bleed=" + std::to_string( square_bleed ),
			square_cause ) );

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_append_diagonal_seam_square( options.cache, glyph_square.x, glyph_square.y, glyph_square.w );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > seam_pixels;
	bool seam_ok = ve_fontcache_backend_test_readback_texture(
		options,
		"glyph_buffer",
		glyph_read.x,
		glyph_read.y,
		glyph_read.w,
		glyph_read.h,
		seam_pixels );
	const ve_fontcache_backend_test_bbox seam_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( seam_pixels, glyph_read.w, glyph_read.h ),
		glyph_read.x,
		glyph_read.y );
	const ve_fontcache_backend_test_center_of_mass seam_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( seam_pixels, glyph_read.w, glyph_read.h ),
		glyph_read.x,
		glyph_read.y );
	const ve_fontcache_backend_test_diff_stats seam_diff = ve_fontcache_backend_test_expected_vs_actual_diff(
		expected_square,
		seam_pixels,
		glyph_read.w,
		glyph_read.h );
	const int seam_components = ve_fontcache_backend_test_connected_component_count( seam_pixels, glyph_read.w, glyph_read.h );
	const bool seam_present = ve_fontcache_backend_test_has_diagonal_seam(
		seam_pixels,
		glyph_read.w,
		glyph_read.h,
		ve_fontcache_backend_test_thresholded_bbox( seam_pixels, glyph_read.w, glyph_read.h ) );
	const bool seam_pass = seam_ok
		&& seam_bbox.valid
		&& seam_components == 1
		&& !seam_present
		&& seam_diff.max_abs_error <= 8;
	ve_fontcache_backend_test_expect(
		result,
		seam_pass,
		ve_fontcache_backend_test_make_failure(
			"glyph_geometry.diagonal_seam",
			expected_square_bbox,
			seam_bbox,
			&expected_square_center,
			&seam_center,
			"diff=" + ve_fontcache_backend_test_format_diff( seam_diff )
				+ ", components=" + std::to_string( seam_components ),
			seam_present ? "seam/crack between GLYPH triangles" : "triangle coverage mismatch in GLYPH rasterisation path" ) );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect l_shape_rect = { 104, 72, 72, 64 };
	const int l_shape_thickness = 20;
	const ve_fontcache_backend_test_rect l_shape_read = { 96, 64, 88, 80 };
	ve_fontcache_backend_test_append_l_shape(
		options.cache,
		l_shape_rect.x,
		l_shape_rect.y,
		l_shape_rect.w,
		l_shape_rect.h,
		l_shape_thickness );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > l_shape_pixels;
	bool l_shape_ok = ve_fontcache_backend_test_readback_texture(
		options,
		"glyph_buffer",
		l_shape_read.x,
		l_shape_read.y,
		l_shape_read.w,
		l_shape_read.h,
		l_shape_pixels );
	std::vector< uint8_t > expected_l_shape = ve_fontcache_backend_test_make_image( l_shape_read.w, l_shape_read.h, 0 );
	ve_fontcache_backend_test_fill_l_shape(
		expected_l_shape,
		l_shape_read.w,
		l_shape_read.h,
		l_shape_rect.x - l_shape_read.x,
		l_shape_rect.y - l_shape_read.y,
		l_shape_rect.w,
		l_shape_rect.h,
		l_shape_thickness,
		255 );
	const ve_fontcache_backend_test_bbox expected_l_shape_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( expected_l_shape, l_shape_read.w, l_shape_read.h ),
		l_shape_read.x,
		l_shape_read.y );
	const ve_fontcache_backend_test_bbox observed_l_shape_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( l_shape_pixels, l_shape_read.w, l_shape_read.h ),
		l_shape_read.x,
		l_shape_read.y );
	const ve_fontcache_backend_test_center_of_mass expected_l_shape_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( expected_l_shape, l_shape_read.w, l_shape_read.h ),
		l_shape_read.x,
		l_shape_read.y );
	const ve_fontcache_backend_test_center_of_mass observed_l_shape_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( l_shape_pixels, l_shape_read.w, l_shape_read.h ),
		l_shape_read.x,
		l_shape_read.y );
	const ve_fontcache_backend_test_diff_stats l_shape_diff = ve_fontcache_backend_test_expected_vs_actual_diff(
		expected_l_shape,
		l_shape_pixels,
		l_shape_read.w,
		l_shape_read.h );
	const ve_fontcache_backend_test_mirror_scores l_shape_scores = ve_fontcache_backend_test_mirror_scores_for_expected(
		expected_l_shape,
		l_shape_pixels,
		l_shape_read.w,
		l_shape_read.h );
	const bool l_shape_pass = l_shape_ok && l_shape_diff.mean_abs_error <= 6.0;
	std::string l_shape_cause = "asymmetric GLYPH geometry mismatch";
	if ( ve_fontcache_backend_test_is_vertical_flip( l_shape_scores ) ) {
		l_shape_cause = "V coordinate inversion in GLYPH geometry path";
	} else if ( ve_fontcache_backend_test_is_horizontal_flip( l_shape_scores ) ) {
		l_shape_cause = "U coordinate inversion in GLYPH geometry path";
	}
	ve_fontcache_backend_test_expect(
		result,
		l_shape_pass,
		ve_fontcache_backend_test_make_failure(
			"glyph_geometry.l_shape_orientation",
			expected_l_shape_bbox,
			observed_l_shape_bbox,
			&expected_l_shape_center,
			&observed_l_shape_center,
			"diff=" + ve_fontcache_backend_test_format_diff( l_shape_diff )
				+ ", mirror_scores={direct=" + std::to_string( l_shape_scores.direct )
				+ ", h=" + std::to_string( l_shape_scores.horizontal )
				+ ", v=" + std::to_string( l_shape_scores.vertical ) + "}",
			l_shape_cause ) );

	const struct glyph_clip_case
	{
		const char* case_id;
		ve_fontcache_backend_test_rect draw_rect;
		ve_fontcache_backend_test_bbox expected_bbox;
	} clip_cases[] = {
		{ "glyph_geometry.clip_left", { -20, 40, 48, 48 }, { true, 0, 40, 28, 48, 28 * 48 } },
		{ "glyph_geometry.clip_right", { VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH - 28, 40, 48, 48 }, { true, VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH - 28, 40, 28, 48, 28 * 48 } },
		{ "glyph_geometry.clip_bottom", { 64, -16, 48, 48 }, { true, 64, 0, 48, 32, 48 * 32 } },
		{ "glyph_geometry.clip_top", { 64, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT - 20, 48, 48 }, { true, 64, VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT - 20, 48, 20, 48 * 20 } },
	};
	for ( const glyph_clip_case& clip_case : clip_cases ) {
		ve_fontcache_backend_test_reset_state( options );
		ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, clip_case.draw_rect );
		ve_fontcache_backend_test_execute_pipeline( options );
		std::vector< uint8_t > clipped_pixels;
		bool clip_ok = ve_fontcache_backend_test_readback_surface_full( options, "glyph_buffer", clipped_pixels );
		const ve_fontcache_backend_test_bbox observed_bbox = ve_fontcache_backend_test_thresholded_bbox(
			clipped_pixels,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT );
		const int outside_count = ve_fontcache_backend_test_outside_rect_count(
			clipped_pixels,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH,
			VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT,
			{ clip_case.expected_bbox.x, clip_case.expected_bbox.y, clip_case.expected_bbox.w, clip_case.expected_bbox.h } );
		const bool clip_pass = clip_ok
			&& observed_bbox.valid
			&& std::abs( observed_bbox.x - clip_case.expected_bbox.x ) <= 1
			&& std::abs( observed_bbox.y - clip_case.expected_bbox.y ) <= 1
			&& std::abs( observed_bbox.w - clip_case.expected_bbox.w ) <= 1
			&& std::abs( observed_bbox.h - clip_case.expected_bbox.h ) <= 1
			&& outside_count == 0;
		ve_fontcache_backend_test_expect(
			result,
			clip_pass,
			ve_fontcache_backend_test_make_failure(
				clip_case.case_id,
				clip_case.expected_bbox,
				observed_bbox,
				nullptr,
				nullptr,
				"outside_pixels=" + std::to_string( outside_count ),
				outside_count > 0 ? "clipping wrapped or stretched geometry in GLYPH pass" : "clip rect mismatch in GLYPH pass" ) );
	}
}

inline void ve_fontcache_backend_test_run_glyph_blend_xor(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"glyph_blend_xor",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect read_rect = { 72, 72, 112, 80 };
	const ve_fontcache_backend_test_rect rect_a = { 80, 80, 64, 64 };
	const ve_fontcache_backend_test_rect rect_b = { 112, 80, 64, 64 };

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_append_square( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, rect_a.x, rect_a.y, rect_a.w );
	ve_fontcache_backend_test_append_square( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, rect_a.x, rect_a.y, rect_a.w );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > cancel_pixels;
	bool cancel_ok = ve_fontcache_backend_test_readback_texture(
		options,
		"glyph_buffer",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		cancel_pixels );
	const ve_fontcache_backend_test_bbox cancel_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( cancel_pixels, read_rect.w, read_rect.h ),
		read_rect.x,
		read_rect.y );
	ve_fontcache_backend_test_expect(
		result,
		cancel_ok && !ve_fontcache_backend_test_any_non_zero( cancel_pixels ),
		ve_fontcache_backend_test_make_failure(
			"glyph_blend_xor.double_cancel",
			{},
			cancel_bbox,
			nullptr,
			nullptr,
			"diff=" + ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 ),
				cancel_pixels,
				read_rect.w,
				read_rect.h ) ),
			"wrong blend mode on glyph pass" ) );

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_append_overlapping_quads(
		options.cache,
		VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH,
		rect_a,
		rect_b );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > ring_pixels;
	bool ring_ok = ve_fontcache_backend_test_readback_texture(
		options,
		"glyph_buffer",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		ring_pixels );
	std::vector< uint8_t > expected_ring = ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 );
	for ( int y = 0; y < read_rect.h; y++ ) {
		for ( int x = 0; x < read_rect.w; x++ ) {
			const bool in_a = x >= rect_a.x - read_rect.x && x < rect_a.x - read_rect.x + rect_a.w
				&& y >= rect_a.y - read_rect.y && y < rect_a.y - read_rect.y + rect_a.h;
			const bool in_b = x >= rect_b.x - read_rect.x && x < rect_b.x - read_rect.x + rect_b.w
				&& y >= rect_b.y - read_rect.y && y < rect_b.y - read_rect.y + rect_b.h;
			expected_ring[ static_cast< size_t >( y ) * read_rect.w + x ] = ( in_a ^ in_b ) ? 255 : 0;
		}
	}
	const ve_fontcache_backend_test_diff_stats ring_diff = ve_fontcache_backend_test_expected_vs_actual_diff(
		expected_ring,
		ring_pixels,
		read_rect.w,
		read_rect.h );
	const bool ring_pass = ring_ok && ring_diff.mean_abs_error <= 6.0;
	ve_fontcache_backend_test_expect(
		result,
		ring_pass,
		ve_fontcache_backend_test_make_failure(
			"glyph_blend_xor.overlap_ring",
			ve_fontcache_backend_test_translate_bbox(
				ve_fontcache_backend_test_thresholded_bbox( expected_ring, read_rect.w, read_rect.h ),
				read_rect.x,
				read_rect.y ),
			ve_fontcache_backend_test_translate_bbox(
				ve_fontcache_backend_test_thresholded_bbox( ring_pixels, read_rect.w, read_rect.h ),
				read_rect.x,
				read_rect.y ),
			nullptr,
			nullptr,
			"diff=" + ve_fontcache_backend_test_format_diff( ring_diff ),
			"wrong blend mode on glyph pass" ) );

	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_append_overlapping_quads( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, rect_a, rect_b );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > order_ab;
	bool order_ok = ve_fontcache_backend_test_readback_texture(
		options,
		"glyph_buffer",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		order_ab );
	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_backend_test_append_overlapping_quads( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH, rect_b, rect_a );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > order_ba;
	order_ok = order_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"glyph_buffer",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		order_ba );
	ve_fontcache_backend_test_expect(
		result,
		order_ok && order_ab == order_ba,
		"glyph_blend_xor.order_invariant: expected identical output for AB and BA overlap order, observed diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				order_ab,
				order_ba,
				read_rect.w,
				read_rect.h ) )
			+ "; probable cause: order-sensitive state leakage in glyph XOR path" );
}

inline void ve_fontcache_backend_test_run_atlas_blit_geometry(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"atlas_blit_geometry",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET | VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect source_rect = { 64, 96, 192, 192 };
	const ve_fontcache_backend_test_rect dest_rect = { 400, 256, 48, 48 };
	const ve_fontcache_backend_test_rect read_rect = { 392, 248, 64, 64 };

	std::vector< uint8_t > source_pattern = ve_fontcache_backend_test_make_image( source_rect.w, source_rect.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		source_pattern,
		source_rect.w,
		source_rect.h,
		0,
		0,
		source_rect.w,
		source_rect.h,
		250,
		170,
		90,
		30 );
	std::vector< uint8_t > reference = ve_fontcache_backend_test_box_downsample(
		source_pattern,
		source_rect.w,
		source_rect.h,
		dest_rect.w,
		dest_rect.h );
	std::vector< uint8_t > expected_local = ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 );
	ve_fontcache_backend_test_fill_rect(
		expected_local,
		read_rect.w,
		read_rect.h,
		dest_rect.x - read_rect.x,
		dest_rect.y - read_rect.y,
		dest_rect.w,
		dest_rect.h,
		0 );
	for ( int y = 0; y < dest_rect.h; y++ ) {
		for ( int x = 0; x < dest_rect.w; x++ ) {
			expected_local[ static_cast< size_t >( y + dest_rect.y - read_rect.y ) * read_rect.w + ( x + dest_rect.x - read_rect.x ) ] =
				reference[ static_cast< size_t >( y ) * dest_rect.w + x ];
		}
	}

	ve_fontcache_backend_test_reset_state( options );
	bool orientation_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"glyph_buffer",
		source_rect.x,
		source_rect.y,
		source_rect.w,
		source_rect.h,
		source_pattern );
	ve_fontcache_backend_test_append_atlas_clear( options.cache, dest_rect );
	ve_fontcache_backend_test_append_atlas_blit( options.cache, dest_rect, source_rect );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > atlas_pixels;
	orientation_ok = orientation_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"atlas",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		atlas_pixels );
	const ve_fontcache_backend_test_diff_stats orientation_diff = ve_fontcache_backend_test_expected_vs_actual_diff(
		expected_local,
		atlas_pixels,
		read_rect.w,
		read_rect.h );
	const ve_fontcache_backend_test_mirror_scores orientation_scores = ve_fontcache_backend_test_mirror_scores_for_expected(
		expected_local,
		atlas_pixels,
		read_rect.w,
		read_rect.h );
	const ve_fontcache_backend_test_bbox expected_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( expected_local, read_rect.w, read_rect.h, 8 ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_bbox observed_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( atlas_pixels, read_rect.w, read_rect.h, 8 ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_center_of_mass expected_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( expected_local, read_rect.w, read_rect.h ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_center_of_mass observed_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( atlas_pixels, read_rect.w, read_rect.h ),
		read_rect.x,
		read_rect.y );
	const int bleed = ve_fontcache_backend_test_outside_rect_count(
		atlas_pixels,
		read_rect.w,
		read_rect.h,
		{ dest_rect.x - read_rect.x, dest_rect.y - read_rect.y, dest_rect.w, dest_rect.h },
		8 );
	std::string orientation_cause = "atlas blit geometry mismatch";
	if ( ve_fontcache_backend_test_is_vertical_flip( orientation_scores ) ) {
		orientation_cause = "V coordinate inversion in ATLAS blit path";
	} else if ( ve_fontcache_backend_test_is_horizontal_flip( orientation_scores ) ) {
		orientation_cause = "U coordinate inversion in ATLAS blit path";
	} else if ( bleed > 0 ) {
		orientation_cause = "atlas write outside expected rect";
	}
	ve_fontcache_backend_test_expect(
		result,
		orientation_ok && orientation_diff.mean_abs_error <= 6.0 && bleed == 0,
		ve_fontcache_backend_test_make_failure(
			"atlas_blit_geometry.orientation",
			expected_bbox,
			observed_bbox,
			&expected_center,
			&observed_center,
			"diff=" + ve_fontcache_backend_test_format_diff( orientation_diff )
				+ ", bleed=" + std::to_string( bleed ),
			orientation_cause ) );

	std::vector< uint8_t > atlas_first = atlas_pixels;
	ve_fontcache_backend_test_append_atlas_clear( options.cache, dest_rect );
	ve_fontcache_backend_test_append_atlas_blit( options.cache, dest_rect, source_rect );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > atlas_second;
	bool repeat_ok = ve_fontcache_backend_test_readback_texture(
		options,
		"atlas",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		atlas_second );
	ve_fontcache_backend_test_expect(
		result,
		repeat_ok && atlas_first == atlas_second,
		"atlas_blit_geometry.repeat_stable: expected identical atlas pixels on repeated blit, observed diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				atlas_first,
				atlas_second,
				read_rect.w,
				read_rect.h ) )
			+ "; probable cause: unstable atlas write or state leakage between blits" );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect clear_scope_read = { 520, 320, 80, 80 };
	std::vector< uint8_t > clear_scope_pattern = ve_fontcache_backend_test_make_image( clear_scope_read.w, clear_scope_read.h, 77 );
	bool clear_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		clear_scope_read.x,
		clear_scope_read.y,
		clear_scope_read.w,
		clear_scope_read.h,
		clear_scope_pattern );
	const ve_fontcache_backend_test_rect clear_scope_dest = { 544, 344, 32, 32 };
	ve_fontcache_backend_test_append_atlas_clear( options.cache, clear_scope_dest );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > clear_scope_pixels;
	clear_ok = clear_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"atlas",
		clear_scope_read.x,
		clear_scope_read.y,
		clear_scope_read.w,
		clear_scope_read.h,
		clear_scope_pixels );
	std::vector< uint8_t > expected_clear_scope = clear_scope_pattern;
	ve_fontcache_backend_test_fill_rect(
		expected_clear_scope,
		clear_scope_read.w,
		clear_scope_read.h,
		clear_scope_dest.x - clear_scope_read.x,
		clear_scope_dest.y - clear_scope_read.y,
		clear_scope_dest.w,
		clear_scope_dest.h,
		0 );
	ve_fontcache_backend_test_expect(
		result,
		clear_ok && expected_clear_scope == clear_scope_pixels,
		"atlas_blit_geometry.clear_scope: expected atlas clear to zero only the destination rect, observed diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				expected_clear_scope,
				clear_scope_pixels,
				clear_scope_read.w,
				clear_scope_read.h ) )
			+ "; probable cause: clear pass not applied or applied to wrong surface" );

	ve_fontcache_backend_test_expect(
		result,
		orientation_ok && orientation_diff.max_abs_error <= 16,
		"atlas_blit_geometry.reference_tolerance: expected downsampled atlas output to stay within byte tolerance, observed diff="
			+ ve_fontcache_backend_test_format_diff( orientation_diff )
			+ "; probable cause: downsample kernel mismatch in ATLAS blit path" );
}

inline void ve_fontcache_backend_test_run_target_sampling_atlas(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"target_sampling_atlas",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET | VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect source_rect = { 800, 768, 64, 64 };
	const ve_fontcache_backend_test_rect dest_rect = { 320, 240, 64, 64 };
	const ve_fontcache_backend_test_rect read_rect = { 312, 232, 80, 80 };
	std::vector< uint8_t > source_pattern = ve_fontcache_backend_test_make_image( source_rect.w, source_rect.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		source_pattern,
		source_rect.w,
		source_rect.h,
		0,
		0,
		source_rect.w,
		source_rect.h,
		250,
		160,
		80,
		20 );
	std::vector< uint8_t > expected_local = ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 );
	for ( int y = 0; y < dest_rect.h; y++ ) {
		for ( int x = 0; x < dest_rect.w; x++ ) {
			expected_local[ static_cast< size_t >( y + dest_rect.y - read_rect.y ) * read_rect.w + ( x + dest_rect.x - read_rect.x ) ] =
				source_pattern[ static_cast< size_t >( y ) * source_rect.w + x ];
		}
	}

	ve_fontcache_backend_test_reset_state( options );
	bool atlas_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		source_rect.x,
		source_rect.y,
		source_rect.w,
		source_rect.h,
		source_pattern );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, dest_rect, source_rect );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > target_pixels;
	atlas_ok = atlas_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		target_pixels );
	const ve_fontcache_backend_test_bbox expected_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( expected_local, read_rect.w, read_rect.h, 8 ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_bbox observed_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( target_pixels, read_rect.w, read_rect.h, 8 ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_rect observed_local_rect = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( ve_fontcache_backend_test_thresholded_bbox( target_pixels, read_rect.w, read_rect.h, 8 ) ),
		2 );
	const std::array< double, 4 > observed_quadrants = ve_fontcache_backend_test_rect_valid( observed_local_rect )
		? ve_fontcache_backend_test_quadrant_means( target_pixels, read_rect.w, read_rect.h, observed_local_rect )
		: std::array< double, 4 > { 0.0, 0.0, 0.0, 0.0 };
	const bool direct_order = observed_quadrants[ 0 ] > observed_quadrants[ 1 ]
		&& observed_quadrants[ 1 ] > observed_quadrants[ 2 ]
		&& observed_quadrants[ 2 ] > observed_quadrants[ 3 ];
	const bool atlas_pass = atlas_ok
		&& observed_bbox.valid
		&& observed_bbox.w >= 46
		&& observed_bbox.h >= 28
		&& direct_order
		&& ( observed_quadrants[ 0 ] - observed_quadrants[ 3 ] ) >= 40.0;
	const ve_fontcache_backend_test_center_of_mass expected_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( expected_local, read_rect.w, read_rect.h ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_center_of_mass observed_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( target_pixels, read_rect.w, read_rect.h ),
		read_rect.x,
		read_rect.y );
	ve_fontcache_backend_test_expect(
		result,
		atlas_pass,
		ve_fontcache_backend_test_make_failure(
			"target_sampling_atlas.quadrants",
			expected_bbox,
			observed_bbox,
			&expected_center,
			&observed_center,
			"quadrants={tl=" + std::to_string( static_cast< int >( observed_quadrants[ 0 ] ) )
				+ ", tr=" + std::to_string( static_cast< int >( observed_quadrants[ 1 ] ) )
				+ ", bl=" + std::to_string( static_cast< int >( observed_quadrants[ 2 ] ) )
				+ ", br=" + std::to_string( static_cast< int >( observed_quadrants[ 3 ] ) ) + "}",
			direct_order
				? "unexpected target quad footprint in atlas sampling path"
				: "quadrant ordering mismatch in TARGET atlas sampling path" ) );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect crop_source = { 900, 900, 96, 96 };
	std::vector< uint8_t > crop_pattern = ve_fontcache_backend_test_make_image( crop_source.w, crop_source.h, 30 );
	ve_fontcache_backend_test_fill_rect( crop_pattern, crop_source.w, crop_source.h, 24, 24, 48, 48, 220 );
	bool crop_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		crop_source.x,
		crop_source.y,
		crop_source.w,
		crop_source.h,
		crop_pattern );
	const ve_fontcache_backend_test_rect crop_uv = { crop_source.x + 24, crop_source.y + 24, 48, 48 };
	const ve_fontcache_backend_test_rect crop_dest = { 440, 240, 48, 48 };
	const ve_fontcache_backend_test_rect crop_read = { 432, 232, 64, 64 };
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, crop_dest, crop_uv );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > crop_pixels;
	crop_ok = crop_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		crop_read.x,
		crop_read.y,
		crop_read.w,
		crop_read.h,
		crop_pixels );
	const ve_fontcache_backend_test_bbox crop_bbox =
		ve_fontcache_backend_test_translate_bbox( ve_fontcache_backend_test_thresholded_bbox( crop_pixels, crop_read.w, crop_read.h, 8 ), crop_read.x, crop_read.y );
	const ve_fontcache_backend_test_rect crop_local_rect = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( ve_fontcache_backend_test_thresholded_bbox( crop_pixels, crop_read.w, crop_read.h, 8 ) ),
		2 );
	const double crop_inside = ve_fontcache_backend_test_rect_valid( crop_local_rect )
		? ve_fontcache_backend_test_mean_in_rect( crop_pixels, crop_read.w, crop_read.h, crop_local_rect )
		: 0.0;
	const int crop_leak = ve_fontcache_backend_test_outside_rect_count(
		crop_pixels,
		crop_read.w,
		crop_read.h,
		ve_fontcache_backend_test_rect_from_bbox( ve_fontcache_backend_test_thresholded_bbox( crop_pixels, crop_read.w, crop_read.h, 8 ) ),
		40 );
	ve_fontcache_backend_test_expect(
		result,
		crop_ok && crop_bbox.valid && crop_inside >= 120.0 && crop_leak == 0,
		"target_sampling_atlas.cropped_window: expected cropped atlas UVs to preserve a bright inner window without surrounding bleed, observed bbox="
			+ ve_fontcache_backend_test_format_bbox( crop_bbox )
			+ ", inside_mean=" + std::to_string( static_cast< int >( crop_inside ) )
			+ ", outside_pixels=" + std::to_string( crop_leak )
			+ "; probable cause: source-rect mapping ignored and full-texture sampling used instead" );
}

inline void ve_fontcache_backend_test_run_target_sampling_flip_detector(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"target_sampling_flip_detector",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET | VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect source_rect = { 800, 768, 64, 64 };
	const ve_fontcache_backend_test_rect dest_rect = { 320, 240, 64, 64 };
	const ve_fontcache_backend_test_rect read_rect = { 312, 232, 80, 80 };
	std::vector< uint8_t > source_pattern = ve_fontcache_backend_test_make_image( source_rect.w, source_rect.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		source_pattern,
		source_rect.w,
		source_rect.h,
		0,
		0,
		source_rect.w,
		source_rect.h,
		250,
		160,
		80,
		20 );
	std::vector< uint8_t > expected_local = ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 );
	for ( int y = 0; y < dest_rect.h; y++ ) {
		for ( int x = 0; x < dest_rect.w; x++ ) {
			expected_local[ static_cast< size_t >( y + dest_rect.y - read_rect.y ) * read_rect.w + ( x + dest_rect.x - read_rect.x ) ] =
				source_pattern[ static_cast< size_t >( y ) * source_rect.w + x ];
		}
	}

	ve_fontcache_backend_test_reset_state( options );
	bool draw_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		source_rect.x,
		source_rect.y,
		source_rect.w,
		source_rect.h,
		source_pattern );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, dest_rect, source_rect );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > target_pixels;
	draw_ok = draw_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		target_pixels );

	const ve_fontcache_backend_test_bbox expected_local_bbox = ve_fontcache_backend_test_thresholded_bbox( expected_local, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_bbox observed_local_bbox = ve_fontcache_backend_test_thresholded_bbox( target_pixels, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_rect expected_local_rect = ve_fontcache_backend_test_inset_rect(
		expected_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( expected_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const ve_fontcache_backend_test_rect observed_local_rect = ve_fontcache_backend_test_inset_rect(
		observed_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( observed_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const bool compare_valid = ve_fontcache_backend_test_rect_valid( expected_local_rect )
		&& ve_fontcache_backend_test_rect_valid( observed_local_rect )
		&& expected_local_rect.w >= 24
		&& expected_local_rect.h >= 24
		&& observed_local_rect.w >= 24
		&& observed_local_rect.h >= 24;
	const std::array< double, 4 > expected_quadrants = compare_valid
		? ve_fontcache_backend_test_quadrant_means( expected_local, read_rect.w, read_rect.h, expected_local_rect )
		: std::array< double, 4 > { 0.0, 0.0, 0.0, 0.0 };
	const std::array< double, 4 > observed_quadrants = compare_valid
		? ve_fontcache_backend_test_quadrant_means( target_pixels, read_rect.w, read_rect.h, observed_local_rect )
		: std::array< double, 4 > { 0.0, 0.0, 0.0, 0.0 };
	const auto make_normalized_summary = []( const std::array< double, 4 >& quadrants ) {
		std::vector< uint8_t > summary( 4, 0 );
		const double min_value = *std::min_element( quadrants.begin(), quadrants.end() );
		const double max_value = *std::max_element( quadrants.begin(), quadrants.end() );
		const double scale = max_value > min_value ? 255.0 / ( max_value - min_value ) : 0.0;
		const auto normalize = [&]( double value ) {
			if ( scale == 0.0 ) {
				return static_cast< uint8_t >( 0 );
			}
			const double scaled = std::clamp( ( value - min_value ) * scale, 0.0, 255.0 );
			return static_cast< uint8_t >( std::lround( scaled ) );
		};

		// Row-major in the test image's bottom-left coordinate convention: [bl, br, tl, tr].
		summary[ 0 ] = normalize( quadrants[ 2 ] );
		summary[ 1 ] = normalize( quadrants[ 3 ] );
		summary[ 2 ] = normalize( quadrants[ 0 ] );
		summary[ 3 ] = normalize( quadrants[ 1 ] );
		return summary;
	};
	constexpr int summary_width = 2;
	constexpr int summary_height = 2;
	const std::vector< uint8_t > expected_summary = compare_valid ? make_normalized_summary( expected_quadrants ) : std::vector< uint8_t > {};
	const std::vector< uint8_t > observed_summary = compare_valid ? make_normalized_summary( observed_quadrants ) : std::vector< uint8_t > {};
	const ve_fontcache_backend_test_diff_stats direct_diff = ve_fontcache_backend_test_expected_vs_actual_diff(
		expected_summary,
		observed_summary,
		summary_width,
		summary_height );
	const ve_fontcache_backend_test_mirror_scores mirror_scores = ve_fontcache_backend_test_mirror_scores_for_expected(
		expected_summary,
		observed_summary,
		summary_width,
		summary_height );
	const ve_fontcache_backend_test_bbox expected_bbox = ve_fontcache_backend_test_translate_bbox(
		expected_local_bbox,
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_bbox observed_bbox = ve_fontcache_backend_test_translate_bbox(
		observed_local_bbox,
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_center_of_mass expected_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( expected_local, read_rect.w, read_rect.h ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_center_of_mass observed_center = ve_fontcache_backend_test_translate_center(
		ve_fontcache_backend_test_center_of_mass_grayscale( target_pixels, read_rect.w, read_rect.h ),
		read_rect.x,
		read_rect.y );
	const bool sane_bbox = observed_bbox.valid
		&& observed_bbox.w >= 46
		&& observed_bbox.h >= 28;
	const bool sane_contrast = compare_valid
		&& ( observed_quadrants[ 0 ] - observed_quadrants[ 3 ] ) >= 40.0;
	const bool vertical_flip = compare_valid && ve_fontcache_backend_test_is_vertical_flip( mirror_scores );
	const bool horizontal_flip = compare_valid && ve_fontcache_backend_test_is_horizontal_flip( mirror_scores );
	std::string failure_cause = "TARGET atlas sampling mismatch";
	if ( vertical_flip ) {
		failure_cause = "V coordinate inversion in TARGET atlas sampling path";
	} else if ( horizontal_flip ) {
		failure_cause = "U coordinate inversion in TARGET atlas sampling path";
	} else if ( !compare_valid ) {
		failure_cause = "insufficient stable quadrant area for TARGET atlas sampling check";
	} else if ( !observed_bbox.valid ) {
		failure_cause = "missing TARGET atlas sample footprint";
	} else if ( !sane_bbox ) {
		failure_cause = "unexpected target quad footprint in atlas sampling path";
	} else if ( !sane_contrast ) {
		failure_cause = "insufficient asymmetric contrast in TARGET atlas sampling path";
	} else if ( direct_diff.mean_abs_error > 25.0 ) {
		failure_cause = "unexpected distortion in TARGET atlas sampling path";
	}

	ve_fontcache_backend_test_expect(
		result,
		draw_ok
			&& sane_bbox
			&& compare_valid
			&& sane_contrast
			&& direct_diff.mean_abs_error <= 25.0
			&& direct_diff.max_abs_error <= 96
			&& !vertical_flip
			&& !horizontal_flip,
		ve_fontcache_backend_test_make_failure(
			"target_sampling_flip_detector.orientation",
			expected_bbox,
			observed_bbox,
			&expected_center,
			&observed_center,
			"diff=" + ve_fontcache_backend_test_format_diff( direct_diff )
				+ ", expected_quadrants={tl=" + std::to_string( static_cast< int >( expected_quadrants[ 0 ] ) )
				+ ", tr=" + std::to_string( static_cast< int >( expected_quadrants[ 1 ] ) )
				+ ", bl=" + std::to_string( static_cast< int >( expected_quadrants[ 2 ] ) )
				+ ", br=" + std::to_string( static_cast< int >( expected_quadrants[ 3 ] ) ) + "}"
				+ ", observed_quadrants={tl=" + std::to_string( static_cast< int >( observed_quadrants[ 0 ] ) )
				+ ", tr=" + std::to_string( static_cast< int >( observed_quadrants[ 1 ] ) )
				+ ", bl=" + std::to_string( static_cast< int >( observed_quadrants[ 2 ] ) )
				+ ", br=" + std::to_string( static_cast< int >( observed_quadrants[ 3 ] ) ) + "}"
				+ ", summary={expected=[" + std::to_string( expected_summary.empty() ? 0 : expected_summary[ 0 ] )
				+ "," + std::to_string( expected_summary.size() > 1 ? expected_summary[ 1 ] : 0 )
				+ "," + std::to_string( expected_summary.size() > 2 ? expected_summary[ 2 ] : 0 )
				+ "," + std::to_string( expected_summary.size() > 3 ? expected_summary[ 3 ] : 0 )
				+ "], observed=[" + std::to_string( observed_summary.empty() ? 0 : observed_summary[ 0 ] )
				+ "," + std::to_string( observed_summary.size() > 1 ? observed_summary[ 1 ] : 0 )
				+ "," + std::to_string( observed_summary.size() > 2 ? observed_summary[ 2 ] : 0 )
				+ "," + std::to_string( observed_summary.size() > 3 ? observed_summary[ 3 ] : 0 ) + "]}"
				+ ", mirror_scores={direct=" + std::to_string( mirror_scores.direct )
				+ ", horizontal=" + std::to_string( mirror_scores.horizontal )
				+ ", vertical=" + std::to_string( mirror_scores.vertical )
				+ ", both=" + std::to_string( mirror_scores.both ) + "}",
			failure_cause ) );
}

inline void ve_fontcache_backend_test_run_target_sampling_uncached(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"target_sampling_uncached",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET | VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect source_rect = { 64, 96, 192, 192 };
	const ve_fontcache_backend_test_rect dest_rect = { 320, 336, 48, 48 };
	const ve_fontcache_backend_test_rect read_rect = { 312, 328, 64, 64 };
	std::vector< uint8_t > source_pattern = ve_fontcache_backend_test_make_image( source_rect.w, source_rect.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		source_pattern,
		source_rect.w,
		source_rect.h,
		0,
		0,
		source_rect.w,
		source_rect.h,
		250,
		170,
		90,
		30 );
	const std::vector< uint8_t > downsampled = ve_fontcache_backend_test_box_downsample(
		source_pattern,
		source_rect.w,
		source_rect.h,
		dest_rect.w,
		dest_rect.h );
	std::vector< uint8_t > expected_local = ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 );
	for ( int y = 0; y < dest_rect.h; y++ ) {
		for ( int x = 0; x < dest_rect.w; x++ ) {
			expected_local[ static_cast< size_t >( y + dest_rect.y - read_rect.y ) * read_rect.w + ( x + dest_rect.x - read_rect.x ) ] =
				downsampled[ static_cast< size_t >( y ) * dest_rect.w + x ];
		}
	}

	ve_fontcache_backend_test_reset_state( options );
	bool uncached_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"glyph_buffer",
		source_rect.x,
		source_rect.y,
		source_rect.w,
		source_rect.h,
		source_pattern );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED, dest_rect, source_rect );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > uncached_pixels;
	uncached_ok = uncached_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		uncached_pixels );
	const ve_fontcache_backend_test_bbox expected_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( expected_local, read_rect.w, read_rect.h, 8 ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_bbox observed_bbox = ve_fontcache_backend_test_translate_bbox(
		ve_fontcache_backend_test_thresholded_bbox( uncached_pixels, read_rect.w, read_rect.h, 8 ),
		read_rect.x,
		read_rect.y );
	const ve_fontcache_backend_test_rect observed_local_rect = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( ve_fontcache_backend_test_thresholded_bbox( uncached_pixels, read_rect.w, read_rect.h, 8 ) ),
		2 );
	const ve_fontcache_backend_test_rect expected_local_rect = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( ve_fontcache_backend_test_thresholded_bbox( expected_local, read_rect.w, read_rect.h, 8 ) ),
		2 );
	const std::array< double, 4 > observed_quadrants = ve_fontcache_backend_test_rect_valid( observed_local_rect )
		? ve_fontcache_backend_test_quadrant_means( uncached_pixels, read_rect.w, read_rect.h, observed_local_rect )
		: std::array< double, 4 > { 0.0, 0.0, 0.0, 0.0 };
	const std::vector< uint8_t > expected_summary = ve_fontcache_backend_test_rect_valid( expected_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( expected_local, read_rect.w, read_rect.h, expected_local_rect )
		: std::vector< uint8_t > {};
	const std::vector< uint8_t > observed_summary = ve_fontcache_backend_test_rect_valid( observed_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( uncached_pixels, read_rect.w, read_rect.h, observed_local_rect )
		: std::vector< uint8_t > {};
	const ve_fontcache_backend_test_diff_stats summary_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( expected_summary, observed_summary, 2, 2 );
	const ve_fontcache_backend_test_mirror_scores mirror_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected_summary, observed_summary, 2, 2 );
	const double top_mean = 0.5 * ( observed_quadrants[ 0 ] + observed_quadrants[ 1 ] );
	const double bottom_mean = 0.5 * ( observed_quadrants[ 2 ] + observed_quadrants[ 3 ] );
	const double left_mean = 0.5 * ( observed_quadrants[ 0 ] + observed_quadrants[ 2 ] );
	const double right_mean = 0.5 * ( observed_quadrants[ 1 ] + observed_quadrants[ 3 ] );
	ve_fontcache_backend_test_expect(
		result,
		uncached_ok
			&& observed_bbox.valid
			&& observed_bbox.w >= 44
			&& observed_bbox.h >= 24
			&& top_mean >= bottom_mean + 15.0
			&& left_mean >= right_mean + 15.0,
		ve_fontcache_backend_test_make_failure(
			"target_sampling_uncached.quadrants",
			expected_bbox,
			observed_bbox,
			nullptr,
			nullptr,
			"quadrants={tl=" + std::to_string( static_cast< int >( observed_quadrants[ 0 ] ) )
				+ ", tr=" + std::to_string( static_cast< int >( observed_quadrants[ 1 ] ) )
				+ ", bl=" + std::to_string( static_cast< int >( observed_quadrants[ 2 ] ) )
				+ ", br=" + std::to_string( static_cast< int >( observed_quadrants[ 3 ] ) ) + "}",
			"wrong source texture bound or quadrant ordering mismatch in TARGET_UNCACHED path" ) );

	ve_fontcache_backend_test_expect(
		result,
		uncached_ok
			&& observed_bbox.valid
			&& observed_bbox.w >= 44
			&& observed_bbox.h >= 24
			&& !ve_fontcache_backend_test_is_vertical_flip( mirror_scores )
			&& !ve_fontcache_backend_test_is_horizontal_flip( mirror_scores )
			&& summary_diff.mean_abs_error <= 18.0
			&& summary_diff.max_abs_error <= 96,
		ve_fontcache_backend_test_make_failure(
			"target_sampling_uncached.orientation",
			expected_bbox,
			observed_bbox,
			nullptr,
			nullptr,
			"diff=" + ve_fontcache_backend_test_format_diff( summary_diff )
				+ ", mirror_scores={direct=" + std::to_string( mirror_scores.direct )
				+ ", horizontal=" + std::to_string( mirror_scores.horizontal )
				+ ", vertical=" + std::to_string( mirror_scores.vertical )
				+ ", both=" + std::to_string( mirror_scores.both ) + "}",
			ve_fontcache_backend_test_is_vertical_flip( mirror_scores )
				? "V coordinate inversion in TARGET_UNCACHED path"
				: ( ve_fontcache_backend_test_is_horizontal_flip( mirror_scores )
					? "U coordinate inversion in TARGET_UNCACHED path"
					: "orientation mismatch in TARGET_UNCACHED path" ) ) );

	ve_fontcache_backend_test_expect(
		result,
		uncached_ok
			&& observed_bbox.valid
			&& observed_bbox.w >= 44
			&& observed_bbox.h >= 24,
		ve_fontcache_backend_test_make_failure(
			"target_sampling_uncached.downsample_geometry",
			expected_bbox,
			observed_bbox,
			nullptr,
			nullptr,
			"expected size=" + std::to_string( dest_rect.w ) + "x" + std::to_string( dest_rect.h ),
			"uniform scale error in TARGET_UNCACHED downsample branch" ) );
}

inline void ve_fontcache_backend_test_run_target_sampling_cpu_cached(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"target_sampling_cpu_cached",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_FREETYPE_MODE
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_CPU_ATLAS ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect source_rect = { 96, 96, 64, 64 };
	const ve_fontcache_backend_test_rect dest_rect = { 360, 300, 64, 64 };
	const ve_fontcache_backend_test_rect read_rect = { 352, 292, 80, 80 };
	std::vector< uint8_t > source_pattern = ve_fontcache_backend_test_make_image( source_rect.w, source_rect.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		source_pattern,
		source_rect.w,
		source_rect.h,
		0,
		0,
		source_rect.w,
		source_rect.h,
		250,
		160,
		80,
		20 );
	std::vector< uint8_t > expected_local = ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 );
	for ( int y = 0; y < dest_rect.h; y++ ) {
		for ( int x = 0; x < dest_rect.w; x++ ) {
			expected_local[ static_cast< size_t >( y + dest_rect.y - read_rect.y ) * read_rect.w + ( x + dest_rect.x - read_rect.x ) ] =
				source_pattern[ static_cast< size_t >( y ) * source_rect.w + x ];
		}
	}

	ve_fontcache_backend_test_reset_state( options );
	bool draw_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		ve_fontcache_backend_test_cpu_atlas_page_surface_name(),
		source_rect.x,
		source_rect.y,
		source_rect.w,
		source_rect.h,
		source_pattern );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED, dest_rect, source_rect );
	draw_ok = draw_ok && ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > target_pixels;
	draw_ok = draw_ok && ve_fontcache_backend_test_readback_texture(
		options,
		ve_fontcache_backend_test_target_linear_surface_name(),
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		target_pixels );

	const ve_fontcache_backend_test_bbox expected_local_bbox = ve_fontcache_backend_test_thresholded_bbox( expected_local, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_bbox observed_local_bbox = ve_fontcache_backend_test_thresholded_bbox( target_pixels, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_rect expected_local_rect = ve_fontcache_backend_test_inset_rect(
		expected_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( expected_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const ve_fontcache_backend_test_rect observed_local_rect = ve_fontcache_backend_test_inset_rect(
		observed_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( observed_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const std::vector< uint8_t > expected_summary = ve_fontcache_backend_test_rect_valid( expected_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( expected_local, read_rect.w, read_rect.h, expected_local_rect )
		: std::vector< uint8_t > {};
	const std::vector< uint8_t > observed_summary = ve_fontcache_backend_test_rect_valid( observed_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( target_pixels, read_rect.w, read_rect.h, observed_local_rect )
		: std::vector< uint8_t > {};
	const ve_fontcache_backend_test_diff_stats summary_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( expected_summary, observed_summary, 2, 2 );
	const ve_fontcache_backend_test_mirror_scores mirror_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected_summary, observed_summary, 2, 2 );
	const ve_fontcache_backend_test_bbox expected_bbox = ve_fontcache_backend_test_translate_bbox( expected_local_bbox, read_rect.x, read_rect.y );
	const ve_fontcache_backend_test_bbox observed_bbox = ve_fontcache_backend_test_translate_bbox( observed_local_bbox, read_rect.x, read_rect.y );

	ve_fontcache_backend_test_expect(
		result,
		draw_ok
			&& observed_bbox.valid
			&& observed_bbox.w >= 46
			&& observed_bbox.h >= 28
			&& !ve_fontcache_backend_test_is_vertical_flip( mirror_scores )
			&& !ve_fontcache_backend_test_is_horizontal_flip( mirror_scores )
			&& summary_diff.mean_abs_error <= 28.0
			&& summary_diff.max_abs_error <= 128,
		ve_fontcache_backend_test_make_failure(
			"target_sampling_cpu_cached.orientation",
			expected_bbox,
			observed_bbox,
			nullptr,
			nullptr,
			"diff=" + ve_fontcache_backend_test_format_diff( summary_diff )
				+ ", mirror_scores={direct=" + std::to_string( mirror_scores.direct )
				+ ", horizontal=" + std::to_string( mirror_scores.horizontal )
				+ ", vertical=" + std::to_string( mirror_scores.vertical )
				+ ", both=" + std::to_string( mirror_scores.both ) + "}",
			ve_fontcache_backend_test_is_vertical_flip( mirror_scores )
				? "V coordinate inversion in TARGET_CPU_CACHED path"
				: ( ve_fontcache_backend_test_is_horizontal_flip( mirror_scores )
					? "U coordinate inversion in TARGET_CPU_CACHED path"
					: "orientation mismatch in TARGET_CPU_CACHED path" ) ) );
}

inline void ve_fontcache_backend_test_run_present_orientation(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"present_orientation",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_TARGET_LINEAR ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect target_rect = { 320, 240, 64, 64 };
	const ve_fontcache_backend_test_rect read_rect = { 312, 232, 80, 80 };
	const ve_fontcache_backend_test_rect atlas_source = { 800, 768, 64, 64 };
	std::vector< uint8_t > source_pattern = ve_fontcache_backend_test_make_image( target_rect.w, target_rect.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		source_pattern,
		target_rect.w,
		target_rect.h,
		0,
		0,
		target_rect.w,
		target_rect.h,
		250,
		160,
		80,
		20 );
	std::vector< uint8_t > expected_local = ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 );
	for ( int y = 0; y < target_rect.h; y++ ) {
		for ( int x = 0; x < target_rect.w; x++ ) {
			expected_local[ static_cast< size_t >( y + target_rect.y - read_rect.y ) * read_rect.w + ( x + target_rect.x - read_rect.x ) ] =
				source_pattern[ static_cast< size_t >( y ) * target_rect.w + x ];
		}
	}

	ve_fontcache_backend_test_reset_state( options );
	bool present_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		ve_fontcache_backend_test_target_linear_surface_name(),
		target_rect.x,
		target_rect.y,
		target_rect.w,
		target_rect.h,
		source_pattern );
	if ( !present_ok ) {
		present_ok = ve_fontcache_backend_test_write_surface_region(
			options,
			"atlas",
			atlas_source.x,
			atlas_source.y,
			atlas_source.w,
			atlas_source.h,
			source_pattern );
		ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, target_rect, atlas_source );
		present_ok = present_ok && ve_fontcache_backend_test_execute_pipeline( options );
	}
	present_ok = present_ok && ve_fontcache_backend_test_execute_present( options );
	std::vector< uint8_t > presented_pixels;
	present_ok = present_ok && ve_fontcache_backend_test_readback_texture(
		options,
		ve_fontcache_backend_test_presented_surface_name(),
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		presented_pixels );

	const ve_fontcache_backend_test_bbox expected_local_bbox = ve_fontcache_backend_test_thresholded_bbox( expected_local, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_bbox observed_local_bbox = ve_fontcache_backend_test_thresholded_bbox( presented_pixels, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_rect expected_local_rect = ve_fontcache_backend_test_inset_rect(
		expected_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( expected_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const ve_fontcache_backend_test_rect observed_local_rect = ve_fontcache_backend_test_inset_rect(
		observed_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( observed_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const std::vector< uint8_t > expected_summary = ve_fontcache_backend_test_rect_valid( expected_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( expected_local, read_rect.w, read_rect.h, expected_local_rect )
		: std::vector< uint8_t > {};
	const std::vector< uint8_t > observed_summary = ve_fontcache_backend_test_rect_valid( observed_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( presented_pixels, read_rect.w, read_rect.h, observed_local_rect )
		: std::vector< uint8_t > {};
	const ve_fontcache_backend_test_diff_stats summary_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( expected_summary, observed_summary, 2, 2 );
	const ve_fontcache_backend_test_mirror_scores mirror_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected_summary, observed_summary, 2, 2 );
	const ve_fontcache_backend_test_bbox expected_bbox = ve_fontcache_backend_test_translate_bbox( expected_local_bbox, read_rect.x, read_rect.y );
	const ve_fontcache_backend_test_bbox observed_bbox = ve_fontcache_backend_test_translate_bbox( observed_local_bbox, read_rect.x, read_rect.y );

	ve_fontcache_backend_test_expect(
		result,
		present_ok
			&& observed_bbox.valid
			&& observed_bbox.w >= 46
			&& observed_bbox.h >= 28
			&& !ve_fontcache_backend_test_is_vertical_flip( mirror_scores )
			&& !ve_fontcache_backend_test_is_horizontal_flip( mirror_scores )
			&& summary_diff.mean_abs_error <= 28.0
			&& summary_diff.max_abs_error <= 128,
		ve_fontcache_backend_test_make_failure(
			"present_orientation.target_linear_to_presented",
			expected_bbox,
			observed_bbox,
			nullptr,
			nullptr,
			"diff=" + ve_fontcache_backend_test_format_diff( summary_diff )
				+ ", mirror_scores={direct=" + std::to_string( mirror_scores.direct )
				+ ", horizontal=" + std::to_string( mirror_scores.horizontal )
				+ ", vertical=" + std::to_string( mirror_scores.vertical )
				+ ", both=" + std::to_string( mirror_scores.both ) + "}",
			ve_fontcache_backend_test_is_vertical_flip( mirror_scores )
				? "V coordinate inversion in PRESENT path"
				: ( ve_fontcache_backend_test_is_horizontal_flip( mirror_scores )
					? "U coordinate inversion in PRESENT path"
					: "orientation mismatch in PRESENT path" ) ) );
}

inline void ve_fontcache_backend_test_run_target_alpha_blend(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"target_alpha_blend",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET | VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect target_read = { 192, 192, 120, 80 };
	const ve_fontcache_backend_test_rect left_rect = { 200, 200, 64, 64 };
	const ve_fontcache_backend_test_rect right_rect = { 232, 200, 64, 64 };
	const double expected_left = 255.0 * 0.40;
	const double expected_right = 255.0 * 0.60;
	const double expected_overlap = 255.0 * ( 0.40 + 0.60 * ( 1.0 - 0.40 ) );

	auto capture_target_bbox =
		[&](
			uint32_t pass,
			const ve_fontcache_backend_test_rect& source_rect,
			const std::vector< uint8_t >& source_pixels ) -> ve_fontcache_backend_test_bbox {
		ve_fontcache_backend_test_reset_state( options );
		if ( !ve_fontcache_backend_test_write_surface_region(
			options,
			pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET ? "atlas" : "glyph_buffer",
			source_rect.x,
			source_rect.y,
			source_rect.w,
			source_rect.h,
			source_pixels ) ) {
			return {};
		}
		ve_fontcache_backend_test_append_quad( options.cache, pass, left_rect, source_rect, 0, { 1.0f, 1.0f, 1.0f, 1.0f } );
		ve_fontcache_backend_test_execute_pipeline( options );
		std::vector< uint8_t > pixels;
		if ( !ve_fontcache_backend_test_readback_texture( options, "target", target_read.x, target_read.y, target_read.w, target_read.h, pixels ) ) {
			return {};
		}
		return ve_fontcache_backend_test_thresholded_bbox( pixels, target_read.w, target_read.h, 8 );
	};

	auto calibrated_blend_check =
		[&](
			const char* case_id,
			uint32_t pass,
			const ve_fontcache_backend_test_rect& source_rect,
			const std::vector< uint8_t >& source_pixels,
			float tolerance ) {
			ve_fontcache_backend_test_bbox left_bbox = capture_target_bbox( pass, source_rect, source_pixels );
			ve_fontcache_backend_test_reset_state( options );
			ve_fontcache_backend_test_write_surface_region(
				options,
				pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET ? "atlas" : "glyph_buffer",
				source_rect.x,
				source_rect.y,
				source_rect.w,
				source_rect.h,
				source_pixels );
			ve_fontcache_backend_test_append_quad( options.cache, pass, right_rect, source_rect, 0, { 1.0f, 1.0f, 1.0f, 1.0f } );
			ve_fontcache_backend_test_execute_pipeline( options );
			std::vector< uint8_t > right_pixels;
			bool ok = ve_fontcache_backend_test_readback_texture( options, "target", target_read.x, target_read.y, target_read.w, target_read.h, right_pixels );
			ve_fontcache_backend_test_bbox right_bbox = ve_fontcache_backend_test_thresholded_bbox( right_pixels, target_read.w, target_read.h, 8 );

			ve_fontcache_backend_test_reset_state( options );
			ok = ok && ve_fontcache_backend_test_write_surface_region(
				options,
				pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET ? "atlas" : "glyph_buffer",
				source_rect.x,
				source_rect.y,
				source_rect.w,
				source_rect.h,
				source_pixels );
			ve_fontcache_backend_test_append_quad( options.cache, pass, left_rect, source_rect, 0, { 1.0f, 1.0f, 1.0f, 0.40f } );
			ve_fontcache_backend_test_append_quad( options.cache, pass, right_rect, source_rect, 0, { 1.0f, 1.0f, 1.0f, 0.60f } );
			ve_fontcache_backend_test_execute_pipeline( options );
			std::vector< uint8_t > blend_pixels;
			ok = ok && ve_fontcache_backend_test_readback_texture( options, "target", target_read.x, target_read.y, target_read.w, target_read.h, blend_pixels );

			const ve_fontcache_backend_test_rect overlap = ve_fontcache_backend_test_inset_rect(
				ve_fontcache_backend_test_intersect_rects( left_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( left_bbox ) : ve_fontcache_backend_test_rect {},
					right_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( right_bbox ) : ve_fontcache_backend_test_rect {} ),
				2 );
			const ve_fontcache_backend_test_rect left_only = ve_fontcache_backend_test_inset_rect(
				ve_fontcache_backend_test_intersect_rects(
					left_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( left_bbox ) : ve_fontcache_backend_test_rect {},
					{ left_bbox.x, left_bbox.y, std::max( 0, right_bbox.x - left_bbox.x ), left_bbox.h } ),
				2 );
			const ve_fontcache_backend_test_rect right_only = ve_fontcache_backend_test_inset_rect(
				ve_fontcache_backend_test_intersect_rects(
					right_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( right_bbox ) : ve_fontcache_backend_test_rect {},
					{ left_bbox.x + left_bbox.w, right_bbox.y, std::max( 0, right_bbox.x + right_bbox.w - ( left_bbox.x + left_bbox.w ) ), right_bbox.h } ),
				2 );

			const double observed_left = ve_fontcache_backend_test_rect_valid( left_only )
				? ve_fontcache_backend_test_mean_in_rect( blend_pixels, target_read.w, target_read.h, left_only )
				: 0.0;
			const double observed_overlap = ve_fontcache_backend_test_rect_valid( overlap )
				? ve_fontcache_backend_test_mean_in_rect( blend_pixels, target_read.w, target_read.h, overlap )
				: 0.0;
			const double observed_right = ve_fontcache_backend_test_rect_valid( right_only )
				? ve_fontcache_backend_test_mean_in_rect( blend_pixels, target_read.w, target_read.h, right_only )
				: 0.0;

			ve_fontcache_backend_test_expect(
				result,
				ok
					&& ve_fontcache_backend_test_rect_valid( left_only )
					&& ve_fontcache_backend_test_rect_valid( overlap )
					&& ve_fontcache_backend_test_rect_valid( right_only )
					&& observed_overlap >= observed_left + tolerance
					&& observed_overlap >= observed_right + 4.0
					&& observed_right >= observed_left + 8.0,
				std::string( case_id )
					+ ": expected left/overlap/right intensities "
					+ std::to_string( static_cast< int >( expected_left ) ) + "/"
					+ std::to_string( static_cast< int >( expected_overlap ) ) + "/"
					+ std::to_string( static_cast< int >( expected_right ) ) + ", observed "
					+ std::to_string( static_cast< int >( observed_left ) ) + "/"
					+ std::to_string( static_cast< int >( observed_overlap ) ) + "/"
					+ std::to_string( static_cast< int >( observed_right ) )
					+ "; probable cause: wrong blend mode on target pass" );
		};

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect atlas_source = { 700, 700, 64, 64 };
	std::vector< uint8_t > white_atlas = ve_fontcache_backend_test_make_image( atlas_source.w, atlas_source.h, 255 );
	ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		atlas_source.x,
		atlas_source.y,
		atlas_source.w,
		atlas_source.h,
		white_atlas );
	calibrated_blend_check(
		"target_alpha_blend.atlas_formula",
		VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET,
		atlas_source,
		white_atlas,
		24.0f );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect glyph_source = { 64, 96, 256, 256 };
	std::vector< uint8_t > white_glyph = ve_fontcache_backend_test_make_image( glyph_source.w, glyph_source.h, 255 );
	ve_fontcache_backend_test_write_surface_region(
		options,
		"glyph_buffer",
		glyph_source.x,
		glyph_source.y,
		glyph_source.w,
		glyph_source.h,
		white_glyph );
	calibrated_blend_check(
		"target_alpha_blend.uncached_formula",
		VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED,
		glyph_source,
		white_glyph,
		28.0f );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect colour_source = { 820, 700, 32, 32 };
	std::vector< uint8_t > colour_source_pixels = ve_fontcache_backend_test_make_image( colour_source.w, colour_source.h, 255 );
	bool colour_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		colour_source.x,
		colour_source.y,
		colour_source.w,
		colour_source.h,
		colour_source_pixels );
	const ve_fontcache_backend_test_rect colour_left = { 200, 304, 32, 32 };
	const ve_fontcache_backend_test_rect colour_right = { 248, 304, 32, 32 };
	const ve_fontcache_backend_test_rect colour_read = { 192, 296, 96, 48 };
	ve_fontcache_backend_test_reset_state( options );
	colour_ok = colour_ok && ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		colour_source.x,
		colour_source.y,
		colour_source.w,
		colour_source.h,
		colour_source_pixels );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, colour_left, colour_source, 0, { 0.25f, 1.0f, 1.0f, 1.0f } );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > colour_left_pixels;
	colour_ok = colour_ok && ve_fontcache_backend_test_readback_texture( options, "target", colour_read.x, colour_read.y, colour_read.w, colour_read.h, colour_left_pixels );
	const ve_fontcache_backend_test_rect colour_left_bbox = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( ve_fontcache_backend_test_thresholded_bbox( colour_left_pixels, colour_read.w, colour_read.h, 8 ) ),
		2 );

	ve_fontcache_backend_test_reset_state( options );
	colour_ok = colour_ok && ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		colour_source.x,
		colour_source.y,
		colour_source.w,
		colour_source.h,
		colour_source_pixels );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, colour_right, colour_source, 0, { 0.75f, 1.0f, 1.0f, 1.0f } );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > colour_right_pixels;
	colour_ok = colour_ok && ve_fontcache_backend_test_readback_texture( options, "target", colour_read.x, colour_read.y, colour_read.w, colour_read.h, colour_right_pixels );
	const ve_fontcache_backend_test_rect colour_right_bbox = ve_fontcache_backend_test_inset_rect(
		ve_fontcache_backend_test_rect_from_bbox( ve_fontcache_backend_test_thresholded_bbox( colour_right_pixels, colour_read.w, colour_read.h, 8 ) ),
		2 );
	const double observed_colour_left = ve_fontcache_backend_test_rect_valid( colour_left_bbox )
		? ve_fontcache_backend_test_mean_in_rect( colour_left_pixels, colour_read.w, colour_read.h, colour_left_bbox )
		: 0.0;
	const double observed_colour_right = ve_fontcache_backend_test_rect_valid( colour_right_bbox )
		? ve_fontcache_backend_test_mean_in_rect( colour_right_pixels, colour_read.w, colour_read.h, colour_right_bbox )
		: 0.0;
	ve_fontcache_backend_test_expect(
		result,
		colour_ok
			&& ve_fontcache_backend_test_rect_valid( colour_left_bbox )
			&& ve_fontcache_backend_test_rect_valid( colour_right_bbox )
			&& observed_colour_right > observed_colour_left * 1.4,
		"target_alpha_blend.colour_modulation: expected the higher-red quad to measure much brighter than the lower-red quad, observed "
			+ std::to_string( static_cast< int >( observed_colour_left ) ) + " and "
			+ std::to_string( static_cast< int >( observed_colour_right ) )
			+ "; probable cause: wrong target colour uniform upload or channel modulation" );
}

inline void ve_fontcache_backend_test_run_cross_pass_sequences(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"cross_pass_sequences",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU | VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET | VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE ) ) {
		return;
	}

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect glyph_source = { 64, 96, 192, 192 };
	const ve_fontcache_backend_test_rect atlas_dest = { 512, 384, 48, 48 };
	const ve_fontcache_backend_test_rect target_dest = { 520, 320, 48, 48 };
	const ve_fontcache_backend_test_rect glyph_read = { 64, 96, 192, 192 };
	const ve_fontcache_backend_test_rect atlas_read = { 504, 376, 64, 64 };
	const ve_fontcache_backend_test_rect target_read = { 512, 312, 64, 64 };
	ve_fontcache_backend_test_append_l_shape( options.cache, glyph_source.x, glyph_source.y, glyph_source.w, glyph_source.h, 64 );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > glyph_pixels;
	bool pipeline_ok = ve_fontcache_backend_test_readback_texture(
		options,
		"glyph_buffer",
		glyph_read.x,
		glyph_read.y,
		glyph_read.w,
		glyph_read.h,
		glyph_pixels );
	std::vector< uint8_t > expected_glyph = ve_fontcache_backend_test_make_image( glyph_read.w, glyph_read.h, 0 );
	ve_fontcache_backend_test_fill_l_shape( expected_glyph, glyph_read.w, glyph_read.h, 0, 0, glyph_read.w, glyph_read.h, 64, 255 );
	const ve_fontcache_backend_test_diff_stats glyph_stage_diff = ve_fontcache_backend_test_expected_vs_actual_diff(
		expected_glyph,
		glyph_pixels,
		glyph_read.w,
		glyph_read.h );

	ve_fontcache_backend_test_append_atlas_clear( options.cache, atlas_dest );
	ve_fontcache_backend_test_append_atlas_blit( options.cache, atlas_dest, glyph_source );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > atlas_pixels;
	pipeline_ok = pipeline_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"atlas",
		atlas_read.x,
		atlas_read.y,
		atlas_read.w,
		atlas_read.h,
		atlas_pixels );
	std::vector< uint8_t > expected_atlas = ve_fontcache_backend_test_make_image( atlas_read.w, atlas_read.h, 0 );
	const std::vector< uint8_t > atlas_reference = ve_fontcache_backend_test_box_downsample(
		expected_glyph,
		glyph_read.w,
		glyph_read.h,
		atlas_dest.w,
		atlas_dest.h );
	for ( int y = 0; y < atlas_dest.h; y++ ) {
		for ( int x = 0; x < atlas_dest.w; x++ ) {
			expected_atlas[ static_cast< size_t >( y + atlas_dest.y - atlas_read.y ) * atlas_read.w + ( x + atlas_dest.x - atlas_read.x ) ] =
				atlas_reference[ static_cast< size_t >( y ) * atlas_dest.w + x ];
		}
	}
	const ve_fontcache_backend_test_diff_stats atlas_stage_diff = ve_fontcache_backend_test_expected_vs_actual_diff(
		expected_atlas,
		atlas_pixels,
		atlas_read.w,
		atlas_read.h );

	ve_fontcache_backend_test_reset_state( options );
	bool baseline_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		atlas_read.x,
		atlas_read.y,
		atlas_read.w,
		atlas_read.h,
		atlas_pixels );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, target_dest, atlas_dest );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > baseline_target;
	baseline_ok = baseline_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		target_read.x,
		target_read.y,
		target_read.w,
		target_read.h,
		baseline_target );

	ve_fontcache_backend_test_reset_state( options );
	pipeline_ok = pipeline_ok && ve_fontcache_backend_test_write_surface_region(
		options,
		"glyph_buffer",
		glyph_source.x,
		glyph_source.y,
		glyph_source.w,
		glyph_source.h,
		expected_glyph );
	ve_fontcache_backend_test_append_atlas_clear( options.cache, atlas_dest );
	ve_fontcache_backend_test_append_atlas_blit( options.cache, atlas_dest, glyph_source );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, target_dest, atlas_dest );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > target_pixels;
	pipeline_ok = pipeline_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		target_read.x,
		target_read.y,
		target_read.w,
		target_read.h,
		target_pixels );
	std::vector< uint8_t > expected_target = ve_fontcache_backend_test_make_image( target_read.w, target_read.h, 0 );
	for ( int y = 0; y < target_dest.h; y++ ) {
		for ( int x = 0; x < target_dest.w; x++ ) {
			expected_target[ static_cast< size_t >( y + target_dest.y - target_read.y ) * target_read.w + ( x + target_dest.x - target_read.x ) ] =
				atlas_reference[ static_cast< size_t >( y ) * target_dest.w + x ];
		}
	}
	const ve_fontcache_backend_test_diff_stats target_stage_diff = ve_fontcache_backend_test_expected_vs_actual_diff(
		baseline_target,
		target_pixels,
		target_read.w,
		target_read.h );
	ve_fontcache_backend_test_expect(
		result,
		pipeline_ok
			&& baseline_ok
			&& glyph_stage_diff.mean_abs_error <= 4.0
			&& atlas_stage_diff.mean_abs_error <= 8.0
			&& target_stage_diff.mean_abs_error <= 8.0,
		"cross_pass_sequences.pipeline: expected GLYPH -> ATLAS -> TARGET sequence to preserve each intermediate surface, observed glyph_diff="
			+ ve_fontcache_backend_test_format_diff( glyph_stage_diff )
			+ ", atlas_diff=" + ve_fontcache_backend_test_format_diff( atlas_stage_diff )
			+ ", target_diff=" + ve_fontcache_backend_test_format_diff( target_stage_diff )
			+ "; probable cause: pass sequencing or state leakage between GLYPH, ATLAS, and TARGET" );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect clear_source = { 860, 720, 64, 64 };
	const ve_fontcache_backend_test_rect clear_target_dest = { 320, 240, 64, 64 };
	const ve_fontcache_backend_test_rect clear_target_read = { 312, 232, 80, 80 };
	std::vector< uint8_t > clear_pattern = ve_fontcache_backend_test_make_image( clear_source.w, clear_source.h, 200 );
	bool clear_case_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		clear_source.x,
		clear_source.y,
		clear_source.w,
		clear_source.h,
		clear_pattern );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, clear_target_dest, clear_source );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > clear_baseline_pixels;
	clear_case_ok = clear_case_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		clear_target_read.x,
		clear_target_read.y,
		clear_target_read.w,
		clear_target_read.h,
		clear_baseline_pixels );
	ve_fontcache_backend_test_reset_state( options );
	clear_case_ok = clear_case_ok && ve_fontcache_backend_test_write_surface_region(
		options,
		"atlas",
		clear_source.x,
		clear_source.y,
		clear_source.w,
		clear_source.h,
		clear_pattern );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, clear_target_dest, clear_source );
	ve_fontcache_backend_test_append_clear( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > clear_case_pixels;
	clear_case_ok = clear_case_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		clear_target_read.x,
		clear_target_read.y,
		clear_target_read.w,
		clear_target_read.h,
		clear_case_pixels );
	ve_fontcache_backend_test_expect(
		result,
		clear_case_ok
			&& ve_fontcache_backend_test_expected_vs_actual_diff(
				clear_baseline_pixels,
				clear_case_pixels,
				clear_target_read.w,
				clear_target_read.h ).mean_abs_error <= 6.0,
		"cross_pass_sequences.target_linear_then_glyph_clear: expected TARGET output to survive a later GLYPH clear draw, observed diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				clear_baseline_pixels,
				clear_case_pixels,
				clear_target_read.w,
				clear_target_read.h ) )
			+ "; probable cause: accidental framebuffer rebinding during GLYPH clear" );

	ve_fontcache_backend_test_reset_state( options );
	const ve_fontcache_backend_test_rect stable_source = { 64, 96, 192, 192 };
	const ve_fontcache_backend_test_rect stable_atlas_dest = { 640, 384, 48, 48 };
	const ve_fontcache_backend_test_rect stable_atlas_read = { 632, 376, 64, 64 };
	const ve_fontcache_backend_test_rect stable_target_dest = { 420, 240, 48, 48 };
	const ve_fontcache_backend_test_rect stable_target_read = { 412, 232, 64, 64 };
	std::vector< uint8_t > stable_pattern = ve_fontcache_backend_test_make_image( stable_source.w, stable_source.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		stable_pattern,
		stable_source.w,
		stable_source.h,
		0,
		0,
		stable_source.w,
		stable_source.h,
		250,
		170,
		90,
		30 );
	bool stable_ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"glyph_buffer",
		stable_source.x,
		stable_source.y,
		stable_source.w,
		stable_source.h,
		stable_pattern );
	ve_fontcache_backend_test_append_atlas_clear( options.cache, stable_atlas_dest );
	ve_fontcache_backend_test_append_atlas_blit( options.cache, stable_atlas_dest, stable_source );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > atlas_first;
	stable_ok = stable_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"atlas",
		stable_atlas_read.x,
		stable_atlas_read.y,
		stable_atlas_read.w,
		stable_atlas_read.h,
		atlas_first );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, stable_target_dest, stable_atlas_dest );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > target_first;
	stable_ok = stable_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		stable_target_read.x,
		stable_target_read.y,
		stable_target_read.w,
		stable_target_read.h,
		target_first );
	ve_fontcache_backend_test_append_atlas_clear( options.cache, stable_atlas_dest );
	ve_fontcache_backend_test_append_atlas_blit( options.cache, stable_atlas_dest, stable_source );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > atlas_second;
	stable_ok = stable_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"atlas",
		stable_atlas_read.x,
		stable_atlas_read.y,
		stable_atlas_read.w,
		stable_atlas_read.h,
		atlas_second );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, stable_target_dest, stable_atlas_dest );
	ve_fontcache_backend_test_execute_pipeline( options );
	std::vector< uint8_t > target_second;
	stable_ok = stable_ok && ve_fontcache_backend_test_readback_texture(
		options,
		"target",
		stable_target_read.x,
		stable_target_read.y,
		stable_target_read.w,
		stable_target_read.h,
		target_second );
	ve_fontcache_backend_test_expect(
		result,
		stable_ok && atlas_first == atlas_second && target_first == target_second,
		"cross_pass_sequences.alternating_stability: expected repeated atlas/target passes to stay stable back-to-back, observed atlas_diff="
			+ ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				atlas_first,
				atlas_second,
				stable_atlas_read.w,
				stable_atlas_read.h ) )
			+ ", target_diff=" + ve_fontcache_backend_test_format_diff( ve_fontcache_backend_test_expected_vs_actual_diff(
				target_first,
				target_second,
				stable_target_read.w,
				stable_target_read.h ) )
			+ "; probable cause: state leakage between atlas and target passes" );
}

inline void ve_fontcache_backend_test_run_pipeline_end_to_end_stb(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"pipeline_end_to_end_stb",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_STB_MODE ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect source_rect = { 64, 96, 192, 192 };
	const ve_fontcache_backend_test_rect atlas_dest = { 640, 384, 48, 48 };
	const ve_fontcache_backend_test_rect atlas_read = { 632, 376, 64, 64 };
	const ve_fontcache_backend_test_rect target_dest = { 420, 240, 48, 48 };
	const ve_fontcache_backend_test_rect target_read = { 412, 232, 64, 64 };
	std::vector< uint8_t > source_pattern = ve_fontcache_backend_test_make_image( source_rect.w, source_rect.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		source_pattern,
		source_rect.w,
		source_rect.h,
		0,
		0,
		source_rect.w,
		source_rect.h,
		250,
		170,
		90,
		30 );
	const std::vector< uint8_t > downsampled = ve_fontcache_backend_test_box_downsample(
		source_pattern,
		source_rect.w,
		source_rect.h,
		atlas_dest.w,
		atlas_dest.h );
	std::vector< uint8_t > expected_atlas = ve_fontcache_backend_test_make_image( atlas_read.w, atlas_read.h, 0 );
	std::vector< uint8_t > expected_target = ve_fontcache_backend_test_make_image( target_read.w, target_read.h, 0 );
	for ( int y = 0; y < atlas_dest.h; y++ ) {
		for ( int x = 0; x < atlas_dest.w; x++ ) {
			const uint8_t value = downsampled[ static_cast< size_t >( y ) * atlas_dest.w + x ];
			expected_atlas[ static_cast< size_t >( y + atlas_dest.y - atlas_read.y ) * atlas_read.w + ( x + atlas_dest.x - atlas_read.x ) ] = value;
			expected_target[ static_cast< size_t >( y + target_dest.y - target_read.y ) * target_read.w + ( x + target_dest.x - target_read.x ) ] = value;
		}
	}

	ve_fontcache_backend_test_reset_state( options );
	bool ok = ve_fontcache_backend_test_write_surface_region(
		options,
		"glyph_buffer",
		source_rect.x,
		source_rect.y,
		source_rect.w,
		source_rect.h,
		source_pattern );
	ve_fontcache_backend_test_append_atlas_clear( options.cache, atlas_dest );
	ve_fontcache_backend_test_append_atlas_blit( options.cache, atlas_dest, source_rect );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET, target_dest, atlas_dest );
	ok = ok && ve_fontcache_backend_test_execute_pipeline_and_present( options );

	std::vector< uint8_t > atlas_pixels;
	std::vector< uint8_t > target_pixels;
	std::vector< uint8_t > presented_pixels;
	ok = ok && ve_fontcache_backend_test_readback_texture( options, "atlas", atlas_read.x, atlas_read.y, atlas_read.w, atlas_read.h, atlas_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options,
		ve_fontcache_backend_test_target_linear_surface_name(),
		target_read.x,
		target_read.y,
		target_read.w,
		target_read.h,
		target_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options,
		ve_fontcache_backend_test_presented_surface_name(),
		target_read.x,
		target_read.y,
		target_read.w,
		target_read.h,
		presented_pixels );

	const ve_fontcache_backend_test_diff_stats atlas_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( expected_atlas, atlas_pixels, atlas_read.w, atlas_read.h );
	const ve_fontcache_backend_test_bbox expected_local_bbox = ve_fontcache_backend_test_thresholded_bbox( expected_target, target_read.w, target_read.h, 8 );
	const ve_fontcache_backend_test_bbox target_local_bbox = ve_fontcache_backend_test_thresholded_bbox( target_pixels, target_read.w, target_read.h, 8 );
	const ve_fontcache_backend_test_bbox presented_local_bbox = ve_fontcache_backend_test_thresholded_bbox( presented_pixels, target_read.w, target_read.h, 8 );
	const ve_fontcache_backend_test_rect expected_local_rect = ve_fontcache_backend_test_inset_rect(
		expected_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( expected_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const ve_fontcache_backend_test_rect target_local_rect = ve_fontcache_backend_test_inset_rect(
		target_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( target_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const ve_fontcache_backend_test_rect presented_local_rect = ve_fontcache_backend_test_inset_rect(
		presented_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( presented_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const std::vector< uint8_t > expected_summary = ve_fontcache_backend_test_rect_valid( expected_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( expected_target, target_read.w, target_read.h, expected_local_rect )
		: std::vector< uint8_t > {};
	const std::vector< uint8_t > target_summary = ve_fontcache_backend_test_rect_valid( target_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( target_pixels, target_read.w, target_read.h, target_local_rect )
		: std::vector< uint8_t > {};
	const std::vector< uint8_t > presented_summary = ve_fontcache_backend_test_rect_valid( presented_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( presented_pixels, target_read.w, target_read.h, presented_local_rect )
		: std::vector< uint8_t > {};
	const ve_fontcache_backend_test_diff_stats target_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( expected_summary, target_summary, 2, 2 );
	const ve_fontcache_backend_test_diff_stats present_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( expected_summary, presented_summary, 2, 2 );
	const ve_fontcache_backend_test_mirror_scores target_mirror_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected_summary, target_summary, 2, 2 );
	const ve_fontcache_backend_test_mirror_scores present_mirror_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected_summary, presented_summary, 2, 2 );
	ve_fontcache_backend_test_expect(
		result,
		ok
			&& atlas_diff.mean_abs_error <= 10.0
			&& target_local_bbox.valid
			&& presented_local_bbox.valid
			&& !ve_fontcache_backend_test_is_vertical_flip( target_mirror_scores )
			&& !ve_fontcache_backend_test_is_horizontal_flip( target_mirror_scores )
			&& !ve_fontcache_backend_test_is_vertical_flip( present_mirror_scores )
			&& !ve_fontcache_backend_test_is_horizontal_flip( present_mirror_scores )
			&& target_diff.mean_abs_error <= 28.0
			&& target_diff.max_abs_error <= 128
			&& present_diff.mean_abs_error <= 28.0
			&& present_diff.max_abs_error <= 128,
		"pipeline_end_to_end.stb_full_chain: expected GLYPH -> ATLAS -> TARGET_LINEAR -> PRESENTED to preserve orientation, observed atlas_diff="
			+ ve_fontcache_backend_test_format_diff( atlas_diff )
			+ ", target_diff=" + ve_fontcache_backend_test_format_diff( target_diff )
			+ ", present_diff=" + ve_fontcache_backend_test_format_diff( present_diff )
			+ "; probable cause: stage-local transform error within the STB pipeline" );
}

inline void ve_fontcache_backend_test_run_pipeline_end_to_end_freetype(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"pipeline_end_to_end_freetype",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_WRITE
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_FREETYPE_MODE
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_CPU_ATLAS ) ) {
		return;
	}

	const ve_fontcache_backend_test_rect source_rect = { 96, 96, 64, 64 };
	const ve_fontcache_backend_test_rect dest_rect = { 360, 300, 64, 64 };
	const ve_fontcache_backend_test_rect read_rect = { 352, 292, 80, 80 };
	std::vector< uint8_t > source_pattern = ve_fontcache_backend_test_make_image( source_rect.w, source_rect.h, 0 );
	ve_fontcache_backend_test_fill_quadrant_pattern(
		source_pattern,
		source_rect.w,
		source_rect.h,
		0,
		0,
		source_rect.w,
		source_rect.h,
		250,
		160,
		80,
		20 );
	std::vector< uint8_t > expected_local = ve_fontcache_backend_test_make_image( read_rect.w, read_rect.h, 0 );
	for ( int y = 0; y < dest_rect.h; y++ ) {
		for ( int x = 0; x < dest_rect.w; x++ ) {
			expected_local[ static_cast< size_t >( y + dest_rect.y - read_rect.y ) * read_rect.w + ( x + dest_rect.x - read_rect.x ) ] =
				source_pattern[ static_cast< size_t >( y ) * source_rect.w + x ];
		}
	}

	ve_fontcache_backend_test_reset_state( options );
	bool ok = ve_fontcache_backend_test_write_surface_region(
		options,
		ve_fontcache_backend_test_cpu_atlas_page_surface_name(),
		source_rect.x,
		source_rect.y,
		source_rect.w,
		source_rect.h,
		source_pattern );
	ve_fontcache_backend_test_append_quad( options.cache, VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED, dest_rect, source_rect );
	ok = ok && ve_fontcache_backend_test_execute_pipeline_and_present( options );

	std::vector< uint8_t > target_pixels;
	std::vector< uint8_t > presented_pixels;
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options,
		ve_fontcache_backend_test_target_linear_surface_name(),
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		target_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options,
		ve_fontcache_backend_test_presented_surface_name(),
		read_rect.x,
		read_rect.y,
		read_rect.w,
		read_rect.h,
		presented_pixels );

	const ve_fontcache_backend_test_bbox expected_local_bbox = ve_fontcache_backend_test_thresholded_bbox( expected_local, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_bbox target_local_bbox = ve_fontcache_backend_test_thresholded_bbox( target_pixels, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_bbox presented_local_bbox = ve_fontcache_backend_test_thresholded_bbox( presented_pixels, read_rect.w, read_rect.h, 8 );
	const ve_fontcache_backend_test_rect expected_local_rect = ve_fontcache_backend_test_inset_rect(
		expected_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( expected_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const ve_fontcache_backend_test_rect target_local_rect = ve_fontcache_backend_test_inset_rect(
		target_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( target_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const ve_fontcache_backend_test_rect presented_local_rect = ve_fontcache_backend_test_inset_rect(
		presented_local_bbox.valid ? ve_fontcache_backend_test_rect_from_bbox( presented_local_bbox ) : ve_fontcache_backend_test_rect {},
		2 );
	const std::vector< uint8_t > expected_summary = ve_fontcache_backend_test_rect_valid( expected_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( expected_local, read_rect.w, read_rect.h, expected_local_rect )
		: std::vector< uint8_t > {};
	const std::vector< uint8_t > target_summary = ve_fontcache_backend_test_rect_valid( target_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( target_pixels, read_rect.w, read_rect.h, target_local_rect )
		: std::vector< uint8_t > {};
	const std::vector< uint8_t > presented_summary = ve_fontcache_backend_test_rect_valid( presented_local_rect )
		? ve_fontcache_backend_test_make_normalized_quadrant_summary( presented_pixels, read_rect.w, read_rect.h, presented_local_rect )
		: std::vector< uint8_t > {};
	const ve_fontcache_backend_test_diff_stats target_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( expected_summary, target_summary, 2, 2 );
	const ve_fontcache_backend_test_diff_stats present_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( expected_summary, presented_summary, 2, 2 );
	const ve_fontcache_backend_test_mirror_scores target_mirror_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected_summary, target_summary, 2, 2 );
	const ve_fontcache_backend_test_mirror_scores present_mirror_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected_summary, presented_summary, 2, 2 );
	const ve_fontcache_backend_test_bbox expected_bbox = ve_fontcache_backend_test_translate_bbox( expected_local_bbox, read_rect.x, read_rect.y );
	const ve_fontcache_backend_test_bbox target_bbox = ve_fontcache_backend_test_translate_bbox( target_local_bbox, read_rect.x, read_rect.y );
	const ve_fontcache_backend_test_bbox presented_bbox = ve_fontcache_backend_test_translate_bbox( presented_local_bbox, read_rect.x, read_rect.y );
	ve_fontcache_backend_test_expect(
		result,
		ok
			&& target_bbox.valid
			&& presented_bbox.valid
			&& target_bbox.w >= 46
			&& target_bbox.h >= 28
			&& presented_bbox.w >= 46
			&& presented_bbox.h >= 28
			&& !ve_fontcache_backend_test_is_vertical_flip( target_mirror_scores )
			&& !ve_fontcache_backend_test_is_horizontal_flip( target_mirror_scores )
			&& !ve_fontcache_backend_test_is_vertical_flip( present_mirror_scores )
			&& !ve_fontcache_backend_test_is_horizontal_flip( present_mirror_scores )
			&& target_diff.mean_abs_error <= 28.0
			&& target_diff.max_abs_error <= 128
			&& present_diff.mean_abs_error <= 28.0
			&& present_diff.max_abs_error <= 128,
		"pipeline_end_to_end.freetype_full_chain: expected CPU_ATLAS_PAGE -> TARGET_LINEAR -> PRESENTED to preserve orientation, observed target_diff="
			+ ve_fontcache_backend_test_format_diff( target_diff )
			+ ", present_diff=" + ve_fontcache_backend_test_format_diff( present_diff )
			+ ", target_bbox=" + ve_fontcache_backend_test_format_bbox( target_bbox )
			+ ", presented_bbox=" + ve_fontcache_backend_test_format_bbox( presented_bbox )
			+ ", target_mirror_scores={direct=" + std::to_string( target_mirror_scores.direct )
			+ ", horizontal=" + std::to_string( target_mirror_scores.horizontal )
			+ ", vertical=" + std::to_string( target_mirror_scores.vertical )
			+ ", both=" + std::to_string( target_mirror_scores.both ) + "}"
			+ ", present_mirror_scores={direct=" + std::to_string( present_mirror_scores.direct )
			+ ", horizontal=" + std::to_string( present_mirror_scores.horizontal )
			+ ", vertical=" + std::to_string( present_mirror_scores.vertical )
			+ ", both=" + std::to_string( present_mirror_scores.both ) + "}"
			+ "; probable cause: stage-local transform error within the FreeType pipeline" );
}

inline void ve_fontcache_backend_test_run_real_text_cached_glyph_orientation(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"real_text_cached_glyph_orientation",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_STB_MODE ) ) {
		return;
	}

	ve_fontcache_backend_test_prepare_real_text( options );
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"real_text_cached_glyph_orientation",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY } ) ) {
		return;
	}

	const ve_font_id font = options.font;
	const std::u8string glyph_text = u8"R";

	const int target_w = ve_fontcache_backend_test_target_width( options.cache );
	const int target_h = ve_fontcache_backend_test_target_height( options.cache );
	const float sx = 1.0f / target_w;
	const float sy = 1.0f / target_h;
	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		font,
		glyph_text,
		false,
		0.42f,
		0.44f,
		sx,
		sy );
	ve_fontcache_backend_test_real_glyph_sample sample;
	bool draw_ok = ve_fontcache_backend_test_extract_real_glyph_sample(
		options,
		*drawlist,
		VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET,
		"atlas",
		sample );
	draw_ok = draw_ok && ve_fontcache_backend_test_execute_pipeline_and_present( options );

	ve_fontcache_backend_test_real_glyph_observation observation;
	const bool capture_ok = ve_fontcache_backend_test_capture_real_glyph_observation( options, "atlas", sample, observation );
	const double best_mirror = std::min(
		std::min( observation.mirror_scores.horizontal, observation.mirror_scores.vertical ),
		observation.mirror_scores.both );
	const bool direct_match = observation.diff.mean_abs_error + 8.0 <= best_mirror;
	const bool scale_error = ve_fontcache_backend_test_is_uniform_scale_error( observation.source_bbox, observation.presented_bbox );
	const bool aspect_error = ve_fontcache_backend_test_is_aspect_ratio_deformation( observation.source_bbox, observation.presented_bbox );
	std::string cause = "orientation mismatch in real-text cached glyph path";
	if ( !draw_ok || !sample.valid ) {
		cause = "missing atlas-backed draw sample in real-text cached glyph path";
	} else if ( !capture_ok || !observation.ok ) {
		cause = "missing atlas or presented glyph data in real-text cached glyph path";
	} else if ( observation.vertical_flip ) {
		cause = "V coordinate inversion in real-text cached glyph path";
	} else if ( observation.horizontal_flip ) {
		cause = "U coordinate inversion in real-text cached glyph path";
	} else if ( scale_error ) {
		cause = "uniform scale drift in real-text cached glyph path";
	} else if ( aspect_error ) {
		cause = "aspect-ratio deformation in real-text cached glyph path";
	} else if ( !direct_match ) {
		cause = observation.asymmetric
			? "unexpected distortion in real-text cached glyph path"
			: "selected atlas-backed glyph was too symmetric for orientation detection";
	}

	ve_fontcache_backend_test_expect(
		result,
		draw_ok
			&& capture_ok
			&& observation.ok
			&& !observation.vertical_flip
			&& !observation.horizontal_flip
			&& !scale_error
			&& !aspect_error
			&& direct_match,
		std::string( "real_text_cached_glyph_orientation: glyph=R" )
			+ ", atlas_bbox=" + ve_fontcache_backend_test_format_bbox( observation.source_bbox )
			+ ", presented_bbox=" + ve_fontcache_backend_test_format_bbox( observation.presented_bbox )
			+ ", atlas_centroid=" + ve_fontcache_backend_test_format_center( observation.source_center )
			+ ", presented_centroid=" + ve_fontcache_backend_test_format_center( observation.presented_center )
			+ ", diff=" + ve_fontcache_backend_test_format_diff( observation.diff )
			+ ", mirror_scores={direct=" + std::to_string( observation.mirror_scores.direct )
			+ ", horizontal=" + std::to_string( observation.mirror_scores.horizontal )
			+ ", vertical=" + std::to_string( observation.mirror_scores.vertical )
			+ ", both=" + std::to_string( observation.mirror_scores.both ) + "}"
			+ ", source_symmetry={horizontal=" + std::to_string( observation.source_symmetry_scores.horizontal )
			+ ", vertical=" + std::to_string( observation.source_symmetry_scores.vertical ) + "}"
			+ "; probable cause: " + cause );
}

inline void ve_fontcache_backend_test_run_real_text_uncached_glyph_orientation(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"real_text_uncached_glyph_orientation",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_STB_MODE ) ) {
		return;
	}

	ve_fontcache_backend_test_prepare_real_text( options );
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"real_text_uncached_glyph_orientation",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HUGE } ) ) {
		return;
	}

	const int target_w = ve_fontcache_backend_test_target_width( options.cache );
	const int target_h = ve_fontcache_backend_test_target_height( options.cache );
	const float sx = 1.0f / target_w;
	const float sy = 1.0f / target_h;
	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		options.huge_font,
		u8"R",
		false,
		0.32f,
		0.34f,
		sx,
		sy );
	bool seen_uncached_target = false;
	drawlist->dcalls.erase(
		std::remove_if(
			drawlist->dcalls.begin(),
			drawlist->dcalls.end(),
			[&seen_uncached_target]( const ve_fontcache_draw& draw ) {
				if ( draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED && draw.end_index > draw.start_index ) {
					seen_uncached_target = true;
					return false;
				}
				return seen_uncached_target
					&& draw.pass == VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH
					&& draw.clear_before_draw
					&& draw.end_index == draw.start_index;
			} ),
		drawlist->dcalls.end() );

	ve_fontcache_backend_test_real_glyph_sample sample;
	bool draw_ok = ve_fontcache_backend_test_extract_real_glyph_sample(
		options,
		*drawlist,
		VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED,
		"glyph_buffer",
		sample );
	draw_ok = draw_ok && ve_fontcache_backend_test_execute_pipeline_and_present( options );

	ve_fontcache_backend_test_real_glyph_observation observation;
	const bool capture_ok = ve_fontcache_backend_test_capture_real_glyph_observation( options, "glyph_buffer", sample, observation );
	const double best_mirror = std::min(
		std::min( observation.mirror_scores.horizontal, observation.mirror_scores.vertical ),
		observation.mirror_scores.both );
	const bool direct_match = observation.diff.mean_abs_error + 8.0 <= best_mirror;
	std::string cause = "orientation mismatch in real-text uncached glyph path";
	if ( !draw_ok || !sample.valid ) {
		cause = "missing glyph-buffer draw sample in real-text uncached glyph path";
	} else if ( !capture_ok || !observation.ok ) {
		cause = "missing glyph-buffer or presented glyph data in real-text uncached glyph path";
	} else if ( observation.vertical_flip ) {
		cause = "V coordinate inversion in real-text uncached glyph path";
	} else if ( observation.horizontal_flip ) {
		cause = "U coordinate inversion in real-text uncached glyph path";
	} else if ( !direct_match ) {
		cause = observation.asymmetric
			? "unexpected distortion in real-text uncached glyph path"
			: "selected uncached glyph was too symmetric for orientation detection";
	}

	ve_fontcache_backend_test_expect(
		result,
		draw_ok
			&& capture_ok
			&& observation.ok
			&& !observation.vertical_flip
			&& !observation.horizontal_flip
			&& direct_match,
		"real_text_uncached_glyph_orientation: glyph_buffer_bbox=" + ve_fontcache_backend_test_format_bbox( observation.source_bbox )
			+ ", presented_bbox=" + ve_fontcache_backend_test_format_bbox( observation.presented_bbox )
			+ ", glyph_buffer_centroid=" + ve_fontcache_backend_test_format_center( observation.source_center )
			+ ", presented_centroid=" + ve_fontcache_backend_test_format_center( observation.presented_center )
			+ ", diff=" + ve_fontcache_backend_test_format_diff( observation.diff )
			+ ", mirror_scores={direct=" + std::to_string( observation.mirror_scores.direct )
			+ ", horizontal=" + std::to_string( observation.mirror_scores.horizontal )
			+ ", vertical=" + std::to_string( observation.mirror_scores.vertical )
			+ ", both=" + std::to_string( observation.mirror_scores.both ) + "}"
			+ ", source_symmetry={horizontal=" + std::to_string( observation.source_symmetry_scores.horizontal )
			+ ", vertical=" + std::to_string( observation.source_symmetry_scores.vertical ) + "}"
			+ "; probable cause: " + cause );
}

inline void ve_fontcache_backend_test_run_real_text_cpu_cached_glyph_orientation(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"real_text_cpu_cached_glyph_orientation",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_FREETYPE_MODE
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_CPU_ATLAS ) ) {
		return;
	}

	ve_fontcache_backend_test_prepare_real_text( options );
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"real_text_cpu_cached_glyph_orientation",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY } ) ) {
		return;
	}

	const ve_font_id font = options.font;
	const std::u8string glyph_text = u8"R";

	const int target_w = ve_fontcache_backend_test_target_width( options.cache );
	const int target_h = ve_fontcache_backend_test_target_height( options.cache );
	const float sx = 1.0f / target_w;
	const float sy = 1.0f / target_h;
	ve_fontcache_backend_test_reset_state( options );
	ve_fontcache_drawlist* drawlist = ve_fontcache_backend_test_draw(
		result,
		options.cache,
		font,
		glyph_text,
		false,
		0.42f,
		0.44f,
		sx,
		sy );
	ve_fontcache_backend_test_real_glyph_sample sample;
	bool draw_ok = ve_fontcache_backend_test_extract_real_glyph_sample(
		options,
		*drawlist,
		VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_CPU_CACHED,
		ve_fontcache_backend_test_cpu_atlas_page_surface_name(),
		sample );
	draw_ok = draw_ok && ve_fontcache_backend_test_execute_pipeline_and_present( options );
	const bool page_zero = sample.valid
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
		&& sample.atlas_page == 0
#else
		&& true
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
		;

	ve_fontcache_backend_test_real_glyph_observation observation;
	const bool capture_ok = page_zero
		&& ve_fontcache_backend_test_capture_real_glyph_observation(
			options,
			ve_fontcache_backend_test_cpu_atlas_page_surface_name(),
			sample,
			observation );
	const double best_mirror = std::min(
		std::min( observation.mirror_scores.horizontal, observation.mirror_scores.vertical ),
		observation.mirror_scores.both );
	const bool direct_match = observation.diff.mean_abs_error + 8.0 <= best_mirror;
	const bool scale_error = ve_fontcache_backend_test_is_uniform_scale_error( observation.source_bbox, observation.presented_bbox );
	const bool aspect_error = ve_fontcache_backend_test_is_aspect_ratio_deformation( observation.source_bbox, observation.presented_bbox );
	std::string cause = "orientation mismatch in real-text CPU-cached glyph path";
	if ( !draw_ok || !sample.valid ) {
		cause = "missing CPU-atlas draw sample in real-text CPU-cached glyph path";
	} else if ( !page_zero ) {
		cause = "glyph uploaded to a CPU atlas page other than page 0";
	} else if ( !capture_ok || !observation.ok ) {
		cause = "missing CPU atlas or presented glyph data in real-text CPU-cached glyph path";
	} else if ( observation.vertical_flip ) {
		cause = "V coordinate inversion in real-text CPU-cached glyph path";
	} else if ( observation.horizontal_flip ) {
		cause = "U coordinate inversion in real-text CPU-cached glyph path";
	} else if ( scale_error ) {
		cause = "uniform scale drift in real-text CPU-cached glyph path";
	} else if ( aspect_error ) {
		cause = "aspect-ratio deformation in real-text CPU-cached glyph path";
	} else if ( !direct_match ) {
		cause = observation.asymmetric
			? "unexpected distortion in real-text CPU-cached glyph path"
			: "selected CPU-cached glyph was too symmetric for orientation detection";
	}

	ve_fontcache_backend_test_expect(
		result,
		draw_ok
			&& page_zero
			&& capture_ok
			&& observation.ok
			&& !observation.vertical_flip
			&& !observation.horizontal_flip
			&& !scale_error
			&& !aspect_error
			&& direct_match,
		std::string( "real_text_cpu_cached_glyph_orientation: glyph=R" )
			+ ", cpu_atlas_bbox=" + ve_fontcache_backend_test_format_bbox( observation.source_bbox )
			+ ", presented_bbox=" + ve_fontcache_backend_test_format_bbox( observation.presented_bbox )
			+ ", cpu_atlas_centroid=" + ve_fontcache_backend_test_format_center( observation.source_center )
			+ ", presented_centroid=" + ve_fontcache_backend_test_format_center( observation.presented_center )
			+ ", diff=" + ve_fontcache_backend_test_format_diff( observation.diff )
			+ ", mirror_scores={direct=" + std::to_string( observation.mirror_scores.direct )
			+ ", horizontal=" + std::to_string( observation.mirror_scores.horizontal )
			+ ", vertical=" + std::to_string( observation.mirror_scores.vertical )
			+ ", both=" + std::to_string( observation.mirror_scores.both ) + "}"
			+ ", source_symmetry={horizontal=" + std::to_string( observation.source_symmetry_scores.horizontal )
			+ ", vertical=" + std::to_string( observation.source_symmetry_scores.vertical ) + "}"
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
			+ ", atlas_page=" + std::to_string( sample.atlas_page )
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION
			+ "; probable cause: " + cause );
}

inline void ve_fontcache_backend_test_run_real_text_canonical_glyph_orientation(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"real_text_canonical_glyph_orientation",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT ) ) {
		return;
	}

	ve_fontcache_backend_test_prepare_real_text( options );
	if ( !ve_fontcache_backend_test_require_any_role(
		result,
		options,
		"real_text_canonical_glyph_orientation",
		{
			VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY,
			VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SECONDARY,
			VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_SMALL,
			VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_LATIN,
			VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_CJK,
			VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HUGE,
		} ) ) {
		return;
	}

	const int target_w = ve_fontcache_backend_test_target_width( options.cache );
	const int target_h = ve_fontcache_backend_test_target_height( options.cache );
	const float sx = 1.0f / target_w;
	const float sy = 1.0f / target_h;

	struct glyph_case
	{
		char32_t codepoint = 0;
		const char* label = "";
		std::array< ve_font_id, 5 > candidate_fonts {};
	};

	const glyph_case cases[] = {
		{ U'R', "R", { options.latin_font, options.secondary_font, options.font, options.small_font, options.huge_font } },
		{ U'ゑ', "U+3091", { options.font, options.cjk_font, options.huge_font, options.secondary_font, options.latin_font } },
	};

	auto pick_font_for_codepoint = [&]( const glyph_case& current ) -> ve_font_id {
		for ( ve_font_id candidate_font : current.candidate_fonts ) {
			if ( !ve_fontcache_is_valid_font_id( options.cache, candidate_font ) ) {
				continue;
			}
			if ( ve_fontcache_backend_test_find_glyph( options.cache, candidate_font, current.codepoint ) != 0 ) {
				return candidate_font;
			}
		}
		return -1;
	};

	bool any_case_ran = false;
	bool informative_case_seen = false;
	bool all_cases_ok = true;
	std::ostringstream details;
	for ( const glyph_case& current : cases ) {
		if ( details.tellp() > 0 ) {
			details << " | ";
		}

		const ve_font_id font = pick_font_for_codepoint( current );
		if ( font < 0 ) {
			details << current.label << "(U+" << ve_fontcache_backend_test_format_codepoint( current.codepoint )
				<< "): missing glyph in supplied fonts";
			continue;
		}

		any_case_ran = true;
		ve_fontcache_backend_test_canonical_glyph_mask canonical;
		ve_fontcache_backend_test_presented_glyph_mask presented;
		const bool canonical_ok = ve_fontcache_backend_test_rasterize_canonical_glyph(
			options.cache,
			font,
			current.codepoint,
			canonical );
		const bool presented_ok = ve_fontcache_backend_test_capture_presented_glyph_mask(
			result,
			options,
			font,
			current.codepoint,
			0.42f,
			0.44f,
			sx,
			sy,
			presented );

		ve_fontcache_backend_test_diff_stats diff;
		ve_fontcache_backend_test_mirror_scores mirror_scores;
		ve_fontcache_backend_test_mirror_scores symmetry_scores;
		bool vertical_flip = false;
		bool horizontal_flip = false;
		bool vertical_informative = false;
		bool horizontal_informative = false;
		bool axis_informative = false;
		bool direct_beats_mirror = true;
		bool large_direct_diff = false;
		if ( canonical_ok && presented_ok ) {
			diff = ve_fontcache_backend_test_expected_vs_actual_diff( canonical.summary, presented.summary, 3, 3 );
			mirror_scores = ve_fontcache_backend_test_mirror_scores_for_expected( canonical.summary, presented.summary, 3, 3 );
			symmetry_scores = ve_fontcache_backend_test_mirror_scores_for_expected( canonical.summary, canonical.summary, 3, 3 );
			horizontal_informative = symmetry_scores.horizontal >= 16.0;
			vertical_informative = symmetry_scores.vertical >= 16.0;
			axis_informative = horizontal_informative || vertical_informative;
			horizontal_flip = horizontal_informative && ve_fontcache_backend_test_is_horizontal_flip( mirror_scores );
			vertical_flip = vertical_informative && ve_fontcache_backend_test_is_vertical_flip( mirror_scores );
			double best_relevant_mirror = std::numeric_limits< double >::infinity();
			if ( horizontal_informative ) {
				best_relevant_mirror = std::min( best_relevant_mirror, mirror_scores.horizontal );
			}
			if ( vertical_informative ) {
				best_relevant_mirror = std::min( best_relevant_mirror, mirror_scores.vertical );
			}
			if ( horizontal_informative && vertical_informative ) {
				best_relevant_mirror = std::min( best_relevant_mirror, mirror_scores.both );
			}
			if ( std::isfinite( best_relevant_mirror ) ) {
				direct_beats_mirror = diff.mean_abs_error + 8.0 <= best_relevant_mirror;
			}
			large_direct_diff = diff.mean_abs_error > 72.0 || diff.max_abs_error > 224;
			informative_case_seen = informative_case_seen || axis_informative;
		}

		std::string cause = "canonical orientation preserved";
		if ( !canonical_ok ) {
			cause = "failed to rasterize canonical CPU glyph";
		} else if ( !presented_ok ) {
			cause = "failed to capture presented glyph";
		} else if ( vertical_flip ) {
			cause = "vertical inversion in displayed glyph";
		} else if ( horizontal_flip ) {
			cause = "horizontal inversion in displayed glyph";
		} else if ( axis_informative && large_direct_diff ) {
			cause = "large direct mismatch against canonical glyph";
		} else if ( axis_informative && !direct_beats_mirror ) {
			cause = "canonical glyph matched a mirrored orientation better than the direct orientation";
		} else if ( !axis_informative ) {
			cause = "selected glyph was too symmetric after normalization";
		}

		const bool case_ok = canonical_ok
			&& presented_ok
			&& !vertical_flip
			&& !horizontal_flip
			&& ( !axis_informative || ( direct_beats_mirror && !large_direct_diff ) );
		all_cases_ok = all_cases_ok && case_ok;

		details << current.label
			<< "(font=" << font
			<< ", cp=U+" << ve_fontcache_backend_test_format_codepoint( current.codepoint )
			<< "): canonical_bbox=" << ve_fontcache_backend_test_format_bbox( canonical.bbox )
			<< ", presented_bbox=" << ve_fontcache_backend_test_format_bbox( presented.bbox )
			<< ", canonical_centroid=" << ve_fontcache_backend_test_format_center( canonical.center )
			<< ", presented_centroid=" << ve_fontcache_backend_test_format_center( presented.center )
			<< ", canonical_bitmap_box=(" << canonical.bitmap_box_x0 << "," << canonical.bitmap_box_y0
			<< ")-(" << canonical.bitmap_box_x1 << "," << canonical.bitmap_box_y1 << ")"
			<< ", presented_draw_rect=(" << presented.draw_rect.x << "," << presented.draw_rect.y
			<< " " << presented.draw_rect.w << "x" << presented.draw_rect.h << ")"
			<< ", presented_read_rect=(" << presented.read_rect.x << "," << presented.read_rect.y
			<< " " << presented.read_rect.w << "x" << presented.read_rect.h << ")"
			<< ", target_pass=" << presented.target_pass
			<< ", diff=" << ve_fontcache_backend_test_format_diff( diff )
			<< ", mirror_scores={direct=" << std::to_string( mirror_scores.direct )
			<< ", horizontal=" << std::to_string( mirror_scores.horizontal )
			<< ", vertical=" << std::to_string( mirror_scores.vertical )
			<< ", both=" << std::to_string( mirror_scores.both ) << "}"
			<< ", canonical_symmetry={horizontal=" << std::to_string( symmetry_scores.horizontal )
			<< ", vertical=" << std::to_string( symmetry_scores.vertical ) << "}"
			<< "; probable cause: " << cause;
	}

	if ( !any_case_ran ) {
		ve_fontcache_backend_test_skip(
			result,
			"real_text_canonical_glyph_orientation skipped: no supplied font contained R or U+3091" );
		return;
	}

	ve_fontcache_backend_test_expect(
		result,
		informative_case_seen && all_cases_ok,
		std::string( "real_text_canonical_glyph_orientation: " ) + details.str()
			+ ( informative_case_seen
				? ""
				: "; probable cause: selected glyphs were too symmetric for authoritative orientation detection" ) );
}

inline void ve_fontcache_backend_test_run_real_text_micro_scene(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"real_text_micro_scene",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT ) ) {
		return;
	}

	const int target_w = ve_fontcache_backend_test_target_width( options.cache );
	const int target_h = ve_fontcache_backend_test_target_height( options.cache );
	const float sx = 1.0f / target_w;
	const float sy = 1.0f / target_h;
	const ve_fontcache_backend_test_rect tl_window = { 32, 760, 640, 220 };
	const ve_fontcache_backend_test_rect tr_window = { 940, 760, 640, 220 };
	const ve_fontcache_backend_test_rect bl_window = { 32, 120, 640, 220 };
	const ve_fontcache_backend_test_rect br_window = { 940, 120, 640, 220 };
	ve_fontcache_backend_test_prepare_real_text( options );
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"real_text_micro_scene",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_PRIMARY } ) ) {
		return;
	}

	auto draw_scene = [&]() {
		ve_fontcache_backend_test_reset_state( options );
		bool ok = ve_fontcache_draw_text( options.cache, options.font, u8"VEFontCache", 0.06f, 0.84f, sx, sy, false );
		ok = ok && ve_fontcache_draw_text(
			options.cache,
			options.font,
			u8"Top-right sample",
			0.60f,
			0.78f,
			sx,
			sy,
			false );
		ok = ok && ve_fontcache_draw_text(
			options.cache,
			options.font,
			u8"Bottom-left sample",
			0.06f,
			0.26f,
			sx,
			sy,
			false );
		ok = ok && ve_fontcache_draw_text(
			options.cache,
			options.font,
			u8"Bottom-right sample",
			0.60f,
			0.26f,
			sx,
			sy,
			false );
		ok = ok && ve_fontcache_backend_test_execute_pipeline_and_present( options );
		return ok;
	};

	std::vector< uint8_t > first_presented;
	std::vector< uint8_t > second_presented;
	auto capture_scene = [&]( std::vector< uint8_t >& presented ) {
		ve_fontcache_backend_test_prepare_real_text( options );
		bool ok = draw_scene();
		ok = ok && ve_fontcache_backend_test_readback_texture(
			options,
			ve_fontcache_backend_test_presented_surface_name(),
			0,
			0,
			target_w,
			target_h,
			presented );
		return ok;
	};
	bool ok = capture_scene( first_presented );
	ok = ok && capture_scene( second_presented );

	std::vector< uint8_t > tl_pixels;
	std::vector< uint8_t > tr_pixels;
	std::vector< uint8_t > bl_pixels;
	std::vector< uint8_t > br_pixels;
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), tl_window.x, tl_window.y, tl_window.w, tl_window.h, tl_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), tr_window.x, tr_window.y, tr_window.w, tr_window.h, tr_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), bl_window.x, bl_window.y, bl_window.w, bl_window.h, bl_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), br_window.x, br_window.y, br_window.w, br_window.h, br_pixels );

	ve_fontcache_backend_test_expect(
		result,
		ok
			&& ve_fontcache_backend_test_any_non_zero( first_presented )
			&& first_presented == second_presented
			&& ve_fontcache_backend_test_any_non_zero( tl_pixels )
			&& ve_fontcache_backend_test_any_non_zero( tr_pixels )
			&& ve_fontcache_backend_test_any_non_zero( bl_pixels )
			&& ve_fontcache_backend_test_any_non_zero( br_pixels ),
		"real_text.micro_scene: expected stable presented output with visible text in all four placement windows, observed hashes={first="
			+ std::to_string( ve_fontcache_backend_test_hash_pixels( first_presented ) )
			+ ", second=" + std::to_string( ve_fontcache_backend_test_hash_pixels( second_presented ) )
			+ "}; probable cause: unstable draw ordering or missing presented-stage output" );
}

inline void ve_fontcache_backend_test_run_real_text_harfbuzz_scene(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"real_text_harfbuzz_scene",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_GPU
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_HARFBUZZ ) ) {
		return;
	}
#ifdef VE_FONTCACHE_FREETYPE_RASTERISATION
	if ( !options.cache->use_freetype ) {
		ve_fontcache_backend_test_skip( result, "real_text.hb_scene skipped: STB mode uses the generic real-text scene only" );
		return;
	}
#else
	ve_fontcache_backend_test_skip( result, "real_text.hb_scene skipped: FreeType mode is not compiled in" );
	return;
#endif // VE_FONTCACHE_FREETYPE_RASTERISATION

	const int target_w = ve_fontcache_backend_test_target_width( options.cache );
	const int target_h = ve_fontcache_backend_test_target_height( options.cache );
	const float sx = 1.0f / target_w;
	const float sy = 1.0f / target_h;
	const ve_fontcache_backend_test_rect top_window = { 200, 640, 1500, 180 };
	const ve_fontcache_backend_test_rect bottom_window = { 200, 360, 1500, 180 };
	ve_fontcache_backend_test_prepare_real_text( options );
	if ( !ve_fontcache_backend_test_require_roles(
		result,
		options,
		"real_text.hb_scene",
		{ VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_ARABIC, VE_FONTCACHE_BACKEND_TEST_FONT_ROLE_HEBREW } ) ) {
		return;
	}

	auto draw_scene = [&]() {
		ve_fontcache_backend_test_reset_state( options );
		bool ok = ve_fontcache_draw_text(
			options.cache,
			options.arabic_font,
			u8"حب السماء لا تمطر غير الأحلام",
			0.18f,
			0.68f,
			sx,
			sy,
			false );
		ok = ok && ve_fontcache_draw_text(
			options.cache,
			options.hebrew_font,
			u8"אז הגיע הלילה של כוכב השביט הראשון",
			0.18f,
			0.44f,
			sx,
			sy,
			false );
		ok = ok && ve_fontcache_backend_test_execute_pipeline_and_present( options );
		return ok;
	};

	std::vector< uint8_t > first_presented;
	std::vector< uint8_t > second_presented;
	std::vector< uint8_t > top_pixels;
	std::vector< uint8_t > bottom_pixels;
	auto capture_scene = [&]( std::vector< uint8_t >& presented ) {
		ve_fontcache_backend_test_prepare_real_text( options );
		bool ok = draw_scene();
		ok = ok && ve_fontcache_backend_test_readback_texture(
			options, ve_fontcache_backend_test_presented_surface_name(), 0, 0, target_w, target_h, presented );
		return ok;
	};
	bool ok = capture_scene( first_presented );
	ok = ok && capture_scene( second_presented );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), top_window.x, top_window.y, top_window.w, top_window.h, top_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), bottom_window.x, bottom_window.y, bottom_window.w, bottom_window.h, bottom_pixels );

	ve_fontcache_backend_test_expect(
		result,
		ok
			&& first_presented == second_presented
			&& ve_fontcache_backend_test_any_non_zero( top_pixels )
			&& ve_fontcache_backend_test_any_non_zero( bottom_pixels ),
		"real_text.hb_scene: expected stable shaped-text output in both Arabic and Hebrew windows, observed hashes={first="
			+ std::to_string( ve_fontcache_backend_test_hash_pixels( first_presented ) )
			+ ", second=" + std::to_string( ve_fontcache_backend_test_hash_pixels( second_presented ) )
			+ "}; probable cause: unstable shaping, missing fonts, or missing presented-stage output" );
}

inline void ve_fontcache_backend_test_run_full_demo_frame_smoke(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	if ( !ve_fontcache_backend_test_require_suite(
		result,
		options,
		"full_demo_frame_smoke",
		VE_FONTCACHE_BACKEND_TEST_REQUIRES_RESET
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_PRESENT
			| VE_FONTCACHE_BACKEND_TEST_REQUIRES_FRAME ) ) {
		return;
	}

	const int target_w = ve_fontcache_backend_test_target_width( options.cache );
	const int target_h = ve_fontcache_backend_test_target_height( options.cache );
	const ve_fontcache_backend_test_rect top_left = { 32, 720, 700, 260 };
	const ve_fontcache_backend_test_rect top_right = { 940, 720, 900, 260 };
	const ve_fontcache_backend_test_rect bottom_left = { 32, 200, 860, 340 };
	const ve_fontcache_backend_test_rect bottom_right = { 940, 200, 900, 340 };

	std::vector< uint8_t > first_presented;
	std::vector< uint8_t > second_presented;
	auto capture_frame = [&]( std::vector< uint8_t >& presented ) {
		ve_fontcache_backend_test_prepare_real_text( options );
		bool ok = ve_fontcache_backend_test_execute_frame( options );
		ok = ok && ve_fontcache_backend_test_readback_texture(
			options, ve_fontcache_backend_test_presented_surface_name(), 0, 0, target_w, target_h, presented );
		return ok;
	};
	bool ok = capture_frame( first_presented );
	ok = ok && capture_frame( second_presented );

	std::vector< uint8_t > tl_pixels;
	std::vector< uint8_t > tr_pixels;
	std::vector< uint8_t > bl_pixels;
	std::vector< uint8_t > br_pixels;
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), top_left.x, top_left.y, top_left.w, top_left.h, tl_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), top_right.x, top_right.y, top_right.w, top_right.h, tr_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), bottom_left.x, bottom_left.y, bottom_left.w, bottom_left.h, bl_pixels );
	ok = ok && ve_fontcache_backend_test_readback_texture(
		options, ve_fontcache_backend_test_presented_surface_name(), bottom_right.x, bottom_right.y, bottom_right.w, bottom_right.h, br_pixels );

	ve_fontcache_backend_test_expect(
		result,
		ok
			&& ve_fontcache_backend_test_any_non_zero( first_presented )
			&& first_presented == second_presented
			&& ve_fontcache_backend_test_any_non_zero( tl_pixels )
			&& ve_fontcache_backend_test_any_non_zero( tr_pixels )
			&& ve_fontcache_backend_test_any_non_zero( bl_pixels )
			&& ve_fontcache_backend_test_any_non_zero( br_pixels ),
		"real_text.full_demo_frame: expected stable demo-frame output with energy in all major regions, observed hashes={first="
			+ std::to_string( ve_fontcache_backend_test_hash_pixels( first_presented ) )
			+ ", second=" + std::to_string( ve_fontcache_backend_test_hash_pixels( second_presented ) )
			+ "}; probable cause: blank frame, region inversion, or nondeterministic demo-frame rendering" );
}

inline void ve_fontcache_backend_test_run_stage_local_suites(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_run_glyph_geometry( result, options );
	ve_fontcache_backend_test_run_glyph_blend_xor( result, options );
	ve_fontcache_backend_test_run_atlas_blit_geometry( result, options );
	ve_fontcache_backend_test_run_target_sampling_atlas( result, options );
	ve_fontcache_backend_test_run_target_sampling_flip_detector( result, options );
	ve_fontcache_backend_test_run_target_sampling_uncached( result, options );
	ve_fontcache_backend_test_run_target_sampling_cpu_cached( result, options );
	ve_fontcache_backend_test_run_present_orientation( result, options );
	ve_fontcache_backend_test_run_target_alpha_blend( result, options );
}

inline void ve_fontcache_backend_test_run_pipeline_suites(
	ve_fontcache_backend_test_result& result,
	const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_run_cross_pass_sequences( result, options );
	ve_fontcache_backend_test_run_pipeline_end_to_end_stb( result, options );
	ve_fontcache_backend_test_run_pipeline_end_to_end_freetype( result, options );
}

inline void ve_fontcache_backend_test_run_real_text_suites(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_run_real_text_cached_glyph_orientation( result, options );
	ve_fontcache_backend_test_run_real_text_uncached_glyph_orientation( result, options );
	ve_fontcache_backend_test_run_real_text_cpu_cached_glyph_orientation( result, options );
	ve_fontcache_backend_test_run_real_text_canonical_glyph_orientation( result, options );
	ve_fontcache_backend_test_run_real_text_micro_scene( result, options );
	ve_fontcache_backend_test_run_real_text_harfbuzz_scene( result, options );
	ve_fontcache_backend_test_run_full_demo_frame_smoke( result, options );
}

inline void ve_fontcache_backend_test_run_synthetic_suites(
	ve_fontcache_backend_test_result& result,
	ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_run_surface_roundtrip( result, options );
	ve_fontcache_backend_test_run_surface_contract( result, options );
	ve_fontcache_backend_test_run_stage_local_suites( result, options );
	ve_fontcache_backend_test_run_pipeline_suites( result, options );
	ve_fontcache_backend_test_run_real_text_suites( result, options );
}

inline ve_fontcache_backend_test_result ve_fontcache_backend_test_run( const ve_fontcache_backend_test_options& options )
{
	ve_fontcache_backend_test_result result;
	ve_fontcache_backend_test_expect( result, options.cache != nullptr, "backend test received a cache" );
	if ( !options.cache ) {
		return result;
	}

	ve_fontcache_backend_test_options resolved_options = ve_fontcache_backend_test_resolve_font_roles( options );

	ve_fontcache_backend_test_run_structural_checks( result, resolved_options );
	ve_fontcache_backend_test_run_caching_checks( result, resolved_options );
	ve_fontcache_backend_test_run_readback_checks( result, resolved_options );
	ve_fontcache_backend_test_run_region_checks( result, resolved_options );
	ve_fontcache_backend_test_run_lru_checks( result, resolved_options );
	ve_fontcache_backend_test_run_edge_case_checks( result, resolved_options );
	ve_fontcache_backend_test_run_reload_checks( result, resolved_options );
	ve_fontcache_backend_test_run_synthetic_suites( result, resolved_options );
	ve_fontcache_backend_test_finalise_state( resolved_options );

	return result;
}

inline ve_fontcache_backend_test_result ve_fontcache_backend_test_run(
	ve_fontcache* cache,
	ve_font_id font,
	ve_fontcache_backend_execute_fn execute = {},
	ve_fontcache_backend_readback_fn readback = {} )
{
	ve_fontcache_backend_test_options options;
	options.cache = cache;
	options.font = font;
	options.execute_pipeline = execute;
	options.readback_surface = readback;
	return ve_fontcache_backend_test_run( options );
}
