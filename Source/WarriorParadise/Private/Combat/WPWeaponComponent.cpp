#include "Combat/WPWeaponComponent.h"
#include "Combat/WPBallisticProjectile.h"
#include "Combat/WPWeaponDefinition.h"
#include "Characters/WPSoldierCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

UWPWeaponComponent::UWPWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    ProjectileClass = AWPBallisticProjectile::StaticClass();
}

void UWPWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    if (DefaultWeaponDefinition)
    {
        ApplyWeaponDefinition(DefaultWeaponDefinition);
    }

    AmmoInMagazine = MagazineSize;
    ReserveAmmo = StartingReserveAmmo;
}

void UWPWeaponComponent::Configure(float InDamage, float InRoundsPerMinute, float InSpreadDegrees)
{
    Damage = FMath::Max(1.0f, InDamage);
    RoundsPerMinute = FMath::Max(30.0f, InRoundsPerMinute);
    HipSpreadDegrees = FMath::Max(0.0f, InSpreadDegrees);
    AimSpreadDegrees = FMath::Min(
        HipSpreadDegrees,
        FMath::Max(0.05f, HipSpreadDegrees * 0.25f));
}

void UWPWeaponComponent::ApplyWeaponDefinition(const UWPWeaponDefinition* Definition)
{
    if (!Definition)
    {
        return;
    }

    Damage = FMath::Max(1.0f, Definition->Damage);
    MuzzleVelocityCmPerSecond = FMath::Max(1000.0f, Definition->MuzzleVelocityCmPerSecond);
    RangeCm = FMath::Max(1000.0f, Definition->RangeCm);
    HipSpreadDegrees = FMath::Max(0.0f, Definition->HipSpreadDegrees);
    AimSpreadDegrees = FMath::Max(0.0f, Definition->AimSpreadDegrees);
    RoundsPerMinute = FMath::Max(30.0f, Definition->RoundsPerMinute);
    bAutomatic = Definition->bAutomatic;
    bRequiresManualCycle =
        Definition->bRequiresManualCycle &&
        !Definition->bAutomatic;
    ManualCycleDuration = FMath::Max(
        0.05f,
        Definition->ManualCycleDuration);
    WeaponDisplayName = Definition->DisplayName.IsEmpty()
        ? Definition->WeaponId.ToString()
        : Definition->DisplayName.ToString();
    MagazineSize = FMath::Max(1, Definition->MagazineSize);
    StartingReserveAmmo = FMath::Max(0, Definition->StartingReserveAmmo);
    MaximumReserveAmmo = FMath::Max(StartingReserveAmmo, Definition->MaximumReserveAmmo);
    ReloadDuration = FMath::Max(0.1f, Definition->ReloadDuration);
    RecoilPitchDegrees = FMath::Max(0.0f, Definition->RecoilPitchDegrees);
    RecoilYawDegrees = FMath::Max(0.0f, Definition->RecoilYawDegrees);

    StopFire();
    CancelReload();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(
            ManualCycleTimerHandle);
    }
    bCyclingAction = false;

    AmmoInMagazine = MagazineSize;
    ReserveAmmo = StartingReserveAmmo;
    OnWeaponStateChanged.Broadcast();
}

void UWPWeaponComponent::SetAiming(bool bNewAiming)
{
    bAiming =
        bNewAiming &&
        !bReloading &&
        !bCyclingAction;
    OnWeaponStateChanged.Broadcast();
}

FString UWPWeaponComponent::GetOperationLabel() const
{
    if (bRequiresManualCycle)
    {
        return TEXT("MANUAL");
    }

    return bAutomatic
        ? TEXT("AUTO")
        : TEXT("SEMI");
}

bool UWPWeaponComponent::HasFiredRecently(
    float WindowSeconds) const
{
    return GetWorld() &&
        GetWorld()->GetTimeSeconds() - LastFireTime <=
            FMath::Max(0.0f, WindowSeconds);
}

void UWPWeaponComponent::SetAccuracyMultiplier(float Multiplier)
{
    AccuracyMultiplier = FMath::Clamp(Multiplier, 0.45f, 3.5f);
}

void UWPWeaponComponent::AddReserveAmmo(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    const int32 PreviousReserve = ReserveAmmo;
    ReserveAmmo = FMath::Clamp(ReserveAmmo + Amount, 0, MaximumReserveAmmo);

    if (ReserveAmmo != PreviousReserve)
    {
        OnWeaponStateChanged.Broadcast();
    }
}

void UWPWeaponComponent::StartFire()
{
    bWantsToFire = true;
    TryFire();

    if (bWantsToFire &&
        bAutomatic &&
        GetWorld() &&
        !GetWorld()->GetTimerManager().IsTimerActive(FireTimerHandle))
    {
        const float Interval = 60.0f / FMath::Max(60.0f, RoundsPerMinute);
        GetWorld()->GetTimerManager().SetTimer(
            FireTimerHandle,
            this,
            &UWPWeaponComponent::TryFire,
            Interval,
            true,
            Interval);
    }
}

void UWPWeaponComponent::StopFire()
{
    bWantsToFire = false;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
    }
}

void UWPWeaponComponent::Reload()
{
    if (bReloading ||
        bCyclingAction ||
        AmmoInMagazine >= MagazineSize ||
        ReserveAmmo <= 0 ||
        !GetWorld())
    {
        return;
    }

    StopFire();
    bAiming = false;
    bReloading = true;
    OnReloadStarted.Broadcast();
    OnWeaponStateChanged.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        ReloadTimerHandle,
        this,
        &UWPWeaponComponent::FinishReload,
        ReloadDuration,
        false);
}

void UWPWeaponComponent::CancelReload()
{
    if (!bReloading)
    {
        return;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
    }

    bReloading = false;
    OnReloadCancelled.Broadcast();
    OnWeaponStateChanged.Broadcast();
}

void UWPWeaponComponent::BeginManualCycle()
{
    if (!bRequiresManualCycle ||
        bCyclingAction ||
        !GetWorld())
    {
        return;
    }

    bCyclingAction = true;
    OnManualCycleStarted.Broadcast();
    OnWeaponStateChanged.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        ManualCycleTimerHandle,
        this,
        &UWPWeaponComponent::FinishManualCycle,
        ManualCycleDuration,
        false);
}

void UWPWeaponComponent::FinishManualCycle()
{
    if (!bCyclingAction)
    {
        return;
    }

    bCyclingAction = false;
    OnManualCycleCompleted.Broadcast();
    OnWeaponStateChanged.Broadcast();
}

void UWPWeaponComponent::FinishReload()
{
    const int32 Needed = MagazineSize - AmmoInMagazine;
    const int32 Transferred = FMath::Min(Needed, ReserveAmmo);
    AmmoInMagazine += Transferred;
    ReserveAmmo -= Transferred;
    bReloading = false;
    OnReloadCompleted.Broadcast();
    OnWeaponStateChanged.Broadcast();
}

bool UWPWeaponComponent::BuildShot(FVector& OutStart, FVector& OutDirection, AController*& OutController) const
{
    const ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return false;
    }

    OutController = Character->GetController();

    FRotator ViewRotation = Character->GetActorRotation();
    OutStart = Character->GetActorLocation() + FVector(0.0f, 0.0f, Character->BaseEyeHeight);

    if (OutController)
    {
        OutController->GetPlayerViewPoint(OutStart, ViewRotation);
    }

    const float BaseSpread = bAiming ? AimSpreadDegrees : HipSpreadDegrees;
    const float Spread = BaseSpread * AccuracyMultiplier;
    OutDirection = FMath::VRandCone(
        ViewRotation.Vector(),
        FMath::DegreesToRadians(Spread));

    return true;
}

void UWPWeaponComponent::TryFire()
{
    if (bReloading ||
        bCyclingAction ||
        !GetWorld())
    {
        return;
    }

    if (AmmoInMagazine <= 0)
    {
        if (ReserveAmmo > 0)
        {
            Reload();
        }
        else
        {
            StopFire();
            OnDryFire.Broadcast();
            OnWeaponStateChanged.Broadcast();
        }
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const float MinimumInterval = 60.0f / FMath::Max(60.0f, RoundsPerMinute);
    if (CurrentTime - LastFireTime + KINDA_SMALL_NUMBER < MinimumInterval)
    {
        return;
    }

    FVector Start;
    FVector Direction;
    AController* InstigatorController = nullptr;
    if (!BuildShot(Start, Direction, InstigatorController))
    {
        return;
    }

    LastFireTime = CurrentTime;
    --AmmoInMagazine;

    if (ProjectileClass)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = GetOwner();
        SpawnParameters.Instigator = Cast<APawn>(GetOwner());
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        const FVector SpawnLocation = Start + Direction * 60.0f;
        if (AWPBallisticProjectile* Projectile = GetWorld()->SpawnActor<AWPBallisticProjectile>(
            ProjectileClass,
            SpawnLocation,
            Direction.Rotation(),
            SpawnParameters))
        {
            const AWPSoldierCharacter* SoldierOwner = Cast<AWPSoldierCharacter>(GetOwner());
            const EWPCombatTeam Team = SoldierOwner
                ? SoldierOwner->GetCombatTeam()
                : EWPCombatTeam::Allies;

            Projectile->InitializeProjectile(
                Direction,
                MuzzleVelocityCmPerSecond,
                Damage,
                RangeCm,
                InstigatorController,
                Team);

            OnProjectileSpawned.Broadcast(Projectile);
        }
    }

    const float PitchKick = RecoilPitchDegrees * (bAiming ? 0.72f : 1.0f);
    const float YawKick = FMath::FRandRange(-RecoilYawDegrees, RecoilYawDegrees) * (bAiming ? 0.65f : 1.0f);
    OnShotFired.Broadcast(PitchKick, YawKick);

    if (bRequiresManualCycle)
    {
        BeginManualCycle();
    }
    else
    {
        OnWeaponStateChanged.Broadcast();
    }
}
