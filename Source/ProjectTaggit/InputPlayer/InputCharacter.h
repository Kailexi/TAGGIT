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

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* SlideAction;

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* MantleAction;

public:
	AInputCharacter();

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;

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
	void StartCrouch();
	void EndCrouch();
	void ToggleCrouch();
	void LogCurrentSpeed();
	void StartSlide();
	void EndSlide();


	//	Speeds/Durations
	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 500.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float SprintSpeed = 1000.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float CrouchSpeed = 300.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float CrouchHeight = 50.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float SlideSpeed = 1200.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float SlideDuration = 1.0f;
	float SlideTimeRemaining;

	//	Crouching parameters
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CameraSettings")
	FVector CrouchEyeOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CameraSettings")
	FVector TargetCrouchEyeOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	float CrouchCameraTransitionSpeed = 5.0f;
		
	//Boolean states
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsJumping;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSliding;


	//Stamina relations
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float SprintCostPerSecond = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float JumpStaminaCost = 250.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float CrouchStaminaCost = 50.0f;
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float SlideStaminaCost = 150.0f;
	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetStaminaForHUD() const;
	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetMaxStaminaForHUD() const;
};