#include "test_common.h"

UTEST( backend_diagnostics, bbox_profiles_and_centroid )
{
	std::vector< uint8_t > pixels = ve_fontcache_backend_test_make_image( 8, 8, 0 );
	ve_fontcache_backend_test_fill_rect( pixels, 8, 8, 2, 1, 3, 4, 20 );

	ve_fontcache_backend_test_bbox bbox = ve_fontcache_backend_test_thresholded_bbox( pixels, 8, 8, 1 );
	std::vector< uint32_t > rows = ve_fontcache_backend_test_row_sum_profile( pixels, 8, 8 );
	std::vector< uint32_t > cols = ve_fontcache_backend_test_column_sum_profile( pixels, 8, 8 );
	ve_fontcache_backend_test_center_of_mass center = ve_fontcache_backend_test_center_of_mass_grayscale( pixels, 8, 8 );

	EXPECT_TRUE( bbox.valid );
	EXPECT_EQ( 2, bbox.x );
	EXPECT_EQ( 1, bbox.y );
	EXPECT_EQ( 3, bbox.w );
	EXPECT_EQ( 4, bbox.h );
	EXPECT_EQ( 12, bbox.area );
	EXPECT_EQ( 60u, rows[ 1 ] );
	EXPECT_EQ( 60u, rows[ 4 ] );
	EXPECT_EQ( 80u, cols[ 2 ] );
	EXPECT_EQ( 80u, cols[ 4 ] );
	EXPECT_TRUE( std::fabs( center.x - 3.5 ) <= 0.01 );
	EXPECT_TRUE( std::fabs( center.y - 3.0 ) <= 0.01 );
}

UTEST( backend_diagnostics, components_and_border_leakage )
{
	std::vector< uint8_t > pixels = ve_fontcache_backend_test_make_image( 6, 6, 0 );
	ve_fontcache_backend_test_fill_rect( pixels, 6, 6, 1, 1, 2, 2, 255 );
	ve_fontcache_backend_test_fill_rect( pixels, 6, 6, 0, 5, 1, 1, 255 );

	EXPECT_EQ( 2, ve_fontcache_backend_test_connected_component_count( pixels, 6, 6, 1 ) );
	EXPECT_EQ( 1, ve_fontcache_backend_test_border_leakage_count( pixels, 6, 6, 1, 1 ) );
	EXPECT_EQ( 1, ve_fontcache_backend_test_outside_rect_count( pixels, 6, 6, { 1, 1, 2, 2 }, 1 ) );
}

UTEST( backend_diagnostics, diff_stats_and_peak_location )
{
	std::vector< uint8_t > expected = ve_fontcache_backend_test_make_image( 4, 4, 0 );
	std::vector< uint8_t > actual = expected;
	actual[ 1 ] = 20;
	actual[ 14 ] = 120;

	ve_fontcache_backend_test_diff_stats diff = ve_fontcache_backend_test_expected_vs_actual_diff( expected, actual, 4, 4 );

	EXPECT_EQ( 140ull, diff.total_abs_error );
	EXPECT_TRUE( std::fabs( diff.mean_abs_error - 8.75 ) <= 0.01 );
	EXPECT_EQ( 120, diff.max_abs_error );
	EXPECT_EQ( 2, diff.peak_x );
	EXPECT_EQ( 3, diff.peak_y );
	EXPECT_EQ( 2, diff.differing_pixels );
}

UTEST( backend_diagnostics, mirror_scores_detect_flips )
{
	std::vector< uint8_t > expected = ve_fontcache_backend_test_make_image( 6, 6, 0 );
	ve_fontcache_backend_test_fill_l_shape( expected, 6, 6, 1, 1, 4, 4, 2, 255 );

	std::vector< uint8_t > vertical = ve_fontcache_backend_test_flip_vertical( expected, 6, 6 );
	std::vector< uint8_t > horizontal = ve_fontcache_backend_test_flip_horizontal( expected, 6, 6 );

	ve_fontcache_backend_test_mirror_scores vertical_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected, vertical, 6, 6 );
	ve_fontcache_backend_test_mirror_scores horizontal_scores =
		ve_fontcache_backend_test_mirror_scores_for_expected( expected, horizontal, 6, 6 );

	EXPECT_TRUE( ve_fontcache_backend_test_is_vertical_flip( vertical_scores ) );
	EXPECT_FALSE( ve_fontcache_backend_test_is_horizontal_flip( vertical_scores ) );
	EXPECT_TRUE( ve_fontcache_backend_test_is_horizontal_flip( horizontal_scores ) );
	EXPECT_FALSE( ve_fontcache_backend_test_is_vertical_flip( horizontal_scores ) );
}

UTEST( backend_diagnostics, classifier_helpers_flag_scale_aspect_and_source_binding )
{
	ve_fontcache_backend_test_bbox expected_bbox { true, 10, 20, 40, 40, 1600 };
	ve_fontcache_backend_test_bbox uniform_scaled { true, 12, 22, 48, 48, 2304 };
	ve_fontcache_backend_test_bbox aspect_scaled { true, 12, 22, 56, 40, 2240 };

	EXPECT_TRUE( ve_fontcache_backend_test_is_uniform_scale_error( expected_bbox, uniform_scaled ) );
	EXPECT_FALSE( ve_fontcache_backend_test_is_aspect_ratio_deformation( expected_bbox, uniform_scaled ) );
	EXPECT_TRUE( ve_fontcache_backend_test_is_aspect_ratio_deformation( expected_bbox, aspect_scaled ) );

	std::vector< uint8_t > pattern = ve_fontcache_backend_test_make_image( 4, 4, 0 );
	ve_fontcache_backend_test_fill_rect( pattern, 4, 4, 0, 0, 2, 4, 255 );
	std::vector< uint8_t > zero = ve_fontcache_backend_test_make_image( 4, 4, 0 );

	ve_fontcache_backend_test_diff_stats expected_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( pattern, zero, 4, 4 );
	ve_fontcache_backend_test_diff_stats alternate_diff =
		ve_fontcache_backend_test_expected_vs_actual_diff( zero, zero, 4, 4 );

	EXPECT_TRUE( ve_fontcache_backend_test_is_wrong_source_texture_bound( expected_diff, alternate_diff ) );
}
