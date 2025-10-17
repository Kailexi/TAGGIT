
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

AInputCharacter::AInputCharacter()
{

	PrimaryActorTick.bCanEverTick = true;

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

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller));
	{
		// Get local player subsystem to add input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Add input context

			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}

	if (UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(TestAction, ETriggerEvent::Triggered, this,  &AInputCharacter::TestInput);
	
	}

}

void AInputCharacter::TestInput()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Input Action Triggered!"));

}

