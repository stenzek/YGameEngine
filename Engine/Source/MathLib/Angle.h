#pragma once
#include "MathLib/Common.h"

class Angle
{
public:
    // Constructs an angle from a degree value
    Angle(float degrees);

    // Copies another angle
    Angle(const Angle &angle);

    // Converts the angle to a degrees value.
    float Degrees() const;

    // Converts the angle to a radians value.
    float Radians() const;

    // Setters
    void SetDegrees(float degrees);
    void SetRadians(float radians);

    // Normalize the angle between [0, 360 or PI*2]
    void Normalize();
    Angle Normalized() const;

    // Constructs a new angle from a radian value.
    static Angle Radians(float radians);

    // Constructs a new angle from a degrees value.
    static Angle Degrees(float degrees);

    // Overloaded operators
    bool operator==(const Angle &other) const;
    bool operator!=(const Angle &other) const;
    bool operator>=(const Angle &other) const;
    bool operator<=(const Angle &other) const;
    bool operator<(const Angle &other) const;
    bool operator>(const Angle &other) const;

    // Modification operators
    Angle &operator=(const Angle &other);
    Angle &operator+=(const Angle &other);
    Angle &operator-=(const Angle &other);

    // Builder operators
    Angle operator+(const Angle &other) const;
    Angle operator-(const Angle &other) const;

private:
    // Empty constructor, sets uninitialized.
    Angle() {}

    // stored internally as radians for fast access
    float m_radians;
};

