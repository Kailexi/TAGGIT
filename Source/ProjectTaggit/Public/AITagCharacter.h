#pragma once
#include "CoreMinimal.h"
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "AITagCharacter.generated.h"


UENUM(BlueprintType)
enum class EAIDifficulty : uint8
{
	Easy UMETA(DisplayName = "Easy"),
	Medium UMETA(DisplayName = "Medium"),
	Hard UMETA(DisplayName = "Hard")
};


UCLASS()
class PROJECTTAGGIT_API AAITagCharacter : public AInputCharacter
{
	GENERATED_BODY()

public:
	AAITagCharacter();

	// AI Difficulty Setting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Settings")
	EAIDifficulty Difficulty = EAIDifficulty::Medium;

	// AI Behavior Parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	float ChaseRadius = 2000.0f;  // (20m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	float DashRadius = 400.0f;    // (4m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	float SprintRadius = 1000.0f; //  (10m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	float PatrolRadius = 500.0f;  //  (5m)

	// AI Movement Functions
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void AIMoveToward(FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void AIPerformTagDash(FVector Direction);

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void AIStartSprint();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void AIEndSprint();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement|Advanced")
	void AIStartLeap(float ChargeTime = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "AI|Movement|Advanced")
	void AIReleaseLeap();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement|Advanced")
	void AIStartSlide();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement|Advanced")
	void AITryMantle();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement|Advanced")
	void AIStartCrouch();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement|Advanced")
	void AIEndCrouch();

	// AI State Queries
	UFUNCTION(BlueprintCallable, Category = "AI|State")
	float GetDistanceToPlayer() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	AInputCharacter* GetPlayerCharacter() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	bool CanUseDash() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	float GetCurrentStamina() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	float GetStaminaPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	bool HasStaminaFor(float Cost) const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	bool IsJumping() const { return bIsJumping; }

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	bool IsMantling() const { return bIsMantling; }

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	bool IsChargingLeap() const { return bIsChargingLeap; }

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	bool IsCrouching() const { return bIsCrouching; }

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	float GetSlideCooldown() const { return SlideCooldownRemaining; }

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	float GetMantleCooldown() const { return MantleCooldownRemaining; }

	UFUNCTION(BlueprintCallable, Category = "AI|Costs")
	float GetLeapCost() const { return LeapExtraStaminaCost; }

	UFUNCTION(BlueprintCallable, Category = "AI|Costs")
	float GetSlideCost() const { return SlideStaminaCost; }

	UFUNCTION(BlueprintCallable, Category = "AI|Costs")
	float GetMantleCost() const { return MantleStaminaCost; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// Cached player reference
	UPROPERTY()
	AInputCharacter* CachedPlayer = nullptr;

	//timers
	float UpdatePlayerCacheTimer = 0.0f;
	float UpdatePlayerCacheInterval = 1.0f;

	float LeapChargeTimer = 0.0f;
	float TargetLeapChargeTime = 0.0f;
	bool bIsChargingLeapAI = false;
};