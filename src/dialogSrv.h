#pragma once
#include "basetype.h"
#include "mylib.h"
#include "list.h"

#include <stddef.h>

class DIALOG;

struct DialogTreeNode
{
    DialogTreeNode* m_left;
    DialogTreeNode* m_parent;
    DialogTreeNode* m_right;
    int m_key;
    DIALOG* m_dialog;
    unsigned int m_colorOrState;

    DialogTreeNode*& LeftLink() { return m_left; }
    DialogTreeNode*& ParentLink() { return m_parent; }
    DialogTreeNode*& RightLink() { return m_right; }
    unsigned int& ColorValue() { return m_colorOrState; }
};

struct DialogTreeInsertResult
{
    DialogTreeNode* m_node;
    unsigned char m_inserted;
    unsigned char m_pad05[3];
};

struct DialogTreeControl
{
    static DialogTreeNode* _Nil;
    static unsigned int _Nilrefs;

    DialogTreeControl();
    ~DialogTreeControl();

    unsigned char m_state0;
    unsigned char m_state1;
    unsigned char m_pad02[2];
    DialogTreeNode* m_head;
    unsigned char m_policyFlag;
    unsigned char m_pad09[3];
    unsigned int m_size;
};

struct COMMENT
{
    STRING m_ownedString;
    int m_dialogKey;

    COMMENT(STRING text, int dialogKey);
};

struct DialogVector8Control
{
    DialogVector8Control() noexcept;
    ~DialogVector8Control();

    unsigned char m_state0;
    unsigned char m_pad01[3];
    COMMENT* m_begin;
    COMMENT* m_end;
    COMMENT* m_capEnd;
};

struct PHRASE
{
    STRING m_text;
    STRING m_voiceFile;
    enum PAUSE_TYPE : int
    {
        PAUSE = 1,
        CLICK_OR_PAUSE = 2
    };

    PAUSE_TYPE m_pauseType;
    int m_sfx;
    int m_duration;

    PHRASE() noexcept;
    PHRASE(const STRING& text, PAUSE_TYPE pauseType, const STRING& voiceFile,
        int sfx, int pauseDuration);
    PHRASE& operator=(const PHRASE& source);

    ~PHRASE();
    unsigned long GetDuration() const;
};

template<> LIST<PHRASE>::~LIST();
template<> void LIST<PHRASE>::Insert(PHRASE value);

class DIALOG
{
public:
    LIST<PHRASE> m_phrases;
    int m_unitVid[2];
    VECTOR m_coor;
    unsigned int m_radio;
    unsigned int m_faceDirection;
    STRING m_defaultFont;

    DIALOG();
    ~DIALOG();
    const STRING& GetPhrase(int index) const;
    int GetSFX(int index) const;
    int GetDuration(int index) const;
    const STRING& GetVoiceFile(int index) const;
    const VECTOR& Coor() const;
};

class zDlgSrv
{
public:
    zDlgSrv();
    virtual ~zDlgSrv() noexcept(false);

    DialogTreeControl m_dialogTree;
    DialogVector8Control m_dialogVector;
    DIALOG m_dialogRecord;
    unsigned int m_textDurationPercent;
    unsigned int m_oneLineDuration;
    unsigned int m_twoLineDuration;
    unsigned int m_threeLineDuration;
    unsigned int m_fourLineDuration;
    unsigned int m_fiveLineDuration;
    unsigned char m_showDialogSubtitles;
    unsigned char m_pad6D[3];
    unsigned int m_activeRadioDialogKey;

    bool IsDlgNumValid(int dialogKey);
    DIALOG* GetDialog(int _iDlgKey);
    int GetDlgKey(int group, int var2);
    void LoadDialogs(const STRING& source);

private:
    int ReleaseData();
    friend int  zDlgSrv_ReleaseData(void* self);
};

typedef zDlgSrv DialogSrv;

extern "C" void*  PhraseVectorDeletingDestructorAlias(void* self, unsigned int flags);

bool  zDlgSrv_IsDlgNumValid(void* self, int key);
DIALOG*  zDlgSrv_GetDialog(void* self, int key);

void __cdecl DialogComment_Construct(COMMENT* destination, const COMMENT* source);


int  zDlgSrv_GetDlgKey(void* self, int group, int index);

void  zDlgSrv_LoadDialogs(void* self, const STRING& source);

void*  zDlgSrv_DeletingDtor(void* self, unsigned int flags);

void  DialogIteratorBoolPair_Ctor(void* self, const void* nodeSource, const void* insertedSource);

extern "C" void*  DialogCtor(void* self);

DialogTreeNode* __cdecl DialogTree_Min(DialogTreeNode* start);

void DialogTreeConstIterator_Inc(DialogTreeNode** iterator);

void __stdcall DialogCommentVector_Ufill(void* begin, unsigned int count, const void* arg);

void __cdecl DialogComment_Fill(void* begin, void* end, const void* src);

unsigned char* __stdcall DialogCommentVector_Ucopy(unsigned char* first, unsigned char* last, unsigned char* dest);

unsigned char* __cdecl DialogComment_CopyBackward(unsigned char* first, unsigned char* last, unsigned char* destEnd);

void DialogTreeConstIterator_Dec(DialogTreeNode** iterator);

void  DialogTree_EraseSubtree(void* self, void* node);

void*  zDlgSrv_Ctor(void* self);
void  zDlgSrv_Dtor(void* self);

unsigned int DialogSrv_StaticCleanupRegister02();
unsigned int DialogSrv_StaticCleanupRegister03();
unsigned int DialogSrv_StaticCleanupRegister04();
unsigned int DialogSrv_StaticCleanupRegister05();
unsigned int DialogSrv_StaticCleanupRegister06();
void __cdecl DialogServiceGlobalStringCleanup();
void  DialogCommentVector_Dtor(void* self);
int  DialogCommentVector_Size(const void* self);
void __stdcall DialogCommentVector_Destroy(void* begin, void* end);
void __cdecl DialogGlobalStringCleanup();
