#include "MathLib/Interpolator.h"

// adapted from jQueryUI source and http://www.robertpenner.com/easing/

float EasingFunction::GetCoefficient(Type type, float p)
{
    switch (type)
    {
    case Linear:
        return p;

    case Quadratic:
        return Math::Pow(p, 2.0f);

    case Cubic:
        return Math::Pow(p, 3.0f);

    case Quart:
        return Math::Pow(p, 4.0f);

    case Quint:
        return Math::Pow(p, 5.0f);

    case Expo:
        return Math::Pow(p, 6.0f);

    case EaseInSine:
        return 1.0f - Math::Cos(p * Y_HALF_PI);

    case EaseInCirc:
        return 1.0f - Math::Sqrt(1.0f - (p * p));

    case EaseInElastic:
        return (p == 0.0f || p == 1.0f) ? p : (-Math::Pow(2.0f, 8.0f * (p - 1.0f)) * Math::Sin(((p - 1.0f) * Math::DegreesToRadians(80.0f) - 7.5f) * (Y_PI / 15.0f)));

    case EaseInBack:
        return (p * p * (3.0f * p - 2.0f));

    case EaseInBounce:
        {
            float pow2;
            float bounce = 4.0f;
            while (p < ((pow2 = Math::Pow(2.0f, -bounce)) - 1.0f) / 11.0f);
            return 1.0f / Math::Pow(4.0f, 3.0f - bounce) - 7.5625f * Math::Pow((pow2 * 3.0f - 2.0f) / 22.0f - p, 2.0f);
        }

    case EaseOutSine:
    case EaseOutCirc:
    case EaseOutElastic:
    case EaseOutBack:
    case EaseOutBounce:
        return 1.0f - GetCoefficient(static_cast<Type>((int)type - 1), p);

    case EaseInOutSine:
    case EaseInOutCirc:
    case EaseInOutElastic:
    case EaseInOutBack:
    case EaseInOutBounce:
        return (p < 0.5f) ?
            (GetCoefficient(static_cast<Type>((int)type - 2), p * 2.0f) / 2.0f) :
            (1.0f - GetCoefficient(static_cast<Type>((int)type - 2), p * -2.0f + 2.0f) / 2.0f);
    }

    UnreachableCode();
    return p;
}

