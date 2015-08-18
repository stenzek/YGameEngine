#pragma once
#include "ContentConverter/Common.h"
#include "YBaseLib/ProgressCallbacks.h"

class BaseImporter
{
public:
    BaseImporter(ProgressCallbacks *pProgressCallbacks);
    virtual ~BaseImporter();

    ProgressCallbacks *GetProgressCallbacks() const { return m_pProgressCallbacks; }
    void SetProgressCallbacks(ProgressCallbacks *pProgressCallbacks) { m_pProgressCallbacks = pProgressCallbacks; }

protected:
    ProgressCallbacks *m_pProgressCallbacks;
};