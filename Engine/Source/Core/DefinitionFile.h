#pragma once
#include "Core/Common.h"
#include "YBaseLib/String.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/TextReader.h"

class DefinitionFileParser
{
public:
    DefinitionFileParser(ByteStream *pStream, const char *FileName);
    ~DefinitionFileParser();

    const String &GetToken() const { return m_szToken; }
    bool IsAtEOF() const { return m_bAtEOF; }

    bool NextToken(bool AllowEOF = false, const char *TokenSeperatorCharacters = " \t");
    bool Expect(const char *ExpectTokens);
    bool ExpectNot(const char *NotExpectTokens);
    int32 Select(const char *SelectTokens);
    void PrintError(const char *Format, ...);
    void PrintWarning(const char *Format, ...);

private:
    bool NextLine();
    int32 InternalSelect(const char *SelectTokens);

    TextReader m_Reader;
    String m_szFileName;
    String m_szLine;
    String m_szToken;
    uint32 m_uLine;
    uint32 m_uCharacter;
    bool m_bAtEOF;

    DeclareNonCopyable(DefinitionFileParser);
};

