#include <iostream>
#include <cmath>
#include <algorithm>
#include <cassert>


//Code generate by AI

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function for single half-day sine integration with thresholds
double calc_half_day_sine(double t_start, double t_end, double t_lower, double t_upper)
{
    // If entire half-day is below lower threshold or above upper (handled contextually)
    if (t_start <= t_lower && t_end <= t_lower) return 0.0;
    if (t_start >= t_upper && t_end >= t_upper) {
        // Entirely clipped at upper threshold
        return (t_upper - t_lower);
    }

    double mean = (t_start + t_end) / 2.0;
    double ampl = std::abs(t_start - t_end) / 2.0;

    if (ampl < 1e-6) {
        // Flat temperature case
        double effective = std::clamp(mean, t_lower, t_upper);
        return std::max(0.0, effective - t_lower);
    }

    // Standard numerical integration approximation or analytical arcsine method
    // Integrating sine wave over the 12-hour block:
    int steps = 12;// hourly step 120;
    double dt = 1.0 / steps;
    double sum = 0.0;

    for (int i = 0; i < steps; ++i) 
    {
        double t = i * dt;
        // Sinusoidal interpolation between t_start and t_end
        double temp = mean + ampl * std::sin(M_PI * (t - 0.5));
        double clamped_temp = std::clamp(temp, t_lower, t_upper);
        sum += (clamped_temp - t_lower);
    }

    assert(sum >= 0);
    assert(!_isnan(sum) && _finite(sum));
    if (_isnan(sum) || !_finite(sum))
        sum = 0;


    return (sum * dt / 2.0); // scaled for half-day contribution
}

// Double Sine Calculation for a full day
// Uses Tmax_prev, Tmin_today, Tmax_today, and Tmin_next (tomorrow's min)
double double_sine_degree_days(double tmin_today, double tmax_today,  double tmin_next, double t_lower, double t_upper) 
{
    // First half-day: from previous min (approximated or prev night) to today's max
    // Standard double sine models 1st half: midnight/min to max, 2nd half: max to next min
    double half1 = calc_half_day_sine(tmin_today, tmax_today, t_lower, t_upper);
    double half2 = calc_half_day_sine(tmax_today, tmin_next, t_lower, t_upper);

    return -(half1 + half2);
}
