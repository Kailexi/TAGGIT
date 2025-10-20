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

	CrouchEyeOffset = FVector(0.0f, 0.0f, 0.0f);
	TargetCrouchEyeOffset = FVector(0.0f, 0.0f, 0.0f);

	bIsSprinting = false;
	bIsJumping = false;
	bIsCrouching = false;
	bIsSliding = false;
	SlideTimeRemaining = 0.0f;
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

	if (bIsSprinting)
	{
		StaminaComponent->TryConsumeStamina(SprintCostPerSecond * DeltaTime);
		if (StaminaComponent->GetCurrentStamina() <= 0.0f)
		{
			EndSprint();
		}
	}
	CrouchEyeOffset = FMath::VInterpTo(CrouchEyeOffset, TargetCrouchEyeOffset, DeltaTime, CrouchCameraTransitionSpeed);
}

void AInputCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AInputCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AInputCharacter::Look);
		
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AInputCharacter::Jump);
		
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AInputCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AInputCharacter::EndSprint);
		
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AInputCharacter::ToggleCrouch);
		
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Triggered, this, &AInputCharacter::StartSlide);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Triggered, this, &AInputCharacter::EndSlide);
	
	}
}

void AInputCharacter::Move(const FInputActionValue& InputValue)
{
	FVector2D MovementVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardDirection, MovementVector.Y);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(RightDirection, MovementVector.X);
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

void AInputCharacter::Jump()
{
	if (StaminaComponent->CanPerformAction(JumpStaminaCost) && !bIsJumping)
	{
		bool bCanJump = CanJump();
		if (!bCanJump && bIsCrouching)
		{
			EndCrouch();
			bCanJump = CanJump();
		}
		if (bCanJump)
		{
			StaminaComponent->TryConsumeStamina(JumpStaminaCost);
			bIsJumping = true;
			Super::Jump();
		}
	}
}

void AInputCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	bIsJumping = false;
}

void AInputCharacter::StartSprint()
{
	if (StaminaComponent->GetCurrentStamina() > 0.0f && !bIsSprinting && !bIsCrouching)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		UE_LOG(LogTemp, Log, TEXT("Sprint started, stamina: %f"), StaminaComponent->GetCurrentStamina());
	}
}

void AInputCharacter::EndSprint()
{
	bIsSprinting = false;
	if (!bIsCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	UE_LOG(LogTemp, Log, TEXT("Sprint ended, stamina: %f"), StaminaComponent->GetCurrentStamina());
}

void AInputCharacter::StartCrouch()
{
	if (StaminaComponent->GetCurrentStamina() >= CrouchStaminaCost && !bIsCrouching)
	{
		UE_LOG(LogTemp, Log, TEXT("StartCrouch called, stamina: %f"), StaminaComponent->GetCurrentStamina());
		StaminaComponent->TryConsumeStamina(CrouchStaminaCost);
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
		Crouch();
		bIsCrouching = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot crouch: Insufficient stamina or already crouching"));
	}
}

void AInputCharacter::EndCrouch()
{
	if (bIsCrouching)
	{
		UE_LOG(LogTemp, Log, TEXT("EndCrouch called"));
		UnCrouch();
		bIsCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

void AInputCharacter::ToggleCrouch()
{
	if (!bIsCrouching)
	{
		StartCrouch();
	}
	else
	{
		EndCrouch();
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

void AInputCharacter::LogCurrentSpeed()
{
	FVector Velocity = GetCharacterMovement()->Velocity;
	float Speed = Velocity.Size2D();
	FString MovementState = bIsSprinting ? TEXT("Sprinting") : bIsCrouching ? TEXT("Crouching") : bIsJumping ? TEXT("Jumping") : TEXT("Walking");
	UE_LOG(LogTemp, Log, TEXT("Current Speed: %f cm/s (%s)"), Speed, *MovementState);
}

void AInputCharacter::StartSlide()
{
	if (StaminaComponent->CanPerformAction(SlideStaminaCost) && !bIsSliding && !bIsJumping && bIsSprinting)
	{
		// Ensure character is crouched during slide
		if (!bIsCrouching)
		{
			StartCrouch();
		}

		// Consume stamina and start sliding
		StaminaComponent->TryConsumeStamina(SlideStaminaCost);
		bIsSliding = true;
		SlideTimeRemaining = SlideDuration;
		GetCharacterMovement()->MaxWalkSpeed = SlideSpeed;
		GetCharacterMovement()->GroundFriction = 0.5f; // Reduce friction for smooth sliding
		GetCharacterMovement()->BrakingDecelerationWalking = 100.0f; // Slow deceleration to maintain momentum

		UE_LOG(LogTemp, Log, TEXT("Slide started, stamina: %f"), StaminaComponent->GetCurrentStamina());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot slide: Insufficient stamina, already sliding, not sprinting, or jumping"));
	}
}

void AInputCharacter::EndSlide()
{
	if (bIsSliding)
	{
		bIsSliding = false;
		SlideTimeRemaining = 0.0f;
		GetCharacterMovement()->GroundFriction = 8.0f; // Restore default friction
		GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f; // Restore default deceleration
		GetCharacterMovement()->MaxWalkSpeed = bIsCrouching ? CrouchSpeed : (bIsSprinting ? SprintSpeed : WalkSpeed);

		UE_LOG(LogTemp, Log, TEXT("Slide ended, stamina: %f"), StaminaComponent->GetCurrentStamina());
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