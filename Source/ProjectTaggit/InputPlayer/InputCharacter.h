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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaminaComponent* StaminaComponent;

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

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* TagDashAction;

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
	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
	void OnBecameTagger();

	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
	void OnBecameHider();

	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
	void OnSuccessfulTag(AInputCharacter* TaggedPlayer);

	UFUNCTION(BlueprintCallable, Category = "GameState")
	bool IsTagger() const { return bIsTagger; }

	UFUNCTION(BlueprintCallable, Category = "GameState")
	bool IsStunned() const { return bIsStunned; }

	UFUNCTION(BlueprintCallable, Category = "GameState")
	bool DidTagFail() const { return bTagFailedThisDash; }

	UFUNCTION(BlueprintCallable, Category = "GameState")
	bool IsSprinting() const { return bIsSprinting; }

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
	void PerformTagDash();
	void TryTag();
	void OnTagged(AInputCharacter* TaggerPlayer);
	void EndDash(bool bTagSuccessful = false);

	UFUNCTION(BlueprintCallable, Category = "GameState")
	void SetTaggerStatus(bool bNewTaggerStatus);

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
	float LeapHeightMultiplier = 6.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Leap")
	float LeapForwardBoost = 400.0f;
	UPROPERTY(EditAnywhere, Category = "Movement|Leap")
	float QuickJumpThreshold = 0.08f;

	float LeapChargeTime = 0.0f;


	//Mantle Settings
	UPROPERTY(EditAnywhere, Category = "Movement|Mantle")
	float MantleReachDistance = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Movement|Mantle")
	float MantleMaxHeight = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Movement|Mantle")
	float MantleMinHeight = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Movement|Mantle")
	float MantleDuration = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Movement|Mantle")
	float MantleCooldown = 0.5f;
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void TryMantle();

	float MantleTimeRemaining = 0.0f;
	float MantleCooldownRemaining = 0.0f;
	FVector MantleTargetLocation;
	FVector MantleStartLocation;

	//Tagging/TAGDASH
	UPROPERTY(EditAnywhere, Category = "Movement|TagDash")
	float TagDashSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Movement|TagDash")
	float TagDashDuration = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Movement|TagDash")
	float TagReachDistance = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Movement|TagDash")
	float TagStunDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Movement|TagDash")
	float TagStunGracePeriod = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Movement|TagDash")
	float TagCooldown = 1.5f;

	float TagDashTimeRemaining = 0.0f;
	float TagCooldownRemaining = 0.0f;
	float StunTimeRemaining = 0.0f;
	float StunGracePeriodRemaining = 0.0f;
	FVector TagDashDirection;

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
	bool bIsMantling = false;

	bool bIsDashing = false;
	bool bIsStunned = false;
	bool bTagFailedThisDash = false;  // Track if dash ended without tagging anyone

	UPROPERTY(BlueprintReadWrite, Category = "GameState")
	bool bIsTagger = false;



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
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float MantleStaminaCost = 50.0f;
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float TagDashStaminaCost = 100.0f;



	//Hud Accessors

	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetStaminaForHUD() const;
	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetMaxStaminaForHUD() const;
	UFUNCTION(BlueprintCallable, Category = "HUD")
	float GetLeapChargePercentage() const;

	// Animation variables
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bAnimIsSprinting = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bAnimIsJumping = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bAnimIsCrouching = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bAnimIsSliding = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bAnimIsChargingLeap = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AnimSpeed = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AnimVerticalVelocity = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bAnimIsDashing = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bAnimIsStunned = false;

};