#include "Game/WPCombatGameMode.h"
#include "AI/WPEnemySoldier.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPSuppressionComponent.h"
#include "Game/WPBattlefieldDirector.h"
#include "Game/WPGameUserSettings.h"
#include "Game/WPPlayerController.h"
#include "Game/WPRuntimeMode.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "GameFramework/DamageType.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/WPCombatHUD.h"

AWPCombatGameMode::AWPCombatGameMode()
{
    DefaultPawnClass = AWPEnemySoldier::StaticClass();
    PlayerControllerClass = AWPPlayerController::StaticClass();
    HUDClass = AWPCombatHUD::StaticClass();
}

void AWPCombatGameMode::StartPlay()
{
    Super::StartPlay();

    if (!GetWorld())
    {
        return;
    }

    if (!WPRuntimeMode::IsHeadlessSimulation())
    {
        if (UWPGameUserSettings* Settings =
            UWPGameUserSettings::Get())
        {
            Settings->LoadSettings(false);

            if (WPRuntimeMode::WantsIntegratedGraphics())
            {
                Settings->ApplyGraphicsPreset(
                    EWPGraphicsPreset::Minimal,
                    false);
            }
            else
            {
                Settings->EnsureInitialHardwareBenchmark();
            }
        }
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (PlayerController && !PlayerController->GetPawn())
    {
        const FTransform SpawnTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(0.0f, 3200.0f, 140.0f));
        if (AWPEnemySoldier* Soldier = GetWorld()->SpawnActorDeferred<AWPEnemySoldier>(
            AWPEnemySoldier::StaticClass(),
            SpawnTransform,
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
        {
            Soldier->FinishSpawning(SpawnTransform);
            PlayerController->Possess(Soldier);
        }
    }

    if (!UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()))
    {
        GetWorld()->SpawnActor<AWPBattlefieldDirector>(
            AWPBattlefieldDirector::StaticClass());
    }

    if (WPRuntimeMode::IsHeadlessSmokeTest())
    {
        StartHeadlessSmokeTest();
    }
}

void AWPCombatGameMode::StartHeadlessSmokeTest()
{
    if (!GetWorld())
    {
        FGenericPlatformMisc::RequestExitWithStatus(
            false,
            2,
            TEXT("Warrior's Paradise headless smoke: no world"));
        return;
    }

    AWPBattlefieldDirector* Director = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(
            this,
            AWPBattlefieldDirector::StaticClass()));

    bHeadlessSmokeInitialStateValid =
        Director != nullptr &&
        Director->GetFriendlySoldiersAlive() >= 6 &&
        Director->GetRemainingEnemies() > 0 &&
        Director->GetActiveObjectiveNumber() > 0;

    AWPSoldierCharacter* Player = Cast<AWPSoldierCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0));

    if (Player && Player->GetSuppressionComponent())
    {
        UWPSuppressionComponent* Suppression =
            Player->GetSuppressionComponent();

        Suppression->ClearSuppression();
        const bool bStartedSteady =
            Suppression->GetSuppressionState() ==
            EWPSuppressionState::Steady;

        Suppression->AddSuppression(40.0f);
        const bool bBecamePressured =
            Suppression->GetSuppressionState() ==
            EWPSuppressionState::Pressured;

        Suppression->AddSuppression(40.0f);
        const bool bBecamePinned =
            Suppression->GetSuppressionState() ==
            EWPSuppressionState::Pinned;

        Suppression->ClearSuppression();
        const bool bRecoveredSteady =
            Suppression->GetSuppressionState() ==
            EWPSuppressionState::Steady;

        bHeadlessSuppressionStateValid =
            bStartedSteady &&
            bBecamePressured &&
            bBecamePinned &&
            bRecoveredSteady;
    }

    if (Player && Player->GetHealthComponent())
    {
        UWPHealthComponent* Health =
            Player->GetHealthComponent();

        Health->RestoreToFull();

        UGameplayStatics::ApplyDamage(
            Player,
            35.0f,
            nullptr,
            this,
            UDamageType::StaticClass());

        const bool bStartedBleeding =
            Health->IsBleeding() &&
            Health->GetBleedingSeverity() > 0.0f;

        const bool bStoppedBleeding =
            Health->StopBleeding() &&
            !Health->IsBleeding();

        Health->RestoreToFull();

        bHeadlessBleedingStateValid =
            bStartedBleeding &&
            bStoppedBleeding &&
            FMath::IsNearlyEqual(
                Health->GetHealth(),
                Health->GetMaxHealth());
    }

    const float Duration =
        WPRuntimeMode::GetHeadlessSmokeDurationSeconds();

    UE_LOG(
        LogTemp,
        Display,
        TEXT(
            "WP_HEADLESS_SMOKE START initial=%s suppression=%s "
            "bleeding=%s duration=%.1f friendly=%d enemies=%d objective=%d"),
        bHeadlessSmokeInitialStateValid ? TEXT("PASS") : TEXT("FAIL"),
        bHeadlessSuppressionStateValid ? TEXT("PASS") : TEXT("FAIL"),
        bHeadlessBleedingStateValid ? TEXT("PASS") : TEXT("FAIL"),
        Duration,
        Director ? Director->GetFriendlySoldiersAlive() : -1,
        Director ? Director->GetRemainingEnemies() : -1,
        Director ? Director->GetActiveObjectiveNumber() : -1);

    GetWorld()->GetTimerManager().SetTimer(
        HeadlessSmokeTimer,
        this,
        &AWPCombatGameMode::FinishHeadlessSmokeTest,
        Duration,
        false);
}

void AWPCombatGameMode::FinishHeadlessSmokeTest()
{
    AWPBattlefieldDirector* Director = Cast<AWPBattlefieldDirector>(
        UGameplayStatics::GetActorOfClass(
            this,
            AWPBattlefieldDirector::StaticClass()));

    const bool bRuntimeStateValid =
        Director != nullptr &&
        Director->GetFriendlySoldiersAlive() > 0 &&
        Director->GetActiveObjectiveNumber() > 0;

    const bool bPassed =
        bHeadlessSmokeInitialStateValid &&
        bHeadlessSuppressionStateValid &&
        bHeadlessBleedingStateValid &&
        bRuntimeStateValid;

    int32 CoordinatedAITroops = 0;
    int32 FriendlyFireHolds = 0;
    int32 FriendlyGrenadeHolds = 0;
    int32 CoordinatedReloads = 0;
    int32 ManeuverSupportWaits = 0;
    int32 CoverPostureUsers = 0;
    int32 ManeuverSmokeUses = 0;

    for (TActorIterator<AWPEnemySoldier> It(GetWorld()); It; ++It)
    {
        const AWPEnemySoldier* Soldier = *It;
        if (!Soldier)
        {
            continue;
        }

        if (Soldier->HasUsedCoordinatedTactic())
        {
            ++CoordinatedAITroops;
        }

        if (Soldier->HasHeldFireForFriendly())
        {
            ++FriendlyFireHolds;
        }

        if (Soldier->HasHeldGrenadeForFriendly())
        {
            ++FriendlyGrenadeHolds;
        }

        if (Soldier->HasUsedCoordinatedReload())
        {
            ++CoordinatedReloads;
        }

        if (Soldier->HasWaitedForManeuverSupport())
        {
            ++ManeuverSupportWaits;
        }

        if (Soldier->HasUsedCoverPosture())
        {
            ++CoverPostureUsers;
        }

        if (Soldier->HasUsedManeuverSmoke())
        {
            ++ManeuverSmokeUses;
        }
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT(
            "WP_HEADLESS_SMOKE %s friendly=%d enemies=%d "
            "enemyArmor=%d enemyLight=%d objective=%d score=%d "
            "coordinatedAI=%d friendlyFireHolds=%d "
            "friendlyGrenadeHolds=%d coordinatedReloads=%d "
            "maneuverSupportWaits=%d coverPostureUsers=%d "
            "maneuverSmokeUses=%d won=%d lost=%d"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        Director ? Director->GetFriendlySoldiersAlive() : -1,
        Director ? Director->GetRemainingEnemies() : -1,
        Director ? Director->GetActiveEnemyVehicles() : -1,
        Director ? Director->GetActiveEnemyLightVehicles() : -1,
        Director ? Director->GetActiveObjectiveNumber() : -1,
        Director ? Director->GetScore() : -1,
        CoordinatedAITroops,
        FriendlyFireHolds,
        FriendlyGrenadeHolds,
        CoordinatedReloads,
        ManeuverSupportWaits,
        CoverPostureUsers,
        ManeuverSmokeUses,
        Director ? Director->IsBattleWon() : false,
        Director ? Director->IsBattleLost() : false);

    FGenericPlatformMisc::RequestExitWithStatus(
        false,
        bPassed ? 0 : 2,
        bPassed
            ? TEXT("Warrior's Paradise headless smoke passed")
            : TEXT("Warrior's Paradise headless smoke failed"));
}
