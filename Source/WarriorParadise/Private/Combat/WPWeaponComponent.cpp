#include "Combat/WPWeaponComponent.h"
#include "Combat/WPBallisticProjectile.h"
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
    AmmoInMagazine = MagazineSize;
    ReserveAmmo = StartingReserveAmmo;
}

void UWPWeaponComponent::Configure(float InDamage, float InRoundsPerMinute, float InSpreadDegrees)
{
    Damage = FMath::Max(1.0f, InDamage);
    RoundsPerMinute = FMath::Max(60.0f, InRoundsPerMinute);
    HipSpreadDegrees = FMath::Max(0.0f, InSpreadDegrees);
    AimSpreadDegrees = FMath::Min(HipSpreadDegrees, FMath::Max(0.05f, HipSpreadDegrees * 0.25f));
}

void UWPWeaponComponent::SetAiming(bool bNewAiming)
{
    bAiming = bNewAiming && !bReloading;
    OnWeaponStateChanged.Broadcast();
}

void UWPWeaponComponent::StartFire()
{
    bWantsToFire = true;
    TryFire();

    if (bAutomatic && GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(FireTimerHandle))
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
    if (bReloading || AmmoInMagazine >= MagazineSize || ReserveAmmo <= 0 || !GetWorld())
    {
        return;
    }

    StopFire();
    bAiming = false;
    bReloading = true;
    OnWeaponStateChanged.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        ReloadTimerHandle,
        this,
        &UWPWeaponComponent::FinishReload,
        ReloadDuration,
        false);
}

void UWPWeaponComponent::FinishReload()
{
    const int32 Needed = MagazineSize - AmmoInMagazine;
    const int32 Transferred = FMath::Min(Needed, ReserveAmmo);
    AmmoInMagazine += Transferred;
    ReserveAmmo -= Transferred;
    bReloading = false;
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

    const float Spread = bAiming ? AimSpreadDegrees : HipSpreadDegrees;
    OutDirection = FMath::VRandCone(
        ViewRotation.Vector(),
        FMath::DegreesToRadians(Spread));

    return true;
}

void UWPWeaponComponent::TryFire()
{
    if (bReloading || !GetWorld())
    {
        return;
    }

    if (AmmoInMagazine <= 0)
    {
        Reload();
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
            Projectile->InitializeProjectile(
                Direction,
                MuzzleVelocityCmPerSecond,
                Damage,
                RangeCm,
                InstigatorController);
        }
    }

    const float PitchKick = RecoilPitchDegrees * (bAiming ? 0.72f : 1.0f);
    const float YawKick = FMath::FRandRange(-RecoilYawDegrees, RecoilYawDegrees) * (bAiming ? 0.65f : 1.0f);
    OnShotFired.Broadcast(PitchKick, YawKick);
    OnWeaponStateChanged.Broadcast();
}
