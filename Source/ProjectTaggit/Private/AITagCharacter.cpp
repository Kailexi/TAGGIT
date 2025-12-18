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

	// AI starts as tagger
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