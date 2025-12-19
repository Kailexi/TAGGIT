#include "AITagController.h"
#include "AITagCharacter.h"
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	ObservationUpdateTimer += DeltaTime;
	if (ObservationUpdateTimer >= ObservationUpdateInterval)
	{
		ObservationUpdateTimer = 0.0f;
		UpdateObservations(DeltaTime);
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
		bool bIsChasing = !TargetPlayer || !TargetPlayer->IsTagger();

		UE_LOG(LogTemp, VeryVerbose, TEXT("Ability Check: Difficulty=%d, Stamina=%.1f%%, Chasing=%d"),
			static_cast<int32>(ControlledAI->Difficulty),
			ControlledAI->GetStaminaPercentage() * 100.0f,
			bIsChasing);

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
	bool bAIIsTagger = ControlledAI->IsTagger();
	bool bPlayerIsTagger = TargetPlayer->IsTagger();
	bool bAIStunned = ControlledAI->IsStunned();
	float StaminaPercent = ControlledAI->GetStaminaPercentage() * 100.0f;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("AI: Tagger=%d | Player: Tagger=%d | Dist=%.1fm | Stunned=%d | Stamina=%.0f%%"),
				bAIIsTagger,
				bPlayerIsTagger,
				DistanceToPlayer / 100.0f,
				bAIStunned,
				StaminaPercent)
		);
	}

	if (bAIStunned)
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> AI is STUNNED, skipping behavior"));
		return;
	}


	if (bPlayerIsTagger)
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> AI FLEEING (Player is tagger, Dist=%.1fm)"), DistanceToPlayer / 100.0f);
		RunAway(DistanceToPlayer);
	}
	// AI is the tagger - chase player
	else
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> AI CHASING (AI is tagger, Dist=%.1fm)"), DistanceToPlayer / 100.0f);
		ChasePlayer(DistanceToPlayer);
	}
}

void AAITagController::ChasePlayer(float DistanceToPlayer)
{
	if (!ControlledAI || !TargetPlayer) return;

	FVector TacticalTarget = GetTacticalPosition(true, DistanceToPlayer);
	FVector DirectionToPlayer;

	if (!TacticalTarget.IsZero())
	{
		DirectionToPlayer = (TacticalTarget - ControlledAI->GetActorLocation()).GetSafeNormal();
	}
	else
	{
		DirectionToPlayer = (ControlledAI->Difficulty == EAIDifficulty::Hard)
			? GetPredictedDirection()
			: GetDirectionToPlayer();
	}

	DirectionToPlayer = AvoidObstacles(DirectionToPlayer);

	if (ShouldAvoidEdge(DirectionToPlayer))
	{
		DirectionToPlayer = DirectionToPlayer.RotateAngleAxis(90.0f, FVector::UpVector);
		UE_LOG(LogTemp, Warning, TEXT("    ChasePlayer: Avoiding edge, turning"));
	}

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
	
	// Too far - patrol/idle
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

	bool bShouldSprint = (DistanceToPlayer <= SprintRadius && ControlledAI->Difficulty != EAIDifficulty::Easy);
	if (bShouldSprint)
	{
		ControlledAI->AIStartSprint();
	}
	else
	{
		ControlledAI->AIEndSprint();
	}

	UCharacterMovementComponent* MovementComp = ControlledAI->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTemp, Error, TEXT("    ChasePlayer: No movement component!"));
		return;
	}

	bool bIsOnGround = MovementComp->IsMovingOnGround();
	bool bIsSprinting = ControlledAI->IsSprinting();
	FVector CurrentVelocity = MovementComp->Velocity;

	if (bIsOnGround)
	{
		FVector DesiredVelocity = DirectionToPlayer * MovementComp->MaxWalkSpeed;
		DesiredVelocity.Z = CurrentVelocity.Z;

		MovementComp->Velocity = DesiredVelocity;

		UE_LOG(LogTemp, VeryVerbose, TEXT("    ChasePlayer: FORCING velocity - Dir=(%.2f,%.2f,%.2f), Sprint=%d, MaxSpeed=%.0f, NewVel=(%.0f,%.0f,%.0f)"),
			DirectionToPlayer.X, DirectionToPlayer.Y, DirectionToPlayer.Z,
			bIsSprinting,
			MovementComp->MaxWalkSpeed,
			DesiredVelocity.X, DesiredVelocity.Y, DesiredVelocity.Z);
	}
	else
	{
		// Use AddMovementInput when in air (for landing direction)
		ControlledAI->AddMovementInput(DirectionToPlayer, 1.0f);
		UE_LOG(LogTemp, VeryVerbose, TEXT("    ChasePlayer: In air, using AddMovementInput"));
	}

	// Debug visualization
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

	FVector TacticalTarget = GetTacticalPosition(false, DistanceToPlayer);
	FVector DirectionAwayFromPlayer;

	if (!TacticalTarget.IsZero())
	{
		DirectionAwayFromPlayer = (TacticalTarget - ControlledAI->GetActorLocation()).GetSafeNormal();
		UE_LOG(LogTemp, VeryVerbose, TEXT("    RunAway: Using tactical position"));
	}
	else
	{
		DirectionAwayFromPlayer = -GetDirectionToPlayer();
	}

	DirectionAwayFromPlayer = AvoidObstacles(DirectionAwayFromPlayer);

	if (ShouldAvoidEdge(DirectionAwayFromPlayer))
	{
		// Turn to avoid edge
		DirectionAwayFromPlayer = DirectionAwayFromPlayer.RotateAngleAxis(90.0f, FVector::UpVector);
		UE_LOG(LogTemp, Warning, TEXT("    RunAway: Avoiding edge, turning"));
	}

	UE_LOG(LogTemp, Warning, TEXT("    RunAway: Dir=(%.2f,%.2f,%.2f), Dist=%.1fm"),
		DirectionAwayFromPlayer.X, DirectionAwayFromPlayer.Y, DirectionAwayFromPlayer.Z,
		DistanceToPlayer / 100.0f);

	// FIX 3: Clean up ALL movement states that could block sprint
	UCharacterMovementComponent* MovementComp = ControlledAI->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTemp, Error, TEXT("    RunAway: No movement component!"));
		return;
	}

	// End crouch if active
	if (ControlledAI->IsCrouching())
	{
		ControlledAI->AIEndCrouch();
		UE_LOG(LogTemp, Warning, TEXT("    RunAway: Ended crouch to allow sprint"));
	}

	// End slide if active (should auto-end but be explicit)
	if (ControlledAI->IsSliding())
	{
		ControlledAI->AIEndSlide();
		UE_LOG(LogTemp, Warning, TEXT("    RunAway: Ended slide to allow sprint"));
	}

	// Sprint away if player is close
	bool bShouldSprint = DistanceToPlayer < 4000.0f;  // 40m
	if (bShouldSprint)
	{
		ControlledAI->AIStartSprint();
	}
	else
	{
		ControlledAI->AIEndSprint();
	}

	bool bIsOnGround = MovementComp->IsMovingOnGround();
	bool bIsSprinting = ControlledAI->IsSprinting();
	bool bIsCrouched = ControlledAI->IsCrouching();
	bool bIsSliding = ControlledAI->IsSliding();
	FVector CurrentVelocity = MovementComp->Velocity;

	UE_LOG(LogTemp, Warning, TEXT("    RunAway: Sprint=%d, Crouch=%d, Slide=%d, OnGround=%d, Vel=(%.0f,%.0f,%.0f)"),
		bIsSprinting, bIsCrouched, bIsSliding, bIsOnGround,
		CurrentVelocity.X, CurrentVelocity.Y, CurrentVelocity.Z);

	if (bIsSprinting && bIsOnGround)
	{
		FVector DesiredVelocity = DirectionAwayFromPlayer * MovementComp->MaxWalkSpeed;
		DesiredVelocity.Z = CurrentVelocity.Z;

		MovementComp->Velocity = DesiredVelocity;

		UE_LOG(LogTemp, Warning, TEXT("    RunAway: FORCING velocity - MaxSpeed=%.0f, NewVel=(%.0f,%.0f,%.0f)"),
			MovementComp->MaxWalkSpeed,
			DesiredVelocity.X, DesiredVelocity.Y, DesiredVelocity.Z);
	}
	else
	{
		ControlledAI->AddMovementInput(DirectionAwayFromPlayer, 1.0f);
		UE_LOG(LogTemp, VeryVerbose, TEXT("    RunAway: Using AddMovementInput (not sprinting)"));
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2, 0.0f, FColor::Red,
			FString::Printf(TEXT("AI RUNNING AWAY! Dist=%.1fm, Sprint=%d, Stunned=%d"),
				DistanceToPlayer / 100.0f,
				ControlledAI->IsSprinting(),
				ControlledAI->IsStunned())
		);
	}
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

	if (ControlledAI->Difficulty == EAIDifficulty::Easy)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Easy difficulty - skipping advanced abilities"));
		return;
	}

	float StaminaPercent = ControlledAI->GetStaminaPercentage();
	float ReserveThreshold = GetStaminaReserve();
	bool bConservingStamina = ShouldConserveStamina();

	if (bConservingStamina)
	{
		UE_LOG(LogTemp, Log, TEXT("Conserving stamina: %.1f%% < %.1f%% threshold - No abilities"),
			StaminaPercent * 100.0f,
			ReserveThreshold * 100.0f);
		return;
	}

	UE_LOG(LogTemp, VeryVerbose, TEXT("Checking abilities: Stamina=%.1f%%, Chasing=%d, Dist=%.1fm"),
		StaminaPercent * 100.0f,
		bIsChasing,
		DistanceToPlayer / 100.0f);

	if (ControlledAI->Difficulty == EAIDifficulty::Hard)
	{
		TryMantle();
	}


	bool bShouldSlide = (bIsChasing && DistanceToPlayer > 1000.0f) ||
		(!bIsChasing && DistanceToPlayer < 2000.0f);
	if (bShouldSlide)
	{
		TrySlide();
	}

	if (ControlledAI->Difficulty == EAIDifficulty::Hard)
	{
		float HeightDiff = GetHeightDifference();
		bool bShouldLeap = (FMath::Abs(HeightDiff) > 100.0f) ||
			(!bIsChasing && DistanceToPlayer > 500.0f && DistanceToPlayer < 1500.0f);
		if (bShouldLeap)
		{
			FVector Direction = bIsChasing ? GetDirectionToPlayer() : -GetDirectionToPlayer();
			TryLeap(Direction);
		}
	}


	if (!bIsChasing && DistanceToPlayer > 3000.0f && DistanceToPlayer < 5000.0f)
	{
		ManageCrouch(true);
		UE_LOG(LogTemp, VeryVerbose, TEXT("AI crouching at medium-far range (%.1fm)"), DistanceToPlayer / 100.0f);
	}
	else
	{
		ManageCrouch(false);
	}
}

void AAITagController::TryLeap(const FVector& TargetDirection)
{
	if (!ControlledAI) return;

	if (ControlledAI->IsJumping())
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot leap: Already jumping"));
		return;
	}
	if (ControlledAI->IsMantling())
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot leap: Currently mantling"));
		return;
	}
	if (ControlledAI->IsChargingLeap())
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot leap: Already charging leap"));
		return;
	}

	float LeapCost = ControlledAI->GetLeapCost();
	if (!ControlledAI->HasStaminaFor(LeapCost))
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot leap: Insufficient stamina (need %.1f)"), LeapCost);
		return;
	}

	ControlledAI->AIStartLeap(0.5f); 

	UE_LOG(LogTemp, Warning, TEXT("AI LEAPING! Stamina: %.1f%%"),
		ControlledAI->GetStaminaPercentage() * 100.0f);
}

void AAITagController::TrySlide()
{
	if (!ControlledAI) return;

	if (!ControlledAI->IsSprinting())
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot slide: Not sprinting"));
		return;
	}

	float Cooldown = ControlledAI->GetSlideCooldown();
	if (Cooldown > 0.0f)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot slide: Cooldown remaining %.1fs"), Cooldown);
		return;
	}

	float SlideCost = ControlledAI->GetSlideCost();
	if (!ControlledAI->HasStaminaFor(SlideCost))
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot slide: Insufficient stamina (need %.1f)"), SlideCost);
		return;
	}

	ControlledAI->AIStartSlide();

	UE_LOG(LogTemp, Warning, TEXT("AI SLIDING! Stamina: %.1f%%"),
		ControlledAI->GetStaminaPercentage() * 100.0f);
}

void AAITagController::TryMantle()
{
	if (!ControlledAI) return;

	float Cooldown = ControlledAI->GetMantleCooldown();
	if (Cooldown > 0.0f)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot mantle: Cooldown remaining %.1fs"), Cooldown);
		return;
	}

	float MantleCost = ControlledAI->GetMantleCost();
	if (!ControlledAI->HasStaminaFor(MantleCost))
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Cannot mantle: Insufficient stamina (need %.1f)"), MantleCost);
		return;
	}

	ControlledAI->AITryMantle();

	UE_LOG(LogTemp, VeryVerbose, TEXT("AI attempting mantle"));
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
		return 0.15f;
	case EAIDifficulty::Medium:
		return 0.10f;
	case EAIDifficulty::Hard:
		return 0.05f;
	default:
		return 0.10f;
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


void AAITagController::UpdateObservations(float DeltaTime)
{
	if (!ControlledAI || !TargetPlayer) return;

	UpdatePlayerTracking();

	UpdateEnvironmentalSensors();

	UCharacterMovementComponent* MovementComp = ControlledAI->GetCharacterMovement();
	if (MovementComp)
	{
		CurrentObservation.AIPosition = ControlledAI->GetActorLocation();
		CurrentObservation.AIVelocity = MovementComp->Velocity;
		CurrentObservation.AISpeed = MovementComp->Velocity.Size();
	}

	CurrentObservation.StaminaPercent = ControlledAI->GetStaminaPercentage();
	CurrentObservation.bAIIsTagger = ControlledAI->IsTagger();

	CurrentObservation.DistanceToPlayer = GetDistanceToPlayer();
	CurrentObservation.HeightDifference = GetHeightDifference();
	CurrentObservation.DirectionToPlayer = GetDirectionToPlayer();
	CurrentObservation.bPlayerVisible = IsPlayerVisible();

	UE_LOG(LogTemp, VeryVerbose, TEXT("Observation: PlayerSpeed=%.1f, ObstacleAhead=%d, EdgeAhead=%d, Dist=%.1fm"),
		CurrentObservation.PlayerSpeed,
		CurrentObservation.bObstacleAhead,
		CurrentObservation.bEdgeAhead,
		CurrentObservation.DistanceToPlayer / 100.0f);
}

void AAITagController::UpdatePlayerTracking()
{
	if (!TargetPlayer) return;

	FVector CurrentPlayerPos = TargetPlayer->GetActorLocation();
	CurrentObservation.PlayerPosition = CurrentPlayerPos;

	if (!PreviousPlayerPosition.IsZero())
	{
		FVector PositionDelta = CurrentPlayerPos - PreviousPlayerPosition;
		CurrentObservation.PlayerVelocity = PositionDelta / ObservationUpdateInterval;
		CurrentObservation.PlayerSpeed = CurrentObservation.PlayerVelocity.Size();

		// fuck deletion by zero
		if (CurrentObservation.PlayerSpeed > 1.0f)
		{
			CurrentObservation.PlayerDirection = CurrentObservation.PlayerVelocity.GetSafeNormal();
		}
		else
		{
			CurrentObservation.PlayerDirection = FVector::ZeroVector;
		}
	}

	PreviousPlayerPosition = CurrentPlayerPos;

	CurrentObservation.bPlayerIsTagger = TargetPlayer->IsTagger();
	CurrentObservation.bPlayerIsSprinting = TargetPlayer->IsSprinting();
}

void AAITagController::UpdateEnvironmentalSensors()
{
	if (!ControlledAI) return;

	UCharacterMovementComponent* MovementComp = ControlledAI->GetCharacterMovement();
	if (!MovementComp) return;

	FVector MovementDirection = MovementComp->Velocity.GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		MovementDirection = ControlledAI->GetActorForwardVector();
	}

	//obstacles are a construct divert to reality
	CurrentObservation.bObstacleAhead = DetectObstacle(MovementDirection, SensorRange);
	CurrentObservation.bObstacleLeft = DetectObstacle(MovementDirection.RotateAngleAxis(-90.0f, FVector::UpVector), SensorRange);
	CurrentObservation.bObstacleRight = DetectObstacle(MovementDirection.RotateAngleAxis(90.0f, FVector::UpVector), SensorRange);

	CurrentObservation.bEdgeAhead = DetectEdge(MovementDirection, EdgeDetectionRange);

	if (CurrentObservation.bObstacleAhead)
	{
		FHitResult HitResult;
		FVector Start = ControlledAI->GetActorLocation();
		FVector End = Start + (MovementDirection * SensorRange);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(ControlledAI);
		QueryParams.AddIgnoredActor(TargetPlayer);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
		{
			CurrentObservation.ObstacleDistance = HitResult.Distance;
		}
		else
		{
			CurrentObservation.ObstacleDistance = SensorRange;
		}
	}
	else
	{
		CurrentObservation.ObstacleDistance = SensorRange;
	}

	CurrentObservation.bCoverNearby = false;
}

bool AAITagController::DetectObstacle(const FVector& Direction, float Range) const
{
	if (!ControlledAI || !GetWorld()) return false;

	FVector Start = ControlledAI->GetActorLocation();
	FVector End = Start + (Direction * Range);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledAI);
	QueryParams.AddIgnoredActor(TargetPlayer);

	// I HECKING LOVE RAYCASTING
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	// Draw debug line ONLY IN EDITOR YOU WILL NOT SEE IT IF YOU PACK THE GAME DON'T KILL ME FOR THAT
#if WITH_EDITOR
	if (bHit)
	{
		DrawDebugLine(GetWorld(), Start, HitResult.Location, FColor::Red, false, 0.1f, 0, 2.0f);
	}
	else
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 0.1f, 0, 1.0f);
	}
#endif

	return bHit;
}

bool AAITagController::DetectEdge(const FVector& Direction, float Range) const
{
	if (!ControlledAI || !GetWorld()) return false;

	FVector Start = ControlledAI->GetActorLocation() + (Direction * 100.0f);
	FVector End = Start + (FVector::DownVector * 500.0f); 

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledAI);

	bool bHitGround = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	bool bEdgeDetected = !bHitGround;

#if WITH_EDITOR
	if (bEdgeDetected)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 0.1f, 0, 3.0f);
	}
#endif

	return bEdgeDetected;
}


FVector AAITagController::AvoidObstacles(const FVector& DesiredDirection)
{
	if (!ControlledAI) return DesiredDirection;

	if (!CurrentObservation.bObstacleAhead)
	{
		return DesiredDirection;
	}

	UE_LOG(LogTemp, Log, TEXT("Obstacle ahead! Steering around (Left=%d, Right=%d)"),
		CurrentObservation.bObstacleLeft,
		CurrentObservation.bObstacleRight);

	if (!CurrentObservation.bObstacleLeft && !CurrentObservation.bObstacleRight)
	{
		FVector SteerDirection = DesiredDirection.RotateAngleAxis(45.0f, FVector::UpVector);
		UE_LOG(LogTemp, Log, TEXT("Both sides clear - steering right"));
		return SteerDirection.GetSafeNormal();
	}
	else if (!CurrentObservation.bObstacleLeft)
	{
		FVector SteerDirection = DesiredDirection.RotateAngleAxis(-45.0f, FVector::UpVector);
		UE_LOG(LogTemp, Log, TEXT("Steering left to avoid obstacle"));
		return SteerDirection.GetSafeNormal();
	}
	else if (!CurrentObservation.bObstacleRight)
	{
		FVector SteerDirection = DesiredDirection.RotateAngleAxis(45.0f, FVector::UpVector);
		UE_LOG(LogTemp, Log, TEXT("Steering right to avoid obstacle"));
		return SteerDirection.GetSafeNormal();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Blocked on all sides! Backing up"));
		return -DesiredDirection.GetSafeNormal();
	}
}

bool AAITagController::ShouldAvoidEdge(const FVector& MovementDirection)
{
	if (!CurrentObservation.bEdgeAhead)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("EDGE DETECTED AHEAD! Avoiding..."));
	return true;
}

FVector AAITagController::FindCoverPosition()
{
	if (!ControlledAI || !TargetPlayer) return FVector::ZeroVector;

	FVector AIPos = ControlledAI->GetActorLocation();
	FVector PlayerPos = TargetPlayer->GetActorLocation();
	FVector DirectionFromPlayer = (AIPos - PlayerPos).GetSafeNormal();

	float BestCoverScore = -1.0f;
	FVector BestCoverPos = FVector::ZeroVector;

	for (int i = -3; i <= 3; i++)
	{
		float Angle = i * 30.0f;  // -90 to +90 degrees
		FVector SearchDir = DirectionFromPlayer.RotateAngleAxis(Angle, FVector::UpVector);
		float SearchDistance = 1000.0f;  // 10m

		FHitResult HitResult;
		FVector Start = AIPos;
		FVector End = Start + (SearchDir * SearchDistance);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(ControlledAI);
		QueryParams.AddIgnoredActor(TargetPlayer);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
		{
			FVector CoverPos = HitResult.Location;
			FVector ToCover = (CoverPos - PlayerPos).GetSafeNormal();
			FVector ToPlayer = (PlayerPos - CoverPos).GetSafeNormal();

			// For true rand i went with a score system here it is based upon some shit here below:
			// 1. Distance from player
			// 2. Angle
			float DistanceScore = FVector::Dist(CoverPos, PlayerPos) / 1000.0f;  
			float AngleScore = FMath::Abs(FVector::DotProduct(ToCover, DirectionFromPlayer));

			float TotalScore = DistanceScore + AngleScore;

			if (TotalScore > BestCoverScore)
			{
				BestCoverScore = TotalScore;
				BestCoverPos = CoverPos;
			}
		}
	}

	if (BestCoverScore > 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("Found cover position! Score=%.2f"), BestCoverScore);
		CurrentObservation.bCoverNearby = true;
		return BestCoverPos;
	}

	CurrentObservation.bCoverNearby = false;
	return FVector::ZeroVector;
}

FVector AAITagController::GetTacticalPosition(bool bIsChasing, float DistanceToPlayer)
{
	if (!ControlledAI || !TargetPlayer) return FVector::ZeroVector;

	FVector AIPos = ControlledAI->GetActorLocation();
	FVector PlayerPos = TargetPlayer->GetActorLocation();

	if (bIsChasing)
	{

		FVector PlayerVelocity = CurrentObservation.PlayerVelocity;
		float PlayerSpeed = CurrentObservation.PlayerSpeed;

		if (PlayerSpeed > 100.0f)  
		{
			float TimeToIntercept = DistanceToPlayer / 1000.0f;  
			FVector PredictedPos = PlayerPos + (PlayerVelocity * TimeToIntercept);

			UE_LOG(LogTemp, VeryVerbose, TEXT("Tactical Chase: Intercepting predicted position"));
			return PredictedPos;
		}
		else
		{
			return PlayerPos;
		}
	}
	else
	{
		FVector CoverPos = FindCoverPosition();

		if (!CoverPos.IsZero() && DistanceToPlayer < 2000.0f)  
		{
			UE_LOG(LogTemp, Log, TEXT("Tactical Flee: Moving to cover"));
			return CoverPos;
		}
		else
		{
			FVector DirectionAway = (AIPos - PlayerPos).GetSafeNormal();

			float ZigzagAngle = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.0f) * 30.0f;
			FVector ZigzagDirection = DirectionAway.RotateAngleAxis(ZigzagAngle, FVector::UpVector);

			FVector TargetPos = AIPos + (ZigzagDirection * 1000.0f);  // 10m ahead
			return TargetPos;
		}
	}
}