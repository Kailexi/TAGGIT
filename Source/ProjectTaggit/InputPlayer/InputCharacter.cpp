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
}

void AInputCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AInputCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AInputCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Input mapping context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		// Get local player subsystem to add input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Add input context
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind the input actions
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AInputCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AInputCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AInputCharacter::Jump);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AInputCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed , this, &AInputCharacter::EndSprint);
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

bool bIsJumping = false;

void AInputCharacter::Jump()
{
    if (bIsJumping)
    {
        UE_LOG(LogTemp, Warning, TEXT("Jump already in progress!"));
        return;
    }

    bIsJumping = true;
    UE_LOG(LogTemp, Warning, TEXT("CurrentStamina before jump: %f"), StaminaComponent->GetCurrentStamina());
    if (StaminaComponent->TryConsumeStamina(JumpStaminaCost)) 
    {
        ACharacter::Jump();
    }
    UE_LOG(LogTemp, Warning, TEXT("CurrentStamina after jump: %f"), StaminaComponent->GetCurrentStamina());
}

void AInputCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    bIsJumping = false; // Reset the flag when the character lands
}

void AInputCharacter::StartSprint()
{
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

void AInputCharacter::Move(const FInputActionValue& InputValue)
{
	FVector2D InputVector = InputValue.Get<FVector2D>();
	if (IsValid(Controller))
	{
		//Get forward/backward direction
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement input
		AddMovementInput(ForwardDirection, InputVector.Y);
		AddMovementInput(RightDirection, InputVector.X);
	}
}

void AInputCharacter::Look(const FInputActionValue& InputValue)
{
	FVector2D InputVector = InputValue.Get<FVector2D>();
	if (IsValid(Controller))
	{
		AddControllerYawInput(InputVector.X);
		AddControllerPitchInput(InputVector.Y);
	}
}