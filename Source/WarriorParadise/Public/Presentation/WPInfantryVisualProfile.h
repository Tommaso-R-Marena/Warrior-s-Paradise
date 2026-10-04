#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WPInfantryVisualProfile.generated.h"

class UAnimInstance;
class USkeletalMesh;

UCLASS(BlueprintType)
class WARRIORPARADISE_API UWPInfantryVisualProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Third Person")
    TSoftObjectPtr<USkeletalMesh> ThirdPersonMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Third Person")
    TSubclassOf<UAnimInstance> ThirdPersonAnimClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Third Person")
    FVector ThirdPersonRelativeLocation = FVector(0.0f, 0.0f, -94.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Third Person")
    FRotator ThirdPersonRelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Third Person")
    FVector ThirdPersonRelativeScale = FVector::OneVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="First Person")
    TSoftObjectPtr<USkeletalMesh> FirstPersonArmsMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="First Person")
    TSubclassOf<UAnimInstance> FirstPersonArmsAnimClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="First Person")
    FVector FirstPersonArmsRelativeLocation = FVector(0.0f, 0.0f, -52.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="First Person")
    FRotator FirstPersonArmsRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="First Person")
    FVector FirstPersonArmsRelativeScale = FVector::OneVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="First Person")
    TSoftObjectPtr<USkeletalMesh> FirstPersonWeaponMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="First Person")
    FName WeaponAttachSocket = TEXT("weapon_r");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="First Person")
    FTransform FirstPersonWeaponRelativeTransform = FTransform::Identity;
};
