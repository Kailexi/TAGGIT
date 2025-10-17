
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputCharacter.generated.h"

UCLASS()
class PROJECTTAGGIT_API AInputCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAcess = "true"));
	class UCameraComponent* Camera;
	
protected:

	UPROPERTY(EditAnywhere, Category = "EnhancedInput");
	class UInputMappingContext* InputMapping;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput");
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput");
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput");
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput");
	class UInputAction* SprintAction;

public:
	// Sets default values for this character's properties
	AInputCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
		void Move(const FInputActionValue& InputValue);
		void Look(const FInputActionValue& InputValue);
		void Jump();

		void StartSprint();
		void EndSprint();
		
			
		UPROPERTY(EditAnywhere, Category = "Movement")
		float WalkSpeed;
		
		UPROPERTY(EditAnywhere, Category = "Movement")
		float SprintSpeed;

		bool bIsSprinting;

		//Stamina System
		void UpdateStamina();

		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float MaxStamina;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float CurrentStamina;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float StaminaDrainRate;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float StaminaRegenRate;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float StaminaRegenTime;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float StaminaRegenDelay;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float DelayBeforeRefill;
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float JumpStaminaCost;

		bool bIsJumping;
		bool bIsExhausted;
		bool bHasStamina;



};
