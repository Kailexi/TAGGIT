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
	class UInputAction* CrouchOrSlideAction;
	
	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* ToggleCrouchOrSlideAction;

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
	void StartSprint();
	void EndSprint();
	void StartCrouch();
	void EndCrouch();
	void CrouchOrSlideToggle();
	void CrouchOrSlideHoldStart();
	void CrouchOrSlideHoldEnd();
	void StartSlide();
	void EndSlide();
	void StartJumpCharge();
	void ReleaseJump();
	void LogCurrentSpeed();

	//Movement 
	UPROPERTY(EditAnywhere, Category = "Movement|Walk")
	float WalkSpeed = 500.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Sprint")
	float SprintSpeed = 1000.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
	
	//Crouch
	float CrouchSpeed = 300.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
	float CrouchHeight = 60.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	
	//Slide
	float SlideSpeed = 1200.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	float SlideDuration = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	float SlideCooldown = 0.5f;
	
	float SlideTimeRemaining;
	float SlideCooldownRemaining;
	FVector SlideDirection;

	//Leap
	UPROPERTY(EditAnywhere, Category = "Movement|Leap")
	float LeapMinChargeTime = 0.15f;
	UPROPERTY(EditAnywhere, Category = "Movement|Leap")
	float LeapMaxChargeTime = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Leap")
	float LeapHeightMultiplier = 4.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Leap")
	float LeapForwardBoost = 800.0f;

	
	
	float LeapChargeTime = 0.0f;


	// Crouching parameters
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CameraSettings")
	FVector CrouchEyeOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CameraSettings")
	FVector TargetCrouchEyeOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	float CrouchCameraTransitionSpeed = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	float UncrouchCameraTransitionSpeed = 1.0f;

	//Boolean states
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsJumping = false;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching = false;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSliding = false;
	
	bool bCrouchToggled = false;
	bool bCrouchKeyHeld = false;
	bool bIsChargingLeap = false;

	// Stamina relations
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float SprintCostPerSecond = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float JumpStaminaCost = 250.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float CrouchStaminaCost = 50.0f;
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float SlideStaminaCost = 150.0f;
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float LeapExtraStaminaCost = 200.0f;

	//Hud Accessors

	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetStaminaForHUD() const;
	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetMaxStaminaForHUD() const;
};