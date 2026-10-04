#include "UI/WPCombatHUD.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Game/WPBattlefieldDirector.h"
#include "Kismet/GameplayStatics.h"

void AWPCombatHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas)
    {
        return;
    }

    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;
    DrawCrosshair(Width * 0.5f, Height * 0.5f);
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
    AWPSoldierCharacter* Soldier = Cast<AWPSoldierCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Soldier || !GEngine)
    {
        return;
    }

    UWPHealthComponent* Health = Soldier->GetHealthComponent();
    UWPWeaponComponent* Weapon = Soldier->GetWeaponComponent();

    const int32 CurrentHealth = Health ? FMath::CeilToInt(Health->GetHealth()) : 0;
    const int32 CurrentAmmo = Weapon ? Weapon->GetAmmoInMagazine() : 0;
    const int32 Reserve = Weapon ? Weapon->GetReserveAmmo() : 0;

    DrawText(
        FString::Printf(TEXT("CONDITION  %03d"), CurrentHealth),
        FLinearColor::White,
        28.0f,
        Height - 58.0f,
        GEngine->GetMediumFont(),
        1.0f,
        false);

    DrawText(
        FString::Printf(TEXT("%02d / %03d"), CurrentAmmo, Reserve),
        FLinearColor::White,
        Width - 150.0f,
        Height - 58.0f,
        GEngine->GetMediumFont(),
        1.0f,
        false);

    if (const AWPBattlefieldDirector* Director = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass())))
    {
        DrawText(
            FString::Printf(
                TEXT("WAVE %d    HOSTILES %d    SCORE %d"),
                Director->GetWave(),
                Director->GetRemainingEnemies(),
                Director->GetScore()),
            FLinearColor::White,
            28.0f,
            24.0f,
            GEngine->GetSmallFont(),
            1.0f,
            false);
    }

    if (Health && Health->IsDead())
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
}
