#include "Game/WPCombatGameMode.h"
#include "AI/WPEnemySoldier.h"
#include "Characters/WPSoldierCharacter.h"
#include "Game/WPBattlefieldDirector.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/WPCombatHUD.h"

AWPCombatGameMode::AWPCombatGameMode()
{
    DefaultPawnClass = AWPEnemySoldier::StaticClass();
    HUDClass = AWPCombatHUD::StaticClass();
}

void AWPCombatGameMode::StartPlay()
{
    Super::StartPlay();

    if (!GetWorld())
    {
        return;
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
        GetWorld()->SpawnActor<AWPBattlefieldDirector>(AWPBattlefieldDirector::StaticClass());
    }
}
