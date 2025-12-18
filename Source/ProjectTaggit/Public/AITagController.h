#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "AITagController.generated.h"

UCLASS()
class PROJECTTAGGIT_API AAITagController : public AAIController
{
	GENERATED_BODY()

public:
	AAITagController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

private:
	// Cached references
	UPROPERTY()
	class AAITagCharacter* ControlledAI = nullptr;

	UPROPERTY()
	class AInputCharacter* TargetPlayer = nullptr;

	// Behavior timers
	float UpdateTargetTimer = 0.0f;
	float UpdateTargetInterval = 0.5f;

	float BehaviorUpdateTimer = 0.0f;
	float BehaviorUpdateInterval = 0.1f;

	float AbilityCheckTimer = 0.0f;
	float AbilityCheckInterval = 0.3f;

	// AI Logic Functions
	void UpdateTarget();
	void UpdateBehavior(float DeltaTime);
	void ChasePlayer(float DistanceToPlayer);
	void RunAway(float DistanceToPlayer);
	void PerformDashAttack(const FVector& DirectionToPlayer);

	void CheckAdvancedAbilities(float DistanceToPlayer, bool bIsChasing);
	void TryLeap(const FVector& TargetDirection);
	void TrySlide();
	void TryMantle();
	void ManageCrouch(bool bShouldCrouch);

	bool ShouldConserveStamina() const;
	float GetStaminaReserve() const;

	FVector PredictPlayerPosition(float TimeAhead) const;
	FVector GetPredictedDirection() const;

	// Helper functions
	float GetDistanceToPlayer() const;
	FVector GetDirectionToPlayer() const;
	bool IsPlayerVisible() const;
	float GetHeightDifference() const;
};