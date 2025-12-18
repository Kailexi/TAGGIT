#include "AITagCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AAITagCharacter::AAITagCharacter()
{

	PrimaryActorTick.bCanEverTick = true;
}

void AAITagCharacter::BeginPlay()
{
	Super::BeginPlay();

	// AI starts as "it"
	bIsTagger = true;
	UE_LOG(LogTemp, Warning, TEXT("AI started as TAGGER"));

	// Find and cache player reference
	CachedPlayer = Cast<AInputCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (CachedPlayer)
	{
		UE_LOG(LogTemp, Log, TEXT("AI found player: %s"), *CachedPlayer->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AI could not find player!"));
	}
}

void AAITagCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Periodically update player cache in case player character changes
	UpdatePlayerCacheTimer += DeltaTime;
	if (UpdatePlayerCacheTimer >= UpdatePlayerCacheInterval)
	{
		UpdatePlayerCacheTimer = 0.0f;

		if (!CachedPlayer)
		{
			CachedPlayer = Cast<AInputCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		}
	}

	if (bIsChargingLeapAI)
	{
		LeapChargeTimer += DeltaTime;
		if (LeapChargeTimer >= TargetLeapChargeTime)
		{
			AIReleaseLeap();
		}
	}
}

void AAITagCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UE_LOG(LogTemp, Log, TEXT("AI skipping player input setup"));
}

void AAITagCharacter::AIMoveToward(FVector TargetLocation)
{
	FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();


	AddMovementInput(Direction, 1.0f);
}

void AAITagCharacter::AIPerformTagDash(FVector Direction)
{
	if (!bIsTagger || bIsStunned || bIsDashing || TagCooldownRemaining > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("AI cannot dash: Tagger=%d, Stunned=%d, Dashing=%d, Cooldown=%.2f"),
			bIsTagger, bIsStunned, bIsDashing, TagCooldownRemaining);
		return;
	}

	if (!StaminaComponent || !StaminaComponent->CanPerformAction(TagDashStaminaCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("AI cannot dash: Insufficient stamina"));
		return;
	}

	StaminaComponent->TryConsumeStamina(TagDashStaminaCost);

	// Set up dash
	bIsDashing = true;
	TagDashTimeRemaining = TagDashDuration;
	TagDashDirection = Direction.GetSafeNormal();

	// Force end sprint/crouch/slide
	if (bIsSprinting) EndSprint();
	if (bIsSliding) EndSlide();
	if (bIsCrouching)
	{
		UnCrouch();
		bIsCrouching = false;
	}

	// Give initial velocity boost
	FVector DashVelocity = TagDashDirection * TagDashSpeed;
	DashVelocity.Z = GetCharacterMovement()->Velocity.Z;
	GetCharacterMovement()->Velocity = DashVelocity;

	UE_LOG(LogTemp, Log, TEXT("AI performing tag dash! Direction: %s"), *TagDashDirection.ToString());
}

void AAITagCharacter::AIStartSprint()
{
	if (!bIsSprinting && !bIsCrouching && !bIsSliding && !bIsStunned)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		UE_LOG(LogTemp, VeryVerbose, TEXT("AI started sprinting"));
	}
}

void AAITagCharacter::AIEndSprint()
{
	if (bIsSprinting)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		UE_LOG(LogTemp, VeryVerbose, TEXT("AI stopped sprinting"));
	}
}

float AAITagCharacter::GetDistanceToPlayer() const
{
	if (!CachedPlayer)
	{
		return -1.0f;
	}

	return FVector::Dist(GetActorLocation(), CachedPlayer->GetActorLocation());
}

AInputCharacter* AAITagCharacter::GetPlayerCharacter() const
{
	return CachedPlayer;
}

bool AAITagCharacter::CanUseDash() const
{
	return bIsTagger &&
		!bIsStunned &&
		!bIsDashing &&
		TagCooldownRemaining <= 0.0f &&
		StaminaComponent &&
		StaminaComponent->CanPerformAction(TagDashStaminaCost);
}


void AAITagCharacter::AIStartLeap(float ChargeTime)
{
	if (bIsStunned || bIsJumping || bIsMantling || !CanJump()) return;

	bIsChargingLeapAI = true;
	LeapChargeTimer = 0.0f;
	TargetLeapChargeTime = FMath::Clamp(ChargeTime, 0.1f, 1.0f);

	StartJumpCharge();

	UE_LOG(LogTemp, Log, TEXT("AI started charging leap (target: %.2fs)"), TargetLeapChargeTime);
}

void AAITagCharacter::AIReleaseLeap()
{
	if (!bIsChargingLeapAI && !bIsChargingLeap) return;

	bIsChargingLeapAI = false;

	ReleaseJump();

	UE_LOG(LogTemp, Log, TEXT("AI released leap (charged: %.2fs)"), LeapChargeTime);
}

void AAITagCharacter::AIStartSlide()
{
	if (!bIsSprinting || bIsStunned || bIsSliding || bIsJumping || bIsMantling) return;
	if (!GetCharacterMovement()->IsMovingOnGround() || SlideCooldownRemaining > 0.0f) return;

	if (!StaminaComponent || !StaminaComponent->CanPerformAction(SlideStaminaCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("AI cannot slide: Insufficient stamina"));
		return;
	}

	StartSlide();

	UE_LOG(LogTemp, Log, TEXT("AI started sliding"));
}

void AAITagCharacter::AITryMantle()
{
	if (bIsMantling || bIsSliding || bIsStunned || MantleCooldownRemaining > 0.0f) return;

	if (!StaminaComponent || !StaminaComponent->CanPerformAction(MantleStaminaCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("AI cannot mantle: Insufficient stamina"));
		return;
	}

	FVector Start = GetActorLocation();
	FVector Forward = GetActorForwardVector();
	FVector End = Start + (Forward * 100.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	if (bHit)
	{
		TryMantle();
		UE_LOG(LogTemp, Log, TEXT("AI attempting mantle"));
	}
}

void AAITagCharacter::AIStartCrouch()
{
	if (bIsCrouching || bIsStunned || bIsDashing) return;

	if (CanCrouch())
	{
		Crouch();
		bIsCrouching = true;
		UE_LOG(LogTemp, VeryVerbose, TEXT("AI started crouching"));
	}
}

void AAITagCharacter::AIEndCrouch()
{
	if (!bIsCrouching) return;

	UnCrouch();
	bIsCrouching = false;
	UE_LOG(LogTemp, VeryVerbose, TEXT("AI stopped crouching"));
}


float AAITagCharacter::GetCurrentStamina() const
{
	if (!StaminaComponent) return 0.0f;
	return StaminaComponent->GetCurrentStamina();
}

float AAITagCharacter::GetStaminaPercentage() const
{
	if (!StaminaComponent) return 0.0f;
	float Current = StaminaComponent->GetCurrentStamina();
	float Max = StaminaComponent->GetMaxStamina();
	if (Max <= 0.0f) return 0.0f;
	return Current / Max;
}

bool AAITagCharacter::HasStaminaFor(float Cost) const
{
	if (!StaminaComponent) return false;
	return StaminaComponent->CanPerformAction(Cost);
}