#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"
#include <cstring>
#include <cstdlib>
#include "list.h"
#include "mylib.h"
#include "zRPGFrameWork.h"
#include "zRPGHero.h"
#include "zRPGWeapon.h"
#include "zRPGAmmo.h"
#include "zRPGArmor.h"
#include "zRPGImplant.h"
#include "zRPGEquip.h"
#include "zRPGQuest.h"
#include "zRPGBaseObject.h"
#include "zm/IniParser/zIniParser.h"
#include "zm/IniParser/zItemInfo.h"
#include "zm/Survive/zSurvive.h"
#include "zm/Game/Campaign/zCampaign.h"
#include "zm/Inventory/zInventoryItem.h"
#include "zm/Utils/String/zString.h"
#include "myerror.h"
#include <new>

#include "basetype.h"

bool g_bGlobalRelease = false;

template<class Value>
struct zFWTreeNode
{
    zFWTreeNode* left;
    zFWTreeNode* parent;
    zFWTreeNode* right;
    int key;
    Value value;
    unsigned int color;
};

template<class Value>
struct zFWTreeInsertResult
{
    zFWTreeNode<Value>* node;
    unsigned char inserted;
    unsigned char reserved[3];
};

template<class Value>
zFWTreeNode<Value>* zFWTreeMap<Value>::_Nil = nullptr;

template<class Value>
unsigned int zFWTreeMap<Value>::_Nilrefs = 0;

template<class Value>
static zFWTreeNode<Value>* FWTreeAllocateNode()
{
    return static_cast<zFWTreeNode<Value>*>(CrtNew(sizeof(zFWTreeNode<Value>)));
}

template<class Value>
static void FWTreeInit(zFWTreeMap<Value>* tree)
{
    zFWTreeNode<Value>* newSentinel = FWTreeAllocateNode<Value>();
    newSentinel->parent = 0;
    newSentinel->color = 1u;
    unsigned int lockWord = 0u;
    Msvc6Runtime::StaticLockConstruct(&lockWord);
    zFWTreeNode<Value>* unusedSentinel = newSentinel;
    if (zFWTreeMap<Value>::_Nil == 0u)
    {
        zFWTreeMap<Value>::_Nil = newSentinel;
        newSentinel->left = 0;
        newSentinel->right = 0;
        unusedSentinel = 0;
    }
    ++zFWTreeMap<Value>::_Nilrefs;
    Msvc6Runtime::StaticLockDestruct(&lockWord);
    if (unusedSentinel)
        CrtFreePointer(unusedSentinel);
    zFWTreeNode<Value>* header = FWTreeAllocateNode<Value>();
    header->parent = zFWTreeMap<Value>::_Nil;
    header->color = 0u;
    header->left = header;
    header->right = header;
    tree->m_head = header;
    tree->m_size = 0u;
}

template<class Value>
static zFWTreeNode<Value>* FWTreeMinimum(zFWTreeNode<Value>* node, zFWTreeNode<Value>* nil)
{
    while (node->left != nil)
        node = node->left;
    return node;
}

template<class Value>
static zFWTreeNode<Value>* FWTreeMaximum(zFWTreeNode<Value>* node, zFWTreeNode<Value>* nil)
{
    while (node->right != nil)
        node = node->right;
    return node;
}

template<class Value>
static zFWTreeNode<Value>* FWTreeSuccessor(zFWTreeNode<Value>* node, zFWTreeNode<Value>* nil)
{
    if (node->right != nil)
        return FWTreeMinimum(node->right, nil);
    zFWTreeNode<Value>* parent = node->parent;
    zFWTreeNode<Value>* current = node;
    while (current == parent->right)
    {
        current = parent;
        parent = parent->parent;
    }
    if (current->right != parent)
        current = parent;
    return current;
}

template<class Value>
static zFWTreeNode<Value>* FWTreePredecessor(zFWTreeNode<Value>* node, zFWTreeNode<Value>* header, zFWTreeNode<Value>* nil)
{
    if (node->left != nil)
        return FWTreeMaximum(node->left, nil);
    zFWTreeNode<Value>* parent = node->parent;
    while (parent != header && node == parent->left)
    {
        node = parent;
        parent = parent->parent;
    }
    return parent;
}

template<class Value>
static void FWTreeIncrement(zFWTreeNode<Value>*& node, zFWTreeNode<Value>* nil)
{
    if (node->right != nil)
    {
        node = node->right;
        while (node->left != nil)
            node = node->left;
        return;
    }
    zFWTreeNode<Value>* parent = node->parent;
    while (node == parent->right)
    {
        node = parent;
        parent = parent->parent;
    }
    if (node->right != parent)
        node = parent;
}

template<class Value>
static void FWTreeRotateLeft(zFWTreeMap<Value>* tree, zFWTreeNode<Value>* node, zFWTreeNode<Value>* nil)
{
    zFWTreeNode<Value>* pivot = node->right;
    node->right = pivot->left;
    if (pivot->left != nil)
        pivot->left->parent = node;
    pivot->parent = node->parent;
    if (node == tree->m_head->parent)
        tree->m_head->parent = pivot;
    else if (node == node->parent->left)
        node->parent->left = pivot;
    else node->parent->right = pivot;
    pivot->left = node;
    node->parent = pivot;
}

template<class Value>
static void FWTreeRotateRight(zFWTreeMap<Value>* tree, zFWTreeNode<Value>* node, zFWTreeNode<Value>* nil)
{
    zFWTreeNode<Value>* pivot = node->left;
    node->left = pivot->right;
    if (pivot->right != nil)
        pivot->right->parent = node;
    pivot->parent = node->parent;
    if (node == tree->m_head->parent)
        tree->m_head->parent = pivot;
    else if (node == node->parent->right)
        node->parent->right = pivot;
    else node->parent->left = pivot;
    pivot->right = node;
    node->parent = pivot;
}

template<class Value>
static zFWTreeNode<Value>* FWTreeLowerBound(const zFWTreeMap<Value>* tree, int key, zFWTreeNode<Value>* nil)
{
    zFWTreeNode<Value>* candidate = tree->m_head;
    zFWTreeNode<Value>* node = tree->m_head->parent;
    while (node != nil)
    {
        if (node->key<key)
            node = node->right;
        else
        {
            candidate = node;
            node = node->left;
        }
    }
    return candidate;
}

template<class Value>
static zFWTreeNode<Value>* FWTreeFind(const zFWTreeMap<Value>* tree, int key, zFWTreeNode<Value>* nil)
{
    zFWTreeNode<Value>* node = FWTreeLowerBound(tree, key, nil);
    return (node == tree->m_head || key<node->key)?tree->m_head:node;
}

template<class Value>
static zFWTreeNode<Value>* FWTreeInsertNode(zFWTreeMap<Value>* tree, zFWTreeNode<Value>* hint, zFWTreeNode<Value>* parent,
                                     int key, Value value, zFWTreeNode<Value>* nil)
{
    zFWTreeNode<Value>* node = FWTreeAllocateNode<Value>();
    node->parent = parent;
    node->color = 0u;
    node->left = nil;
    node->right = nil;
    node->key = key;
    node->value = value;
    ++tree->m_size;
    const bool putLeft = (parent == tree->m_head) || (hint != nil) || key<parent->key;
    if (putLeft)
    {
        parent->left = node;
        if (parent == tree->m_head)
        {
            tree->m_head->parent = node;
            tree->m_head->right = node;
        }
        else if (parent == tree->m_head->left)
        {
            tree->m_head->left = node;
        }
    }
    else
    {
        parent->right = node;
        if (parent == tree->m_head->right)
            tree->m_head->right = node;
    }
    zFWTreeNode<Value>* current = node;
    while (current != tree->m_head->parent && current->parent->color == 0u)
    {
        zFWTreeNode<Value>* parentNode = current->parent;
        zFWTreeNode<Value>* grandparent = parentNode->parent;
        if (parentNode == grandparent->left)
        {
            zFWTreeNode<Value>* uncle = grandparent->right;
            if (uncle->color == 0u)
            {
                parentNode->color = 1u;
                uncle->color = 1u;
                grandparent->color = 0u;
                current = grandparent;
            }
            else
            {
                if (current == parentNode->right)
                {
                    current = parentNode;
                    FWTreeRotateLeft(tree, current, nil);
                    parentNode = current->parent;
                    grandparent = parentNode->parent;
                }
                parentNode->color = 1u;
                grandparent->color = 0u;
                FWTreeRotateRight(tree, grandparent, nil);
            }
        }
        else
        {
            zFWTreeNode<Value>* uncle = grandparent->left;
            if (uncle->color == 0u)
            {
                parentNode->color = 1u;
                uncle->color = 1u;
                grandparent->color = 0u;
                current = grandparent;
            }
            else
            {
                if (current == parentNode->left)
                {
                    current = parentNode;
                    FWTreeRotateRight(tree, current, nil);
                    parentNode = current->parent;
                    grandparent = parentNode->parent;
                }
                parentNode->color = 1u;
                grandparent->color = 0u;
                FWTreeRotateLeft(tree, grandparent, nil);
            }
        }
    }
    tree->m_head->parent->color = 1u;
    return node;
}

template<class Value>
static zFWTreeInsertResult<Value> FWTreeInsertUnique(zFWTreeMap<Value>* tree, int key, Value value, zFWTreeNode<Value>* nil)
{
    zFWTreeNode<Value>* node = tree->m_head->parent;
    zFWTreeNode<Value>* parent = tree->m_head;
    bool insertLeft = true;
    while (node != nil)
    {
        parent = node;
        insertLeft = key<node->key;
        node = insertLeft?node->left:node->right;
    }
    bool shouldInsert = tree->m_multi != 0u;
    zFWTreeNode<Value>* existingNode = parent;
    if (!shouldInsert)
    {
        if (insertLeft)
        {
            if (parent == tree->m_head->left)
                shouldInsert = true;
            else
            {
                existingNode = FWTreePredecessor(parent, tree->m_head, nil);
                shouldInsert = existingNode->key<key;
            }
        }
        else shouldInsert = parent->key<key;
    }
    zFWTreeInsertResult<Value> result =
    {
        existingNode, 0u,
        {
            0u, 0u, 0u
        }
    }
    ;
    if (shouldInsert)
    {
        result.node = FWTreeInsertNode(tree, node, parent, key, value, nil);
        result.inserted = 1u;
    }
    return result;
}

template<class Value>
static zFWTreeNode<Value>* FWTreeEraseNode(zFWTreeMap<Value>* tree, zFWTreeNode<Value>* eraseNode, zFWTreeNode<Value>* nil)
{
    zFWTreeNode<Value>* next = FWTreeSuccessor(eraseNode, nil);
    zFWTreeNode<Value>* releaseNode = eraseNode;
    zFWTreeNode<Value>* replacement = nil;
    zFWTreeNode<Value>* replacementParent = tree->m_head;
    if (releaseNode->left == nil)
        replacement = releaseNode->right;
    else if (releaseNode->right == nil)
        replacement = releaseNode->left;
    else
    {
        releaseNode = releaseNode->right;
        while (releaseNode->left != nil)
            releaseNode = releaseNode->left;
        replacement = releaseNode->right;
    }
    unsigned int runtimeGuard = 0u;
    Msvc6Runtime::StaticLockConstruct(&runtimeGuard);
    if (releaseNode != eraseNode)
    {
        eraseNode->left->parent = releaseNode;
        releaseNode->left = eraseNode->left;
        if (releaseNode != eraseNode->right)
        {
            replacementParent = releaseNode->parent;
            if (replacement != nil)
                replacement->parent = replacementParent;
            replacementParent->left = replacement;
            releaseNode->right = eraseNode->right;
            eraseNode->right->parent = releaseNode;
        }
        else replacementParent = releaseNode;
        if (tree->m_head->parent == eraseNode)
            tree->m_head->parent = releaseNode;
        else if (eraseNode->parent->left == eraseNode)
            eraseNode->parent->left = releaseNode;
        else eraseNode->parent->right = releaseNode;
        releaseNode->parent = eraseNode->parent;
        const unsigned int oldColor = eraseNode->color;
        eraseNode->color = releaseNode->color;
        releaseNode->color = oldColor;
        releaseNode = eraseNode;
    }
    else
    {
        replacementParent = releaseNode->parent;
        if (replacement != nil)
            replacement->parent = replacementParent;
        if (tree->m_head->parent == eraseNode)
            tree->m_head->parent = replacement;
        else if (replacementParent->left == eraseNode)
            replacementParent->left = replacement;
        else replacementParent->right = replacement;
        if (tree->m_head->left == eraseNode)
        {
            tree->m_head->left = (eraseNode->right == nil)?replacementParent:FWTreeMinimum(replacement, nil);
        }
        if (tree->m_head->right == eraseNode)
        {
            tree->m_head->right = (eraseNode->left == nil)?replacementParent:FWTreeMaximum(replacement, nil);
        }
    }
    if (releaseNode->color == 1u)
    {
        while (replacement != tree->m_head->parent && replacement->color == 1u)
        {
            if (replacement == replacementParent->left)
            {
                zFWTreeNode<Value>* sibling = replacementParent->right;
                if (sibling->color == 0u)
                {
                    sibling->color = 1u;
                    replacementParent->color = 0u;
                    FWTreeRotateLeft(tree, replacementParent, nil);
                    sibling = replacementParent->right;
                }
                if (sibling->left->color == 1u && sibling->right->color == 1u)
                {
                    sibling->color = 0u;
                    replacement = replacementParent;
                    replacementParent = replacementParent->parent;
                }
                else
                {
                    if (sibling->right->color == 1u)
                    {
                        sibling->left->color = 1u;
                        sibling->color = 0u;
                        FWTreeRotateRight(tree, sibling, nil);
                        sibling = replacementParent->right;
                    }
                    sibling->color = replacementParent->color;
                    replacementParent->color = 1u;
                    sibling->right->color = 1u;
                    FWTreeRotateLeft(tree, replacementParent, nil);
                    replacement = tree->m_head->parent;
                    break;
                }
            }
            else
            {
                zFWTreeNode<Value>* sibling = replacementParent->left;
                if (sibling->color == 0u)
                {
                    sibling->color = 1u;
                    replacementParent->color = 0u;
                    FWTreeRotateRight(tree, replacementParent, nil);
                    sibling = replacementParent->left;
                }
                if (sibling->right->color == 1u && sibling->left->color == 1u)
                {
                    sibling->color = 0u;
                    replacement = replacementParent;
                    replacementParent = replacementParent->parent;
                }
                else
                {
                    if (sibling->left->color == 1u)
                    {
                        sibling->right->color = 1u;
                        sibling->color = 0u;
                        FWTreeRotateLeft(tree, sibling, nil);
                        sibling = replacementParent->left;
                    }
                    sibling->color = replacementParent->color;
                    replacementParent->color = 1u;
                    sibling->left->color = 1u;
                    FWTreeRotateRight(tree, replacementParent, nil);
                    replacement = tree->m_head->parent;
                    break;
                }
            }
        }
        replacement->color = 1u;
    }
    Msvc6Runtime::StaticLockDestruct(&runtimeGuard);
    CrtFreePointer(releaseNode);
    --tree->m_size;
    return next;
}

template<class Value>
static void FWTreeDestroySubtree(zFWTreeNode<Value>* node, zFWTreeNode<Value>* nil)
{
    while (node != nil)
    {
        FWTreeDestroySubtree(node->right, nil);
        zFWTreeNode<Value>* left = node->left;
        CrtFreePointer(node);
        node = left;
    }
}

template<class Value>
static void FWTreeClear(zFWTreeMap<Value>* tree, zFWTreeNode<Value>* nil)
{
    if (tree->m_size != 0u)
    {
        FWTreeDestroySubtree(tree->m_head->parent, nil);
        tree->m_head->parent = nil;
        tree->m_size = 0u;
        tree->m_head->left = tree->m_head;
        tree->m_head->right = tree->m_head;
    }
    else
    {
        zFWTreeNode<Value>* node = tree->m_head->left;
        while (node != tree->m_head)
        {
            zFWTreeNode<Value>* old = node;
            FWTreeIncrement(node, nil);
            (void)FWTreeEraseNode(tree, old, nil);
        }
    }
}

template<class Value>
static void FWTreeReleaseStorage(zFWTreeMap<Value>* tree)
{
    zFWTreeNode<Value>* nil = zFWTreeMap<Value>::_Nil;
    if (tree->m_head)
    {
        FWTreeClear(tree, nil);
        CrtFreePointer(tree->m_head);
        tree->m_head = 0;
        tree->m_size = 0u;
    }
    unsigned int lockWord = 0u;
    Msvc6Runtime::StaticLockConstruct(&lockWord);
    const unsigned int remainingMaps = zFWTreeMap<Value>::_Nilrefs-1u;
    zFWTreeMap<Value>::_Nilrefs = remainingMaps;
    zFWTreeNode<Value>* releasedSentinel = 0;
    if (remainingMaps == 0u)
    {
        releasedSentinel = zFWTreeMap<Value>::_Nil;
        zFWTreeMap<Value>::_Nil = nullptr;
    }
    Msvc6Runtime::StaticLockDestruct(&lockWord);
    if (releasedSentinel)
        CrtFreePointer(releasedSentinel);
}

template<class Value>
zFWTreeMap<Value>::zFWTreeMap()
    : m_multi(0)
{
    FWTreeInit(this);
}

template<class Value>
zFWTreeMap<Value>::~zFWTreeMap() noexcept(false)
{
    FWTreeReleaseStorage(this);
}

iTextDB* zRPGFrameWork::AsTextDB()
{
    return static_cast<iTextDB*>(this);
}

void zRPGFrameWork::SetPlayer(iPlayer* player)
{
    m_pPlayer = player;
}

int zRPGFrameWork::GetItemsCnt()
{
    return m_iItemsNum;
}

void zRPGFrameWork::SetGammaItemDisabled(int red, int green, int blue)
{
    m_disabledGammaRed = red;
    m_disabledGammaGreen = green;
    m_disabledGammaBlue = blue;
}

void zRPGFrameWork::GetGammaItemDisabled(int* red, int* green, int* blue)
{
    *red = m_disabledGammaRed;
    *green = m_disabledGammaGreen;
    *blue = m_disabledGammaBlue;
}

void* zRPGFrameWork::QueryInterface(int interfaceId) const
{
    if (interfaceId == 23)
        return const_cast<iRPGFrameWork*>(static_cast<const iRPGFrameWork*>(this));
    if (interfaceId == 28)
        return const_cast<iTextDB*>(static_cast<const iTextDB*>(this));
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 51);
    return 0;
}

void zRPGFrameWork::operator delete(void* pointer)
{
    CrtFreePointer(pointer);
}

int zRPGFrameWork::iRelease()
{
    if (PtrToU32(this) != 0u)
    {
        this->~zRPGFrameWork();
        zRPGFrameWork::operator delete(this);
    }
    return 0;
}

iRPGObj* zRPGFrameWork::GetObjCopyS(const char* name, bool logIfMissing)
{
    iRPGObj* source = GetObjByNamee(name, logIfMissing);
    return source ? GetObjCopy(source) : 0;
}

iRPGObj* zRPGFrameWork::GetObjCopy(iRPGObj* sourceObject)
{
    const int type = sourceObject->GetGameObj();
    iRPGObj* copiedObject = GetNewRPGObj(static_cast<OBJ_TYPE>(type), false);
    copiedObject->CopyParametersFrom(sourceObject);
    return copiedObject;
}

int zRPGFrameWork::GetSkillValue(int objectId, int skill)
{
    return GetSkillValue(GetItemById(objectId), skill);
}

int zRPGFrameWork::GetSkillValue(iRPGObj* object, int skill)
{
    return object ? object->GetSkillValue(skill) : 0;
}

const char* zRPGFrameWork::GetStringValue(int kind, int index)
{
    const char** sequence = 0;
    if (kind == RPGHERO_RANK)
        sequence = m_ranks->GetData();
    else if (kind == RPG_HEROSKILL_TEXT)
        sequence = m_heroSkillNames->GetData();
    if (!sequence)
        return 0;
    int stringIndex = 0;
    if (sequence[0])
    {
        while (stringIndex <= index)
        {
            ++stringIndex;
            if (!sequence[stringIndex])
                break;
        }
    }
    return sequence[stringIndex-1];
}

const char** zRPGFrameWork::GetHeroInfo(int objectId)
{
    iRPGObj* item = GetItemById(objectId);
    if (!item)
        return 0;
    const char* heroName = item->GetName();
    return m_heroInfo->GetData(heroName);
}

void zRPGFrameWork::SetPlayerName(const char* name)
{
    m_playerName.Set(name);
}

const char* zRPGFrameWork::GetPlayerName()
{
    return static_cast<const char*>(m_playerName);
}

iRPGObj* zRPGFrameWork::FirstObject(int objectMask, int typeOrClass, int extraId)
{
    m_objectFilterMask = objectMask;
    m_objectFilterValue = typeOrClass;
    m_objectIteratorIndex = -1;
    m_objectFilterDirection = extraId;
    return NextObject();
}

int zRPGFrameWork::GetArcadeMod_()
{
    return m_pPlayer?m_pPlayer->GetArcadeMod_():0;
}

void zRPGFrameWork::WinLevel()
{
    if (m_pPlayer)
        m_pPlayer->WinLevel();
}

void zRPGFrameWork::PlaySFXCurArmy(int sfxId)
{
    if (m_pPlayer)
        m_pPlayer->PlaySFx(sfxId, false);
}

int zRPGFrameWork::ScriptGetNFunction(const char* functionName)
{
    return m_pPlayer?m_pPlayer->ScriptGetNFunction(functionName):-1;
}

int zRPGFrameWork::ScriptRun(int scriptId, const char* functionName, int argument1, int argument2, int argument3)
{
    return m_pPlayer?m_pPlayer->ScriptRun(scriptId, functionName, argument1, argument2, argument3):0;
}

int zRPGFrameWork::GetNoSprites(int vidIndex, unsigned int army)
{
    return m_pPlayer?m_pPlayer->GetNoSprites(vidIndex, army):0;
}

iRPGObj* zRPGFrameWork::GetItemById(int objectId)
{
    const int itemIndex = GetItemPosById(objectId);
    if (itemIndex<0)
        return 0;
    return m_apItems[itemIndex];
}

int zRPGFrameWork::GetCurFileVer()
{
    if (m_iIniFilesCnt <= 0)
        (void)DebugAssert(5u, "m_iIniFilesCnt > 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 1161u);
    return m_aIniFileVersions[m_iIniFilesCnt-1];
}

void zRPGFrameWork::SaveInfosToFile(const char* fileName)
{
    if (!fileName)
        (void)DebugAssert(4u, "_sFileName", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 1060u);
    m_objectInfo->SaveInfosToFile(fileName);
}

void zRPGFrameWork::SetPerkForAllHeroes(int perkId)
{
    for (int itemIndex = 0;itemIndex<m_iItemsNum;++itemIndex)
    {
        if (m_apItems[itemIndex]->GetGameObj() == TYPE_HERO)
            m_apItems[itemIndex]->SetPerkSkill(perkId);
    }
}

void zRPGFrameWork::SetObjInfo(const iRPGObj* obj, const char* text)
{
    if (!obj)
        DebugAssert(5u, "item", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 697u);
    SetObjInfo(obj->GetName(), text);
}

const char* zRPGFrameWork::GetObjInfo(const iRPGObj* obj, bool expand)
{
    if (!obj)
        DebugAssert(5u, "item", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 686u);
    const int infoLineCount = GetItemInfo(obj, m_infoLines, expand);
    return GetSpeciallyDecodedString(const_cast<const char**>(m_infoLines), infoLineCount);
}

const char* zRPGFrameWork::GetStrIniString(const char* section, const char* key)
{
    zString& scratchText = m_textScratch;
    m_pPlayer->GetStrIniString(&scratchText, section, key);
    return scratchText.IsEmpty()?0:static_cast<const char*>(scratchText);
}

const char* zRPGFrameWork::GetObjInfo(const char* itemName)
{
    const char** const lines = m_objectInfo->GetData(itemName);
    int infoLineCount = 0;
    if (lines)
        while (lines[infoLineCount]) ++infoLineCount;
    return GetSpeciallyDecodedString(lines, infoLineCount);
}

const char* zRPGFrameWork::GetFirstString(int vid, int direction)
{
    zString& key = m_textScratch;
    key.Format("%d_%d", vid, direction);
    return m_VidDirs->GetFirstValuableString(static_cast<const char*>(key));
}

void zRPGFrameWork::EndDataStringsReading()
{
}

const char** zRPGFrameWork::GetExtInfo(int selector, int firstKeyComponent, int secondKeyComponent)
{
    zString& key = m_textScratch;
    key.Format("%d_%d", firstKeyComponent, secondKeyComponent);
    return GetExtInfo(selector, static_cast<const char*>(key));
}

int zRPGFrameWork::GetFileVer(const char* fileName)
{
    for (int fileIndex = 0;fileIndex<m_iIniFilesCnt;++fileIndex)
    {
        if (static_cast<const char*>(m_loadedIniFileNames[fileIndex]) == fileName)
            return m_aIniFileVersions[fileIndex];
    }
    return 0;
}

bool zRPGFrameWork::ReleaseObject(iRPGObj* obj)
{
    if (!obj)
    {
        DebugAssert(5u, "_pObject", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 835u);
        return false;
    }
    const int key = static_cast<int>(obj->GetExtType());
    const int i = GetItemPosById(key);
    if (i<0)
    {
        DebugAssert(5u, "pos >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 841u);
        return false;
    }
    return ReleaseObject(i);
}

const char** zRPGFrameWork::GetExtInfo(int selector, const char* arg)
{
    if (!m_VidDirs)
        DebugAssert(5u, "m_VidDirs", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 572u);
    if (selector == 0)
        return m_VidDirs->GetData(arg);
    if (selector == 1)
        return m_mouseTips->GetData(arg);
    DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 582u);
    return 0;
}

iRPGObj* zRPGFrameWork::GetGrndObject(int vidNumber, int wantedDirection)
{
    for (int itemIndex = 0;itemIndex<m_iItemsNum;++itemIndex)
    {
        if (!m_apItems[itemIndex]->IsBaseObject())
            continue;
        const int vid = m_apItems[itemIndex]->GetVidNum(0);
        const int direction = m_apItems[itemIndex]->GetDirNum(0);
        if (vid == vidNumber && (wantedDirection == -1 || direction == wantedDirection))
            return m_apItems[itemIndex];
    }
    return 0;
}

void zRPGFrameWork::ResetScriptFunctions()
{
    zRPGObj::ResetScriptFunctions();
    zRPGHero::ResetScriptFunctions();
}

void zRPGFrameWork::AddDataString(const char* argument)
{
    switch (static_cast<unsigned int>(m_readingGroup))
    {
        case 2u:
            m_objectInfo->AddDataString(argument);
        return;
        case 1u:
            m_heroInfo->AddDataString(argument);
        return;
        case 3u:
            m_ranks->AddDataString(argument);
        return;
        case 4u:
            m_heroSkillNames->AddDataString(argument);
        return;
        case 5u:
            m_VidDirs->AddDataString(argument);
        return;
        case 6u:
            m_mouseTips->AddDataString(argument);
        return;
        default:
            DebugAssert(4u, "0",                      "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 547u);
        return;
    }
}

void zRPGFrameWork::SetFileVer(const char* fileName, int fileVersion)
{
    int versionSlot = -1;
    for (int fileIndex = 0;fileIndex<m_iIniFilesCnt;++fileIndex)
    {
        const char* loadedFileName = static_cast<const char*>(m_loadedIniFileNames[fileIndex]);
        if (CStringEqualUnchecked(loadedFileName, fileName))
        {
            versionSlot = fileIndex;
            break;
        }
    }
    if (versionSlot == -1)
    {
        if (m_iIniFilesCnt >= 50)
            DebugAssert(5u, "m_iIniFilesCnt < MAX_LOADED_FILES", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 1128u);
        versionSlot = m_iIniFilesCnt++;
    }
    m_loadedIniFileNames[versionSlot].Set(fileName);
    m_aIniFileVersions[versionSlot] = fileVersion;
}

const char* zRPGFrameWork::GetSpeciallyDecodedString(const char** lines, int maxLines)
{
    if (!lines || maxLines == 0)
        return 0;
    zString& decodedText = m_textScratch;
    decodedText.Set("");
    int pendingLineBreaks = 0;
    bool firstLine = true;
    for (int lineIndex = 0;lines[lineIndex] && (maxLines == -1 || lineIndex<maxLines);++lineIndex)
    {
        if (!firstLine)
            ++pendingLineBreaks;
        firstLine = false;
        const char* const text = lines[lineIndex];
        if (text && *text)
        {
            while (pendingLineBreaks>0)
            {
                decodedText += "\r\n";
                --pendingLineBreaks;
            }
            pendingLineBreaks = 0;
            decodedText += lines[lineIndex];
        }
    }
    return static_cast<const char*>(decodedText);
}

void zRPGFrameWork::SetObjInfo(const char* itemName, const char* text)
{
    if (!itemName)
        DebugAssert(5u, "_sItemName", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 706u);
    m_objectInfo->ReleaseData(itemName);
    if (!text)
        return;
    m_objectInfo->BeginReading(itemName);
    const char* current = text;
    char line[500];
    while (*current)
    {
        const char* lineBegin = current;
        while (*current && *current != '\r' && *current != '\n')
            ++current;
        const unsigned int length = static_cast<unsigned int>(current-lineBegin);
        CopyBytes(line, lineBegin, length);
        line[length] = 0;
        while (*current == '\r' || *current == '\n')
            ++current;
        m_objectInfo->AddDataString(line);
    }
}

const char* zRPGFrameWork::GetTypeName(OBJ_TYPE objectType)
{
    m_textScratch.Set("");
    switch (static_cast<unsigned int>(objectType))
    {
        case TYPE_HERO:
            case 1u: m_textScratch.Set("Character");
        break;
        case TYPE_WEAPON:
            m_textScratch.Set("Weapon");
        break;
        case TYPE_AMMO:
            m_textScratch.Set("Ammo");
        break;
        case TYPE_ARMOR:
            m_textScratch.Set("Armor");
        break;
        case TYPE_IMPLANT:
            m_textScratch.Set("Implant");
        break;
        case TYPE_EQUIP:
            m_textScratch.Set("Equip");
        break;
        case 7u:
            m_textScratch.Set("Objects");
        break;
        case TYPE_QUEST:
            m_textScratch.Set("Quest");
        break;
        case TYPE_LEVEL:
            m_textScratch.Set("Map");
        break;
        case 10u:
            m_textScratch.Set("SurviveMap");
        break;
        case 11u:
            m_textScratch.Set("Interval");
        break;
        case 12u:
            m_textScratch.Set("CampaignMap");
        break;
        default:
            DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 1029u);
        break;
    }
    return static_cast<const char*>(m_textScratch);
}

iRPGObj* zRPGFrameWork::GetObjByNamee(const char* name, bool logIfMissing)
{
    const int key = static_cast<int>(zutl::GetHashPJW(name));
    zFWTreeNode<iRPGObj*>* node = FWTreeLowerBound(&m_NamesMap, key, zFWTreeMap<iRPGObj*>::_Nil);
    iRPGObj* result = 0;
    if (node != m_NamesMap.m_head && key >= node->key)
        result = node->value;
    if (!result && logIfMissing && !(std::strcmp(name, "Empty") == 0))
    {
        iLogConsole* log = ::gpLogCons;
        (void)log->LogStr(LT_NOTICE, "Object %s was not found!!!", name);
    }
    return result;
}

bool zRPGFrameWork::BeginDataStringsReading(const char* name)
{
    unsigned int mode = 0;
    if ((std::strcmp(name, "ListOfRanks") == 0))
        mode = 3;
    else if ((std::strcmp(name, "ListOfHeroSkills") == 0))
        mode = 4;
    else if ((std::strcmp(name, "MouseTips") == 0))
        mode = 6;
    else if ((std::strcmp(name, "VidDirs") == 0))
        mode = 5;
    else return false;
    m_readingGroup = static_cast<int>(mode);
    return true;
}

bool zRPGFrameWork::BeginObjectInfoReading(const char* fieldName)
{
    iRPGObj* entry = GetObjByNamee(fieldName, false);
    if (!entry)
    {
        if (std::strstr(fieldName, "Coment2") != 0 || std::strstr(fieldName, "Description") != 0)
        {
            m_objectInfo->BeginReading(fieldName);
            m_readingGroup = 2;
            return true;
        }
    }
    else if (entry->GetGameObj() != TYPE_HERO)
    {
        m_objectInfo->BeginReading(fieldName);
        m_readingGroup = 2;
        return true;
    }
    else
    {
        m_heroInfo->BeginReading(fieldName);
        m_readingGroup = 1;
        return true;
    }
    if (m_readingGroup == 5)
    {
        m_VidDirs->BeginReading(fieldName);
        return true;
    }
    if (m_readingGroup == 6)
    {
        m_mouseTips->BeginReading(fieldName);
        return true;
    }
    return false;
}

iRPGObj* zRPGFrameWork::NextObject()
{
    for (;;)
    {
        m_objectIteratorIndex = AddSigned32Wrap(m_objectIteratorIndex, 1);
        if (m_objectIteratorIndex >= m_iItemsNum)
            return 0;
        iRPGObj* const object = m_apItems[m_objectIteratorIndex];
        bool matches = false;
        if (m_objectFilterMask&1)
            matches = object->IsBaseObject();
        if (m_objectFilterMask&2)
        {
            const bool nonBase = !object->IsBaseObject();
            matches = matches || nonBase;
        }
        if (matches && (m_objectFilterMask&0x20))
            matches = object->GetGameObj() == m_objectFilterValue;
        if (matches && (m_objectFilterMask&0x10))
            matches = object->GetAddMoney() == m_objectFilterValue;
        if (matches && (m_objectFilterMask&4))
            matches = object->GetVidNum(0) == m_objectFilterValue;
        const int direction = object->GetDirNum(0);
        if (matches && ((m_objectFilterMask&8) == 0 || direction == -1 || direction == m_objectFilterDirection))
            return object;
    }
}
template<class Object>
static Object* CreateRPGInfo()
{
    Object* const storage = static_cast<Object*>(CrtNew(sizeof(Object)));
    if (!storage)
        return nullptr;
    try
    {
        return new (storage) Object();
    }
    catch (...)
    {
        CrtFreePointer(storage);
        throw;
    }
}

zRPGFrameWork::zRPGFrameWork()
    : m_apItems(nullptr), m_iItemsNum(0), m_itemsCapacity(0),
      m_readingGroup(0), m_objectIteratorIndex(0), m_objectFilterMask(0),
      m_objectFilterValue(0), m_objectFilterDirection(0),
      m_disabledGammaRed(0), m_disabledGammaGreen(0), m_disabledGammaBlue(0),
      m_iIniFilesCnt(0), m_pPlayer(nullptr)
{
    m_heroInfo = CreateRPGInfo<zItemInfoList>();
    m_objectInfo = CreateRPGInfo<zItemInfoList>();
    m_VidDirs = CreateRPGInfo<zItemInfoList>();
    m_mouseTips = CreateRPGInfo<zItemInfoList>();
    m_ranks = CreateRPGInfo<zItemInfo>();
    m_heroSkillNames = CreateRPGInfo<zItemInfo>();
    m_infoLines = static_cast<char**>(CrtAllocate(122u * sizeof(*m_infoLines)));
    for (unsigned int index = 0; index < 122; ++index)
        m_infoLines[index] = static_cast<char*>(CrtAllocate(200));
}

void __cdecl GetNew_zRPGFrameWork(iRPGFrameWork** outObject)
{
    zRPGFrameWork* object = static_cast<zRPGFrameWork*>(CrtNew(sizeof(zRPGFrameWork)));
    if (object)
    {
        try
        {
            new (object) zRPGFrameWork();
        }
        catch (...)
        {
            CrtFreePointer(object);
            throw;
        }
    }
    *outObject = object;
}

int zRPGFrameWork::GetItemPosById(int objectId)
{
    if (objectId <= 0)
        return -1;
    int firstIndex = 0, lastIndex = m_iItemsNum-1;
    while (firstIndex <= lastIndex)
    {
        const int middleIndex = (firstIndex+lastIndex)/2;
        iRPGObj* object = m_apItems[middleIndex];
        const int currentId = static_cast<int>(object->GetExtType());
        if (currentId == objectId)
            return middleIndex;
        if (currentId>objectId)
            lastIndex = middleIndex-1;
        else firstIndex = middleIndex+1;
    }
    return -1;
}

bool zRPGFrameWork::ReleaseObject(int objectIndex)
{
    if (objectIndex<0 || objectIndex >= m_iItemsNum)
        DebugAssert(5u, "_iNum >=0 && _iNum < m_iItemsNum", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 862u);
    const bool isQuest = m_apItems[objectIndex]->GetGameObj() == TYPE_QUEST;
    const bool hasTreeKey = m_apItems[objectIndex]->IsBaseObject();
    if (hasTreeKey && !isQuest)
        DebugAssert(5u, "!m_apItems[ _iNum ]->IsBaseObject() || bIsQuest", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 865u);
    if (m_apItems[objectIndex]->IsBaseObject())
    {
        const char* const name = m_apItems[objectIndex]->GetName();
        const int key = static_cast<int>(zutl::GetHashPJW(name));
        zFWTreeNode<iRPGObj*>* candidate = FWTreeFind(&m_NamesMap, key, zFWTreeMap<iRPGObj*>::_Nil);
        if (candidate == m_NamesMap.m_head)
        {
            DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 880u);
        }
        else
        {
            (void)FWTreeEraseNode(&m_NamesMap, candidate, zFWTreeMap<iRPGObj*>::_Nil);
            if (FWTreeFind(&m_NamesMap, key, zFWTreeMap<iRPGObj*>::_Nil) != m_NamesMap.m_head)
                DebugAssert(5u, "m_NamesMap.find(hash) == m_NamesMap.end()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 877u);
        }
    }
    (void)m_apItems[objectIndex]->iRelease();
    for (int sourceIndex = objectIndex+1;sourceIndex<m_iItemsNum;++sourceIndex)
        m_apItems[sourceIndex-1] = m_apItems[sourceIndex];
    m_iItemsNum = AddSigned32Wrap(m_iItemsNum, -1);
    return true;
}

const char* zRPGFrameWork::GetFormattedSkill(const iRPGObj* object, int type)
{
    if (!object)
        DebugAssert(5u, "_pObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 916u);
    iRPGObj* objectRaw = const_cast<iRPGObj*>(object);
    if (objectRaw->GetGameObj() != TYPE_HERO)
        DebugAssert(5u, "_pObj->GetType() == TYPE_HERO", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 917u);
    zString& textSink = m_textScratch;
    textSink = "";
    const unsigned int skillValue = static_cast<unsigned int>(object->GetSkillValue(type));
    if (type == RPGHERO_SEX)
    {
        const STRING empty("");
        const STRING selector(skillValue == 1u?"Female":"Male");
        const STRING section("menu");
        PROFILE* const registry = ::Strings;
        {
            const STRING result = registry->GetString(section, selector, empty);
            textSink = result.value;
        }
    }
    else if (type == RPGHERO_EXP)
    {
        const unsigned int nextLevelExperience = static_cast<unsigned int>(object->GetSkillValue(RPGHERO_NEXTLEVELEXP));
        textSink.Format("%d/%d", skillValue, nextLevelExperience);
    }
    else if (type == RPGHERO_RANK)
    {
        const char* text = GetStringValue(type, static_cast<int>(skillValue));
        textSink = text;
    }
    else if (type == RPGHERO_MONEY)
    {
        textSink.Format("$%d", skillValue);
    }
    else
    {
        textSink.Format("%d", skillValue);
    }
    return static_cast<const char*>(textSink);
}

void zRPGFrameWork::AddBaseName(const char* name, iRPGObj* object)
{
    const int key = static_cast<int>(zutl::GetHashPJW(name));
    zFWTreeNode<iRPGObj*>* nil = zFWTreeMap<iRPGObj*>::_Nil;
    if (FWTreeFind(&m_NamesMap, key, nil) != m_NamesMap.m_head)
    {
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 1258u);
        iLogConsole* log = ::gpLogCons;
        (void)log->LogStr(LT_ERR, "Base object %s has duplicate names", name);
        return;
    }
    (void)FWTreeInsertUnique(&m_NamesMap, key, object, nil);
}

int zRPGFrameWork::GetItemInfo(const iRPGObj* object, char** outSlots, bool expand)
{
    if (!object)
        DebugAssert(5u, "obj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 340u);
    iRPGObj* const mutableObject = const_cast<iRPGObj*>(object);
    const char* const displayName = GetItemName(object);
    const char* const specialName = object->GetName();
    const char* const firstInfoLine = m_objectInfo->GetFirstValuableString(specialName);
    const char* const dataName = object->GetName();
    const char** const lines = m_objectInfo->GetData(dataName);
    if (!lines)
        return 0;
    zIniParser parser;
    parser.SetGlobalFrameWork(this);
    parser.SetReadFrameWork(0);
    int written = 0;
    const char* const templatePrefix = "<Template>";
    for (const char** current = lines;*current;++current)
    {
        if (expand)
        {
            if (std::strncmp(*current, templatePrefix, CStringLength(templatePrefix)) == 0)
            {
                const int expandedLines = object->GetExtInfoLen();
                if (expandedLines>0)
                {
                    const int firstOutputSlot = written;
                    written = AddSigned32Wrap(written, expandedLines);
                    for (int lineIndex = 0;lineIndex<expandedLines;++lineIndex)
                    {
                        const char* const expanded = *current+CStringLength(templatePrefix);
                        parser.ConvertObjStr(expanded, mutableObject, outSlots[firstOutputSlot+lineIndex], lineIndex);
                    }
                }
            }
            else
            {
                const char* const text = *current;
                const char* const selectedText = (text == firstInfoLine)?displayName:text;
                parser.ConvertObjStr(selectedText, mutableObject, outSlots[written], -1);
                written = AddSigned32Wrap(written, 1);
            }
        }
        else
        {
            const char* const text = *current;
            char* const destination = outSlots[written];
            unsigned int lineIndex = 0u;
            do
            {
                destination[lineIndex] = text[lineIndex];
            }
            while (text[lineIndex++]);
            written = AddSigned32Wrap(written, 1);
        }
    }
    return written;
}

const char* zRPGFrameWork::GetItemName(const iRPGObj* object)
{
    iRPGObj* objectRaw = const_cast<iRPGObj*>(object);
    if (!objectRaw)
        DebugAssert(5u, "_pObject", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 399u);
    const unsigned int type = static_cast<unsigned int>(objectRaw->GetGameObj());
    const char* const name = object->GetName();
    const char* resolved = (type?m_objectInfo:m_heroInfo)->GetFirstValuableString(name);
    if (!resolved || CStringLength(resolved) == 0u)
        resolved = object->GetCharValue(RPG_FULLNAME);
    if (!resolved || CStringLength(resolved) == 0u)
    {
        if (objectRaw->GetGameObj() == TYPE_HERO)
        {
            const char* const id = object->GetName();
            if (id && CStringLength(id) == 9u)
            {
                STRING owned;
                owned = "Char";
                {
                    const STRING middle = owned+(id+8);
                    owned = middle;
                }
                resolved = m_heroInfo->GetFirstValuableString(owned.value);
            }
        }
    }
    if (!resolved || CStringLength(resolved) == 0u)
    {
        if (objectRaw->GetGameObj() != TYPE_QUEST)
            return object->GetName();
    }
    return resolved;
}

int zRPGFrameWork::CheckHashesDebug()
{
    zFWTreeMap<const char*> tempMap;
    for (int i = 0;i<m_iItemsNum;++i)
    {
        if (!m_apItems[i]->IsBaseObject())
            continue;
        for (unsigned int which = 0u;which<2u;++which)
        {
            if (which == 1u && m_apItems[i]->GetGameObj() != TYPE_QUEST)
                break;
            const char* const text = which == 0u                 ? m_apItems[i]->GetName()                 : m_apItems[i]->GetCharValue(RPG_COMENT2);
            if (!text)
                continue;
            const int hash = static_cast<int>(zutl::GetHashPJW(text));
            zFWTreeNode<const char*>* found = FWTreeFind(&tempMap, hash, zFWTreeMap<const char*>::_Nil);
            if (found != tempMap.m_head)
            {
                DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", which == 0u ? 1206u : 1224u);
                iLogConsole* log = ::gpLogCons;
                (void)log->LogStr(LT_ERR, "Objects %s and %s have equal hashes %d, %d",                     text, static_cast<const char*>(found->value), hash,                     found->key);
            }
            else
            {
                (void)FWTreeInsertUnique(&tempMap, hash, text, zFWTreeMap<const char*>::_Nil);
            }
        }
    }
    return 0;
}

zRPGFrameWork::~zRPGFrameWork() noexcept(false)
{
    for (unsigned int entryIndex = 0u;entryIndex<122u;++entryIndex)
        CrtFreePointer(m_infoLines[entryIndex]);
    CrtFreePointer(m_infoLines);
    g_bGlobalRelease = true;
    for (int entryIndex = 0;entryIndex<m_iItemsNum;++entryIndex)
    {
        iRPGObj* object = m_apItems[entryIndex];
        (void)object->iRelease();
    }
    FWTreeClear(&m_NamesMap, zFWTreeMap<iRPGObj*>::_Nil);
    g_bGlobalRelease = false;
    if (m_apItems)
    {
        CrtFreePointer(m_apItems);
        m_apItems = 0;
    }
    (void)m_heroInfo->iRelease();
    (void)m_objectInfo->iRelease();
    (void)m_ranks->iRelease();
    (void)m_heroSkillNames->iRelease();
    (void)m_VidDirs->iRelease();
    (void)m_mouseTips->iRelease();
}

iRPGObj* zRPGFrameWork::GetNewRPGObj(OBJ_TYPE type, bool clearLoadedFlag)
{
    iRPGObj* object = 0;
    const int typeValue = static_cast<int>(type);
    void* allocation = nullptr;
    try
    {
        switch (typeValue)
        {
            case TYPE_HERO:
            {
                allocation = CrtNew(sizeof(zRPGHero));
                zRPGHero* complete = static_cast<zRPGHero*>(allocation);
                if (complete)
                    complete = new (complete) zRPGHero(this);
                if (complete)
                    object = static_cast<iRPGObj*>(static_cast<zRPGObj*>(complete));
                break;
            }
            case 1:
                break;
            case TYPE_WEAPON:
            {
                allocation = CrtNew(sizeof(zRPGWeapon));
                zRPGWeapon* complete = static_cast<zRPGWeapon*>(allocation);
                if (complete)
                    new (complete) zRPGWeapon();
                if (complete)
                    object = static_cast<iRPGObj*>(static_cast<zRPGObj*>(complete));
                break;
            }
            case TYPE_AMMO:
            {
                allocation = CrtNew(sizeof(zRPGAmmo));
                zRPGAmmo* complete = static_cast<zRPGAmmo*>(allocation);
                if (complete)
                {
                    new (complete) zRPGAmmo();
                }
                if (complete)
                    object = static_cast<iRPGObj*>(static_cast<zRPGObj*>(complete));
                break;
            }
            case TYPE_ARMOR:
            {
                allocation = CrtNew(sizeof(zRPGArmor));
                zRPGArmor* complete = static_cast<zRPGArmor*>(allocation);
                if (complete)
                    new (complete) zRPGArmor();
                if (complete)
                    object = static_cast<iRPGObj*>(static_cast<zRPGObj*>(complete));
                break;
            }
            case TYPE_IMPLANT:
            {
                allocation = CrtNew(sizeof(zRPGImplant));
                zRPGImplant* complete = static_cast<zRPGImplant*>(allocation);
                if (complete)
                    new (complete) zRPGImplant(AsTextDB());
                if (complete)
                    object = static_cast<iRPGObj*>(static_cast<zRPGObj*>(complete));
                break;
            }
            case TYPE_EQUIP:
            {
                allocation = CrtNew(sizeof(zRPGEquip));
                zRPGEquip* complete = static_cast<zRPGEquip*>(allocation);
                if (complete)
                    new (complete) zRPGEquip();
                if (complete)
                    object = static_cast<iRPGObj*>(static_cast<zRPGObj*>(complete));
                break;
            }
            case 7:
            {
                allocation = CrtNew(sizeof(zRPGObj));
                zRPGObj* complete = static_cast<zRPGObj*>(allocation);
                if (complete)
                    new (complete) zRPGObj(static_cast<OBJ_TYPE>(7), 0);
                object = complete;
                break;
            }
            case TYPE_QUEST:
                case TYPE_LEVEL:
            {
                allocation = CrtNew(sizeof(zRPGQuest));
                zRPGQuest* complete = static_cast<zRPGQuest*>(allocation);
                if (complete)
                    new (complete) zRPGQuest(type);
                object = complete;
                break;
            }
            case 10:
                case 11:
            {
                allocation = CrtNew(sizeof(zSurvive));
                zSurvive* complete = static_cast<zSurvive*>(allocation);
                if (complete)
                    new (complete) zSurvive(type, this);
                object = complete;
                break;
            }
            case 12:
            {
                allocation = CrtNew(sizeof(zCampaign));
                zCampaign* complete = static_cast<zCampaign*>(allocation);
                if (complete)
                    new (complete) zCampaign(type, this);
                object = complete;
                break;
            }
            default:
                break;
        }
    }
    catch (...)
    {
        if (allocation)
            CrtFreePointer(allocation);
        throw;
    }
    int nextId = 0;
    if (m_iItemsNum != 0)
    {
        iRPGObj* previous = m_apItems[m_iItemsNum-1];
        nextId = static_cast<int>(previous->GetExtType());
    }
    if (!object)
        DebugAssert(5u, "NewObject", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGFrameWork.cpp", 181u);
    object->SetOwnerFrameWork(this);
    object->Init();
    zItem* const item = static_cast<zItem*>(object);
    item->m_extType = AddSigned32Wrap(nextId, 1);
    object->SetName("unnamed", false);
    if (m_iItemsNum == m_itemsCapacity)
    {
        m_itemsCapacity = AddSigned32Wrap(m_itemsCapacity, 64);
        const unsigned int bytes = static_cast<unsigned int>(m_itemsCapacity)*static_cast<unsigned int>(sizeof(*m_apItems));
        m_apItems = static_cast<iRPGObj**>(std::realloc(m_apItems, bytes));
    }
    m_apItems[m_iItemsNum] = object;
    m_iItemsNum = AddSigned32Wrap(m_iItemsNum, 1);
    if (clearLoadedFlag)
        item->m_justCreated = 0u;
    return object;
}

