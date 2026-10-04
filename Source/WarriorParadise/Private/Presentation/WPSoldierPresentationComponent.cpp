#include "Presentation/WPSoldierPresentationComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/WPSoldierCharacter.h"
#include "Combat/WPHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/WPSoldierPresentationProfile.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

UWPSoldierPresentationComponent::UWPSoldierPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.04f;
}

void UWPSoldierPresentationComponent::BeginPlay()
{
    Super::BeginPlay();

    SoldierOwner = Cast<AWPSoldierCharacter>(GetOwner());
    HealthComponent = SoldierOwner.IsValid()
        ? SoldierOwner->GetHealthComponent()
        : nullptr;

    RuntimeAttenuation = NewObject<USoundAttenuation>(
        this,
        TEXT("RuntimeSoldierAttenuation"));

    if (RuntimeAttenuation)
    {
        FSoundAttenuationSettings& Settings =
            RuntimeAttenuation->Attenuation;

        Settings.bAttenuate = true;
        Settings.bSpatialize = true;
        Settings.AttenuationShape = EAttenuationShape::Sphere;
        Settings.AttenuationShapeExtents = FVector(150.0f);
        Settings.FalloffDistance = 4200.0f;
        Settings.bEnableOcclusion = true;
        Settings.OcclusionTraceChannel = ECC_Visibility;
        Settings.OcclusionLowPassFilterFrequency = 2200.0f;
        Settings.OcclusionVolumeAttenuation = 0.55f;
        Settings.OcclusionInterpolationTime = 0.08f;
    }

    if (SoldierOwner.IsValid())
    {
        LastMovementSampleLocation = SoldierOwner->GetActorLocation();
    }

    BindEvents();
}

void UWPSoldierPresentationComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdateMovementFoley();
}

void UWPSoldierPresentationComponent::SetPresentationProfile(
    UWPSoldierPresentationProfile* NewProfile)
{
    PresentationProfile = NewProfile;
}

void UWPSoldierPresentationComponent::BindEvents()
{
    AWPSoldierCharacter* Soldier = SoldierOwner.Get();
    UWPHealthComponent* Health = HealthComponent.Get();

    if (!Soldier || !Health)
    {
        return;
    }

    Health->OnDamageReceived.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleDamageReceived);
    Health->OnDowned.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleDowned);
    Health->OnRevived.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleRevived);
    Health->OnDeath.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleDeath);

    Soldier->OnMeleeAttack.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleMelee);
    Soldier->OnBandageStarted.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleBandageStarted);
    Soldier->OnBandageCompleted.AddUniqueDynamic(
        this,
        &UWPSoldierPresentationComponent::HandleBandageCompleted);
}

void UWPSoldierPresentationComponent::PlayWorldSound(
    USoundBase* Sound,
    float VolumeMultiplier) const
{
    if (!Sound || !PresentationProfile || !SoldierOwner.IsValid())
    {
        return;
    }

    USoundAttenuation* Attenuation =
        PresentationProfile->Attenuation.LoadSynchronous();

    if (!Attenuation)
    {
        Attenuation = RuntimeAttenuation;
    }

    const float Pitch = 1.0f + FMath::FRandRange(
        -PresentationProfile->PitchRandomization,
        PresentationProfile->PitchRandomization);

    UGameplayStatics::PlaySoundAtLocation(
        this,
        Sound,
        SoldierOwner->GetActorLocation(),
        PresentationProfile->Volume * VolumeMultiplier,
        Pitch,
        0.0f,
        Attenuation,
        nullptr,
        nullptr);
}

void UWPSoldierPresentationComponent::PlayMontage(
    UAnimMontage* Montage) const
{
    if (!Montage || !SoldierOwner.IsValid())
    {
        return;
    }

    SoldierOwner->PlayAnimMontage(Montage);
}

void UWPSoldierPresentationComponent::SpawnEffectAtSoldier(
    UNiagaraSystem* System,
    const FVector& Scale) const
{
    if (!System || !SoldierOwner.IsValid())
    {
        return;
    }

    const FVector Location =
        SoldierOwner->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this,
        System,
        Location,
        SoldierOwner->GetActorRotation(),
        Scale,
        true,
        true,
        ENCPoolMethod::AutoRelease,
        true);
}

void UWPSoldierPresentationComponent::UpdateMovementFoley()
{
    AWPSoldierCharacter* Soldier = SoldierOwner.Get();

    if (!Soldier || !PresentationProfile)
    {
        return;
    }

    const FVector CurrentLocation = Soldier->GetActorLocation();
    const float SampleDistance = FVector::Dist2D(
        CurrentLocation,
        LastMovementSampleLocation);

    LastMovementSampleLocation = CurrentLocation;

    if (!Soldier->IsCombatCapable() ||
        !Soldier->GetCharacterMovement() ||
        Soldier->GetCharacterMovement()->IsFalling() ||
        Soldier->GetVelocity().Size2D() < 35.0f)
    {
        MovementDistanceAccumulator = 0.0f;
        return;
    }

    if (SampleDistance > 300.0f)
    {
        MovementDistanceAccumulator = 0.0f;
        return;
    }

    MovementDistanceAccumulator += SampleDistance;

    float TriggerDistance = PresentationProfile->WalkStepDistanceCm;

    if (Soldier->IsProne())
    {
        TriggerDistance = PresentationProfile->CrawlSoundDistanceCm;
    }
    else if (Soldier->IsSprinting())
    {
        TriggerDistance = PresentationProfile->SprintStepDistanceCm;
    }
    else if (Soldier->IsCrouchedStance())
    {
        TriggerDistance = PresentationProfile->CrouchStepDistanceCm;
    }

    if (MovementDistanceAccumulator < TriggerDistance)
    {
        return;
    }

    MovementDistanceAccumulator =
        FMath::Fmod(MovementDistanceAccumulator, TriggerDistance);

    if (Soldier->IsProne())
    {
        PlayWorldSound(
            PresentationProfile->CrawlSound.LoadSynchronous(),
            PresentationProfile->FootstepVolume * 0.72f);
        return;
    }

    PlayFootstep();
}

void UWPSoldierPresentationComponent::PlayFootstep()
{
    AWPSoldierCharacter* Soldier = SoldierOwner.Get();

    if (!Soldier || !PresentationProfile || !GetWorld())
    {
        return;
    }

    const FVector Start =
        Soldier->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
    const FVector End =
        Soldier->GetActorLocation() - FVector(0.0f, 0.0f, 160.0f);

    FHitResult Hit;
    FCollisionQueryParams Params(
        SCENE_QUERY_STAT(WPFootstepSurface),
        false,
        Soldier);
    Params.AddIgnoredActor(Soldier);

    AActor* SurfaceActor = nullptr;
    if (GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        Params))
    {
        SurfaceActor = Hit.GetActor();
    }

    TSoftObjectPtr<USoundBase> Footstep =
        PresentationProfile->FootstepDirt;

    if (SurfaceActor)
    {
        if (SurfaceActor->ActorHasTag(TEXT("WP_Surface_Wood")))
        {
            Footstep = PresentationProfile->FootstepWood;
        }
        else if (SurfaceActor->ActorHasTag(TEXT("WP_Surface_Metal")))
        {
            Footstep = PresentationProfile->FootstepMetal;
        }
        else if (SurfaceActor->ActorHasTag(TEXT("WP_Surface_Stone")) ||
                 SurfaceActor->ActorHasTag(TEXT("WP_Surface_Brick")) ||
                 SurfaceActor->ActorHasTag(TEXT("WP_Surface_Plaster")) ||
                 SurfaceActor->ActorHasTag(TEXT("WP_Surface_Concrete")))
        {
            Footstep = PresentationProfile->FootstepStone;
        }
    }

    if (USoundBase* Sound = Footstep.LoadSynchronous())
    {
        PlayWorldSound(
            Sound,
            PresentationProfile->FootstepVolume);
    }
}

void UWPSoldierPresentationComponent::HandleDamageReceived(
    float Damage,
    AActor* DamageCauser)
{
    if (!PresentationProfile || Damage <= 0.0f)
    {
        return;
    }

    if (UNiagaraSystem* System =
        PresentationProfile->HitEffect.LoadSynchronous())
    {
        SpawnEffectAtSoldier(
            System,
            PresentationProfile->HitEffectScale);
    }

    if (UAnimMontage* Montage =
        PresentationProfile->HitReactMontage.LoadSynchronous())
    {
        PlayMontage(Montage);
    }

    if (!GetWorld() ||
        GetWorld()->GetTimeSeconds() < NextPainSoundTime)
    {
        return;
    }

    if (USoundBase* Sound =
        PresentationProfile->PainSound.LoadSynchronous())
    {
        PlayWorldSound(Sound);
        NextPainSoundTime =
            GetWorld()->GetTimeSeconds() +
            PresentationProfile->PainSoundCooldown;
    }
}

void UWPSoldierPresentationComponent::HandleDowned()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->DownedSound.LoadSynchronous());

    PlayMontage(
        PresentationProfile->DownedMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleRevived()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->ReviveSound.LoadSynchronous(),
        0.82f);

    PlayMontage(
        PresentationProfile->ReviveMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleDeath()
{
    if (!PresentationProfile)
    {
        return;
    }

    if (UNiagaraSystem* System =
        PresentationProfile->DeathEffect.LoadSynchronous())
    {
        SpawnEffectAtSoldier(
            System,
            PresentationProfile->DeathEffectScale);
    }

    PlayWorldSound(
        PresentationProfile->DeathSound.LoadSynchronous());

    PlayMontage(
        PresentationProfile->DeathMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleMelee()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->MeleeSound.LoadSynchronous(),
        0.75f);

    PlayMontage(
        PresentationProfile->MeleeMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleBandageStarted()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->BandageStartSound.LoadSynchronous(),
        0.55f);

    PlayMontage(
        PresentationProfile->BandageMontage.LoadSynchronous());
}

void UWPSoldierPresentationComponent::HandleBandageCompleted()
{
    if (!PresentationProfile)
    {
        return;
    }

    PlayWorldSound(
        PresentationProfile->BandageCompleteSound.LoadSynchronous(),
        0.55f);
}
