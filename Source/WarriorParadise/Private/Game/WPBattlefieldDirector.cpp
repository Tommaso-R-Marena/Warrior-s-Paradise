#include "Game/WPBattlefieldDirector.h"
#include "AI/WPEnemySoldier.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPCombatTypes.h"
#include "Combat/WPHealthComponent.h"
#include "Combat/WPWeaponComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Game/WPCaptureObjective.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AWPBattlefieldDirector::AWPBattlefieldDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    AISoldierClass = AWPEnemySoldier::StaticClass();
    ObjectiveClass = AWPCaptureObjective::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        CubeMesh = CubeAsset.Object;
    }
}

void AWPBattlefieldDirector::BeginPlay()
{
    Super::BeginPlay();

    BuildBattlefield();

    if (AWPSoldierCharacter* Player = Cast<AWPSoldierCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        Player->SetCombatTeam(EWPCombatTeam::Allies);

        const FRotator StartRotation(0.0f, -90.0f, 0.0f);
        Player->SetActorLocationAndRotation(
            FVector(0.0f, 3200.0f, 140.0f),
            StartRotation,
            false,
            nullptr,
            ETeleportType::TeleportPhysics);

        if (AController* Controller = Player->GetController())
        {
            Controller->SetControlRotation(StartRotation);
        }
    }

    BuildObjectives();
    SpawnFriendlySquad();
    SpawnWave();
}

int32 AWPBattlefieldDirector::GetActiveObjectiveNumber() const
{
    return Objectives.IsValidIndex(ActiveObjectiveIndex) ? ActiveObjectiveIndex + 1 : 0;
}

float AWPBattlefieldDirector::GetActiveObjectiveProgress() const
{
    if (Objectives.IsValidIndex(ActiveObjectiveIndex) && Objectives[ActiveObjectiveIndex])
    {
        return Objectives[ActiveObjectiveIndex]->GetCaptureProgress();
    }

    return bBattleWon ? 1.0f : 0.0f;
}

void AWPBattlefieldDirector::SpawnBox(const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
    if (!GetWorld() || !CubeMesh)
    {
        return;
    }

    AStaticMeshActor* Box = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Box)
    {
        return;
    }

    UStaticMeshComponent* MeshComponent = Box->GetStaticMeshComponent();
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetStaticMesh(CubeMesh);
    MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
    MeshComponent->SetCastShadow(true);
    Box->SetActorScale3D(Scale);
}

void AWPBattlefieldDirector::BuildBattlefield()
{
    if (!GetWorld())
    {
        return;
    }

    SpawnBox(FVector(0.0f, 0.0f, -30.0f), FVector(60.0f, 60.0f, 0.6f));

    SpawnBox(FVector(-1750.0f, -500.0f, 280.0f), FVector(7.0f, 5.0f, 3.4f));
    SpawnBox(FVector(1650.0f, -1450.0f, 320.0f), FVector(8.0f, 6.0f, 3.8f));
    SpawnBox(FVector(-1900.0f, -2550.0f, 260.0f), FVector(6.5f, 5.0f, 3.2f));
    SpawnBox(FVector(1850.0f, -3300.0f, 300.0f), FVector(7.5f, 5.5f, 3.6f));

    SpawnBox(FVector(-600.0f, 850.0f, 60.0f), FVector(4.0f, 0.5f, 0.8f), FRotator(0.0f, 8.0f, 0.0f));
    SpawnBox(FVector(650.0f, -1100.0f, 60.0f), FVector(3.5f, 0.5f, 0.8f), FRotator(0.0f, -12.0f, 0.0f));
    SpawnBox(FVector(-850.0f, -2850.0f, 55.0f), FVector(5.0f, 0.45f, 0.7f));
    SpawnBox(FVector(900.0f, -3900.0f, 55.0f), FVector(4.5f, 0.45f, 0.7f));

    SpawnBox(FVector(-5900.0f, 0.0f, 250.0f), FVector(1.0f, 60.0f, 3.0f));
    SpawnBox(FVector(5900.0f, 0.0f, 250.0f), FVector(1.0f, 60.0f, 3.0f));
    SpawnBox(FVector(0.0f, -5900.0f, 250.0f), FVector(60.0f, 1.0f, 3.0f));
    SpawnBox(FVector(0.0f, 5900.0f, 250.0f), FVector(60.0f, 1.0f, 3.0f));

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector::ZeroVector,
        FRotator(-38.0f, -28.0f, 0.0f));
    if (Sun && Sun->GetLightComponent())
    {
        Sun->GetLightComponent()->SetIntensity(7.5f);
        Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.86f, 0.69f));
        Sun->GetLightComponent()->SetCastShadows(true);
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky && Sky->GetLightComponent())
    {
        Sky->GetLightComponent()->SetIntensity(0.75f);
        Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sky->GetLightComponent()->RecaptureSky();
    }

    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    if (Fog && Fog->GetComponent())
    {
        Fog->GetComponent()->SetFogDensity(0.012f);
        Fog->GetComponent()->SetFogHeightFalloff(0.22f);
        Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.46f, 0.48f, 0.49f));
        Fog->GetComponent()->SetVolumetricFog(true);
    }
}

void AWPBattlefieldDirector::BuildObjectives()
{
    if (!GetWorld() || !ObjectiveClass)
    {
        return;
    }

    static const FVector ObjectiveLocations[] =
    {
        FVector(0.0f, 1050.0f, 40.0f),
        FVector(100.0f, -1250.0f, 40.0f),
        FVector(-150.0f, -3550.0f, 40.0f)
    };

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ObjectiveLocations); ++Index)
    {
        if (AWPCaptureObjective* Objective = GetWorld()->SpawnActor<AWPCaptureObjective>(
            ObjectiveClass,
            ObjectiveLocations[Index],
            FRotator::ZeroRotator))
        {
            Objective->InitializeObjective(Index);
            Objective->SetObjectiveActive(Index == 0);
            Objective->OnCaptured.AddDynamic(this, &AWPBattlefieldDirector::HandleObjectiveCaptured);
            Objectives.Add(Objective);
        }
    }
}

void AWPBattlefieldDirector::SpawnFriendlySquad()
{
    if (!GetWorld() || !AISoldierClass)
    {
        return;
    }

    static const FVector Offsets[] =
    {
        FVector(-260.0f, 80.0f, 0.0f),
        FVector(260.0f, 80.0f, 0.0f),
        FVector(-420.0f, 280.0f, 0.0f),
        FVector(420.0f, 280.0f, 0.0f),
        FVector(0.0f, 430.0f, 0.0f)
    };

    FriendlySoldiersAlive = 0;

    for (const FVector& Offset : Offsets)
    {
        const FTransform SpawnTransform(
            FRotator(0.0f, -90.0f, 0.0f),
            FVector(0.0f, 2850.0f, 120.0f) + Offset);

        if (AWPEnemySoldier* Soldier = GetWorld()->SpawnActorDeferred<AWPEnemySoldier>(
            AISoldierClass,
            SpawnTransform,
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
        {
            Soldier->SetCombatTeam(EWPCombatTeam::Allies);
            Soldier->FinishSpawning(SpawnTransform);
            FriendlySquad.Add(Soldier);
            ++FriendlySoldiersAlive;
        }
    }
}

void AWPBattlefieldDirector::SpawnWave()
{
    SpawnEnemyGroup(FMath::Min(6 + Wave * 2, 20), true);
}

void AWPBattlefieldDirector::SpawnEnemyGroup(int32 Count, bool bResetEnemyCounter)
{
    if (!GetWorld() || !AISoldierClass || Count <= 0)
    {
        return;
    }

    static const FVector SpawnPoints[] =
    {
        FVector(-2200.0f, -4200.0f, 120.0f),
        FVector(-1200.0f, -4700.0f, 120.0f),
        FVector(0.0f, -5000.0f, 120.0f),
        FVector(1200.0f, -4700.0f, 120.0f),
        FVector(2200.0f, -4200.0f, 120.0f),
        FVector(-3100.0f, -3000.0f, 120.0f),
        FVector(3100.0f, -3000.0f, 120.0f),
        FVector(-3600.0f, -900.0f, 120.0f),
        FVector(3600.0f, -900.0f, 120.0f)
    };

    if (bResetEnemyCounter)
    {
        RemainingEnemies = 0;
    }

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FVector Base = SpawnPoints[Index % UE_ARRAY_COUNT(SpawnPoints)];
        const FVector Jitter(
            FMath::FRandRange(-240.0f, 240.0f),
            FMath::FRandRange(-240.0f, 240.0f),
            0.0f);

        const FTransform SpawnTransform(
            FRotator(0.0f, 90.0f, 0.0f),
            Base + Jitter);

        if (AWPEnemySoldier* Enemy = GetWorld()->SpawnActorDeferred<AWPEnemySoldier>(
            AISoldierClass,
            SpawnTransform,
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
        {
            Enemy->SetCombatTeam(EWPCombatTeam::Axis);
            Enemy->FinishSpawning(SpawnTransform);
            ++RemainingEnemies;
        }
    }
}

void AWPBattlefieldDirector::NotifySoldierKilled(AWPSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    if (Soldier->GetCombatTeam() == EWPCombatTeam::Allies)
    {
        if (FriendlySquad.Contains(Soldier))
        {
            FriendlySoldiersAlive = FMath::Max(0, FriendlySoldiersAlive - 1);
        }

        if (Soldier->IsPlayerControlled())
        {
            TransferPlayerControl(Soldier);
        }

        return;
    }

    RemainingEnemies = FMath::Max(0, RemainingEnemies - 1);
    Score += 100;

    if (RemainingEnemies > 0 || !GetWorld())
    {
        return;
    }

    if (bFinalCounterattack)
    {
        bBattleWon = true;
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        NextWaveTimer,
        this,
        &AWPBattlefieldDirector::SpawnNextWave,
        InterWaveDelay,
        false);
}

void AWPBattlefieldDirector::HandleObjectiveCaptured(AWPCaptureObjective* Objective)
{
    if (!Objective ||
        !Objectives.IsValidIndex(ActiveObjectiveIndex) ||
        Objectives[ActiveObjectiveIndex] != Objective ||
        !GetWorld())
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(NextWaveTimer);
    Score += 750;

    if (ActiveObjectiveIndex + 1 < Objectives.Num())
    {
        ++ActiveObjectiveIndex;
        Objectives[ActiveObjectiveIndex]->SetObjectiveActive(true);
        ++Wave;
        SpawnEnemyGroup(4 + ActiveObjectiveIndex * 2, false);
        return;
    }

    bFinalCounterattack = true;
    ++Wave;
    SpawnEnemyGroup(12, false);

    if (RemainingEnemies == 0)
    {
        bBattleWon = true;
    }
}

void AWPBattlefieldDirector::TransferPlayerControl(AWPSoldierCharacter* FallenSoldier)
{
    if (!FallenSoldier)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(FallenSoldier->GetController());
    if (!PlayerController)
    {
        return;
    }

    AWPSoldierCharacter* BestCandidate = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();

    for (AWPSoldierCharacter* Candidate : FriendlySquad)
    {
        if (!Candidate ||
            Candidate == FallenSoldier ||
            !Candidate->GetHealthComponent() ||
            Candidate->GetHealthComponent()->IsDead())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared(
            FallenSoldier->GetActorLocation(),
            Candidate->GetActorLocation());

        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestCandidate = Candidate;
        }
    }

    if (!BestCandidate)
    {
        return;
    }

    PlayerController->Possess(BestCandidate);
    BestCandidate->GetWeaponComponent()->Configure(34.0f, 540.0f, 1.15f);
    BestCandidate->GetWeaponComponent()->SetAiming(false);
    PlayerController->SetControlRotation(BestCandidate->GetActorRotation());
}

void AWPBattlefieldDirector::SpawnNextWave()
{
    if (bBattleWon || bFinalCounterattack)
    {
        return;
    }

    ++Wave;
    SpawnWave();
}
