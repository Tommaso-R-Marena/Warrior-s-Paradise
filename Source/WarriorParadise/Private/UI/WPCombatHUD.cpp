#include "UI/WPCombatHUD.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Game/WPBattlefieldDirector.h"
#include "Game/WPGameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/WPArmoredVehicle.h"
#include "Vehicles/WPLightVehicle.h"
#include "World/WPGunEmplacement.h"

namespace
{
const TCHAR* RoleLabel(EWPSoldierRole Role)
{
    switch (Role)
    {
        case EWPSoldierRole::Assault:
            return TEXT("ASSAULT");
        case EWPSoldierRole::Medic:
            return TEXT("MEDIC");
        case EWPSoldierRole::Engineer:
            return TEXT("ENGINEER");
        case EWPSoldierRole::Rifleman:
        default:
            return TEXT("RIFLEMAN");
    }
}

const TCHAR* SquadOrderLabel(EWPSquadOrder Order)
{
    switch (Order)
    {
        case EWPSquadOrder::Follow:
            return TEXT("FOLLOW");
        case EWPSquadOrder::HoldPosition:
            return TEXT("HOLD");
        case EWPSquadOrder::AttackObjective:
        default:
            return TEXT("ATTACK");
    }
}
}

void AWPCombatHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas)
    {
        return;
    }

    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;

    DrawGraphicsReadout(Width);

    APawn* PlayerPawn =
        UGameplayStatics::GetPlayerPawn(this, 0);

    AWPSoldierCharacter* Soldier =
        Cast<AWPSoldierCharacter>(PlayerPawn);

    if (Soldier)
    {
        const bool bDrawCrosshair =
            Soldier->IsCombatCapable() &&
            Soldier->GetWeaponComponent() &&
            !Soldier->GetWeaponComponent()->IsAiming();

        if (bDrawCrosshair)
        {
            DrawCrosshair(Width * 0.5f, Height * 0.5f);
        }

        DrawCombatReadout(Width, Height);
        return;
    }

    if (Cast<AWPArmoredVehicle>(PlayerPawn))
    {
        DrawCrosshair(
            Width * 0.5f,
            Height * 0.5f);
        DrawTankReadout(Width, Height);
        return;
    }

    if (AWPLightVehicle* Vehicle =
        Cast<AWPLightVehicle>(PlayerPawn))
    {
        if (Vehicle->GetVehicleType() ==
            EWPLightVehicleType::ArmoredCar)
        {
            DrawCrosshair(
                Width * 0.5f,
                Height * 0.5f);
        }

        DrawVehicleReadout(Width, Height);
        return;
    }

    if (Cast<AWPGunEmplacement>(PlayerPawn))
    {
        DrawCrosshair(
            Width * 0.5f,
            Height * 0.5f);
        DrawEmplacementReadout(Width, Height);
    }
}

void AWPCombatHUD::DrawGraphicsReadout(float Width)
{
    if (!GEngine)
    {
        return;
    }

    const UWPGameUserSettings* Settings =
        UWPGameUserSettings::Get();

    if (!Settings)
    {
        return;
    }

    DrawText(
        FString::Printf(
            TEXT("GFX %s   F10 PRESET   F9 AUTO"),
            *Settings->GetGraphicsPresetLabel()),
        FLinearColor(0.72f, 0.82f, 0.92f, 0.82f),
        Width - 225.0f,
        22.0f,
        GEngine->GetSmallFont(),
        0.82f,
        false);
}

void AWPCombatHUD::DrawCrosshair(float CenterX, float CenterY)
{
    const FLinearColor Color(0.95f, 0.95f, 0.92f, 0.9f);
    const float Gap = 4.0f;
    const float Length = 7.0f;

    DrawLine(CenterX - Gap - Length, CenterY, CenterX - Gap, CenterY, Color, 1.0f);
    DrawLine(CenterX + Gap, CenterY, CenterX + Gap + Length, CenterY, Color, 1.0f);
    DrawLine(CenterX, CenterY - Gap - Length, CenterX, CenterY - Gap, Color, 1.0f);
    DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Gap + Length, Color, 1.0f);
}

void AWPCombatHUD::DrawCombatReadout(float Width, float Height)
{
    AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0));

    if (!Soldier || !GEngine)
    {
        return;
    }

    UWPHealthComponent* Health = Soldier->GetHealthComponent();
    UWPWeaponComponent* Weapon = Soldier->GetWeaponComponent();

    const int32 CurrentHealth = Health ? FMath::CeilToInt(Health->GetHealth()) : 0;
    const int32 CurrentAmmo = Weapon ? Weapon->GetAmmoInMagazine() : 0;
    const int32 Reserve = Weapon ? Weapon->GetReserveAmmo() : 0;
    const int32 SuppressionPercent = Soldier->GetSuppressionComponent()
        ? FMath::RoundToInt(
            Soldier->GetSuppressionComponent()->GetNormalizedSuppression() * 100.0f)
        : 0;

    DrawText(
        FString::Printf(
            TEXT("%s   CONDITION %03d   STAMINA %03d   %s"),
            RoleLabel(Soldier->GetSoldierRole()),
            CurrentHealth,
            FMath::RoundToInt(Soldier->GetStaminaNormalized() * 100.0f),
            Soldier->IsProne()
                ? TEXT("PRONE")
                : (Soldier->IsCrouchedStance() ? TEXT("CROUCH") : TEXT("STAND"))),
        FLinearColor::White,
        28.0f,
        Height - 58.0f,
        GEngine->GetMediumFont(),
        1.0f,
        false);

    if (SuppressionPercent > 2)
    {
        DrawText(
            FString::Printf(TEXT("SUPPRESSION %d%%"), SuppressionPercent),
            FLinearColor(1.0f, 0.55f, 0.16f, 1.0f),
            28.0f,
            Height - 86.0f,
            GEngine->GetSmallFont(),
            1.0f,
            false);
    }

    if (Soldier->IsBandaging())
    {
        DrawText(
            TEXT("BANDAGING"),
            FLinearColor(0.45f, 0.92f, 0.55f, 1.0f),
            28.0f,
            Height - 108.0f,
            GEngine->GetSmallFont(),
            1.0f,
            false);
    }

    const FString WeaponName =
        Weapon
            ? Weapon->GetWeaponDisplayName()
            : TEXT("WEAPON");

    const FString WeaponState =
        Weapon && Weapon->IsCyclingAction()
            ? TEXT("CYCLING")
            : (Weapon && Weapon->IsReloading()
                ? TEXT("RELOADING")
                : (Weapon ? Weapon->GetOperationLabel() : TEXT("")));

    DrawText(
        FString::Printf(
            TEXT("%s  [%s]  %02d / %03d"),
            *WeaponName,
            *WeaponState,
            CurrentAmmo,
            Reserve),
        FLinearColor::White,
        Width - 360.0f,
        Height - 58.0f,
        GEngine->GetMediumFont(),
        0.92f,
        false);

    DrawText(
        FString::Printf(
            TEXT("FRAG %d   SMOKE %d   MED %d"),
            Soldier->GetGrenadesRemaining(),
            Soldier->GetSmokeGrenadesRemaining(),
            Soldier->GetMedkitsRemaining()),
        FLinearColor(0.82f, 0.86f, 0.92f, 0.9f),
        Width - 240.0f,
        Height - 84.0f,
        GEngine->GetSmallFont(),
        0.9f,
        false);

    const AWPBattlefieldDirector* Director = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()));

    if (Director)
    {
        DrawText(
            FString::Printf(
                TEXT("WAVE %d   HOSTILES %d   ARMOR %d   LIGHT %d   SQUAD %d   RESERVES A:%d X:%d   SCORE %d"),
                Director->GetWave(),
                Director->GetRemainingEnemies(),
                Director->GetActiveEnemyVehicles(),
                Director->GetActiveEnemyLightVehicles(),
                Director->GetFriendlySoldiersAlive(),
                Director->GetAlliedTickets(),
                Director->GetAxisTickets(),
                Director->GetScore()),
            FLinearColor::White,
            28.0f,
            24.0f,
            GEngine->GetSmallFont(),
            1.0f,
            false);

        const float Cooldown = Director->GetArtilleryCooldownRemaining();
        const FString ArtilleryText = Cooldown > 0.0f
            ? FString::Printf(
                TEXT("ARTILLERY %d   READY %.0fs"),
                Director->GetArtilleryCharges(),
                Cooldown)
            : FString::Printf(
                TEXT("ARTILLERY %d   READY"),
                Director->GetArtilleryCharges());

        const float AirCooldown =
            Director->GetAirSupportCooldownRemaining();

        const FString AirText = AirCooldown > 0.0f
            ? FString::Printf(
                TEXT("AIR %d READY %.0fs"),
                Director->GetAirSupportCharges(),
                AirCooldown)
            : FString::Printf(
                TEXT("AIR %d READY"),
                Director->GetAirSupportCharges());

        DrawText(
            FString::Printf(
                TEXT("SQUAD %s   %s   %s"),
                SquadOrderLabel(Director->GetSquadOrder()),
                *ArtilleryText,
                *AirText),
            FLinearColor(0.78f, 0.88f, 1.0f, 1.0f),
            28.0f,
            48.0f,
            GEngine->GetSmallFont(),
            1.0f,
            false);

        if (Director->IsBattleWon())
        {
            DrawText(
                TEXT("BATTLE WON"),
                FLinearColor(0.25f, 0.92f, 0.35f, 1.0f),
                Width * 0.5f - 100.0f,
                Height * 0.28f,
                GEngine->GetLargeFont(),
                1.35f,
                false);
        }
        else if (Director->IsBattleLost())
        {
            DrawText(
                TEXT("BATTLE LOST"),
                FLinearColor(0.92f, 0.18f, 0.12f, 1.0f),
                Width * 0.5f - 100.0f,
                Height * 0.28f,
                GEngine->GetLargeFont(),
                1.35f,
                false);
        }
        else if (Director->IsFinalCounterattack())
        {
            DrawText(
                FString::Printf(
                    TEXT("HOLD THE VILLAGE   ENEMY REMAINING %d"),
                    Director->GetRemainingEnemies()),
                FLinearColor(1.0f, 0.64f, 0.16f, 1.0f),
                Width * 0.5f - 180.0f,
                72.0f,
                GEngine->GetMediumFont(),
                1.0f,
                false);
        }
        else
        {
            const int32 ProgressPercent = FMath::RoundToInt(
                Director->GetActiveObjectiveProgress() * 100.0f);

            const FString ObjectiveText = Director->IsActiveObjectiveContested()
                ? FString::Printf(
                    TEXT("OBJECTIVE %d   CONTESTED   A%d / X%d"),
                    Director->GetActiveObjectiveNumber(),
                    Director->GetActiveObjectiveAlliedPresence(),
                    Director->GetActiveObjectiveAxisPresence())
                : FString::Printf(
                    TEXT("OBJECTIVE %d   CAPTURE %d%%"),
                    Director->GetActiveObjectiveNumber(),
                    ProgressPercent);

            DrawText(
                ObjectiveText,
                Director->IsActiveObjectiveContested()
                    ? FLinearColor(1.0f, 0.28f, 0.18f, 1.0f)
                    : FLinearColor(1.0f, 0.82f, 0.28f, 1.0f),
                Width * 0.5f - 145.0f,
                72.0f,
                GEngine->GetMediumFont(),
                1.0f,
                false);
        }
    }

    if (Health && Health->IsDowned())
    {
        DrawText(
            FString::Printf(
                TEXT("DOWNED   BLEEDOUT %.1fs   WAIT FOR MEDIC"),
                Health->GetBleedoutRemaining()),
            FLinearColor(1.0f, 0.42f, 0.12f, 1.0f),
            Width * 0.5f - 190.0f,
            Height * 0.40f,
            GEngine->GetLargeFont(),
            1.1f,
            false);
    }
    else if (Health && Health->IsDead())
    {
        DrawText(
            TEXT("KILLED IN ACTION"),
            FLinearColor(0.75f, 0.08f, 0.06f, 1.0f),
            Width * 0.5f - 120.0f,
            Height * 0.38f,
            GEngine->GetLargeFont(),
            1.2f,
            false);
    }

    DrawText(
        TEXT("X SWITCH  1 FOLLOW  2 ATTACK  3 HOLD  E USE/REVIVE  T ARTILLERY  Y AIR  Z PRONE  F MELEE  V BANDAGE  G FRAG  H SMOKE"),
        FLinearColor(0.78f, 0.78f, 0.74f, 0.85f),
        Width * 0.5f - 260.0f,
        Height - 28.0f,
        GEngine->GetSmallFont(),
        0.9f,
        false);
}


void AWPCombatHUD::DrawTankReadout(
    float Width,
    float Height)
{
    AWPArmoredVehicle* Tank =
        Cast<AWPArmoredVehicle>(
            UGameplayStatics::GetPlayerPawn(this, 0));

    if (!Tank || !GEngine)
    {
        return;
    }

    const TCHAR* ShellLabel =
        Tank->GetSelectedShellType() ==
            EWPArmorShellType::ArmorPiercing
            ? TEXT("AP")
            : TEXT("HE");

    DrawText(
        FString::Printf(
            TEXT("TANK   HULL %03d   ENG %03d   TRACK %03d   TURRET %03d   RACK %03d"),
            FMath::RoundToInt(Tank->GetHealthNormalized() * 100.0f),
            FMath::RoundToInt(
                Tank->GetSubsystemHealthNormalized(
                    EWPVehicleSubsystem::Engine) * 100.0f),
            FMath::RoundToInt(
                Tank->GetSubsystemHealthNormalized(
                    EWPVehicleSubsystem::Tracks) * 100.0f),
            FMath::RoundToInt(
                Tank->GetSubsystemHealthNormalized(
                    EWPVehicleSubsystem::Turret) * 100.0f),
            FMath::RoundToInt(
                Tank->GetSubsystemHealthNormalized(
                    EWPVehicleSubsystem::AmmoRack) * 100.0f)),
        FLinearColor::White,
        28.0f,
        Height - 58.0f,
        GEngine->GetMediumFont(),
        0.95f,
        false);

    DrawText(
        FString::Printf(
            TEXT("SHELL %s   W/S DRIVE  A/D TURN  MOUSE TURRET  LMB CANNON  RMB COAX  R CHANGE SHELL  E EXIT"),
            ShellLabel),
        FLinearColor(0.78f, 0.88f, 1.0f, 1.0f),
        Width * 0.5f - 310.0f,
        Height - 28.0f,
        GEngine->GetSmallFont(),
        0.9f,
        false);
}

void AWPCombatHUD::DrawVehicleReadout(
    float Width,
    float Height)
{
    AWPLightVehicle* Vehicle =
        Cast<AWPLightVehicle>(
            UGameplayStatics::GetPlayerPawn(this, 0));

    if (!Vehicle || !GEngine)
    {
        return;
    }

    const TCHAR* VehicleLabel =
        Vehicle->GetVehicleType() ==
            EWPLightVehicleType::ArmoredCar
            ? TEXT("ARMORED CAR")
            : TEXT("TRANSPORT");

    DrawText(
        FString::Printf(
            TEXT("%s   HULL %03d"),
            VehicleLabel,
            FMath::RoundToInt(
                Vehicle->GetHealthNormalized() * 100.0f)),
        FLinearColor::White,
        28.0f,
        Height - 58.0f,
        GEngine->GetMediumFont(),
        1.0f,
        false);

    DrawText(
        Vehicle->GetVehicleType() ==
                EWPLightVehicleType::ArmoredCar
            ? TEXT("W/S DRIVE   MOUSE STEER   FIRE MG   E DISEMBARK")
            : TEXT("W/S DRIVE   MOUSE STEER   E DISEMBARK SQUAD"),
        FLinearColor(0.78f, 0.88f, 1.0f, 1.0f),
        Width * 0.5f - 210.0f,
        Height - 28.0f,
        GEngine->GetSmallFont(),
        0.95f,
        false);
}

void AWPCombatHUD::DrawEmplacementReadout(
    float Width,
    float Height)
{
    AWPGunEmplacement* Emplacement =
        Cast<AWPGunEmplacement>(
            UGameplayStatics::GetPlayerPawn(this, 0));

    if (!Emplacement || !GEngine)
    {
        return;
    }

    const TCHAR* GunLabel =
        Emplacement->GetEmplacementType() ==
            EWPEmplacementType::AntiTankGun
            ? TEXT("ANTI-TANK GUN")
            : TEXT("HEAVY MACHINE GUN");

    DrawText(
        FString::Printf(
            TEXT("%s   CONDITION %03d"),
            GunLabel,
            FMath::RoundToInt(
                Emplacement->GetHealthNormalized() * 100.0f)),
        FLinearColor::White,
        28.0f,
        Height - 58.0f,
        GEngine->GetMediumFont(),
        1.0f,
        false);

    DrawText(
        TEXT("MOUSE AIM   HOLD FIRE   E LEAVE EMPLACEMENT"),
        FLinearColor(0.78f, 0.88f, 1.0f, 1.0f),
        Width * 0.5f - 190.0f,
        Height - 28.0f,
        GEngine->GetSmallFont(),
        0.95f,
        false);
}
