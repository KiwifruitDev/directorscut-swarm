#ifndef TUTORIAL_H
#define TUTORIAL_H

#include "KeyValues.h"
#include "filesystem.h"

class TutorialSection
{
public:
    TutorialSection( const char *pszSectionName, const char *pszSectionText)
    {
        m_pszSectionName = pszSectionName;
        m_pszSectionText = pszSectionText;
    }
    const char* GetName()
    {
        return m_pszSectionName;
    }
    const char* GetInnerText()
    {
        return m_pszSectionText;
    }
protected:
    const char *m_pszSectionName;
    const char *m_pszSectionText;
};

#endif