#pragma once
#include "MathLib/Common.h"
#include "MathLib/Quaternion.h"
#include "YBaseLib/Assert.h"

class EasingFunction
{
public:
    enum Type
    {
        Linear,
        Quadratic,
        Cubic,
        Quart,
        Quint,
        Expo,
        EaseInSine,
        EaseOutSine,
        EaseInOutSine,
        EaseInCirc,
        EaseOutCirc,
        EaseInOutCirc,
        EaseInElastic,
        EaseOutElastic,
        EaseInOutElastic,
        EaseInBack,
        EaseOutBack,
        EaseInOutBack,
        EaseInBounce,
        EaseOutBounce,
        EaseInOutBounce,
        TypeCount,
    };

    static float GetCoefficient(Type type, float p);
};

template<typename T>
class Interpolator
{
public:
    static T CalculateValue(EasingFunction::Type function, const T &start, const T &end, float factor);

    enum State
    {
        State_Forward,
        State_ForwardPaused,
        State_Reverse,
        State_ReversePaused,
        State_Complete,
        State_Count,
    };

public:
    Interpolator()
        : m_moveDuration(1.0f), 
          m_pauseDuration(0.0f), 
          m_reverseCycle(false), 
          m_repeatCount(0), 
          m_function(EasingFunction::Linear),
          m_timeRemaining(0.0f),
          m_cyclesRemaining(0),
          m_state(State_Complete)
    {
    }

    Interpolator(const T &startValue, 
                 const T &endValue, 
                 float moveDuration = 1.0f, 
                 float pauseDuration = 0.0f, 
                 bool reverseCycle = true, 
                 uint32 repeatCount = 0, 
                 EasingFunction::Type easingFunction = EasingFunction::Linear)
        : m_startValue(startValue),
          m_endValue(endValue),
          m_moveDuration(moveDuration),
          m_pauseDuration(pauseDuration),
          m_reverseCycle(reverseCycle),
          m_repeatCount(repeatCount),
          m_function(easingFunction),
          m_timeRemaining(0.0f),
          m_cyclesRemaining(repeatCount),
          m_state(State_Complete),
          m_currentValue(endValue)

    {
    }

    // start/end values
    const T &GetStartValue() const { return m_startValue; }
    void SetStartValue(const T &value) { m_startValue = value; }
    const T &GetEndValue() const { return m_endValue; }
    void SetEndValue(const T &value) { m_endValue = value; }
    void SetValues(const T &start, const T &end) { m_startValue = start; m_endValue = end; m_currentValue = start; }

    // durations -- in seconds
    float GetMoveDuration() const { return m_moveDuration; }
    void SetMoveDuration(float duration) { DebugAssert(duration > 0.0f); m_moveDuration = duration; }
    float GetPauseDuration() const { return m_pauseDuration; }
    void SetPauseDuration(float duration) { DebugAssert(duration >= 0.0f); m_pauseDuration = duration; }

    // enable/disable repeat cycle
    bool GetReverseCycle() const { return m_reverseCycle; }
    void SetReverseCycle(bool reverseCycle) { m_reverseCycle = reverseCycle; }

    // repeat count, 0 = infinite
    uint32 GetRepeatCount() const { return m_repeatCount; }
    void SetRepeatCount(uint32 repeatCount) { m_repeatCount = repeatCount; }

    // function
    const EasingFunction::Type GetFunction() const { return m_function; }
    void SetFunction(EasingFunction::Type function) { m_function = function; }
    
    // reset/reverse
    void Reset() { m_timeRemaining = m_moveDuration; m_cyclesRemaining = m_repeatCount; m_state = State_Forward; m_currentValue = m_startValue; }
    void WarpToEnd() { m_timeRemaining = 0.0f; m_cyclesRemaining = 0; m_state = State_Complete; m_currentValue = m_endValue; }
    void Reverse() { Swap(m_startValue, m_endValue); Reset(); }

    // value reader
    float GetTimeRemaining() const { return m_timeRemaining; }
    uint32 GetCyclesRemaining() const { return m_cyclesRemaining; }
    bool IsActive() const { return (m_state != State_Complete); }
    const T &GetCurrentValue() const { return m_currentValue; }

    // value updater
    void Update(float deltaTime)
    {
        if (m_state == State_Complete)
            return;

        // handle cases (or lag) where deltaTime > timeRemaining
        while (deltaTime > 0.0f)
        {
            float thisDelta = Min(deltaTime, m_timeRemaining);
            deltaTime -= thisDelta;

            if (thisDelta >= m_timeRemaining)
            {
                // cycle complete
                if (m_state == State_Forward)
                {
                    // set to end
                    m_currentValue = m_endValue;

                    // enter pause phase or reverse stage
                    if (m_pauseDuration > 0.0f)
                    {
                        m_state = State_ForwardPaused;
                        m_timeRemaining = m_pauseDuration;
                    }
                    else if (m_reverseCycle)
                    {
                        m_state = State_Reverse;
                        m_timeRemaining = m_moveDuration;
                    }
                    else
                    {
                        m_state = State_Complete;
                    }
                }
                else if (m_state == State_ForwardPaused)
                {
                    // enter either the reverse or complete state
                    if (m_reverseCycle)
                    {
                        m_state = State_Reverse;
                        m_timeRemaining = m_moveDuration;
                    }
                    else
                    {
                        m_state = State_Complete;
                    }
                }
                else if (m_state == State_Reverse)
                {
                    // set to start
                    m_currentValue = m_startValue;

                    // completed a reverse cycle, enter pause or complete
                    if (m_pauseDuration > 0.0f)
                    {
                        m_state = State_ReversePaused;
                        m_timeRemaining = m_pauseDuration;
                    }
                    else
                    {
                        m_state = State_Complete;
                    }
                }
                else if (m_state == State_ReversePaused)
                {
                    // cycle completed
                    m_state = State_Complete;
                }

                // was a cycle just completed?
                if (m_state == State_Complete)
                {
                    // repeat infinitely or a number of times
                    if (m_repeatCount == 0 || (m_cyclesRemaining > 0 && (--m_cyclesRemaining) > 0))
                    {
                        // enter forward state again
                        m_state = State_Forward;
                        m_timeRemaining = m_moveDuration;
                    }
                }
            }
            else
            {
                // not a complete cycle
                m_timeRemaining -= thisDelta;

                // update the value
                if (m_state == State_Forward)
                    m_currentValue = CalculateValue(m_function, m_startValue, m_endValue, 1.0f - (m_timeRemaining / m_moveDuration));
                else if (m_state == State_Reverse)
                    m_currentValue = CalculateValue(m_function, m_endValue, m_startValue, 1.0f - (m_timeRemaining / m_moveDuration));
            }
        }
    }

private:
    // start/end
    T m_startValue;
    T m_endValue;

    // parameters
    float m_moveDuration;
    float m_pauseDuration;
    bool m_reverseCycle;
    uint32 m_repeatCount;
    EasingFunction::Type m_function;

    // state
    float m_timeRemaining;
    uint32 m_cyclesRemaining;
    State m_state;
    T m_currentValue;
};

// default value calculator, uses overloaded operators
template<typename T>
inline T Interpolator<T>::CalculateValue(EasingFunction::Type function, const T &start, const T &end, float factor)
{
    float coefficient = EasingFunction::GetCoefficient(function, factor);
    return start + ((end - start) * coefficient);
}

// overloaded interpolator for quaternions :: @TODO MOVE TO .cpp please
template<>
inline Quaternion Interpolator<Quaternion>::CalculateValue(EasingFunction::Type function, const Quaternion &start, const Quaternion &end, float factor)
{
    float coefficient = EasingFunction::GetCoefficient(function, factor);
    return Quaternion::LinearInterpolate(start, end, coefficient);
}
