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

	AbilityCheckTimer += DeltaTime;
	if (AbilityCheckTimer >= AbilityCheckInterval)
	{
		AbilityCheckTimer = 0.0f;
		float DistanceToPlayer = GetDistanceToPlayer();
		bool bIsChasing = ControlledAI->IsTagger();
		CheckAdvancedAbilities(DistanceToPlayer, bIsChasing);
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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("AI: IsTagger=%d, Player IsTagger=%d, Dist=%.1fm"),
				ControlledAI->IsTagger(),
				TargetPlayer->IsTagger(),
				DistanceToPlayer / 100.0f)
		);
	}

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

	FVector DirectionToPlayer = (ControlledAI->Difficulty == EAIDifficulty::Hard)
		? GetPredictedDirection()
		: GetDirectionToPlayer();

	float ChaseRadius = ControlledAI->ChaseRadius;
	float SprintRadius = ControlledAI->SprintRadius;
	float DashRadius = ControlledAI->DashRadius;

	switch (ControlledAI->Difficulty)
	{
	case EAIDifficulty::Easy:
		ChaseRadius = 6000.0f;   // 60m
		SprintRadius = 3200.0f;  // 32m
		DashRadius = 300.0f;     // 3m
		break;
	case EAIDifficulty::Medium:
		ChaseRadius = 8000.0f;   // 80m
		SprintRadius = 4000.0f;  // 40m
		DashRadius = 400.0f;     // 4m
		break;
	case EAIDifficulty::Hard:
		ChaseRadius = 12000.0f;  // 120m
		SprintRadius = 6000.0f;  // 60m
		DashRadius = 500.0f;     // 5m
		break;
	}

	if (DistanceToPlayer > ChaseRadius)
	{
		ControlledAI->AIEndSprint();
		StopMovement();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				3, 0.0f, FColor::Orange,
				TEXT("AI IDLE - Player too far")
			);
		}
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

	ControlledAI->AddMovementInput(DirectionToPlayer, 1.0f);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			3, 0.0f, FColor::Green,
			FString::Printf(TEXT("AI CHASING: Dist=%.1fm, Sprint=%d, CanDash=%d"),
				DistanceToPlayer / 100.0f,
				ControlledAI->IsSprinting(),
				ControlledAI->CanUseDash())
		);
	}
}

void AAITagController::RunAway(float DistanceToPlayer)
{
	if (!ControlledAI || !TargetPlayer) return;

	StopMovement();

	FVector DirectionAwayFromPlayer = -GetDirectionToPlayer();
	FVector TargetLocation = ControlledAI->GetActorLocation() + (DirectionAwayFromPlayer * 2000.0f);  // Run 20m away

	if (DistanceToPlayer < 4000.0f)  // 40m
	{
		ControlledAI->AIStartSprint();
	}
	else
	{
		ControlledAI->AIEndSprint();
	}

	ControlledAI->AddMovementInput(DirectionAwayFromPlayer, 1.0f);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2, 0.0f, FColor::Red,
			FString::Printf(TEXT("AI RUNNING AWAY! Dist=%.1fm, Sprint=%d"),
				DistanceToPlayer / 100.0f,
				ControlledAI->IsSprinting())
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("AI running away from player (Distance: %.1fm)"), DistanceToPlayer / 100.0f);
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

	return !bHit || HitResult.GetActor() == TargetPlayer;
}

void AAITagController::CheckAdvancedAbilities(float DistanceToPlayer, bool bIsChasing)
{
	if (!ControlledAI || !TargetPlayer) return;

	if (ControlledAI->Difficulty == EAIDifficulty::Easy) return;

	if (ShouldConserveStamina()) return;

	if (ControlledAI->Difficulty == EAIDifficulty::Hard)
	{
		TryMantle();
	}

	if (bIsChasing && DistanceToPlayer > 1000.0f)
	{
		TrySlide();
	}

	if (ControlledAI->Difficulty == EAIDifficulty::Hard)
	{
		float HeightDiff = GetHeightDifference();
		if (FMath::Abs(HeightDiff) > 100.0f)
		{
			FVector Direction = bIsChasing ? GetDirectionToPlayer() : -GetDirectionToPlayer();
			TryLeap(Direction);
		}
	}

	if (!bIsChasing)
	{
		ManageCrouch(DistanceToPlayer < 2000.0f);
	}
}

void AAITagController::TryLeap(const FVector& TargetDirection)
{
	if (!ControlledAI) return;

	if (ControlledAI->IsJumping() || ControlledAI->IsMantling() || ControlledAI->IsChargingLeap()) return;

	if (!ControlledAI->HasStaminaFor(ControlledAI->GetLeapCost())) return;

	ControlledAI->AIStartLeap(0.5f);

	UE_LOG(LogTemp, Log, TEXT("AI using leap!"));
}

void AAITagController::TrySlide()
{
	if (!ControlledAI) return;

	if (!ControlledAI->IsSprinting()) return;

	if (ControlledAI->GetSlideCooldown() > 0.0f) return;

	if (!ControlledAI->HasStaminaFor(ControlledAI->GetSlideCost())) return;

	ControlledAI->AIStartSlide();

	UE_LOG(LogTemp, Log, TEXT("AI sliding!"));
}

void AAITagController::TryMantle()
{
	if (!ControlledAI) return;

	// Check cooldown
	if (ControlledAI->GetMantleCooldown() > 0.0f) return;

	// Check stamina
	if (!ControlledAI->HasStaminaFor(ControlledAI->GetMantleCost())) return;

	// Try mantle
	ControlledAI->AITryMantle();
}

void AAITagController::ManageCrouch(bool bShouldCrouch)
{
	if (!ControlledAI) return;

	if (bShouldCrouch && !ControlledAI->IsCrouching())
	{
		ControlledAI->AIStartCrouch();
	}
	else if (!bShouldCrouch && ControlledAI->IsCrouching())
	{
		ControlledAI->AIEndCrouch();
	}
}


bool AAITagController::ShouldConserveStamina() const
{
	if (!ControlledAI) return true;

	float StaminaPercent = ControlledAI->GetStaminaPercentage();
	float Reserve = GetStaminaReserve();

	return StaminaPercent < Reserve;
}

float AAITagController::GetStaminaReserve() const
{
	if (!ControlledAI) return 0.5f;

	switch (ControlledAI->Difficulty)
	{
	case EAIDifficulty::Easy:
		return 0.3f;
	case EAIDifficulty::Medium:
		return 0.25f;
	case EAIDifficulty::Hard:
		return 0.20f;
	default:
		return 0.25f;
	}
}

FVector AAITagController::PredictPlayerPosition(float TimeAhead) const
{
	if (!TargetPlayer) return FVector::ZeroVector;

	FVector CurrentPos = TargetPlayer->GetActorLocation();
	FVector Velocity = TargetPlayer->GetVelocity();

	return CurrentPos + (Velocity * TimeAhead);
}

FVector AAITagController::GetPredictedDirection() const
{
	if (!ControlledAI || !TargetPlayer) return FVector::ZeroVector;

	if (ControlledAI->Difficulty != EAIDifficulty::Hard)
	{
		return GetDirectionToPlayer();
	}

	FVector PredictedPos = PredictPlayerPosition(0.5f);
	FVector Direction = PredictedPos - ControlledAI->GetActorLocation();
	return Direction.GetSafeNormal();
}

float AAITagController::GetHeightDifference() const
{
	if (!ControlledAI || !TargetPlayer) return 0.0f;

	return TargetPlayer->GetActorLocation().Z - ControlledAI->GetActorLocation().Z;
}