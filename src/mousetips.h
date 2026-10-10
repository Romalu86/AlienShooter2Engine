#pragma once

class INPUT;
class SPRITE;
class MOUSETIPS
{
public:
    virtual ~MOUSETIPS();
    SPRITE* tip;
    void Tact(const INPUT* input);
    void Clear();
    void DeletePointerToSprite(SPRITE* sprite);
};
