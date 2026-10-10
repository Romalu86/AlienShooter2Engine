#pragma once


#include <stddef.h>
#include "mylib.h"
#include "input.h"
#include "sprite_list.h"
#include "sprite.h"

class SPRITE;
class STREAM;

extern int g_iHeroIdForInvuln;

struct PLAYER_HISTORY_RECORD
{
    VECTOR position;
    unsigned char direction;
    unsigned char reserved0D[3];
    unsigned int marker;
    unsigned int time;
};

class PLAYER
{
public:


    PLAYER_HISTORY_RECORD m_moveHistory[7];
    unsigned int m_moveHistoryCount;
    unsigned int m_moveRollback;
    unsigned int m_moveHistoryTime;
    VECTOR m_moveFallback;
    int m_money;
    int m_relation[8];
    VECTOR2 m_previousInput;
    unsigned int m_stateF0;
    unsigned int m_playerIndex;
    int m_controlMode;
    unsigned int m_controlPauseState;
    PTR_SPRITE m_flagman;
    SPRITE_LIST m_spriteList;
    PTR_SPRITE m_transientSprite;
    unsigned char m_cachedDirection;
    unsigned char m_reserved119[3];
    float m_targetX;
    float m_targetY;
    unsigned int m_targetTime;
    INPUT m_input;


    explicit PLAYER(unsigned int playerIndex);
    virtual ~PLAYER() noexcept(false);


    virtual void InitForLoadMap();
    virtual void DeletePointerToSprite(SPRITE* value);
    virtual void Save(STREAM* stream) const;
    virtual void Load(STREAM* stream);
    virtual void Release();
    virtual void DrawSecondaryInfo();
    virtual void SetFlagman(SPRITE* value);
    virtual void Control(INPUT* input);
    virtual void StateBarOn();
    virtual void StateBarOff();
    virtual void PutMessage(const STRING& text,float x,float y);
    virtual void AddUnitToStateBar(SPRITE* sprite);
    virtual int GetQuestState(int group,int quest);
    virtual bool SetQuestState(int group,int quest,int state);
    virtual STRING GetMouseTipsString() const;
    void* ScalarDeletingDestructor(unsigned int flags);
    void ControlFlagman(INPUT* input);
    SPRITE* Flagman() const;
    void GroupsTact();


    int RelationTo(unsigned int index) const;
    void SetRelationTo(unsigned int index,int relation);
    int GetMoney() const;
    void SetMoney(int money);
    void SetPlayerControl(int control);
};

