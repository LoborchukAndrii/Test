// Copyright Epic Games, Inc. All Rights Reserved.

#include "FaromCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Weapon.h"

//////////////////////////////////////////////////////////////////////////
// AFaromCharacter

AFaromCharacter::AFaromCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	AmmoCount = 20;
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AFaromCharacter::OnOverlapBegin);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFaromCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump); 
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AFaromCharacter::Shoot);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AFaromCharacter::Reload);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AFaromCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AFaromCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AFaromCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AFaromCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AFaromCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AFaromCharacter::TouchStopped);
}

void AFaromCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AFaromCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AFaromCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AFaromCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AFaromCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AFaromCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AFaromCharacter::Shoot()
{
	if (Reloading.IsValid())
	{
		GetWorldTimerManager().ClearTimer(Reloading);
		OnStopReloading.Broadcast();
	}
	FVector Location = DoLineTrace();
	CharWeapon->Shoot(Location);
}

void AFaromCharacter::Reload()
{
	if (AmmoCount > 0 && CharWeapon->CurrentAmmo < CharWeapon->MaxAmmo && !Reloading.IsValid())
	{
		OnStartReloading.Broadcast();
		GetWorldTimerManager().SetTimer(Reloading, this, &AFaromCharacter::OnReload,  0.5f, true);
	}
}

void AFaromCharacter::OnReload()
{
	CharWeapon->AmmoLoad(AmmoCount);
	if (CharWeapon->CurrentAmmo == CharWeapon->MaxAmmo || AmmoCount == 0)
	{
		GetWorldTimerManager().ClearTimer(Reloading);
		OnStopReloading.Broadcast();
	}
}

FVector AFaromCharacter::DoLineTrace()
{
	FVector Result;
	FVector StartLocation = FollowCamera->GetComponentLocation();
	FVector Direction = FollowCamera->GetForwardVector();
	float TraceDistance = 1500;
	FVector EndLocation = Direction * 1500 + StartLocation;
	FHitResult HitDetails = FHitResult(ForceInit);
	bool bIsHit = GetWorld()->LineTraceSingleByChannel(HitDetails, StartLocation, EndLocation, ECollisionChannel::ECC_Visibility);
	if (bIsHit)
	{
		Result = HitDetails.Location;
	}
	else 
	{
		Result = HitDetails.TraceEnd;
	}
	return Result;
}

void AFaromCharacter::CallOnCharacterHit()
{
	OnCharacterHit.Broadcast();
}

void AFaromCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	CharWeapon = Cast<AWeapon>(OtherActor);
	if (CharWeapon->IsValidLowLevel())
	{
		CharWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("Weapon"));
		CharWeapon->PickUp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CharWeapon->SetOwner(this);
	}
}
