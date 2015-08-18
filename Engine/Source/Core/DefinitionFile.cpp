#include "Core/PrecompiledHeader.h"
#include "Core/DefinitionFile.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/Log.h"
Log_SetChannel(DefinitionFile);

DefinitionFileParser::DefinitionFileParser(ByteStream *pStream, const char *FileName) : m_Reader(pStream), m_szFileName(FileName)
{
    m_uCharacter = 0;
    m_uLine = 0;
    m_bAtEOF = false;
}

DefinitionFileParser::~DefinitionFileParser()
{

}

bool DefinitionFileParser::NextLine()
{
    for (;;)
    {
        m_uCharacter = 0;
        m_uLine++;

        if (!m_Reader.ReadLine(&m_szLine))
        {
            m_bAtEOF = true;
            return false;
        }
    
        // remove whitespace characters
        m_szLine.Strip(" \t\r\n");

        // check length
        if (m_szLine.GetLength() == 0)
            continue;

        // skip comment lines
        if (m_szLine[0] == '#')
            continue;

        // line is good
        return true;
    }
}

bool DefinitionFileParser::NextToken(bool AllowEOF /* = false */, const char *TokenSeperatorCharacters /* = " " */)
{
    uint32 i;

    if (m_bAtEOF)
    {
        if (!AllowEOF)
            PrintError("reached EOF while searching for token");

        return false;
    }

    if (m_uCharacter == m_szLine.GetLength())
    {
        // read another line
        if (!NextLine())
        {
            if (!AllowEOF)
                PrintError("reached EOF while searching for token");

            return false;
        }
    }

    uint32 nTokenSeperatorCharacters = Y_strlen(TokenSeperatorCharacters);
    DebugAssert(nTokenSeperatorCharacters > 0);

    // skip till the start of the token
    while (m_uCharacter < m_szLine.GetLength())
    {
        for (i = 0; i < nTokenSeperatorCharacters; i++)
        {
            if (m_szLine[m_uCharacter] == TokenSeperatorCharacters[i])
                break;
        }

        if (i == nTokenSeperatorCharacters)
            break;
        else
            m_uCharacter++;
    }

    // no token?
    if (m_uCharacter == m_szLine.GetLength())
        return NextToken(AllowEOF);

    // append characters
    char QuotedStringCharacter = 0;
    m_szToken.Clear();
    while (m_uCharacter < m_szLine.GetLength())
    {
        char chCharacter = m_szLine[m_uCharacter];
        if (chCharacter == QuotedStringCharacter)
        {
            QuotedStringCharacter = 0;
            m_uCharacter++;
            continue;
        }
        else if (chCharacter == '\"' || chCharacter == '\'')
        {
            QuotedStringCharacter = chCharacter;
            m_uCharacter++;
            continue;
        }
        else 
        {
            if (QuotedStringCharacter == 0)
            {
                for (i = 0; i < nTokenSeperatorCharacters; i++)
                {
                    if (chCharacter == TokenSeperatorCharacters[i])
                    {
                        // token complete
                        return true;
                    }
                }
            }

            m_szToken.AppendCharacter(chCharacter);
            m_uCharacter++;
        }
    }

    if (QuotedStringCharacter != 0)
    {
        PrintError("reached EOF while reading quoted string");
        return false;
    }

    // should be at least one
    //DebugAssert(m_szToken.GetLength() > 0);
    return true;
}

int32 DefinitionFileParser::InternalSelect(const char *SelectTokens)
{
    uint32 len = Y_strlen(SelectTokens);
    uint32 i = 0;
    uint32 start = 0;
    uint32 TokenIndex = 0;

    for (; i < len; i++)
    {
        if (SelectTokens[i] == '|')
        {
            if ((i - start) == m_szToken.GetLength() && Y_strnicmp(SelectTokens + start, m_szToken.GetCharArray(), i - start) == 0)
                return TokenIndex;

            start = i + 1;
            TokenIndex++;
        }
    }

    if (start < len)
    {
        if ((len - start) == m_szToken.GetLength() &&Y_strnicmp(SelectTokens + start, m_szToken.GetCharArray(), len - start) == 0)
            return TokenIndex;
    }

    return -1;
}

bool DefinitionFileParser::Expect(const char *ExpectTokens)
{
    if (InternalSelect(ExpectTokens) < 0)
    {
        PrintError("got '%s', expecting '%s'", m_szToken.GetCharArray(), ExpectTokens);
        return false;
    }

    return true;
}

bool DefinitionFileParser::ExpectNot(const char *NotExpectTokens)
{
    if (InternalSelect(NotExpectTokens) >= 0)
    {
        PrintError("unexpected '%s', expecting anything but '%s'", m_szToken.GetCharArray(), NotExpectTokens);
        return false;
    }

    return true;
}

int32 DefinitionFileParser::Select(const char *SelectTokens)
{
    int32 ReturnValue = InternalSelect(SelectTokens);
    if (ReturnValue < 0)
    {
        PrintError("got '%s', expecting '%s'", m_szToken.GetCharArray(), SelectTokens);
        return -1;
    }

    return ReturnValue;
}

void DefinitionFileParser::PrintError(const char *Format, ...)
{
    va_list ap;
    va_start(ap, Format);

    char buf[256];
    Y_vsnprintf(buf, countof(buf), Format, ap);
    va_end(ap);

    Log_ErrorPrintf("%s:%u:%u: %s", m_szFileName.GetCharArray(), m_uLine + 1, m_uCharacter + 1, buf);
}

void DefinitionFileParser::PrintWarning(const char *Format, ...)
{
    va_list ap;
    va_start(ap, Format);

    char buf[256];
    Y_vsnprintf(buf, countof(buf), Format, ap);
    va_end(ap);

    Log_WarningPrintf("%s:%u:%u: %s", m_szFileName.GetCharArray(), m_uLine + 1, m_uCharacter + 1, buf);
}
