#include <iostream>
#include <cmath>
//#include <vector>
#include <algorithm>


//Code generate by AI



#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Calculates negative sine degree-days for a single half-day period.
 * Under this interpretation, it counts "degree days of cold accumulation" below the thresholds.
 *
 * @param L Lower temperature threshold
 * @param U Upper temperature threshold
 * @param tmin Minimum temperature of the half-day
 * @param tmax Maximum temperature of the half-day
 * @return Double representing the negative degree-day accumulation (divided by 2 for a half day)
 */

double calc_half_day_negative_sine(double L, double U, double tmin, double tmax)
{
	// Sanity check: if data is malformed
	if (tmin > tmax) std::swap(tmin, tmax);

	// Case 1: Entirely above the upper threshold. No negative degree days accumulated.
	if (tmin >= U) {
		return 0.0;
	}

	// Case 2: Entirely below the lower threshold. Max possible cold accumulation.
	if (tmax <= L) {
		return (L - (tmin + tmax) / 2.0) / 2.0;
	}

	double alpha = (tmax + tmin) / 2.0;
	double R = (tmax - tmin) / 2.0; // Amplitude

	// Case 3: Completely between L and U
	if (tmin >= L && tmax <= U) {
		return (L - alpha) / 2.0;
	}

	// Intersections for sine-wave integration
	// theta is the angle where the sine wave crosses a threshold
	double theta_L = (tmin < L && tmax > L) ? std::asin((L - alpha) / R) : 0.0;
	double theta_U = (tmin < U && tmax > U) ? std::asin((U - alpha) / R) : 0.0;

	// Case 4: Intersects L but stays below U
	if (tmin < L && tmax <= U) {
		double area_above_L = R * std::cos(theta_L) - (L - alpha) * (M_PI / 2.0 - theta_L);
		double total_area_above_tmin = (alpha - tmin) * M_PI; // simple approximation / baseline
		// Alternatively, standard geometric integration under the curve:
		double mean_temp_below_L = ((alpha - L) * (theta_L + M_PI / 2.0) - R * std::cos(theta_L)) / M_PI;
		return (L - mean_temp_below_L) / 2.0;
	}

	// Case 5: Starts above L but intersects U
	if (tmin >= L && tmax > U) {
		// Here we track deficit from L, capped at U
		double mean_temp = ((alpha - L) * (theta_U + M_PI / 2.0) + R * std::cos(theta_U) + (U - L) * (M_PI / 2.0 - theta_U)) / M_PI;
		return (L - mean_temp) / 2.0;
	}

	// Case 6: Spans across both thresholds (tmin < L and tmax > U)
	if (tmin < L && tmax > U) {
		double part1 = (alpha - L) * (theta_U - theta_L);
		double part2 = R * (std::cos(theta_L) - std::cos(theta_U));
		double part3 = (U - L) * (M_PI / 2.0 - theta_U);
		double mean_temp = (part1 + part2 + part3) / M_PI;
		return (L - mean_temp) / 2.0;
	}

	return 0.0;
}

/**
 * Double Sine Method combining two half-days.
 *
 * @param L Lower threshold
 * @param U Upper threshold
 * @param tmin_today Minimum temperature today
 * @param tmax_today Maximum temperature today
 * @param tmin_tomorrow Minimum temperature tomorrow
 * @return Total negative double-sine degree days for the 24 hour block
 */
double calc_double_sine_negative_dd(double L, double U, double tmin_today, double tmax_today, double tmin_tomorrow)
{
	double first_half = calc_half_day_negative_sine(L, U, tmin_today, tmax_today);
	double second_half = calc_half_day_negative_sine(L, U, tmin_tomorrow, tmax_today);
	return -(first_half + second_half);
}

int test()
{
	// Example parameters
	double lower_threshold = 45.0;
	double upper_threshold = 65.0;

	double tmin_today = 38.0;
	double tmax_today = 55.0;
	double tmin_tomorrow = 40.0;

	double neg_dd = calc_double_sine_negative_dd(lower_threshold, upper_threshold, tmin_today, tmax_today, tmin_tomorrow);

	std::cout << "Negative Double-Sine Degree-Days: " << neg_dd << std::endl;

	return 0;
}



// Define a structural blueprint for the daily temperature input
//struct DailyTemp {
//    double t_min;
//    double t_max;
//};
//
//// Calculates negative degree days for a single half-day period
//double calculateHalfDayNegativeDD(double t_min, double t_max, double t_base) 
//{
//    // Safety check: ensure max is not less than min due to malformed data
//    if (t_min > t_max) {
//        std::swap(t_min, t_max);
//    }
//
//    // Case 1: Entire half-day is below or equal to the threshold
//    if (t_max <= t_base) {
//        return t_base - (t_max + t_min) / 2.0;
//    }
//
//    // Case 2: Entire half-day is above the threshold (no negative degree days)
//    if (t_min >= t_base) {
//        return 0.0;
//    }
//
//    // Case 3: The threshold intersects the half-sine wave profile
//    double mean_t = (t_max + t_min) / 2.0;
//    double amplitude = (t_max - t_min) / 2.0;
//
//    // Compute the sine wave intersection angle theta
//    double sin_theta = (t_base - mean_t) / amplitude;
//
//    // Boundary check for precision safety before arc-sin
//    sin_theta = std::max(-1.0, std::min(1.0, sin_theta));
//    double theta = std::asin(sin_theta);
//
//    constexpr double PI = 3.14159265358979323846;
//
//    // Intercept area integration representing time below the base threshold
//    double area = ((t_base - mean_t) * (PI / 2.0 - theta) + amplitude * std::cos(theta)) / (2.0 * PI);
//
//    return area;
//}
//
//// Accumulates total degree days across a time series using the Double Sine approach
//double accumulateDoubleSineNegativeDD(const std::vector<DailyTemp>& weather_data, double t_base) 
//{
//    if (weather_data.empty()) return 0.0;
//
//    double total_dd = 0.0;
//
//    // Iterate through data; requires day 'i' and next day's minimum 'i+1'
//    for (size_t i = 0; i < weather_data.size() - 1; ++i) 
//    {
//        double t_min1 = weather_data[i].t_min;
//        double t_max = weather_data[i].t_max;
//        double t_min2 = weather_data[i + 1].t_min; // Looks ahead to model afternoon cooling
//
//        // 1. First half-day: from today's min to today's max
//        double dd_half1 = calculateHalfDayNegativeDD(t_min1, t_max, t_base);
//
//        // 2. Second half-day: from today's max to tomorrow's min
//        double dd_half2 = calculateHalfDayNegativeDD(t_min2, t_max, t_base);
//
//        // Total daily contribution is the sum of both asymmetric half-days
//        total_dd += (dd_half1 + dd_half2);
//    }
//
//    return total_dd;
//}
//
//int test() 
//{
//    // Example Threshold (e.g., 0°C freezing point threshold)
//    double freezing_threshold = 0.0;
//
//    // Mock weather dataset containing sequentially recorded [Min, Max] pairs
//    std::vector<DailyTemp> dataset = {
//        {-5.0,  3.0}, // Day 1
//        {-8.0, -1.0}, // Day 2
//        {-3.0,  5.0}, // Day 3
//        {-6.0,  2.0}  // Day 4 (Provides t_min2 for Day 3 execution loop)
//    };
//
//    double accumulated_ndd = accumulateDoubleSineNegativeDD(dataset, freezing_threshold);
//
//    std::cout << "Accumulated Negative Degree Days: " << accumulated_ndd << " DD" << std::endl;
//
//    return 0;
//}




//#include <iostream>
//#include <vector>
//#include <cmath>
//#include <algorithm>
//
//// Define Mathematical Constants
//#ifndef M_PI
//#define M_PI 3.14159265358979323846
//#endif
//
///**
// * Helper function to calculate the area under a sine wave segment (Baskerville-Emin / Allen formula).
// * This computes heat accumulation above a specific threshold value.
// */
//double calculate_sine_area(double t_max, double t_min, double threshold) 
//{
//	double sum = t_max + t_min;
//	double diff = t_max - t_min;
//
//	// If the entire sine wave is above the threshold
//	if (t_min >= threshold) {
//		return (sum / 2.0) - threshold;
//	}
//
//	// If the wave intercepts the threshold, compute using the arc-sine area formula
//	double alpha = std::asin((2.0 * threshold - sum) / diff);
//	double area = (diff * std::cos(alpha) - (2.0 * threshold - sum) * (M_PI / 2.0 - alpha)) / (2.0 * M_PI);
//
//	return area;
//}
//
///**
// * Calculates a single half-day's growing degree days.
// * Handles both lower and upper thresholds using a horizontal cutoff method.
// */
//double calculate_half_day_gdd(double t_max, double t_min, double t_lower, double t_upper) 
//{
//	// 1. Extreme Case: Completely below lower threshold or invalid data
//	if (t_max <= t_lower || t_max < t_min) {
//		return 0.0;
//	}
//
//	// 2. Extreme Case: Completely above upper threshold
//	if (t_min >= t_upper) {
//		return (t_upper - t_lower) / 2.0; // Scaled for a half-day
//	}
//
//	// Calculate heat accumulation above the lower threshold
//	double gdd = calculate_sine_area(t_max, t_min, t_lower);
//
//	// Adjust for the horizontal cutoff if max temperature exceeds the upper threshold
//	if (t_max > t_upper) {
//		double excess = calculate_sine_area(t_max, t_min, t_upper);
//		gdd -= excess;
//	}
//
//	// Since this is evaluated per half-day period, divide the full-day equivalence by 2
//	return gdd / 2.0;
//}
//
///**
// * Accumulates historical degree days over a series of daily records.
// * Note: Requires the minimum temperature of the following day for each step.
// *
// * @param t_max_vec Vector of daily maximum temperatures.
// * @param t_min_vec Vector of daily minimum temperatures (must include Day N+1 at the end).
// * @param t_lower   Lower developmental threshold.
// * @param t_upper   Upper developmental threshold.
// * @return Total accumulated degree days over the period.
// */
//double accumulate_double_sine_gdd(const std::vector<double>& t_max_vec,
//	const std::vector<double>& t_min_vec,
//	double t_lower,
//	double t_upper) {
//	if (t_max_vec.empty() || t_min_vec.size() <= t_max_vec.size()) {
//		std::cerr << "Error: t_min_vec must contain at least one more element than t_max_vec to account for the next day's minimum.\n";
//		return 0.0;
//	}
//
//	double total_accumulated_gdd = 0.0;
//
//	for (size_t i = 0; i < t_max_vec.size(); ++i) 
//	{
//		double max_today = t_max_vec[i];
//		double min_today = t_min_vec[i];
//		double min_tomorrow = t_min_vec[i + 1]; // Crucial for the double-sine method
//
//		// Phase 1: Warming half-day (Today's Min -> Today's Max)
//		double first_half = calculate_half_day_gdd(max_today, min_today, t_lower, t_upper);
//
//		// Phase 2: Cooling half-day (Today's Max -> Tomorrow's Min)
//		double second_half = calculate_half_day_gdd(max_today, min_tomorrow, t_lower, t_upper);
//
//		double daily_gdd = first_half + second_half;
//		total_accumulated_gdd += daily_gdd;
//
//		std::cout << "Day " << (i + 1) << ": Max=" << max_today
//			<< ", Min=" << min_today << ", Next Min=" << min_tomorrow
//			<< " | GDD Accumulation = " << daily_gdd << "\n";
//	}
//
//	return total_accumulated_gdd;
//}
//
//int test() 
//{
//	// Example thresholds (e.g., standard Fahrenheit limits for certain agricultural pests)
//	double lower_threshold = 50.0;
//	double upper_threshold = 86.0;
//
//	// Vector of daily maximums (5-day window)
//	std::vector<double> max_temps = { 75.0, 82.0, 90.0, 88.0, 68.0 };
//
//	// Vector of daily minimums (Must include Day 6 minimum at index 5)
//	std::vector<double> min_temps = { 45.0, 48.0, 55.0, 60.0, 52.0, 47.0 };
//
//	std::cout << "--- Double Sine Degree Days Calculation ---\n";
//	double total_gdd = accumulate_double_sine_gdd(max_temps, min_temps, lower_threshold, upper_threshold);
//
//	std::cout << "-------------------------------------------\n";
//	std::cout << "Total Accumulated GDD: " << total_gdd << "\n";
//
//	return 0;
//}
