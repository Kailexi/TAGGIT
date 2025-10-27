#include "InputCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ProjectTaggit/StaminaComponent.h"
#include "Math/UnrealMathUtility.h"

AInputCharacter::AInputCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetRootComponent());
	Camera->bUsePawnControlRotation = true;

	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	CrouchEyeOffset = FVector::ZeroVector;
	TargetCrouchEyeOffset = FVector::ZeroVector;

	bIsSprinting = false;
	bIsJumping = false;
	bIsCrouching = false;
	bIsSliding = false;
	bIsChargingLeap = false;
	LeapChargeTime = 0.0f;
	SlideTimeRemaining = 0.0f;
	SlideCooldownRemaining = 0.0f;
	SlideDirection = FVector::ZeroVector;
	
	bCrouchToggled = false;
	bCrouchKeyHeld = false;
}

void AInputCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}
}

void AInputCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSliding)
	{
		SlideTimeRemaining -= DeltaTime;
		GetCharacterMovement()->MaxWalkSpeed = SlideSpeed;
		GetCharacterMovement()->MaxWalkSpeedCrouched = SlideSpeed;
		if (SlideTimeRemaining <= 0.0f)
		{
			EndSlide();
		}
	}
	else if (bIsSprinting)
	{
		if (GetVelocity().Size2D() > 0.0f)
		{
			StaminaComponent->TryConsumeStamina(SprintCostPerSecond * DeltaTime);
		}
		if (StaminaComponent->GetCurrentStamina() <= 0.0f)
		{
			EndSprint();
		}
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
	else if (bIsCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
		GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}

	if (SlideCooldownRemaining > 0.0f)
	{
		SlideCooldownRemaining -= DeltaTime;
	}

	// Use different interpolation and speed for crouch vs uncrouch
	if (TargetCrouchEyeOffset.Z > CrouchEyeOffset.Z)
	{
		CrouchEyeOffset = FMath::VInterpConstantTo(CrouchEyeOffset, TargetCrouchEyeOffset, DeltaTime, UncrouchCameraTransitionSpeed);
	}
	else
	{
		CrouchEyeOffset = FMath::VInterpTo(CrouchEyeOffset, TargetCrouchEyeOffset, DeltaTime, CrouchCameraTransitionSpeed);
	}

	if (bIsChargingLeap)
	{
		LeapChargeTime += DeltaTime;
		LeapChargeTime = FMath::Min(LeapChargeTime, LeapMaxChargeTime);
	}
}

void AInputCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AInputCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AInputCharacter::Look);
		
		//Sprint

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AInputCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AInputCharacter::EndSprint);
		
		//Toggle crouch/slide
		EnhancedInputComponent->BindAction(CrouchOrSlideAction, ETriggerEvent::Started, this, &AInputCharacter::CrouchOrSlideHoldStart);
		EnhancedInputComponent->BindAction(CrouchOrSlideAction, ETriggerEvent::Completed, this, &AInputCharacter::CrouchOrSlideHoldEnd);
		
		//Toggle crouch/slide
		EnhancedInputComponent->BindAction(ToggleCrouchOrSlideAction, ETriggerEvent::Started, this, &AInputCharacter::CrouchOrSlideToggle);
		
		//Jump/Leap
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AInputCharacter::StartJumpCharge);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AInputCharacter::ReleaseJump);
	}
}

void AInputCharacter::Move(const FInputActionValue& InputValue)
{
	FVector2D MovementVector = InputValue.Get<FVector2D>();
	if (!Controller) return;

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (bIsSliding)
	{
		AddMovementInput(Right, MovementVector.X * 0.3f);
	}
	else
	{
		AddMovementInput(Forward, MovementVector.Y);
		AddMovementInput(Right, MovementVector.X);
	}
}

void AInputCharacter::Look(const FInputActionValue& InputValue)
{
	FVector2D LookAxisVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AInputCharacter::StartJumpCharge()
{
	if (bIsChargingLeap || bIsJumping || bIsSliding || !CanJump()) return;

	if (bIsCrouching) EndCrouch();

	bIsChargingLeap = true;
	LeapChargeTime = 0.0f;
}

void AInputCharacter::ReleaseJump()
{
	if (!bIsChargingLeap)
	{
		if (CanJump() && StaminaComponent->CanPerformAction(JumpStaminaCost))
		{
			StaminaComponent->TryConsumeStamina(JumpStaminaCost);
			bIsJumping = true;
			Super::Jump();
		}
		return;
	}

	bIsChargingLeap = false;
	bIsJumping = true;

	const bool bIsLeap = (LeapChargeTime >= LeapMinChargeTime);
	const float ChargeFraction = bIsLeap
		? FMath::Clamp((LeapChargeTime - LeapMinChargeTime) / (LeapMaxChargeTime - LeapMinChargeTime), 0.0f, 1.0f)
		: 0.0f;

	const float TotalStamina = JumpStaminaCost + (bIsLeap ? LeapExtraStaminaCost * ChargeFraction : 0.0f);

	if (!StaminaComponent->CanPerformAction(TotalStamina))
	{
		bIsJumping = false;
		return;
	}

	StaminaComponent->TryConsumeStamina(TotalStamina);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const float OriginalJumpZ = MoveComp->JumpZVelocity;

	MoveComp->JumpZVelocity *= (1.0f + ChargeFraction * (LeapHeightMultiplier - 1.0f));

	if (bIsCrouching) EndCrouch();

	Super::Jump();

	if (bIsLeap)
	{
		const FVector ForwardBoost = GetActorForwardVector() * (LeapForwardBoost * ChargeFraction);
		MoveComp->Velocity += ForwardBoost;
	}

	MoveComp->JumpZVelocity = OriginalJumpZ;
}

void AInputCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	bIsJumping = false;
}

void AInputCharacter::StartSprint()
{
	if (!bIsSprinting && !bIsCrouching && !bIsSliding)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void AInputCharacter::EndSprint()
{
	if (bIsSprinting)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}


//Crouch
void AInputCharacter::StartCrouch()
{
	if (StaminaComponent->GetCurrentStamina() >= CrouchStaminaCost && !bIsCrouching)
	{
		StaminaComponent->TryConsumeStamina(CrouchStaminaCost);
		bIsSprinting = false;
		Crouch();
		bIsCrouching = true;
	}
}

void AInputCharacter::EndCrouch()
{
	if (bIsCrouching)
	{
		UnCrouch();
		bIsCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
		bCrouchToggled = false;
	}
}

void AInputCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	TargetCrouchEyeOffset.Z = -HalfHeightAdjust;
}

void AInputCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	TargetCrouchEyeOffset.Z = 0.0f;
}

void AInputCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);
	OutResult.Location += CrouchEyeOffset;
}

void AInputCharacter::StartSlide()
{
	if (!StaminaComponent->CanPerformAction(SlideStaminaCost) || bIsSliding || bIsJumping || !bIsSprinting || !GetCharacterMovement()->IsMovingOnGround() || SlideCooldownRemaining > 0.0f)
		return;

	if (!bIsCrouching) StartCrouch();

	StaminaComponent->TryConsumeStamina(SlideStaminaCost);
	bIsSliding = true;
	SlideTimeRemaining = SlideDuration;
	SlideCooldownRemaining = SlideCooldown;

	GetCharacterMovement()->GroundFriction = 0.5f;
	GetCharacterMovement()->BrakingDecelerationWalking = 50.0f;

	FVector CurrentVelocity = GetVelocity();
	SlideDirection = CurrentVelocity.Size2D() > 0.0f ? CurrentVelocity.GetSafeNormal2D() : GetActorForwardVector();
	FVector SlideVel = SlideDirection * SlideSpeed;
	SlideVel.Z = CurrentVelocity.Z;
	GetCharacterMovement()->Velocity = SlideVel;
}

void AInputCharacter::EndSlide()
{
	if (!bIsSliding) return;

	bIsSliding = false;
	SlideTimeRemaining = 0.0f;
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	if (bIsCrouching && !bCrouchToggled && !bCrouchKeyHeld)
	{
		EndCrouch();
	}
}

void AInputCharacter::CrouchOrSlideHoldStart()
{
	bCrouchKeyHeld = true;
	if (bIsSprinting && !bIsSliding && !bIsJumping && GetCharacterMovement()->IsMovingOnGround() && StaminaComponent->CanPerformAction(SlideStaminaCost) && SlideCooldownRemaining <= 0.0f)
	{
		StartSlide();
	}
	else if (!bIsSliding && !bIsCrouching)
	{
		StartCrouch();
	}
}

void AInputCharacter::CrouchOrSlideHoldEnd()
{
	bCrouchKeyHeld = false;
	if (bIsCrouching && !bCrouchToggled && !bIsSliding)
	{
		EndCrouch();
	}
}

void AInputCharacter::CrouchOrSlideToggle()
{
	if (bIsSprinting && !bIsSliding && !bIsJumping && GetCharacterMovement()->IsMovingOnGround() && StaminaComponent->CanPerformAction(SlideStaminaCost) && SlideCooldownRemaining <= 0.0f)
	{
		StartSlide();
		bCrouchToggled = false;
	}
	else if (!bIsSliding)
	{
		if (bIsCrouching)
		{
			EndCrouch();
			bCrouchToggled = false;
		}
		else
		{
			StartCrouch();
			bCrouchToggled = true;
		}
	}
}

float AInputCharacter::GetStaminaForHUD() const
{
	return StaminaComponent ? StaminaComponent->GetCurrentStamina() : 0.0f;
}

float AInputCharacter::GetMaxStaminaForHUD() const
{
	return StaminaComponent ? StaminaComponent->GetMaxStamina() : 1000.0f;
}