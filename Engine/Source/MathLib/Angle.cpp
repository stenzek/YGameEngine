#include "MathLib/Angle.h"

Angle::Angle(float degrees)
    : m_radians(Math::DegreesToRadians(degrees))
{

}

Angle::Angle(const Angle &angle)
    : m_radians(angle.m_radians)
{
    
}

float Angle::Degrees() const
{
    return Math::RadiansToDegrees(m_radians);
}

float Angle::Radians() const
{
    return m_radians;
}

Angle Angle::Degrees(float degrees)
{
    Angle ret;
    ret.m_radians = Math::DegreesToRadians(degrees);
    return ret;
}

Angle Angle::Radians(float radians)
{
    Angle ret;
    ret.m_radians = radians;
    return ret;
}

void Angle::SetDegrees(float degrees)
{
    m_radians = Math::DegreesToRadians(degrees);
}

void Angle::SetRadians(float radians)
{
    m_radians = radians;
}

void Angle::Normalize()
{
    m_radians = Math::NormalizeAngle(m_radians);
}

Angle Angle::Normalized() const
{
    Angle ret;
    ret.m_radians = Math::NormalizeAngle(m_radians);
    return ret;
}

bool Angle::operator==(const Angle &other) const
{
    return Math::NearEqual(m_radians, other.m_radians, Y_FLT_EPSILON);
}

bool Angle::operator!=(const Angle &other) const
{
    return !Math::NearEqual(m_radians, other.m_radians, Y_FLT_EPSILON);
}

bool Angle::operator>=(const Angle &other) const
{
    return m_radians >= other.m_radians;
}

bool Angle::operator<=(const Angle &other) const
{
    return m_radians <= other.m_radians;
}

bool Angle::operator<(const Angle &other) const
{
    return m_radians < other.m_radians;
}

bool Angle::operator>(const Angle &other) const
{
    return m_radians > other.m_radians;
}

Angle &Angle::operator=(const Angle &other)
{
    m_radians = other.m_radians;
    return *this;
}

Angle &Angle::operator+=(const Angle &other)
{
    m_radians += other.m_radians;
    return *this;
}

Angle &Angle::operator-=(const Angle &other)
{
    m_radians -= other.m_radians;
    return *this;
}

Angle Angle::operator+(const Angle &other) const
{
    Angle ret;
    ret.m_radians = m_radians + other.m_radians;
    return ret;
}

Angle Angle::operator-(const Angle &other) const
{
    Angle ret;
    ret.m_radians = m_radians - other.m_radians;
    return ret;
}

