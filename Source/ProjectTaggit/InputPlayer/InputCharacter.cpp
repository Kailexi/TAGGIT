#include "InputCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ProjectTaggit/StaminaComponent.h"
#include "ProjectTaggit/Public/AITagCharacter.h"
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


	//Sprint state
	bIsSprinting = false;

	//Jump state
	bIsJumping = false;

	//Crouch states
	bIsCrouching = false;
	bCrouchToggled = false;
	bCrouchKeyHeld = false;

	//Leap states
	bIsChargingLeap = false;
	LeapChargeTime = 0.0f;

	//Slide states
	bIsSliding = false;
	SlideTimeRemaining = 0.0f;
	SlideCooldownRemaining = 0.0f;
	SlideDirection = FVector::ZeroVector;

	//Mantle states
	bIsMantling = false;
	MantleTimeRemaining = 0.0f;
	MantleCooldownRemaining = 0.0f;
	MantleTargetLocation = FVector::ZeroVector;

	//Tag Dash states
	bIsDashing = false;
	bIsStunned = false;
	bTagFailedThisDash = false;
	TagDashTimeRemaining = 0.0f;
	TagCooldownRemaining = 0.0f;
	StunTimeRemaining = 0.0f;
	StunGracePeriodRemaining = 0.0f;
	TagDashDirection = FVector::ZeroVector;
	MantleStartLocation = FVector::ZeroVector;

}

void AInputCharacter::BeginPlay()
{
	Super::BeginPlay();
	bIsTagger = false;
	TagCooldownRemaining = 0.5f;

	if (bIsTagger) {
		UE_LOG(LogTemp, Warning, TEXT("Player started as TAGGER"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Player started as HIDER"));
	};

	// Force-initialize tag dash values if they're invalid
	if (TagDashDuration <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("TagDashDuration was %.3f! Forcing to 0.75"), TagDashDuration);
		TagDashDuration = 0.75f;
	}
	if (TagDashSpeed <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("TagDashSpeed was %.3f! Forcing to 2000"), TagDashSpeed);
		TagDashSpeed = 2000.0f;
	}
	if (TagCooldown <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("TagCooldown was %.3f! Forcing to 1.5"), TagCooldown);
		TagCooldown = 1.5f;
	}
	if (TagStunDuration <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("TagStunDuration was %.3f! Forcing to 0.5"), TagStunDuration);
		TagStunDuration = 0.5f;
	}

	UE_LOG(LogTemp, Log, TEXT("Tag Dash Config: Duration=%.3f, Speed=%.0f, Reach=%.0f, Cooldown=%.2f, Stun=%.2f"),
		TagDashDuration, TagDashSpeed, TagReachDistance, TagCooldown, TagStunDuration);

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

	// Animation variables update
	const FVector Velocity = GetVelocity();
	AnimSpeed = Velocity.Size2D();
	AnimVerticalVelocity = Velocity.Z;

	bAnimIsSprinting = bIsSprinting;
	bAnimIsJumping = bIsJumping;
	bAnimIsCrouching = bIsCrouching;
	bAnimIsSliding = bIsSliding;
	bAnimIsChargingLeap = bIsChargingLeap;
	bAnimIsDashing = bIsDashing;
	bAnimIsStunned = bIsStunned;

	//STUN HANDLING
	if (bIsStunned)
	{
		StunTimeRemaining -= DeltaTime;

		// Grace period - let momentum carry through briefly before freezing
		if (StunGracePeriodRemaining > 0.0f)
		{
			StunGracePeriodRemaining -= DeltaTime;
			// During grace period, gradually slow down instead of instant freeze
			GetCharacterMovement()->MaxWalkSpeed = TagDashSpeed * (StunGracePeriodRemaining / TagStunGracePeriod);
		}
		else
		{
			// Grace period over - fully freeze movement
			GetCharacterMovement()->MaxWalkSpeed = 0.0f;
			GetCharacterMovement()->MaxWalkSpeedCrouched = 0.0f;
			GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}

		// Debug display
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Red,
				FString::Printf(TEXT("STUNNED: %.2fs"), StunTimeRemaining));
		}

		if (StunTimeRemaining <= 0.0f)
		{
			bIsStunned = false;
			StunGracePeriodRemaining = 0.0f;
			UE_LOG(LogTemp, Log, TEXT("Stun ended - restoring movement"));

			// Restore movement speed
			if (bIsSprinting)
				GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
			else
				GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

			GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
		}

		return;
	}

	//TAG DASH HANDLING
	if (bIsDashing)
	{
		TagDashTimeRemaining -= DeltaTime;

		UE_LOG(LogTemp, VeryVerbose, TEXT("Dashing: TimeRemaining=%.3f, DeltaTime=%.3f"), TagDashTimeRemaining, DeltaTime);

		// Ensure we're NEVER in crouch mode during dash
		if (GetCharacterMovement()->IsCrouching())
		{
			UnCrouch();
			UE_LOG(LogTemp, Warning, TEXT("Forced uncrouch during dash!"));
		}

		TryTag();

		GetCharacterMovement()->MaxWalkSpeed = TagDashSpeed;
		AddMovementInput(TagDashDirection, 1.0f);

		if (TagDashTimeRemaining <= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Dash ending: TimeRemaining=%.3f"), TagDashTimeRemaining);
			EndDash();
		}

		return;
	}

	if (TagCooldownRemaining > 0.0f)
	{
		TagCooldownRemaining -= DeltaTime;
	}

	//Slide handling
	if (bIsSliding)
	{
		SlideTimeRemaining -= DeltaTime;
		GetCharacterMovement()->MaxWalkSpeed = SlideSpeed;
		GetCharacterMovement()->MaxWalkSpeedCrouched = SlideSpeed;
		if (SlideTimeRemaining <= 0.0f) EndSlide();
	}

	//Sprint handling/crouch speed handling/walk speed handling
	if (TagCooldownRemaining <= 0.0f)
	{
		if (bIsSprinting)
		{
			if (AnimSpeed > 0.0f) StaminaComponent->TryConsumeStamina(SprintCostPerSecond * DeltaTime);

			if (StaminaComponent->GetCurrentStamina() <= 0.0f) EndSprint();

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
	}
	else
	{
		if (bIsSprinting && AnimSpeed > 0.0f)
		{
			StaminaComponent->TryConsumeStamina(SprintCostPerSecond * DeltaTime);
			if (StaminaComponent->GetCurrentStamina() <= 0.0f) EndSprint();
		}
	}
	//Camera crouch offset handling,thanks my friend from Ravensoft and Call of Duty!
	// Use different interpolation and speed for crouch vs uncrouch
	if (TargetCrouchEyeOffset.Z > CrouchEyeOffset.Z)
		CrouchEyeOffset = FMath::VInterpConstantTo(CrouchEyeOffset, TargetCrouchEyeOffset, DeltaTime, UncrouchCameraTransitionSpeed);
	else
		CrouchEyeOffset = FMath::VInterpTo(CrouchEyeOffset, TargetCrouchEyeOffset, DeltaTime, CrouchCameraTransitionSpeed);

	// Leap charging handling
	if (bIsChargingLeap)
	{
		LeapChargeTime += DeltaTime;
		LeapChargeTime = FMath::Min(LeapChargeTime, LeapMaxChargeTime);
	}

	// Mantling handling
	if (bIsMantling)
	{
		MantleTimeRemaining -= DeltaTime;

		float Alpha = FMath::Clamp(1.0f - (MantleTimeRemaining / MantleDuration), 0.0f, 1.0f);

		// Smooth curve from Apex code lmao i love stealing
		float SmoothedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

		FVector NewLocation = FMath::Lerp(MantleStartLocation, MantleTargetLocation, SmoothedAlpha);
		SetActorLocation(NewLocation, true); //collision detection use sweep don't change please

		if (MantleTimeRemaining <= 0.0f)
		{
			bIsMantling = false;
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			MantleCooldownRemaining = MantleCooldown;

			FVector Forward = GetActorForwardVector();
			GetCharacterMovement()->Velocity = Forward * 300.0f;

			UE_LOG(LogTemp, Log, TEXT("Mantle completed"));
		}
	}

	if (MantleCooldownRemaining > 0.0f)
	{
		MantleCooldownRemaining -= DeltaTime;
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

		//Hold crouch/slide
		EnhancedInputComponent->BindAction(CrouchOrSlideAction, ETriggerEvent::Started, this, &AInputCharacter::CrouchOrSlideHoldStart);
		EnhancedInputComponent->BindAction(CrouchOrSlideAction, ETriggerEvent::Completed, this, &AInputCharacter::CrouchOrSlideHoldEnd);

		//Toggle crouch/slide
		EnhancedInputComponent->BindAction(ToggleCrouchOrSlideAction, ETriggerEvent::Started, this, &AInputCharacter::CrouchOrSlideToggle);

		//Jump/Leap
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AInputCharacter::StartJumpCharge);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AInputCharacter::ReleaseJump);

		//Mantle
		EnhancedInputComponent->BindAction(MantleAction, ETriggerEvent::Started, this, &AInputCharacter::TryMantle);

		//Tag Dash
		EnhancedInputComponent->BindAction(TagDashAction, ETriggerEvent::Started, this, &AInputCharacter::PerformTagDash);
	}
}

void AInputCharacter::Move(const FInputActionValue& InputValue)
{
	if (bIsStunned || bIsDashing) return;

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
	if (bIsStunned) return;
	FVector2D LookAxisVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AInputCharacter::StartJumpCharge()
{
	if (bIsJumping || bIsSliding || bIsMantling || bIsStunned || !CanJump()) return;

	if (bIsChargingLeap)
	{
		LeapChargeTime = 0.0f;
	}

	if (bIsCrouching) EndCrouch();

	bIsChargingLeap = true;
	LeapChargeTime = 0.0f;
}

void AInputCharacter::ReleaseJump()
{
	// If not charging, or charge time is below quick jump threshold, perform regular jump
	if (!bIsChargingLeap || LeapChargeTime < QuickJumpThreshold)
	{
		if (bIsChargingLeap)
		{
			bIsChargingLeap = false;
			LeapChargeTime = 0.0f;
		}

		// Normal jump - FREE, no stamina cost
		if (CanJump())
		{
			bIsJumping = true;
			Super::Jump();
			UE_LOG(LogTemp, VeryVerbose, TEXT("Normal jump (no stamina cost)"));
		}
		return;
	}

	// Charged leap - costs stamina
	bIsChargingLeap = false;
	bIsJumping = true;

	const bool bIsLeap = (LeapChargeTime >= LeapMinChargeTime);
	const float ChargeFraction = bIsLeap
		? FMath::Clamp((LeapChargeTime - LeapMinChargeTime) / (LeapMaxChargeTime - LeapMinChargeTime), 0.0f, 1.0f)
		: 0.0f;

	const float TotalStamina = LeapExtraStaminaCost * ChargeFraction;  // Only leap costs stamina

	if (!StaminaComponent->CanPerformAction(TotalStamina))
	{
		bIsJumping = false;
		LeapChargeTime = 0.0f;
		UE_LOG(LogTemp, Warning, TEXT("Leap failed: Insufficient stamina"));
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
		UE_LOG(LogTemp, Log, TEXT("Leap! Charge: %.2f%%, Stamina: %.0f"), ChargeFraction * 100.0f, TotalStamina);
	}

	MoveComp->JumpZVelocity = OriginalJumpZ;
	LeapChargeTime = 0.0f;
}

void AInputCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	bIsJumping = false;
}

void AInputCharacter::StartSprint()
{
	if (!bIsSprinting && !bIsCrouching && !bIsSliding && !bIsMantling && !bIsStunned)
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
	// NEVER allow crouch during dash or stun
	if (bIsDashing || bIsStunned)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot crouch during dash or stun"));
		return;
	}

	if (StaminaComponent->CanPerformAction(CrouchStaminaCost) && !bIsCrouching && !bIsMantling)
	{
		StaminaComponent->TryConsumeStamina(CrouchStaminaCost);
		bIsSprinting = false;
		Crouch();
		bIsCrouching = true;
		UE_LOG(LogTemp, Log, TEXT("Crouch started"));
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
	// Reduce camera offset to 50% for less extreme crouch view
	TargetCrouchEyeOffset.Z = -HalfHeightAdjust * 0.5f;
}

void AInputCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	TargetCrouchEyeOffset.Z = 0.0f;
}

void AInputCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);

	// NEVER apply camera offset during dash or stun - keep camera at eye level
	if (bIsDashing || bIsStunned)
	{
		return;
	}

	OutResult.Location += CrouchEyeOffset;
}

void AInputCharacter::StartSlide()
{
	if (!StaminaComponent->CanPerformAction(SlideStaminaCost) || bIsSliding || bIsJumping || bIsMantling || !bIsSprinting || bIsStunned || !GetCharacterMovement()->IsMovingOnGround() || SlideCooldownRemaining > 0.0f)
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
	if (bIsSprinting && !bIsSliding && !bIsJumping && !bIsStunned && GetCharacterMovement()->IsMovingOnGround() && StaminaComponent->CanPerformAction(SlideStaminaCost) && SlideCooldownRemaining <= 0.0f)

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
	// Can't toggle crouch during dash or stun (except to turn OFF)
	if ((bIsDashing || bIsStunned) && !bIsCrouching)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot toggle crouch ON during dash or stun"));
		return;
	}

	if (bIsSprinting && !bIsSliding && !bIsJumping && !bIsStunned && GetCharacterMovement()->IsMovingOnGround() && StaminaComponent->CanPerformAction(SlideStaminaCost) && SlideCooldownRemaining <= 0.0f)
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
			UE_LOG(LogTemp, Log, TEXT("Toggle crouch OFF"));
		}
		else
		{
			StartCrouch();
			if (bIsCrouching)  // Only set toggle if crouch actually started
			{
				bCrouchToggled = true;
				UE_LOG(LogTemp, Log, TEXT("Toggle crouch ON"));
			}
		}
	}
}


void AInputCharacter::TryMantle()
{
	if (bIsMantling || bIsSliding || bIsStunned || MantleCooldownRemaining > 0.0f)
	{
		return;
	}

	if (!StaminaComponent->CanPerformAction(MantleStaminaCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("Mantle failed: Insufficient stamina"));
		return;
	}

	//Tracing
	FVector CharacterLocation = GetActorLocation();
	FVector ForwardVector = GetActorForwardVector();
	FVector TraceStart = CharacterLocation + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f);
	FVector TraceEnd = TraceStart + (ForwardVector * MantleReachDistance);

	FHitResult WallHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	//WALL EXIST?
	bool bHitWall = GetWorld()->LineTraceSingleByChannel(WallHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	if (!bHitWall)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mantle failed: No wall detected"));
		return;
	}

	//DO WE HAVE A LEDGE?
	FVector UpTraceStart = WallHit.ImpactPoint;
	FVector UpTraceEnd = UpTraceStart + FVector(0, 0, MantleMaxHeight);

	FHitResult CeilingHit;
	bool bHitCeiling = GetWorld()->LineTraceSingleByChannel(CeilingHit, UpTraceStart, UpTraceEnd, ECC_Visibility, QueryParams);

	//SURFACE WHERE?
	FVector DownTraceStart = WallHit.ImpactPoint + FVector(0, 0, MantleMaxHeight) + (ForwardVector * 30.0f);
	FVector DownTraceEnd = DownTraceStart - FVector(0, 0, MantleMaxHeight + 100.0f);

	FHitResult LedgeHit;
	bool bFoundLedge = GetWorld()->LineTraceSingleByChannel(LedgeHit, DownTraceStart, DownTraceEnd, ECC_Visibility, QueryParams);

	if (!bFoundLedge)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mantle failed: No ledge surface found"));
		return;
	}

	float HeightDifference = LedgeHit.ImpactPoint.Z - CharacterLocation.Z;


	if (HeightDifference < MantleMinHeight || HeightDifference > MantleMaxHeight)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mantle failed: Height difference %f out of range (%f - %f)"),
			HeightDifference, MantleMinHeight, MantleMaxHeight);
		return;
	}

	if (bHitCeiling && (CeilingHit.ImpactPoint.Z - CharacterLocation.Z) < HeightDifference)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mantle failed: Ceiling obstruction"));
		return;
	}

	// Valid mantle found!
	StaminaComponent->TryConsumeStamina(MantleStaminaCost);

	MantleStartLocation = CharacterLocation;
	MantleTargetLocation = LedgeHit.ImpactPoint;
	MantleTargetLocation += ForwardVector * 30.0f;
	MantleTargetLocation.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); // Account for capsule height

	bIsMantling = true;
	MantleTimeRemaining = MantleDuration;

	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	UE_LOG(LogTemp, Log, TEXT("Mantle started! Height: %f, Target: %s"), HeightDifference, *MantleTargetLocation.ToString());
}

void AInputCharacter::PerformTagDash()
{
	// Auto-uncrouch/end slide if trying to dash - don't block, just fix the state
	if (bIsCrouching)
	{
		UnCrouch();
		bIsCrouching = false;
		bCrouchToggled = false;
		TargetCrouchEyeOffset = FVector::ZeroVector;
		CrouchEyeOffset = FVector::ZeroVector;
		UE_LOG(LogTemp, Log, TEXT("Auto-uncrouch for tag dash"));
	}
	if (bIsSliding)
	{
		EndSlide();
		UE_LOG(LogTemp, Log, TEXT("Auto-end slide for tag dash"));
	}

	// Cannot dash if: not tagger, stunned, already dashing, in air, or on cooldown
	if (!bIsTagger || bIsStunned || bIsDashing || bIsJumping || bIsMantling ||
		!GetCharacterMovement()->IsMovingOnGround() || TagCooldownRemaining > 0.0f)
	{
		if (!bIsTagger)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot tag dash: You are not the tagger!"));
		}
		return;
	}

	if (!StaminaComponent->CanPerformAction(TagDashStaminaCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot tag dash: Insufficient stamina"));
		return;
	}

	StaminaComponent->TryConsumeStamina(TagDashStaminaCost);

	bIsDashing = true;
	TagDashTimeRemaining = TagDashDuration;

	FVector InputDirection = GetCharacterMovement()->GetLastInputVector();
	if (InputDirection.SizeSquared() > 0.01f)
	{
		TagDashDirection = InputDirection.GetSafeNormal();
	}
	else
	{
		TagDashDirection = GetActorForwardVector();
	}

	// Force end all movement states before dash
	if (bIsSprinting) EndSprint();
	if (bIsSliding) EndSlide();
	if (bIsCrouching || GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
		bIsCrouching = false;
		bCrouchToggled = false;
	}

	// Give initial velocity boost for impactful dash feel
	FVector DashVelocity = TagDashDirection * TagDashSpeed;
	DashVelocity.Z = GetCharacterMovement()->Velocity.Z; // Keep vertical velocity
	GetCharacterMovement()->Velocity = DashVelocity;

	UE_LOG(LogTemp, Log, TEXT("Tag dash started! Direction: %s, Speed: %.0f"),
		*TagDashDirection.ToString(), TagDashSpeed);
}

void AInputCharacter::TryTag()
{
	if (!bIsDashing || !bIsTagger) return;


	FVector Start = GetActorLocation();
	FVector End = Start + (TagDashDirection * TagReachDistance);

	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;


	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(50.0f),  // 50cm radius for tag detection
		QueryParams
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AInputCharacter* OtherPlayer = Cast<AInputCharacter>(Hit.GetActor());

			if (OtherPlayer && OtherPlayer != this && !OtherPlayer->bIsTagger)
			{
				// Prevent AI-to-AI tagging (AIs can only tag the player)
				bool bThisIsAI = Cast<AAITagCharacter>(this) != nullptr;
				bool bOtherIsAI = Cast<AAITagCharacter>(OtherPlayer) != nullptr;

				if (bThisIsAI && bOtherIsAI)
				{
					// Both are AIs - skip this tag
					UE_LOG(LogTemp, Verbose, TEXT("Prevented AI-to-AI tag attempt"));
					continue;
				}

				// Successful tag!
				UE_LOG(LogTemp, Log, TEXT("TAG! Hit player: %s"), *OtherPlayer->GetName());

				// Notify Blueprint about successful tag
				OnSuccessfulTag(OtherPlayer);

				OtherPlayer->OnTagged(this);
				SetTaggerStatus(false);
				OnBecameHider();
				EndDash(true);  // Pass true to indicate successful tag

				// Apply LONGER stun to the NEW tagger (prevent instant re-tag)
				OtherPlayer->bIsStunned = true;
				OtherPlayer->StunTimeRemaining = TaggedStunDuration;  // Use longer stun duration
				OtherPlayer->StunGracePeriodRemaining = TagStunGracePeriod;

				break;  // Only tag one player at a time
			}
		}
	}
}

void AInputCharacter::OnTagged(AInputCharacter* TaggerPlayer)
{
	if (bIsTagger) return;  // Already tagger, ignore

	UE_LOG(LogTemp, Warning, TEXT("You've been tagged by %s! You are now IT!"),
		TaggerPlayer ? *TaggerPlayer->GetName() : TEXT("Unknown"));

	SetTaggerStatus(true);

	// Visual/audio feedback
	// For now, just log i am too lazy lmao
}

void AInputCharacter::SetTaggerStatus(bool bNewTaggerStatus)
{
	if (bIsTagger == bNewTaggerStatus) return;

	bIsTagger = bNewTaggerStatus;

	if (bIsTagger)
	{
		UE_LOG(LogTemp, Warning, TEXT("You are now the TAGGER! Chase others!"));
		OnBecameTagger();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("You are now HIDING! Avoid the tagger!"));
		OnBecameHider();
	}
}

void AInputCharacter::EndDash(bool bTagSuccessful)
{
	if (!bIsDashing) return;

	bIsDashing = false;
	TagDashTimeRemaining = 0.0f;
	TagCooldownRemaining = TagCooldown;

	// If tag failed, apply stun penalty to tagger
	if (!bTagSuccessful && bIsTagger)
	{
		bIsStunned = true;
		StunTimeRemaining = TagStunDuration;  // Full stun duration for failed tag (0.5s)
		StunGracePeriodRemaining = TagStunGracePeriod;  // Initialize grace period for momentum
		bTagFailedThisDash = true;

		UE_LOG(LogTemp, Warning, TEXT("Tag FAILED - Applying stun penalty (%.2fs) with grace period (%.2fs)"),
			StunTimeRemaining, StunGracePeriodRemaining);
	}
	else
	{
		bTagFailedThisDash = false;
		UE_LOG(LogTemp, Log, TEXT("Tag dash ended successfully"));
	}

	// Restore movement speed
	if (bIsSprinting)
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	else
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}




float AInputCharacter::GetStaminaForHUD() const
{
	return StaminaComponent ? StaminaComponent->GetCurrentStamina() : 0.0f;
}

float AInputCharacter::GetMaxStaminaForHUD() const
{
	return StaminaComponent ? StaminaComponent->GetMaxStamina() : 1000.0f;
}

float AInputCharacter::GetLeapChargePercentage() const
{
	if (!bIsChargingLeap) return 0.0f;
	return FMath::Clamp(LeapChargeTime / LeapMaxChargeTime, 0.0f, 1.0f);
}