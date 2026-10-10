#pragma once

#include "basetype.h"
#include "myerror.h"

class DIALOG;
class SPRITE;
class STRING;
enum DLG_TYPE : int;

struct DIALOG_UNIT_RECORD
{
    int dialogId;
    int phraseIndex;
    unsigned int phraseStartTime;
    bool skipRequested;
    unsigned char reserved0D[3];
    int lastDialogId;
};

class DIALOG_UNIT
{
    friend class MAN;
public:
    static DIALOG_UNIT* m_pRadioMesUser;
    DIALOG_UNIT();
    virtual ~DIALOG_UNIT() noexcept(false);

    virtual SPRITE* Link() const=0;
    virtual void Error(TYPE_ERROR type, const STRING& subject, unsigned long code) const=0;
    virtual const VECTOR& Coor() const=0;
    virtual int AddLinkToLast(SPRITE* sprite)=0;
    virtual void PlaySFX(int sfxId) const=0;

    void SetDialog(int dialogKey);
    void ClearDialogs();
    int GetDialog() const;

protected:
    void SetRadioMsg(int first, int second, bool enable);
    bool ShowCurPhrase(DLG_TYPE type, bool force);
    void ClearDialog(DLG_TYPE type, bool keepRuntimeState);
    void DialogTact();
    void DialogTact(DLG_TYPE type);

    DIALOG_UNIT_RECORD m_dialog[2];
};

