#pragma once

class RESOURCE;

class CONSTANT
{
public:
    float MaxScrollSpeedX;
    float MaxScrollSpeedY;
    float Gravitation;
    float Gravitation2;
    int RepairSpeed;
    int AmmoReloadTime;
    float RailRepairSpeed;
    float MasterRepairSpeed;
    float Friction;
    int DepoMillisecondsInSecond;
    int DebugMode;
    int DepoAutoRepairTimeInSeconds;
    int MasterAutoRepairTimeInSeconds;
    int MouseTipsTime;
    int DepoAutoAddHpPerSecond;
    int MasterAutoAddHpPerSecond;
    int FortCannonsAutoAddHpPerSecond;
    int RepairSettingMineTime;
    int RepairDestroyingMineTime;
    int DirijbanAmmoReloadTime;
    int SelectUnitGamma;
    int AttackUnitGamma;
    int LightedUnitGamma;
    int NukeForBirth;
    float SafeClashSpeed;
    int MessageStartDelay;

    explicit CONSTANT(RESOURCE* resource);
};

extern CONSTANT* Const;
