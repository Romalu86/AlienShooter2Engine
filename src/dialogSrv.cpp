#include <cstdio>
#include <new>
#include "engine_globals.h"
#include "myerror.h"
#include "zm/Utils/zLogConsole.h"
#include "list.h"
#include "basetype.h"

#include "mylib.h"
#include "zm/Utils/zTreeOperations.h"
#include "dialogSrv.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct DialogMapValuePair
{
    int m_key;
    DIALOG* m_dialog;
};

int zDlgSrv::GetDlgKey(int group, int var2)
{
    if (var2 >= 1000)
        DebugAssert(4u, "var2 < 1000", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\dialogSrv.cpp", 292u);
    return BitsToSigned32(static_cast<unsigned int>(MultiplySigned32Wrap(group, 1000))+static_cast<unsigned int>(var2));
}
#include "graph.h"
#include "map.h"
#include "resread.h"
#include "unit.h"
#include "zm/RPGHero/zRPGFrameWork.h"

int  zDlgSrv_ReleaseData(void* self);
DialogTreeNode** DialogTreeIterator_PostIncrement(DialogTreeNode** iterator, DialogTreeNode** outCurrent, int unused);
DialogTreeNode** DialogTree_EraseIterator(DialogTreeControl* self, DialogTreeNode** outIterator, DialogTreeNode* node);
void  Dialog_Dtor(void* self);
DialogTreeNode** DialogTree_InsertNode(DialogTreeControl* self, DialogTreeNode** outNode, DialogTreeNode* hintNode, DialogTreeNode* parent, const DialogMapValuePair* valueSource);
void  zDlgSrv_LoadDialogs(void* self, const STRING& source);

PHRASE::PHRASE() noexcept
{
}

void*  zDlgSrv_DeletingDtor(void* self, unsigned int flags)
{
    zDlgSrv_Dtor(self);
    if (flags & 1u) CrtFreePointer( self);
    return self;
}

void  DialogIteratorBoolPair_Ctor(void* self, const void* nodeSource, const void* insertedSource)
{
    DialogTreeInsertResult* result = static_cast<DialogTreeInsertResult*>(self);
    result->m_node = *static_cast<DialogTreeNode* const*>(nodeSource);
    result->m_inserted = *static_cast<const unsigned char*>(insertedSource);
}

void __cdecl DialogComment_Construct(COMMENT* destination, const COMMENT* source);

DialogTreeNode* __cdecl DialogTree_Min(DialogTreeNode* start)
{
    DialogTreeNode* const sentinel = DialogTreeControl::_Nil;
    DialogTreeNode* node = start;
    if (node->m_left == sentinel) return node;
    node = node->m_left;
    while (node->m_left != sentinel) node = node->m_left;
    return node;
}

PHRASE& PHRASE::operator=(const PHRASE& source)
{
    m_text = source.m_text;
    m_voiceFile = source.m_voiceFile;
    m_pauseType = source.m_pauseType;
    m_sfx = source.m_sfx;
    m_duration = source.m_duration;
    return *this;
}

void DialogTreeConstIterator_Inc(DialogTreeNode** iterator)
{
    DialogTreeNode* const sentinel = DialogTreeControl::_Nil;
    DialogTreeNode* node = *iterator;
    DialogTreeNode* next = node->m_right;
    if (next != sentinel)
    {
        while (next->m_left != sentinel) next = next->m_left;
        *iterator = next;
        return;
    }
    next = node->m_parent;
    while (node == next->m_right)
    {
        node = next;
        next = next->m_parent;
    }
    if (node->m_right != next) node = next;
    *iterator = node;
}

void __stdcall DialogCommentVector_Ufill(void* begin, unsigned int count, const void* sourceElement)
{
    COMMENT* destination = static_cast<COMMENT*>(begin);
    const COMMENT* source = static_cast<const COMMENT*>(sourceElement);
    for (unsigned int elementIndex = 0; elementIndex<count; ++elementIndex) DialogComment_Construct(destination+elementIndex, source);
}

void __cdecl DialogComment_Fill(void* begin, void* end, const void* sourceElement)
{
    COMMENT* current = static_cast<COMMENT*>(begin);
    COMMENT* finish = static_cast<COMMENT*>(end);
    const COMMENT* source = static_cast<const COMMENT*>(sourceElement);
    while (current != finish)
    {
        current->m_ownedString = source->m_ownedString;
        current->m_dialogKey = source->m_dialogKey;
        ++current;
    }
}

unsigned char* __stdcall DialogCommentVector_Ucopy(unsigned char* sourceBegin, unsigned char* sourceEnd, unsigned char* destinationBegin)
{
    COMMENT* source = reinterpret_cast<COMMENT*>(sourceBegin);
    COMMENT* finish = reinterpret_cast<COMMENT*>(sourceEnd);
    COMMENT* destination = reinterpret_cast<COMMENT*>(destinationBegin);
    while (source != finish) DialogComment_Construct(destination++, source++);
    return reinterpret_cast<unsigned char*>(destination);
}

unsigned char* __cdecl DialogComment_CopyBackward(unsigned char* sourceBegin, unsigned char* sourceEnd, unsigned char* destinationEnd)
{
    COMMENT* begin = reinterpret_cast<COMMENT*>(sourceBegin);
    COMMENT* source = reinterpret_cast<COMMENT*>(sourceEnd);
    COMMENT* destination = reinterpret_cast<COMMENT*>(destinationEnd);
    if (begin == source) return destinationEnd;
    do
    {
        --source;
        --destination;
        destination->m_ownedString = source->m_ownedString;
        destination->m_dialogKey = source->m_dialogKey;
    }
    while (source != begin);
    return reinterpret_cast<unsigned char*>(destination);
}

void DialogTreeConstIterator_Dec(DialogTreeNode** nodeIterator)
{
    DialogTreeNode* node = *nodeIterator;
    if (node->m_colorOrState == 0u && node->m_parent->m_parent == node)
    {
        *nodeIterator = node->m_right;
        return;
    }
    DialogTreeNode* const sentinel = DialogTreeControl::_Nil;
    DialogTreeNode* left = node->m_left;
    if (left != sentinel)
    {
        while (left->m_right != sentinel) left = left->m_right;
        *nodeIterator = left;
        return;
    }
    DialogTreeNode* parent = node->m_parent;
    while (node == parent->m_left)
    {
        node = parent;
        parent = parent->m_parent;
    }
    *nodeIterator = parent;
}

void  DialogTree_EraseSubtree(void* self, void* node)
{
    DialogTreeNode* current = static_cast<DialogTreeNode*>(node);
    while (current != DialogTreeControl::_Nil)
    {
        DialogTree_EraseSubtree(self, current->m_right);
        DialogTreeNode* next = current->m_left;
        CrtFreePointer(current);
        current = next;
    }
}

DialogTreeNode* DialogTreeControl::_Nil = 0;
unsigned int DialogTreeControl::_Nilrefs = 0;

DialogTreeControl::DialogTreeControl()
    : m_state0(0), m_state1(0), m_policyFlag(0)
{
    DialogTreeNode* candidate = static_cast<DialogTreeNode*>(CrtNew(sizeof(DialogTreeNode)));
    candidate->m_parent = 0;
    candidate->m_colorOrState = 1u;
    unsigned int staticLock = 0;
    Msvc6Runtime::StaticLockConstruct(&staticLock);
    if (!_Nil)
    {
        _Nil = candidate;
        candidate->m_left = 0;
        _Nil->m_right = 0;
        candidate = 0;
    }
    ++_Nilrefs;
    Msvc6Runtime::StaticLockDestruct(&staticLock);
    if (candidate) CrtFreePointer(candidate);

    DialogTreeNode* const sentinel = _Nil;
    DialogTreeNode* const head = static_cast<DialogTreeNode*>(CrtNew(sizeof(DialogTreeNode)));
    head->m_parent = sentinel;
    head->m_colorOrState = 0;
    m_head = head;
    m_size = 0;
    head->m_left = head;
    m_head->m_right = m_head;
}

DialogTreeControl::~DialogTreeControl()
{
    DialogTreeNode* const finish = m_head;
    DialogTreeNode* first = finish->m_left;
    if (m_size != 0)
    {
        DialogTreeNode* node = finish->m_parent;
        while (node != _Nil)
        {
            DialogTree_EraseSubtree(this, node->m_right);
            DialogTreeNode* const next = node->m_left;
            CrtFreePointer(node);
            node = next;
        }
        m_head->m_parent = _Nil;
        m_size = 0;
        m_head->m_left = m_head;
        m_head->m_right = m_head;
    }
    else
    {
        while (first != finish)
        {
            DialogTreeNode* const eraseNode = first;
            DialogTreeConstIterator_Inc(&first);
            DialogTreeNode* ignored = 0;
            (void) DialogTree_EraseIterator(this, &ignored, eraseNode);
        }
    }
    CrtFreePointer(m_head);
    m_head = 0;
    m_size = 0;

    unsigned int staticLock = 0;
    DialogTreeNode* detachedSentinel = 0;
    Msvc6Runtime::StaticLockConstruct(&staticLock);
    if (--_Nilrefs == 0)
    {
        detachedSentinel = _Nil;
        _Nil = 0;
    }
    Msvc6Runtime::StaticLockDestruct(&staticLock);
    if (detachedSentinel) CrtFreePointer(detachedSentinel);
}

DialogVector8Control::DialogVector8Control() noexcept
    : m_state0(0), m_begin(0), m_end(0), m_capEnd(0)
{
}

DialogVector8Control::~DialogVector8Control()
{
    DialogCommentVector_Dtor(this);
}

zDlgSrv::zDlgSrv()
{
    m_textDurationPercent = 100;
    m_showDialogSubtitles = 1;
    m_activeRadioDialogKey = 0;
    m_oneLineDuration = m_twoLineDuration = m_threeLineDuration = m_fourLineDuration = m_fiveLineDuration = 0;
    if (!::gpLogCons) GetNew_zLogConsole(&::gpLogCons);
}

zDlgSrv::~zDlgSrv() noexcept(false)
{
    ReleaseData();
}

void* zDlgSrv_Ctor(void* self)
{
    return ::new (self) zDlgSrv();
}

void zDlgSrv_Dtor(void* self)
{
    static_cast<zDlgSrv*>(self)->~zDlgSrv();
}

unsigned int __cdecl DialogSrv_StaticCleanupRegister02()
{
    return RegisterCrtAtExit(&DialogServiceGlobalStringCleanup);
}

unsigned int __cdecl DialogSrv_StaticCleanupRegister03()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

unsigned int __cdecl DialogSrv_StaticCleanupRegister04()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

unsigned int __cdecl DialogSrv_StaticCleanupRegister05()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

unsigned int __cdecl DialogSrv_StaticCleanupRegister06()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}



void  DialogCommentVector_Dtor(void* self)
{
    DialogVector8Control* vector = static_cast<DialogVector8Control*>(self);
    COMMENT* current = vector->m_begin;
    COMMENT* const finish = vector->m_end;
    while (current != finish)
        (current++)->~COMMENT();
    CrtFreePointer(vector->m_begin);
    vector->m_begin = 0;
    vector->m_end = 0;
    vector->m_capEnd = 0;
}

int  DialogCommentVector_Size(const void* self)
{
    const DialogVector8Control* vector = static_cast<const DialogVector8Control*>(self);
    if (!vector->m_begin) return 0;
    return static_cast<int>(vector->m_end-vector->m_begin);
}

void __stdcall DialogCommentVector_Destroy(void* begin, void* end)
{
    COMMENT* current = static_cast<COMMENT*>(begin);
    const COMMENT* finish = static_cast<const COMMENT*>(end);
    while (current != finish)
    {
        current->~COMMENT();
        ++current;
    }
}



DialogTreeNode** DialogTreeIterator_PostIncrement(DialogTreeNode** nodeIterator, DialogTreeNode** outCurrent, int unused)
{
    (void) unused;
    DialogTreeNode* const sentinel = DialogTreeControl::_Nil;
    DialogTreeNode* current = *nodeIterator;
    DialogTreeNode* next = current->m_right;
    if (next != sentinel)
    {
        while (next->m_left != sentinel) next = next->m_left;
        *nodeIterator = next;
    }
    else
    {
        DialogTreeNode* parent = current->m_parent;
        DialogTreeNode* node = current;
        while (node == parent->m_right)
        {
            *nodeIterator = parent;
            parent = parent->m_parent;
            node = *nodeIterator;
        }
        if (node->m_right != parent)*nodeIterator = parent;
    }
    *outCurrent = current;
    return outCurrent;
}

void*  DialogCommentVector_Erase(void* self, void* begin, void* eraseEnd)
{
    DialogVector8Control* vector = static_cast<DialogVector8Control*>(self);
    COMMENT* destination = static_cast<COMMENT*>(begin);
    COMMENT* source = static_cast<COMMENT*>(eraseEnd);
    COMMENT* const copyEnd = vector->m_end;
    while (source != copyEnd)
    {
        destination->m_ownedString = source->m_ownedString;
        destination->m_dialogKey = source->m_dialogKey;
        ++source;
        ++destination;
    }
    COMMENT* const newEnd = destination;
    COMMENT* const destroyEnd = vector->m_end;
    while (destination != destroyEnd)
        (destination++)->~COMMENT();
    vector->m_end = newEnd;
    return begin;
}

void __cdecl DialogSrvRegisterStaticCleanupSet()
{
    typedef unsigned int (__cdecl *CleanupRegistration)();
    const CleanupRegistration registrations[4] = {
        &DialogSrv_StaticCleanupRegister03,
        &DialogSrv_StaticCleanupRegister04,
        &DialogSrv_StaticCleanupRegister05,
        &DialogSrv_StaticCleanupRegister06
    };
    for (unsigned int registrationIndex = 0u; registrationIndex<4u; ++registrationIndex)
    {
        const unsigned char guard = CrtStaticInitializationGuards[registrationIndex];
        if ((guard&1u) == 0u)
            CrtStaticInitializationGuards[registrationIndex] = static_cast<unsigned char>(guard | 1u);
        (void) registrations[registrationIndex]();
    }
}

STRING FGets(std::FILE* file)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(unsigned int));
    char* const readBuffer = std::fgets(buffer, 1024, file);
    const char* lineText = readBuffer?buffer:StringGlobals::EmptyText();
    return STRING(lineText);
}

template<> void LIST<PHRASE>::Insert(PHRASE value)
{
    if (m_count >= m_capacity)
    {
        const unsigned int grown = static_cast<unsigned int>(m_capacity)*2u+4u;
        const int newCapacity = BitsToSigned32(grown);
        if (newCapacity > m_capacity)
        {
            PHRASE* const oldPhrases = m_data;
            unsigned int* const allocation = static_cast<unsigned int*>(
                CrtNew(grown*sizeof(PHRASE)+sizeof(unsigned int)));
            PHRASE* newPhrases = 0;
            if (allocation)
            {
                *allocation = grown;
                newPhrases = reinterpret_cast<PHRASE*>(allocation+1);
                as2::core::constructArray(newPhrases, sizeof(PHRASE), grown,
                    [](void* element){ ::new (element) PHRASE(); });
            }
            m_data = newPhrases;
            if (!newPhrases) ReportAllocationFailure(grown);
            if (oldPhrases)
            {
                for (int index = 0; index < m_capacity; ++index)
                    m_data[index] = oldPhrases[index];
                (void) PhraseVectorDeletingDestructorAlias(oldPhrases, 3u);
            }
            m_capacity = newCapacity;
        }
    }
    PHRASE* const destination = m_data+m_count;
    m_count = BitsToSigned32(static_cast<unsigned int>(m_count)+1u);
    *destination = value;
}

template<> LIST<PHRASE>::~LIST()
{
    if (m_data)
        (void) PhraseVectorDeletingDestructorAlias(m_data, 3u);
    m_data = 0;
    m_count = 0;
}

int zDlgSrv::ReleaseData()
{
    zDlgSrv* service = this;
    DialogTreeControl* tree = &service->m_dialogTree;
    DialogTreeNode* root = tree->m_head;
    DialogTreeNode* const sentinel = DialogTreeControl::_Nil;

    DialogTreeNode* iterator = root->m_left;
    while (iterator != root)
    {
        if (iterator->m_dialog)
        {
            Dialog_Dtor(iterator->m_dialog);
            CrtFreePointer(iterator->m_dialog);
        }

        if (iterator->m_right != sentinel) iterator = DialogTree_Min(iterator->m_right);
        else
        {
            DialogTreeNode* node = iterator;
            DialogTreeNode* parent = node->m_parent;
            while (node == parent->m_right)
            {
                node = parent;
                parent = parent->m_parent;
            }
            if (node->m_right != parent) node = parent;
            iterator = node;
        }
    }

    DialogTreeNode* first = root->m_left;
    if (tree->m_size != 0u)
    {
        DialogTreeNode* node = root->m_parent;
        while (node != sentinel)
        {
            DialogTree_EraseSubtree(tree, node->m_right);
            DialogTreeNode* left = node->m_left;
            CrtFreePointer(node);
            node = left;
        }
        root->m_parent = sentinel;
        tree->m_size = 0;
        root->m_left = root;
        root->m_right = root;
        return static_cast<int>(PtrToU32(root));
    }

    while (first != root)
    {
        DialogTreeNode* eraseNode = first;
        DialogTreeConstIterator_Inc(&first);
        DialogTreeNode* outIterator = 0;
        (void) DialogTree_EraseIterator(tree, &outIterator, eraseNode);
    }
    return static_cast<int>(PtrToU32(root));
}

bool zDlgSrv::IsDlgNumValid(int key)
{
    zDlgSrv* service = this;
    DialogTreeControl* tree = &service->m_dialogTree;
    DialogTreeNode* const sentinel = DialogTreeControl::_Nil;
    DialogTreeNode* node = tree->m_head->m_parent;
    DialogTreeNode* parent = tree->m_head;
    bool insertOnLeft = true;

    while (node != sentinel)
    {
        parent = node;
        insertOnLeft = key<node->m_key;
        node = insertOnLeft?node->m_left:node->m_right;
    }

    DialogTreeNode* const insertionParent = parent;
    bool insertRequired = tree->m_policyFlag != 0;
    if (!insertRequired)
    {
        if (insertOnLeft)
        {
            if (parent == tree->m_head->m_left) insertRequired = true;
            else
            {
                DialogTreeConstIterator_Dec(&parent);
            }
        }
        if (!insertRequired && parent->m_key<key) insertRequired = true;
    }

    DialogTreeNode* resultNode = parent;
    if (insertRequired)
    {
        DialogMapValuePair value = {key, 0};
        (void) DialogTree_InsertNode(tree, &resultNode, node, insertionParent, &value);
    }
    return resultNode->m_dialog != 0;
}

DialogTreeInsertResult* DialogMap_Insert(DialogTreeControl* tree, DialogTreeInsertResult* resultPair, const DialogMapValuePair* value)
{
    DialogTreeNode* const sentinel = DialogTreeControl::_Nil;
    DialogTreeNode* node = tree->m_head->m_parent;
    DialogTreeNode* parent = tree->m_head;
    bool insertOnLeft = true;
    while (node != sentinel)
    {
        parent = node;
        insertOnLeft = value->m_key<node->m_key;
        node = insertOnLeft?node->m_left:node->m_right;
    }
    DialogTreeNode* const insertionParent = parent;
    bool insertRequired = tree->m_policyFlag != 0u;
    if (!insertRequired)
    {
        if (insertOnLeft)
        {
            if (parent == tree->m_head->m_left) insertRequired = true;
            else
            {
                DialogTreeConstIterator_Dec(&parent);
                insertRequired = parent->m_key<value->m_key;
            }
        }
        else insertRequired = parent->m_key<value->m_key;
    }
    DialogTreeNode* result = parent;
    if (insertRequired)
    {
        (void) DialogTree_InsertNode(tree, &result, node, insertionParent, value);
    }
    resultPair->m_node = result;
    resultPair->m_inserted = insertRequired?1u:0u;
    return resultPair;
}

DIALOG* zDlgSrv::GetDialog(int _iDlgKey)
{
    if (!IsDlgNumValid(_iDlgKey))
    {
        if (!IsDlgNumValid(_iDlgKey))
            DebugAssert(5u, "IsDlgNumValid(_iDlgKey)", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\dialogSrv.cpp", 275u);
        iLogConsole* logger = ::gpLogCons;
        (void) logger->LogStr(LT_ERR, "invalid dialog %d in GetDialog", (unsigned int) _iDlgKey);
    }

    zDlgSrv* service = this;
    DialogTreeControl* tree = &service->m_dialogTree;
    DialogTreeNode* const sentinel = DialogTreeControl::_Nil;
    DialogTreeNode* node = tree->m_head->m_parent;
    DialogTreeNode* parent = tree->m_head;
    bool insertOnLeft = true;

    while (node != sentinel)
    {
        parent = node;
        insertOnLeft = _iDlgKey<node->m_key;
        node = insertOnLeft?node->m_left:node->m_right;
    }

    DialogTreeNode* const insertionParent = parent;
    bool insertRequired = tree->m_policyFlag != 0u;
    if (!insertRequired)
    {
        if (insertOnLeft)
        {
            if (parent == tree->m_head->m_left) insertRequired = true;
            else
            {
                DialogTreeConstIterator_Dec(&parent);
            }
        }
        if (!insertRequired && parent->m_key<_iDlgKey) insertRequired = true;
    }

    DialogTreeNode* resultNode = parent;
    if (insertRequired)
    {
        DialogMapValuePair value = {_iDlgKey, 0};
        (void) DialogTree_InsertNode(tree, &resultNode, node, insertionParent, &value);
    }
    return resultNode->m_dialog;
}

DialogTreeNode** DialogTree_InsertNode(DialogTreeControl* self, DialogTreeNode** outNode, DialogTreeNode* hintNode, DialogTreeNode* parent, const DialogMapValuePair* valueSource)
{
    *outNode = TreeOperations::Insert(self->m_head, self->m_size, hintNode, parent,
        DialogTreeControl::_Nil,
        [valueSource](DialogTreeNode* node)
        {
            node->m_key = valueSource->m_key;
            node->m_dialog = valueSource->m_dialog;
        },
        [valueSource](const DialogTreeNode* node) { return valueSource->m_key<node->m_key; });
    return outNode;
}

DialogTreeNode** DialogTree_EraseIterator(DialogTreeControl* self, DialogTreeNode** outIterator, DialogTreeNode* node)
{
    *outIterator = TreeOperations::Erase(self->m_head, self->m_size, node, DialogTreeControl::_Nil);
    return outIterator;
}

void*  DialogCommentVector_Insert(void* self, void* position, const void* value)
{
    DialogVector8Control* vector = static_cast<DialogVector8Control*>(self);
    COMMENT* const insertionPosition = static_cast<COMMENT*>(position);
    const COMMENT* const sourceValue = static_cast<const COMMENT*>(value);
    COMMENT* const end = vector->m_end;
    COMMENT* const capEnd = vector->m_capEnd;
    COMMENT* const begin = vector->m_begin;
    const unsigned int index = static_cast<unsigned int>(BitsToSigned32(PtrToU32(insertionPosition)-PtrToU32(begin))>>3);
    const unsigned int freeSlots = static_cast<unsigned int>(BitsToSigned32(PtrToU32(capEnd)-PtrToU32(end))>>3);

    if (freeSlots < 1u)
    {
        const unsigned int oldCount = begin != 0?static_cast<unsigned int>(BitsToSigned32(PtrToU32(end)-PtrToU32(begin))>>3):0u;
        const unsigned int growth = oldCount > 1u?oldCount:1u;
        const unsigned int newCapacity = oldCount+growth;
        const unsigned int allocationCount = BitsToSigned32(newCapacity) < 0?0u:newCapacity;
        COMMENT* const newBegin = static_cast<COMMENT*>(CrtNew(allocationCount*sizeof(COMMENT)));
        COMMENT* destination = newBegin;
        COMMENT* source = vector->m_begin;
        while (source != insertionPosition) DialogComment_Construct(destination++, source++);
        DialogComment_Construct(destination++, sourceValue);
        (void) DialogCommentVector_Ucopy(reinterpret_cast<unsigned char*>(insertionPosition),
            reinterpret_cast<unsigned char*>(vector->m_end), reinterpret_cast<unsigned char*>(destination));
        DialogCommentVector_Destroy(vector->m_begin, vector->m_end);
        CrtFreePointer(vector->m_begin);
        vector->m_capEnd = reinterpret_cast<COMMENT*>(U32ToPtr(PtrToU32(newBegin)+newCapacity*sizeof(COMMENT)));
        const unsigned int oldSize = static_cast<unsigned int>(DialogCommentVector_Size(vector));
        vector->m_begin = newBegin;
        vector->m_end = reinterpret_cast<COMMENT*>(U32ToPtr(PtrToU32(newBegin)+(oldSize+1u)*sizeof(COMMENT)));
        return U32ToPtr(PtrToU32(newBegin)+index*sizeof(COMMENT));
    }

    const unsigned int elementsAfterPosition = static_cast<unsigned int>(BitsToSigned32(PtrToU32(end)-PtrToU32(insertionPosition))>>3);
    if (elementsAfterPosition < 1u)
    {
        (void) DialogCommentVector_Ucopy(reinterpret_cast<unsigned char*>(insertionPosition),
            reinterpret_cast<unsigned char*>(end), reinterpret_cast<unsigned char*>(insertionPosition+1));
        COMMENT* const fillEnd = vector->m_end;
        const unsigned int elementsAfterCopy = static_cast<unsigned int>(BitsToSigned32(PtrToU32(fillEnd)-PtrToU32(insertionPosition))>>3);
        DialogCommentVector_Ufill(fillEnd, 1u-elementsAfterCopy, sourceValue);
        DialogComment_Fill(insertionPosition, vector->m_end, sourceValue);
    }
    else
    {
        (void) DialogCommentVector_Ucopy(reinterpret_cast<unsigned char*>(end-1),
            reinterpret_cast<unsigned char*>(end), reinterpret_cast<unsigned char*>(end));
        COMMENT* const copyEnd = vector->m_end;
        (void) DialogComment_CopyBackward(reinterpret_cast<unsigned char*>(insertionPosition),
            reinterpret_cast<unsigned char*>(copyEnd-1), reinterpret_cast<unsigned char*>(copyEnd));
        DialogComment_Fill(insertionPosition, insertionPosition+1, sourceValue);
    }
    vector->m_end = vector->m_end+1;
    return U32ToPtr(PtrToU32(vector->m_begin)+index*sizeof(COMMENT));
}

namespace {
static int DialogParseInteger(const char* text)
{
    if (text[1] == 'x')
    {
        int value;
        (void) sscanf(text, "%i", &value);
        return value;
    }
    return atoi(text);
}

static DIALOG* DialogRecordForKey(DialogSrv* service, int key)
{
    DialogMapValuePair value = {key, 0};
    DialogTreeInsertResult result = {0, 0u, {0u, 0u, 0u}};
    (void) DialogMap_Insert(&service->m_dialogTree, &result, &value);
    return result.m_node->m_dialog;
}

static PHRASE::PAUSE_TYPE DialogLastPauseType(DIALOG* dialog)
{
    if (dialog->m_phrases.m_count == 0) return PHRASE::PAUSE;
    unsigned int index = static_cast<unsigned int>(dialog->m_phrases.m_count-1);

    if (dialog->m_phrases.m_count>999999) index = 999999u;
    return dialog->m_phrases.m_data[index].m_pauseType;
}

static void DialogInsertPhrase(DIALOG* dialog, const STRING& text,
    PHRASE::PAUSE_TYPE pauseType, const STRING& voiceFile, int pauseDuration,
    int sfx)
{
    dialog->m_phrases.Insert(PHRASE(text, pauseType, voiceFile, sfx, pauseDuration));
}

static void DialogInsertComment(DialogSrv* service, const STRING& text, int dialogKey)
{
    COMMENT comment(text, dialogKey);
    (void) DialogCommentVector_Insert(&service->m_dialogVector, service->m_dialogVector.m_end, &comment);
}

static void DialogLogOpenFailure(const STRING& source)
{
    STRING category("DIALOG");
    (void)::Error->Error(category, E_LOAD, source, 0u);
}

static void DialogLogDeprecatedPauseDuration()
{
    iLogConsole* const logger = ::gpLogCons;
    (void) logger->LogStr(LT_WARN, "default <PauseDuration> not used any more");
}
}

void zDlgSrv::LoadDialogs(const STRING& source)
{
    zDlgSrv* service = this;

    ReleaseData();
    (void) DialogCommentVector_Erase(&service->m_dialogVector,
        service->m_dialogVector.m_begin, service->m_dialogVector.m_end);

    std::FILE* file = 0;
    {
        STRING mode("rb");
        if (source.m_value[0] != 0)
            file = std::fopen(source.m_value, mode.m_value);
    }
    if (!file)
    {
        if (source.m_value[0] != 0) DialogLogOpenFailure(source);
        return;
    }

    STRING phraseText(StringGlobals::EmptyText());
    STRING voiceFile(StringGlobals::EmptyText());
    STRING lastLine;

    service->m_dialogRecord.m_coor.x = 0u;
    service->m_dialogRecord.m_coor.y = 0u;
    service->m_dialogRecord.m_coor.z = 0.0f;
    service->m_dialogRecord.m_radio = 0u;
    service->m_dialogRecord.m_faceDirection = 0u;
    service->m_dialogRecord.m_unitVid[0] = 2u;

    int pauseDuration = 0;
    int sfx = 0;
    int currentDialogKey = 0;
    int pendingBlankLines = 0;

    for (; ;)
    {
        if (std::feof(file) != 0) break;

        STRING fetchedLine = FGets(file);
        STRING line(fetchedLine);
        lastLine = line;
        line.RemoveEndChars(" \n\r\t");

        const char* text = line.m_value;
        const bool numericDialogTag = (text[0] == '<' && CrtIsDigit(static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(text[1])))) != 0u && strstr(text, ">") != 0);
        const bool commentLine = (text[0] == ';');

        if (text[0] == 0)
        {
            ++pendingBlankLines;

        }
        else if (numericDialogTag || commentLine)
        {
            while (pendingBlankLines>0)
            {
                STRING empty;
                DialogInsertComment(service, empty, currentDialogKey);
                --pendingBlankLines;
            }
            pendingBlankLines = 0;

            if (commentLine)
            {
                DialogInsertComment(service, line, currentDialogKey);
                continue;
            }
        }
        else
        {
            pendingBlankLines = 0;
        }

        if (numericDialogTag)
        {
            phraseText.RemoveEndChars(" \n\r\t");
            if (currentDialogKey != 0 && phraseText.m_value[0] != 0)
            {
                const PHRASE::PAUSE_TYPE pauseType = DialogLastPauseType(DialogRecordForKey(service, currentDialogKey));
                DialogInsertPhrase(DialogRecordForKey(service, currentDialogKey), phraseText, pauseType, voiceFile, pauseDuration, sfx);
                voiceFile = "";
            }

            STRING tagBody = line.After("<").Before(">");

            if (tagBody.m_value[0] != 0)
            {
                const int groupValue = DialogParseInteger(tagBody.Before("_").m_value);
                const int indexValue = DialogParseInteger(tagBody.After("_").m_value);

                currentDialogKey = (groupValue != 0 && indexValue != 0)
                    ? GetDlgKey(groupValue, indexValue)
                    : GetDlgKey(DialogParseInteger(tagBody.m_value), 0);

                DIALOG* newDialog = static_cast<DIALOG*>(CrtNew(sizeof(DIALOG)));
                if (newDialog)(void) DialogCtor(newDialog);
                DialogMapValuePair value = {currentDialogKey, 0};
                DialogTreeInsertResult result = {0, 0u, {0u, 0u, 0u}};
                (void) DialogMap_Insert(&service->m_dialogTree, &result, &value);
                result.m_node->m_dialog = newDialog;
            }

            const int radioFont = service->m_dialogRecord.m_unitVid[1];
            DialogRecordForKey(service, currentDialogKey)->m_unitVid[1] = radioFont;
            const int normalFont = service->m_dialogRecord.m_unitVid[0];
            DialogRecordForKey(service, currentDialogKey)->m_unitVid[0] = normalFont;
            phraseText = "";
            voiceFile = "";
            sfx = 0;
            pauseDuration = 0;
            DIALOG* currentDialog = DialogRecordForKey(service, currentDialogKey);
            currentDialog->m_coor = service->m_dialogRecord.m_coor;
            const unsigned int faceDirection = service->m_dialogRecord.m_faceDirection;
            const unsigned int radio = service->m_dialogRecord.m_radio;
            currentDialog = DialogRecordForKey(service, currentDialogKey);
            currentDialog->m_radio = radio;
            currentDialog->m_faceDirection = faceDirection;
            continue;
        }

        if (strcmp(text, "<pause>") == 0 || strcmp(text, "<click or pause>") == 0)
        {
            phraseText.RemoveEndChars(" \n\r\t");
            if (currentDialogKey != 0)
            {
                DIALOG* currentDialog = DialogRecordForKey(service, currentDialogKey);
                DialogInsertPhrase(currentDialog, phraseText,
                    strcmp(text, "<pause>") == 0?PHRASE::PAUSE:PHRASE::CLICK_OR_PAUSE,
                    voiceFile, pauseDuration, sfx);
                pauseDuration = 0u;
                sfx = 0u;
                voiceFile = "";
            }
            phraseText = "";
            continue;
        }

        if (strncmp(text, "<Speech=", 8u) == 0)
        {
            voiceFile = line.After("<Speech=").Before(">");
            continue;
        }

        if (strncmp(text, "<Font=", 6u) == 0)
        {
            if (currentDialogKey != 0)
            {
                const int font = DialogParseInteger(line.After("<Font=").Before(">").m_value);
                DialogRecordForKey(service, currentDialogKey)->m_unitVid[0] = font;
                DialogRecordForKey(service, currentDialogKey)->m_unitVid[1] = font;
            }
            else service->m_dialogRecord.m_unitVid[0] = DialogParseInteger(line.After("<Font=").Before(">").m_value);
            continue;
        }

        if (strncmp(text, "<DefaultFontForRadio=", 21u) == 0)
        {
            service->m_dialogRecord.m_unitVid[1] = DialogParseInteger(line.After("<DefaultFontForRadio=").Before(">").m_value);
            continue;
        }

        if (strncmp(text, "<SFX=", 5u) == 0)
        {
            sfx = DialogParseInteger(line.After("<SFX=").Before(">").m_value);
            continue;
        }

        if (strncmp(text, "<PauseDuration=", 15u) == 0)
        {
            pauseDuration = DialogParseInteger(line.After("<PauseDuration=").Before(">").m_value);
            if (currentDialogKey == 0) DialogLogDeprecatedPauseDuration();
            continue;
        }

        if (strncmp(text, "<Coor=", 6u) == 0)
        {
            float x, y, z;
            (void) sscanf(line.After("<Coor=").Before(">").m_value, "%f, %f, %f", &x, &y, &z);
            if (currentDialogKey != 0)
            {
                DIALOG* currentDialog = DialogRecordForKey(service, currentDialogKey);
                currentDialog->m_coor.x = x;
                currentDialog->m_coor.y = y;
                currentDialog->m_coor.z = z;
            }
            else
            {
                service->m_dialogRecord.m_coor.x = x;
                service->m_dialogRecord.m_coor.y = y;
                service->m_dialogRecord.m_coor.z = z;
            }
            continue;
        }

        if (strncmp(text, "<Icon=", 6u) == 0)
        {
            int groupText, indexText;
            (void) sscanf(line.After("<Icon=").Before(">").m_value, "%d, %d", &groupText, &indexText);
            if (currentDialogKey != 0)
            {
                DIALOG* currentDialog = DialogRecordForKey(service, currentDialogKey);
                currentDialog->m_radio = static_cast<unsigned int>(groupText);
                currentDialog->m_faceDirection = static_cast<unsigned int>(indexText);
            }
            else
            {
                service->m_dialogRecord.m_radio = static_cast<unsigned int>(groupText);
                service->m_dialogRecord.m_faceDirection = static_cast<unsigned int>(indexText);
            }
            continue;
        }

        phraseText += fetchedLine;
    }

    phraseText.RemoveEndChars(" \n\r\t");
    if (currentDialogKey != 0 && phraseText.m_value[0] != 0)
    {
        const PHRASE::PAUSE_TYPE pauseType = DialogLastPauseType(DialogRecordForKey(service, currentDialogKey));
        DialogInsertPhrase(DialogRecordForKey(service, currentDialogKey), phraseText, pauseType, voiceFile, pauseDuration, sfx);
    }

    (void) std::fclose(file);
}

void __cdecl DialogComment_Construct(
    COMMENT* destination, const COMMENT* source)
{
    if (destination != 0)
        ::new (static_cast<void*>(destination)) COMMENT(*source);
}



void Dialog_Dtor(void* self)
{
    static_cast<DIALOG*>(self)->~DIALOG();
}

PHRASE::PHRASE(const STRING& text, PAUSE_TYPE pauseType,
    const STRING& voiceFile, int sfx, int pauseDuration)
{
    m_sfx = sfx;
    m_text = text;
    m_text.RemoveEndChars(" \n\r\t");
    m_pauseType = pauseType;
    m_voiceFile = voiceFile;
    m_duration = pauseDuration;
}

COMMENT::COMMENT(STRING text, int dialogKey)
    : m_ownedString(text), m_dialogKey(dialogKey)
{
}

int  zDlgSrv_GetDlgKey(void* self, int group, int index)
{
    return static_cast<zDlgSrv*>(self)->GetDlgKey(group, index);
}
int  zDlgSrv_ReleaseData(void* self)
{
    return static_cast<zDlgSrv*>(self)->ReleaseData();
}
bool  zDlgSrv_IsDlgNumValid(void* self, int key)
{
    return static_cast<zDlgSrv*>(self)->IsDlgNumValid(key);
}
DIALOG*  zDlgSrv_GetDialog(void* self, int key)
{
    return static_cast<zDlgSrv*>(self)->GetDialog(key);
}
void  zDlgSrv_LoadDialogs(void* self, const STRING& source)
{
    static_cast<zDlgSrv*>(self)->LoadDialogs(source);
}
