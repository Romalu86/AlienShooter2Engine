#include "mylib.h"
#include "engine_globals.h"
#include "terrain.h"
#include <new>
#include "basetype.h"
#include "sprite_collector.h"
#include "stream.h"

iQuestEvent* gpQuestEvent=0;

TERRAIN::TERRAIN(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : SPRITE(vid, position, direction, parent)
{
    if (gpQuestEvent)
        (void)gpQuestEvent->OnSpriteCreate(this);
}

TERRAIN::~TERRAIN()
{
    if (!gpQuestEvent)
        return;
    SPRITE_COLLECTOR* const collector = ::Sprites;
    if (collector->deletingAll == 0)
        (void)gpQuestEvent->OnSpriteDeath(this);
}

int TERRAIN::Action(int action, int argument1, int argument2, int argument3)
{
    if (action != ACT_LEGACY_RESTORE)
        return SPRITE::Action(action, argument1, argument2, argument3);
    (void)SPRITE::Action(action, argument1, argument2, argument3);
    STREAM* const stream = reinterpret_cast<STREAM*>(U32ToPtr(static_cast<unsigned int>(argument1)));
    unsigned int stackValue = static_cast<unsigned int>(argument3);
    const unsigned int count = argument2>7?2u:1u;
    (void)stream->Read(&stackValue, count);
    return 0;
}

