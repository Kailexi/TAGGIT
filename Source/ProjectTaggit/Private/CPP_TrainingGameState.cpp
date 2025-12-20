// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPP_TrainingGameState.h"
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "Kismet/GameplayStatics.h"

ACPP_TrainingGameState::ACPP_TrainingGameState()
{
	roundNumber = 1;
	startSeconds = 999999;
	seconds = startSeconds;
	PrimaryActorTick.bCanEverTick = true;
}

void ACPP_TrainingGameState::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("=== TRAINING MODE ACTIVE ==="));
	UE_LOG(LogTemp, Warning, TEXT("Training: Infinite time, no win conditions - optimized for ML"));

	if (bInfiniteTime)
	{
		seconds = -1;
	}
	else
	{
		initializeRound(false);
	}

	FindPlayer();

	if (PlayerCharacter)
	{
		PlayerCharacter->SetTaggerStatus(false);
		UE_LOG(LogTemp, Log, TEXT("Training: Player set as Hider"));
	}
}

void ACPP_TrainingGameState::Tick(float DeltaTime)
{


	if (!PlayerCharacter)
	{
		FindPlayer();
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			DeltaTime,
			FColor::Cyan,
			TEXT("TRAINING MODE - Learning Agents Active")
		);
	}
}

bool ACPP_TrainingGameState::winConditionCheck()
{
	// No win conditions during training
	// Training episodes are managed by Learning Agents Manager BLUERPINTS YOU STUPID PROGRAMM
	return false;
}

void ACPP_TrainingGameState::FindPlayer()
{
	if (!PlayerCharacter)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		PlayerCharacter = Cast<AInputCharacter>(PlayerPawn);
	}
}
