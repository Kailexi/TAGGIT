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

	// AI Logic Functions
	void UpdateTarget();
	void UpdateBehavior(float DeltaTime);
	void ChasePlayer(float DistanceToPlayer);
	void RunAway(float DistanceToPlayer);
	void PerformDashAttack(const FVector& DirectionToPlayer);

	// Helper functions
	float GetDistanceToPlayer() const;
	FVector GetDirectionToPlayer() const;
	bool IsPlayerVisible() const;
};