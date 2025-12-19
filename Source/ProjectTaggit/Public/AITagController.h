#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "AITagController.generated.h"
USTRUCT(BlueprintType)
struct FAIObservation
{
	GENERATED_BODY()

	// Player tracking
	UPROPERTY(BlueprintReadOnly, Category = "Observation|Player")
	FVector PlayerPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Player")
	FVector PlayerVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Player")
	float PlayerSpeed = 0.0f;  // Magnitude of velocity

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Player")
	FVector PlayerDirection = FVector::ZeroVector;  

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Player")
	bool bPlayerIsTagger = false;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Player")
	bool bPlayerIsSprinting = false;

	// AI self-awareness
	UPROPERTY(BlueprintReadOnly, Category = "Observation|Self")
	FVector AIPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Self")
	FVector AIVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Self")
	float AISpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Self")
	float StaminaPercent = 0.0f;  

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Self")
	bool bAIIsTagger = false;

	// Spatial awareness
	UPROPERTY(BlueprintReadOnly, Category = "Observation|Spatial")
	float DistanceToPlayer = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Spatial")
	float HeightDifference = 0.0f;  

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Spatial")
	FVector DirectionToPlayer = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Spatial")
	bool bPlayerVisible = false;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Environment")
	bool bObstacleAhead = false; 

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Environment")
	bool bObstacleLeft = false;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Environment")
	bool bObstacleRight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Environment")
	bool bEdgeAhead = false;  

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Environment")
	float ObstacleDistance = 0.0f;  

	UPROPERTY(BlueprintReadOnly, Category = "Observation|Environment")
	bool bCoverNearby = false;  
};


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

	FAIObservation CurrentObservation;
	FVector PreviousPlayerPosition = FVector::ZeroVector;
	float ObservationUpdateTimer = 0.0f;
	float ObservationUpdateInterval = 0.1f;  // Update observations every 0.1s

	float SensorRange = 500.0f;  
	float EdgeDetectionRange = 300.0f;  

	//Phase 3D difficulty refinement
	float ReactionDelayTimer = 0.0f;
	float LastDecisionTime = 0.0f;
	FVector CachedDecisionDirection = FVector::ZeroVector;
	bool bShouldMakeMistake = false;

	//Unstuck system
	FVector LastPosition = FVector::ZeroVector;
	float StuckTimer = 0.0f;
	float UnstuckCooldown = 0.0f;

	//Nav-Mesh
	bool bUseNavMesh = true;
	FVector CurrentPathTarget = FVector::ZeroVector;
	TArray<FVector> PatrolPoints;
	int32 CurrentPatrolIndex = 0;
	float PatrolWaitTimer = 0.0f;
	bool bIsPatrolling = false;

	// AI Logic Functions
	void UpdateTarget();
	void UpdateBehavior(float DeltaTime);
	void ChasePlayer(float DistanceToPlayer);
	void RunAway(float DistanceToPlayer);
	void PerformDashAttack(const FVector& DirectionToPlayer);

	// Observation system functions
	void UpdateObservations(float DeltaTime);
	void UpdatePlayerTracking();
	void UpdateEnvironmentalSensors();
	bool DetectObstacle(const FVector& Direction, float Range) const;
	bool DetectEdge(const FVector& Direction, float Range) const;

	// Ability checks and maneuvers update lmao
	FVector AvoidObstacles(const FVector& DesiredDirection);
	bool ShouldAvoidEdge(const FVector& MovementDirection);
	FVector FindCoverPosition();
	FVector GetTacticalPosition(bool bIsChasing, float DistanceToPlayer);

	//Evasion
	bool ShouldJumpObstacle();
	bool ShouldUseSlideForObstacle();
	bool ShouldBreakLineOfSight();
	FVector GetEnhancedZigzagDirection(const FVector& BaseDirection, float DistanceToPlayer);

	//Difficulty scales
	float GetReactionDelay() const;
	float GetMistakeChance() const;
	bool ShouldDelayDecision(float DeltaTime);
	FVector ApplyMistakeToDirection(const FVector& Direction);
	float GetDifficultyStaminaReserve() const;

	//Unstuck system
	bool IsStuck(float DeltaTime);
	FVector GetUnstuckDirection();

	void MoveToLocationWithNav(const FVector& TargetLocation);
	void UpdatePatrol(float DeltaTime);
	FVector GetNextPatrolPoint();
	void InitializePatrolPoints();
	bool HasReachedDestination() const;
	void StopAndResetNavMesh();

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