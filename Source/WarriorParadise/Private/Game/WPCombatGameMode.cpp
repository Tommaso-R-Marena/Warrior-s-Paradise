#include "Game/WPCombatGameMode.h"
#include "Characters/WPSoldierCharacter.h"
#include "Game/WPBattlefieldDirector.h"
#include "Kismet/GameplayStatics.h"
#include "UI/WPCombatHUD.h"

AWPCombatGameMode::AWPCombatGameMode()
{
    DefaultPawnClass = AWPSoldierCharacter::StaticClass();
    HUDClass = AWPCombatHUD::StaticClass();
}

void AWPCombatGameMode::StartPlay()
{
    Super::StartPlay();

    if (!UGameplayStatics::GetActorOfClass(this, AWPBattlefieldDirector::StaticClass()) && GetWorld())
    {
        GetWorld()->SpawnActor<AWPBattlefieldDirector>(AWPBattlefieldDirector::StaticClass());
    }
}
