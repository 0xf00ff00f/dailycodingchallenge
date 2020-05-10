#include "tween.h"

namespace {

template<float (*Tween)(float)>
float out(float t)
{
    return 1.f - Tween(1.f - t);
}

template<float (*Tween)(float)>
float in_out(float t)
{
    if (t < .5f)
        return .5f * Tween(2.f * t);
    else
        return .5f + .5f * out<Tween>(2.f * t - 1);
}

} // namespace

float linear(float t)
{
    return t;
}

float in_quadratic(float t)
{
    return t * t;
}

float out_quadratic(float t)
{
    return out<in_quadratic>(t);
}

float in_out_quadratic(float t)
{
    return in_out<in_quadratic>(t);
}

// stolen from robert penner

namespace {
constexpr const auto back_s = 1.70158f;
};

float in_back(float t)
{
    return t * t * ((back_s + 1.f) * t - back_s);
}

float out_back(float t)
{
    return out<in_back>(t);
}

float in_out_back(float t)
{
    return in_out<in_back>(t);
}

float out_bounce(float t)
{
    if (t < 1. / 2.75) {
        return 7.5625 * t * t;
    } else if (t < 2. / 2.75) {
        t -= 1.5 / 2.75;
        return 7.5625 * t * t + .75;
    } else if (t < 2.5 / 2.75) {
        t -= 2.25 / 2.75;
        return 7.5625 * t * t + .9375;
    } else {
        t -= 2.625 / 2.75;
        return 7.5625 * t * t + .984375;
    }
}

float in_bounce(float t)
{
    return out<out_bounce>(t);
}

float in_out_bounce(float t)
{
    return in_out<in_bounce>(t);
}
