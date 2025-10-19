#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectTaggit/StaminaComponent.h"

AInputCharacter::AInputCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(GetRootComponent());
	Camera->bUsePawnControlRotation = true;

	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>("StaminaComponent");
	CrouchEyeOffset = FVector(0.f);

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	
	bIsSprinting = false;
	bIsJumping = false;
	bIsCrouching = false;
}

void AInputCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AInputCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!StaminaComponent) return;

	if (bIsSprinting)
	{
		// Drain stamina while sprinting
		bool bConsumed = StaminaComponent->TryConsumeStamina(SprintCostPerSecond * DeltaTime);
		if (!bConsumed || StaminaComponent->GetCurrentStamina() <= 0.0f)
		{
			EndSprint();
			EndCrouch(); // End crouch if sprinting stops due to stamina
		}
	}

	// Smoothly interpolate camera for crouch transitions
	if (CrouchEyeOffset != FVector::ZeroVector)
	{
		CrouchEyeOffset = FMath::VInterpTo(CrouchEyeOffset, FVector::ZeroVector, DeltaTime, 5.0f);
	}

	LogCurrentSpeed();

}

void AInputCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AInputCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AInputCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AInputCharacter::Jump);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AInputCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AInputCharacter::EndSprint);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AInputCharacter::StartCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AInputCharacter::EndCrouch);
	}
}

float AInputCharacter::GetStaminaForHUD() const
{
	return StaminaComponent ? StaminaComponent->GetCurrentStamina() : 0.0f;
}

float AInputCharacter::GetMaxStaminaForHUD() const
{
	return StaminaComponent ? StaminaComponent->GetMaxStamina() : 0.0f;
}

void AInputCharacter::Jump()
{
	if (!StaminaComponent) return;

	if (StaminaComponent->CanPerformAction(JumpStaminaCost) && !bIsJumping)
	{
		StaminaComponent->TryConsumeStamina(JumpStaminaCost);
		bIsJumping = true;
		ACharacter::Jump();
		UE_LOG(LogTemp, Log, TEXT("Jump executed, stamina: %f"), StaminaComponent->GetCurrentStamina());
	}
}

void AInputCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	bIsJumping = false;
}

void AInputCharacter::StartSprint()
{
	if (!StaminaComponent) return;

	if (StaminaComponent->CanPerformAction(SprintCostPerSecond))
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void AInputCharacter::EndSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AInputCharacter::StartCrouch()
{
	if (!StaminaComponent || bIsSprinting || bIsCrouching) return;

	if (StaminaComponent->CanPerformAction(CrouchStaminaCost))
	{
		StaminaComponent->TryConsumeStamina(CrouchStaminaCost);
		bIsCrouching = true;
		ACharacter::Crouch();
		UE_LOG(LogTemp, Log, TEXT("StartCrouch called, stamina: %f"), StaminaComponent->GetCurrentStamina());
	}
}

void AInputCharacter::EndCrouch()
{
	if (bIsCrouching)
	{
		bIsCrouching = false;
		ACharacter::UnCrouch();
		UE_LOG(LogTemp, Log, TEXT("EndCrouch called"));
	}
}

void AInputCharacter::LogCurrentSpeed()
{
	UE_LOG(LogTemp, Log, TEXT("Current Speed: %f"), GetVelocity().Size());
}

void AInputCharacter::Look(const FInputActionValue& InputValue)
{
	FVector2D InputVector = InputValue.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(InputVector.X);
		AddControllerPitchInput(InputVector.Y);
	}
}

void AInputCharacter::Move(const FInputActionValue& InputValue)
{
	FVector2D InputVector = InputValue.Get<FVector2D>();
	if (IsValid(Controller))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, InputVector.Y);
		AddMovementInput(RightDirection, InputVector.X);
	}
}

void AInputCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CrouchEyeOffset.Z -= HalfHeightAdjust;
}

void AInputCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CrouchEyeOffset.Z += HalfHeightAdjust; 
}

void AInputCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);
	OutResult.Location += CrouchEyeOffset; 
}