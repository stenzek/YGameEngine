#pragma once
#include <QtCore/QObject>

template<class T>
class SignalBlocker
{
public:
    SignalBlocker(T *pBlocked)
        : m_pBlocked(pBlocked),
          m_prevState(pBlocked->blockSignals(true))
    {

    }

    ~SignalBlocker()
    {
        m_pBlocked->blockSignals(m_prevState);
    }

    T *operator->()
    {
        return m_pBlocked;
    }

private:
    T *m_pBlocked;
    bool m_prevState;
};

template<class T>
static inline SignalBlocker<T> BlockSignalsForCall(T *pBlocked)
{
    return SignalBlocker<T>(pBlocked);
}
