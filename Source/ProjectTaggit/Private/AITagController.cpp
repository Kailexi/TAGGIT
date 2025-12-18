#include "AITagController.h"
#include "AITagCharacter.h"
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AAITagController::AAITagController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
}

void AAITagController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("AITagController: BeginPlay"));
}

void AAITagController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledAI = Cast<AAITagCharacter>(InPawn);
	if (ControlledAI)
	{
		UE_LOG(LogTemp, Warning, TEXT("AITagController possessed: %s (Difficulty: %d)"),
			*ControlledAI->GetName(),
			static_cast<int32>(ControlledAI->Difficulty));

		UpdateTarget();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AITagController: Failed to cast possessed pawn to AITagCharacter!"));
	}
}

void AAITagController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ControlledAI || !TargetPlayer)
	{
		UpdateTargetTimer += DeltaTime;
		if (UpdateTargetTimer >= UpdateTargetInterval)
		{
			UpdateTargetTimer = 0.0f;
			UpdateTarget();
		}
		return;
	}

	BehaviorUpdateTimer += DeltaTime;
	if (BehaviorUpdateTimer >= BehaviorUpdateInterval)
	{
		BehaviorUpdateTimer = 0.0f;
		UpdateBehavior(DeltaTime);
	}
}

void AAITagController::UpdateTarget()
{
	if (!ControlledAI) return;

	TargetPlayer = ControlledAI->GetPlayerCharacter();

	if (TargetPlayer)
	{
		UE_LOG(LogTemp, Log, TEXT("AITagController: Target acquired - %s"), *TargetPlayer->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AITagController: No player target found!"));
	}
}

void AAITagController::UpdateBehavior(float DeltaTime)
{
	if (!ControlledAI || !TargetPlayer) return;

	float DistanceToPlayer = GetDistanceToPlayer();

	// AI is the tagger - chase player
	if (ControlledAI->IsTagger())
	{
		ChasePlayer(DistanceToPlayer);
	}
	// Player is the tagger - run away
	else
	{
		RunAway(DistanceToPlayer);
	}
}

void AAITagController::ChasePlayer(float DistanceToPlayer)
{
	if (!ControlledAI || !TargetPlayer) return;

	FVector DirectionToPlayer = GetDirectionToPlayer();

	float ChaseRadius = ControlledAI->ChaseRadius;
	float SprintRadius = ControlledAI->SprintRadius;
	float DashRadius = ControlledAI->DashRadius;

	switch (ControlledAI->Difficulty)
	{
	case EAIDifficulty::Easy:
		ChaseRadius = 1500.0f;   // 15m
		SprintRadius = 800.0f;   // 8m
		DashRadius = 300.0f;     // 3m
		break;
	case EAIDifficulty::Medium:
		ChaseRadius = 2000.0f;   // 20m
		SprintRadius = 1000.0f;  // 10m
		DashRadius = 400.0f;     // 4m
		break;
	case EAIDifficulty::Hard:
		ChaseRadius = 3000.0f;   // 30m
		SprintRadius = 1500.0f;  // 15m
		DashRadius = 500.0f;     // 5m
		break;
	}

	//Patrol logic
	if (DistanceToPlayer > ChaseRadius)
	{
		ControlledAI->AIEndSprint();
		ControlledAI->AIMoveToward(TargetPlayer->GetActorLocation());
		return;
	}

	if (DistanceToPlayer <= DashRadius && ControlledAI->CanUseDash())
	{
		PerformDashAttack(DirectionToPlayer);
		return;
	}

	if (DistanceToPlayer <= SprintRadius && ControlledAI->Difficulty != EAIDifficulty::Easy)
	{
		ControlledAI->AIStartSprint();
	}
	else
	{
		ControlledAI->AIEndSprint();
	}

	ControlledAI->AIMoveToward(TargetPlayer->GetActorLocation());

	if (GEngine && ControlledAI->Difficulty == EAIDifficulty::Hard)
	{
		GEngine->AddOnScreenDebugMessage(
			INDEX_NONE, 0.0f, FColor::Yellow,
			FString::Printf(TEXT("AI Chase: Dist=%.1fm, Sprint=%d, CanDash=%d"),
				DistanceToPlayer / 100.0f,
				ControlledAI->IsSprinting(),
				ControlledAI->CanUseDash())
		);
	}
}

void AAITagController::RunAway(float DistanceToPlayer)
{
	if (!ControlledAI || !TargetPlayer) return;

	FVector DirectionAwayFromPlayer = -GetDirectionToPlayer();
	FVector TargetLocation = ControlledAI->GetActorLocation() + (DirectionAwayFromPlayer * 1000.0f);

	if (DistanceToPlayer < 1000.0f)  // 10m
	{
		ControlledAI->AIStartSprint();
	}
	else
	{
		ControlledAI->AIEndSprint();
	}

	ControlledAI->AIMoveToward(TargetLocation);

	UE_LOG(LogTemp, VeryVerbose, TEXT("AI running away from player (Distance: %.1fm)"), DistanceToPlayer / 100.0f);
}

void AAITagController::PerformDashAttack(const FVector& DirectionToPlayer)
{
	if (!ControlledAI || !TargetPlayer) return;

	ControlledAI->AIPerformTagDash(DirectionToPlayer);

	UE_LOG(LogTemp, Warning, TEXT("AI attempting tag dash! Distance: %.1fm"), GetDistanceToPlayer() / 100.0f);
}

float AAITagController::GetDistanceToPlayer() const
{
	if (!ControlledAI || !TargetPlayer) return -1.0f;

	return FVector::Dist(ControlledAI->GetActorLocation(), TargetPlayer->GetActorLocation());
}

FVector AAITagController::GetDirectionToPlayer() const
{
	if (!ControlledAI || !TargetPlayer) return FVector::ZeroVector;

	FVector Direction = TargetPlayer->GetActorLocation() - ControlledAI->GetActorLocation();
	return Direction.GetSafeNormal();
}

bool AAITagController::IsPlayerVisible() const
{
	if (!ControlledAI || !TargetPlayer) return false;

	FHitResult HitResult;
	FVector Start = ControlledAI->GetActorLocation();
	FVector End = TargetPlayer->GetActorLocation();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledAI);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	// Player is visible if we hit the player or nothing
	return !bHit || HitResult.GetActor() == TargetPlayer;
}