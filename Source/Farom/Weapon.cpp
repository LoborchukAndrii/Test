// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "FaromCharacter.h"

// Sets default values
AWeapon::AWeapon()
{
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(RootComponent);

	PickUp = CreateDefaultSubobject<USphereComponent>("PickUp");
	PickUp->AttachToComponent(Weapon, FAttachmentTransformRules::KeepRelativeTransform);

	MaxAmmo = 5;
	CurrentAmmo = 3;
}

void AWeapon::Shoot(FVector ToLocation)
{
	if (CurrentAmmo != 0)
	{
		FTransform SocketTransform = Weapon->GetSocketTransform("Muzzle");
		const FVector Location = SocketTransform.GetLocation();
		const FRotator Rotator = (ToLocation - Location).Rotation();
		FActorSpawnParameters SpawnParam;
		SpawnParam.Owner = this->GetOwner();
		GetWorld()->SpawnActor(BulletClass, &Location, &Rotator, SpawnParam);
		CurrentAmmo = CurrentAmmo - 1;
		OnAmmoChanged.Broadcast();
	}
}

void AWeapon::AmmoLoad(int& AllAmmo)
{
	CurrentAmmo = CurrentAmmo + 1;
	AllAmmo = AllAmmo - 1;
	OnAmmoChanged.Broadcast();
}
