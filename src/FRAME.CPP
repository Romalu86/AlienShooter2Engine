

#include "engine_globals.h"
#include <cstring>
#include "graph.h"
#include "map.h"
#include "region.h"
#include "mylib.h"
#include "script.h"
#include "frame.h"
#include "sprite_list.h"
#include "stream.h"
#include "vid.h"

namespace {

static VID* STextFontByIndex(int index)
{
    if (index >= 0) {
        VID* const font = Map->Vid(index);
        if (font)
            return font;
    }
    return ::EmptyVid;
}

static bool STextUnsignedFontSlotExists(unsigned int index)
{
    return Map->StoredVid(static_cast<int>(index)) != 0;
}
}

FRAME::FRAME(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : SPRITE(vid, (vid->spriteType & 0x20u) != 0u
        ? VECTOR(Map->ShiftXY().x + position.x, Map->ShiftXY().y + position.y, position.z)
        : position, direction, parent)
{
    if ((m_vid->spriteType & 0x20u) != 0u)
        Map->MenuState().Insert(this);
}

FRAME::~FRAME()
{
    if ((m_vid->spriteType & 0x20u) != 0u) {
        MENU& menu = Map->MenuState();
        if (menu.m_selectedSprite == this)
            menu.m_selectedSprite = 0;
        (void)menu.Delete(this);
    }
    Map->DeletePointerToSprite(this);
}


int FRAME::Action(int command, int argument1, int argument2, int argument3)
{
    if (command != static_cast<int>(ACT_NEXT_COMMAND))
        return SPRITE::Action(command, argument1, argument2, argument3);
    if (m_currentAnimation == 14)
        ChangeAnimation(0);
    return 0;
}

STRING MAP::ScriptVariable(const STRING& expression)
{
    return m_script.GetVariableStr(expression);
}

void STEXT::CalcTextProperty()
{
    static_cast<VID_FONT*>(m_vid)->CalcTextSize(
        m_renderedText, &m_textWidth, &m_textHeight, &m_lineCount, &m_lineOffsets, m_behave);

    const unsigned int length = static_cast<unsigned int>(std::strlen(m_renderedText.value));
    m_firstLine = 0u;
    m_cursor = length;
    --m_lineCount;
}

void STEXT::ResolveMenuRegistryText(STRING* key)
{
    STRING menuKey("menu");
    PROFILE* const registry = ::Strings;
    STRING resolved = registry->GetString(menuKey, *key, *key);
    m_renderedText = resolved;
}

void STEXT::SetAndDecodeSource(const STRING* source)
{
    if (source)
        m_sourceText = *source;
    else
        m_sourceText = STRING("");

    STRING* const storedSource = &m_sourceText;
    STRING* const rendered = &m_renderedText;

    const unsigned int textMode = m_behave & 0x70u;
    if (textMode == 0x10u) {
        ResolveMenuRegistryText(storedSource);
        CalcTextProperty();
        return;
    }
    if (textMode == 0x20u) {
        (void)rendered->LoadFile(*storedSource);
        CalcTextProperty();
        return;
    }
    if (textMode == 0u) {
        *rendered = *storedSource;
        CalcTextProperty();
        return;
    }

    {
        STRING converted = Map->ScriptVariable(*storedSource);
        *rendered = converted;
    }
    const unsigned int resolvedMode = m_behave & 0x70u;
    if (resolvedMode == 0x40u)
        (void)rendered->LoadFile(*rendered);
    else if (resolvedMode == 0x50u)
        ResolveMenuRegistryText(rendered);
    CalcTextProperty();
}

int STEXT::AdvanceCursor()
{
    const char* text = m_renderedText.value;
    if (text[0] == '\0' || m_stateTimer != 0u)
        return 0u;

    const int length = static_cast<int>(static_cast<short>(static_cast<unsigned int>(std::strlen(text)) & 0xFFFFu));
    unsigned int cursor = m_cursor;
    if (static_cast<int>(cursor) >= length) {
        if (m_currentAnimation != 0)
            ChangeAnimation(0);
        return 0u;
    }

    m_cursor = cursor + 1u;
    const int firstCursorChar = static_cast<int>(static_cast<signed char>(text[cursor]));
    if (CrtIsSpace(static_cast<unsigned int>(firstCursorChar)) == 0u) {
        ChangeAnimation(1);
        m_runtimeFlags &= ~0x8000u;
        return 0u;
    }

    bool crossedNewline = false;
    cursor = m_cursor;
    while (static_cast<int>(cursor)<length) {
        if (text[cursor] == '\n')
            crossedNewline = true;
        m_cursor = cursor + 1u;
        const int cursorChar = static_cast<int>(static_cast<signed char>(text[cursor]));
        if (CrtIsSpace(static_cast<unsigned int>(cursorChar)) == 0u)
            break;
        cursor = m_cursor;
    }

    if (crossedNewline) {
        cursor = m_cursor;
        if (static_cast<int>(cursor)<length)
            m_cursor = cursor - 1u;
        m_stateTimer = 150u;
        ChangeAnimation(2);
    } else {
        ChangeAnimation(1);
        m_runtimeFlags &= ~0x8000u;
    }
    return 0u;
}

int STEXT::Action(int command, int argument1, int argument2, int argument3)
{
    switch (static_cast<unsigned int>(command)) {
    case ACT_SAVE:
    {
        (void)FRAME::Action(command, argument1, argument2, argument3);
        STREAM* const stream = reinterpret_cast<STREAM*>(U32ToPtr(static_cast<unsigned int>(argument1)));
        const char* text = m_sourceText.value;
        (void)stream->Write(text, static_cast<unsigned int>(std::strlen(text)) + 1u);
        return 0;
    }
    case ACT_RESTORE:
    {
        (void)FRAME::Action(command, argument1, argument2, argument3);
        STREAM* const stream = reinterpret_cast<STREAM*>(U32ToPtr(static_cast<unsigned int>(argument1)));
        if (argument2 < 19)
            (void)stream->Read(&m_behave, 4u);
        m_sourceText.Read(stream);
        (void)Action(ACT_SET_TEXT, static_cast<int>(PtrToU32(&m_sourceText)), 0, 0);
        return 0;
    }
    case ACT_SET_BEHAVE:
        m_behave = static_cast<unsigned int>(argument1);
        CalcTextProperty();
        return 0;
    case ACT_SET_TEXT:
        SetAndDecodeSource(reinterpret_cast<const STRING*>(U32ToPtr(static_cast<unsigned int>(argument1))));
        return 0;
    case ACT_GET_TEXT_DESC:
        return static_cast<int>(PtrToU32(&m_sourceText));
    case ACT_SET_TEXT_COUNT:
        if (argument1 < 0)
            return static_cast<int>(m_cursor) >= static_cast<int>(static_cast<unsigned int>(std::strlen(m_renderedText.value))) ? 1 : 0;
        {
            const unsigned int length = static_cast<unsigned int>(std::strlen(m_renderedText.value));
            m_cursor = static_cast<unsigned int>(argument1)<length ? static_cast<unsigned int>(argument1) : length;
        }
        return 0;
    case ACT_SET_FILE:
        (void) m_renderedText.LoadFile(*reinterpret_cast<const STRING*>(U32ToPtr(static_cast<unsigned int>(argument1))));
        m_cursor = static_cast<unsigned int>(std::strlen(m_renderedText.value));
        return 0;
    case ACT_GET_TEXT:
        return static_cast<int>(PtrToU32(&m_renderedText));
    case ACT_SET_TEXT_ROW:
        m_firstLine = static_cast<unsigned int>(argument1);
        m_lineCount = argument2;
        return 0;
    case ACT_NEXT_COMMAND:
        return AdvanceCursor();
    default:
        return FRAME::Action(command, argument1, argument2, argument3);
    }
}

void STEXT::DrawFallback()
{
    const VECTOR2 screen = ScreenXY();
    COLOR color;
    color.value = 0xFFFFFFFFu;
    Graph->PutsXY(screen, m_renderedText, color);
}

void STEXT::DrawGlyphs(VID* initialFont, int verticalOffset)
{
    if (initialFont->noDir <= 126)
        return;
    const char* text = m_renderedText.value;
    if (text[0] == 0)
        return;

    const int savedGlyph = m_currentFrame;
    const float savedX = m_xyz.x;
    const float savedY = m_xyz.y;
    VID* font = initialFont;
    m_xyz.y = savedY + static_cast<float>(verticalOffset);
    const int lineStart = TruncateDoubleToInt32(static_cast<double>(savedX) - font->sizeXYZ.x);

    for (int characterIndex = 0; characterIndex < static_cast<int>(m_cursor); ++characterIndex) {
        text = m_renderedText.value;
        const unsigned char character = static_cast<unsigned char>(text[characterIndex]);
        if (character == '\n') {
            m_xyz.y += font->sizeXYZ.y;
            m_xyz.x = static_cast<float>(lineStart);
        } else if (character == '\r') {
            m_xyz.x = static_cast<float>(lineStart);
        } else if (character == '\t') {
            const float tabColumns = 7.0f;
            m_xyz.x = static_cast<float>(static_cast<double>(font->sizeXYZ.x) * tabColumns + m_xyz.x);
        } else if (character == '<' && std::strncmp(text, "<Font=", 6) == 0) {
            {
                STRING fontIndexText = m_renderedText.After("<Font=");
                font = STextFontByIndex(fontIndexText.Int());
            }
            m_xyz.x -= font->sizeXYZ.x;
            STRING fontPrefixText = m_renderedText.Before("/>");
            characterIndex += static_cast<int>(static_cast<unsigned int>(std::strlen(fontPrefixText.value))) + 1;
        } else if (character == '\x1B' && characterIndex + 1 < static_cast<int>(m_cursor)) {
            const unsigned int unsignedFontIndex = static_cast<unsigned char>(text[characterIndex + 1]);
            if (STextUnsignedFontSlotExists(unsignedFontIndex)) {
                ++characterIndex;
                const int signedFontIndex = static_cast<int>(static_cast<signed char>(text[characterIndex]));
                font = STextFontByIndex(signedFontIndex);
                m_xyz.x -= font->sizeXYZ.x;
            }
        } else if (character >= ' ') {
            m_currentFrame = static_cast<int>(character);
            font->Draw(this);
        }
        m_xyz.x += font->sizeXYZ.x;
    }

    m_xyz.x = savedX;
    m_xyz.y = savedY;
    m_currentFrame = savedGlyph;
}

void STEXT::Draw()
{
    const VID* const sentinel = STextFontByIndex(-1);
    if (!m_vid || m_vid == sentinel) {
        DrawFallback();
        return;
    }

    if ((m_behave & 0x70u) == 0x60u) {
        {
            STRING resolved = Map->ScriptVariable(m_sourceText);
            m_renderedText = resolved;
        }
        CalcTextProperty();
    }

    const unsigned int flags = m_behave;
    VID* const owner = m_vid;
    int verticalOffset = 0;
    if ((flags & 8u) != 0u)
        verticalOffset = -(m_textHeight / 2);
    else if ((flags & 4u) != 0u)
        verticalOffset = -m_textHeight;

    if ((owner->type & 0x4000u) == 0u) {
        DrawGlyphs(owner, verticalOffset);
        return;
    }

    const unsigned int color = GetGamma().add;
    const VECTOR2 point(m_xyz.x, m_xyz.y - m_xyz.z);
    const VECTOR2 screen = ((m_vid->spriteType & 0x20u) == 0u)
        ? Map->ToScreenScaled(point)
        : point - Map->ShiftXY();

    int horizontalOffset = 0;
    if ((m_behave & 1u) != 0u)
        horizontalOffset = -(m_textWidth / 2);
    else if ((m_behave & 2u) != 0u)
        horizontalOffset = -m_textWidth;

    if ((m_vid->property & 0x4000u) != 0u) {
        const VECTOR2 leftTop(screen.x + static_cast<float>(horizontalOffset - 4),
                              screen.y + static_cast<float>(verticalOffset));
        const VECTOR2 rightBottom(leftTop.x + static_cast<float>(m_textWidth + 8),
                                  leftTop.y + static_cast<float>(m_textHeight));
        COLOR background;
        background.value = 0x80000000u;
        Graph->Bar(leftTop, rightBottom, background);
    }

    if ((m_behave & 0x100u) != 0u && (::CurrentTime & 0x200u) != 0u) {
        COLOR cursorColor;
        cursorColor.value = (color & 0x00FFFFFFu) != 0u
            ? color | 0xFF000000u : 0xFFFFFFFFu;
        int cursorHorizontal = m_textWidth;
        if ((m_behave & 1u) != 0u)
            cursorHorizontal = m_textWidth / 2;
        else if ((m_behave & 2u) != 0u)
            cursorHorizontal = 0;

        const int ownerColumns = static_cast<int>(m_vid->vidSizeX);
        const int ownerRows = static_cast<int>(m_vid->vidSizeY);
        const VECTOR2 corner(
            screen.x + static_cast<float>(ownerColumns + cursorHorizontal),
            screen.y + static_cast<float>(verticalOffset + m_textHeight));
        const VECTOR2 opposite(
            corner.x - static_cast<float>(ownerColumns - 1),
            corner.y - static_cast<float>((ownerRows / 8) + 1));
        Graph->Bar(corner, opposite, cursorColor);
    }

    if (m_renderedText.value[0] == 0)
        return;
    COLOR textColor;
    textColor.value = color;
    static_cast<VID_FONT*>(m_vid)->DrawTextA(
        TruncateFloatToInt32(screen.x), TruncateFloatToInt32(screen.y) + verticalOffset,
        m_renderedText, static_cast<int>(m_cursor), static_cast<int>(m_firstLine),
        m_lineCount, textColor, m_lineOffsets);
}

COLOR::COLOR(const COLOR& source)
    : value(source.value)
{
}

void GRAPH::PutsXY(const VECTOR2& position, const STRING& text, COLOR color)
{
    PutsXY(position.x, position.y, text.CharPtr(), color);
}

