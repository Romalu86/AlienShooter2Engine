#include "myerror.h"
#include "basetype.h"
#include "mylib.h"
#include "constant.h"
#include "resread.h"



CONSTANT* Const=0;


CONSTANT::CONSTANT(RESOURCE* resource)
{
    if (resource->GoBegin(ResourceType::Constants) != 0)
    {
        ::Error->Log("!!!ERROR!!! CNST Load Constant section not found");
        return;
    }

    (void)resource->Read(&MaxScrollSpeedX, sizeof(MaxScrollSpeedX));
    (void)resource->Read(&MaxScrollSpeedY, sizeof(MaxScrollSpeedY));
    (void)resource->Read(&Gravitation, sizeof(Gravitation));
    (void)resource->Read(&Gravitation2, sizeof(Gravitation2));
    (void)resource->Read(&RepairSpeed, sizeof(RepairSpeed));
    (void)resource->Read(&AmmoReloadTime, sizeof(AmmoReloadTime));
    (void)resource->Read(&RailRepairSpeed, sizeof(RailRepairSpeed));
    (void)resource->Read(&MasterRepairSpeed, sizeof(MasterRepairSpeed));
    (void)resource->Read(&Friction, sizeof(Friction));
    (void)resource->Read(&DepoMillisecondsInSecond, sizeof(DepoMillisecondsInSecond));

    int serializedDebugMode;
    (void)resource->Read(&serializedDebugMode, sizeof(serializedDebugMode));

    (void)resource->Read(&DepoAutoRepairTimeInSeconds, sizeof(DepoAutoRepairTimeInSeconds));
    (void)resource->Read(&MasterAutoRepairTimeInSeconds, sizeof(MasterAutoRepairTimeInSeconds));
    (void)resource->Read(&MouseTipsTime, sizeof(MouseTipsTime));
    (void)resource->Read(&DepoAutoAddHpPerSecond, sizeof(DepoAutoAddHpPerSecond));
    (void)resource->Read(&MasterAutoAddHpPerSecond, sizeof(MasterAutoAddHpPerSecond));
    (void)resource->Read(&FortCannonsAutoAddHpPerSecond, sizeof(FortCannonsAutoAddHpPerSecond));
    (void)resource->Read(&RepairSettingMineTime, sizeof(RepairSettingMineTime));
    (void)resource->Read(&RepairDestroyingMineTime, sizeof(RepairDestroyingMineTime));
    (void)resource->Read(&DirijbanAmmoReloadTime, sizeof(DirijbanAmmoReloadTime));
    (void)resource->Read(&SelectUnitGamma, sizeof(SelectUnitGamma));
    (void)resource->Read(&AttackUnitGamma, sizeof(AttackUnitGamma));
    (void)resource->Read(&LightedUnitGamma, sizeof(LightedUnitGamma));
    (void)resource->Read(&NukeForBirth, sizeof(NukeForBirth));
    (void)resource->Read(&SafeClashSpeed, sizeof(SafeClashSpeed));
    (void)resource->Read(&MessageStartDelay, sizeof(MessageStartDelay));


    const float milliseconds = 0.001f;
    const float millisecondsSquared = 0.000001f;
    MaxScrollSpeedX*=milliseconds;
    MaxScrollSpeedY*=milliseconds;
    Gravitation*=millisecondsSquared;
    Gravitation2*=millisecondsSquared;
    Friction*=millisecondsSquared;
    MasterRepairSpeed*=milliseconds;
    RailRepairSpeed*=milliseconds;
    SafeClashSpeed*=milliseconds;
}
