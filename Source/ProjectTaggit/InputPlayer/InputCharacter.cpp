
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include  "Camera/CameraComponent.h"


AInputCharacter::AInputCharacter()
{

	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");	
	Camera->SetupAttachment(GetRootComponent());
	Camera->bUsePawnControlRotation = true;



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
		
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AInputCharacter::Jump);
	
	
	
	}




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

void AInputCharacter::Jump()
{
	ACharacter::Jump();
}



