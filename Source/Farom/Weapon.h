// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAmmoChanged);

UCLASS()
class FAROM_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera)
	USkeletalMeshComponent* Weapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera)
	UShapeComponent* PickUp;

	UFUNCTION()
	void Shoot(FVector ToLocation);

	UFUNCTION()
	void AmmoLoad(int& AllAmmo);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int CurrentAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MaxAmmo;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> BulletClass;

	UPROPERTY(BlueprintAssignable)
	FAmmoChanged OnAmmoChanged;
};
