#pragma once
#include "sprite.h"
#include "list.h"

class FRAME : public SPRITE
{
public:
    FRAME(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent = 0);
    virtual ~FRAME();
    virtual int Action(int command, int argument1, int argument2, int argument3);
};

class STEXT : public FRAME
{
    friend class zShopItemInfo;
public:
    STEXT(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent = 0);
    virtual ~STEXT();
    virtual int Action(int command, int argument1, int argument2, int argument3);
    virtual void Draw();
private:
    void CalcTextProperty();
    void ResolveMenuRegistryText(STRING* key);
    void SetAndDecodeSource(const STRING* source);
    int AdvanceCursor();
    void DrawFallback();
    void DrawGlyphs(VID* font, int verticalOffset);

    STRING m_renderedText;
    STRING m_sourceText;
    unsigned int m_cursor;
    int m_textWidth;
    int m_textHeight;
    unsigned int m_firstLine;
    int m_lineCount;
    LIST<int> m_lineOffsets;
};

