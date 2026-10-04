#include "UI/WPCombatHUD.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Game/WPBattlefieldDirector.h"
#include "Kismet/GameplayStatics.h"

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

    AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0));

    const bool bDrawCrosshair =
        Soldier &&
        Soldier->IsCombatCapable() &&
        Soldier->GetWeaponComponent() &&
        !Soldier->GetWeaponComponent()->IsAiming();

    if (bDrawCrosshair)
    {
        DrawCrosshair(Width * 0.5f, Height * 0.5f);
    }

    DrawCombatReadout(Width, Height);
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
                : (Soldier->bIsCrouched ? TEXT("CROUCH") : TEXT("STAND"))),
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

    DrawText(
        FString::Printf(
            TEXT("%02d / %03d   FRAG %d   SMOKE %d   MED %d"),
            CurrentAmmo,
            Reserve,
            Soldier->GetGrenadesRemaining(),
            Soldier->GetSmokeGrenadesRemaining(),
            Soldier->GetMedkitsRemaining()),
        FLinearColor::White,
        Width - 245.0f,
        Height - 58.0f,
        GEngine->GetMediumFont(),
        1.0f,
        false);

    const AWPBattlefieldDirector* Director = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()));

    if (Director)
    {
        DrawText(
            FString::Printf(
                TEXT("WAVE %d   HOSTILES %d   ARMOR %d   SQUAD %d   RESERVES A:%d X:%d   SCORE %d"),
                Director->GetWave(),
                Director->GetRemainingEnemies(),
                Director->GetActiveEnemyVehicles(),
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

        DrawText(
            FString::Printf(
                TEXT("SQUAD %s   %s"),
                SquadOrderLabel(Director->GetSquadOrder()),
                *ArtilleryText),
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

            DrawText(
                FString::Printf(
                    TEXT("OBJECTIVE %d   CAPTURE %d%%"),
                    Director->GetActiveObjectiveNumber(),
                    ProgressPercent),
                FLinearColor(1.0f, 0.82f, 0.28f, 1.0f),
                Width * 0.5f - 110.0f,
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
        TEXT("1 FOLLOW  2 ATTACK  3 HOLD  Z PRONE  F MELEE  V BANDAGE  G FRAG  H SMOKE  E REVIVE  T ARTILLERY"),
        FLinearColor(0.78f, 0.78f, 0.74f, 0.85f),
        Width * 0.5f - 260.0f,
        Height - 28.0f,
        GEngine->GetSmallFont(),
        0.9f,
        false);
}
