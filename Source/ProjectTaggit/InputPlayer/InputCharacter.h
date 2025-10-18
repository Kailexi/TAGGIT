#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ProjectTaggit/StaminaComponent.h"
#include "InputCharacter.generated.h"

UCLASS()
class PROJECTTAGGIT_API AInputCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaminaComponent* StaminaComponent;

protected:

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputMappingContext* InputMapping;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* SprintAction;

public:
	AInputCharacter();

protected:

	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;

public:

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	void Move(const FInputActionValue& InputValue);
	void Look(const FInputActionValue& InputValue);

	void Jump();
	void StartSprint();
	void EndSprint();


	//Movement-speeds / states
	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float SprintSpeed;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsJumping;




	//Stamina-relations
	UPROPERTY(EditAnywhere, Category = "Stamina");
	float SprintCostPerSecond;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float JumpStaminaCost = 250.0f;




	//HUD Accessors
	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetStaminaForHUD() const;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetMaxStaminaForHUD() const;

};