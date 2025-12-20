// Copyright Epic Games, Inc. All Rights Reserved.

#include "TagAITrainingEnvironment.h"
#include "AITagCharacter.h"
#include "InputPlayer/InputCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

void UTagAITrainingEnvironment::GatherAgentReward_Implementation(float& OutReward, const int32 AgentId)
{
	OutReward = 0.0f;

	AAITagCharacter* AIAgent = Cast<AAITagCharacter>(GetAgent(AgentId));
	if (!AIAgent)
	{
		return;
	}

	if (!PlayerCharacter)
	{
		PlayerCharacter = FindPlayerCharacter();
		if (!PlayerCharacter)
		{
			return;
		}
	}

	float Distance = FVector::Dist(AIAgent->GetActorLocation(), PlayerCharacter->GetActorLocation());

	// Different reward strategies based on role
	if (AIAgent->IsTagger())
	{


		// Reward for getting closer to player (negative distance = reward for proximity)
		float ProximityReward = -Distance * 0.01f;
		OutReward += ProximityReward;

		// Penalty for being too far away
		if (Distance > 1000.0f)
		{
			OutReward -= 0.1f;
		}

		// Big reward for being in tagging range
		if (Distance < 200.0f)
		{
			OutReward += 10.0f;
		}
	}
	else
	{
		// AI HIDE
		// Goal: Stay away from tagger

		// Reward for staying far from player (if player is tagger)
		if (PlayerCharacter->IsTagger())
		{
			float DistanceReward = Distance * 0.005f;
			OutReward += DistanceReward;

			// Survival reward (constant small reward for each tick alive)
			OutReward += 0.1f;
		}
		else
		{
			// If player is not tagger, neutral behavior
			OutReward += 0.05f;
		}
	}
}

void UTagAITrainingEnvironment::GatherAgentCompletion_Implementation(ELearningAgentsCompletion& OutCompletion, const int32 AgentId)
{
	OutCompletion = ELearningAgentsCompletion::Running;

	AAITagCharacter* AIAgent = Cast<AAITagCharacter>(GetAgent(AgentId));
	if (!AIAgent)
	{
		return;
	}

	if (!PlayerCharacter)
	{
		PlayerCharacter = FindPlayerCharacter();
		if (!PlayerCharacter)
		{
			return;
		}
	}

	float Distance = FVector::Dist(AIAgent->GetActorLocation(), PlayerCharacter->GetActorLocation());

	if (AIAgent->IsTagger() && Distance < 150.0f)
	{
		// Truncation - AI successfully completed objective
		OutCompletion = ELearningAgentsCompletion::Truncation;
		UE_LOG(LogLearning, Verbose, TEXT("Agent %d successfully tagged player!"), AgentId);
		return;
	}

	// Check if AI got tagged (AI is hider and player is close)
	if (!AIAgent->IsTagger() && PlayerCharacter->IsTagger() && Distance < 150.0f)
	{
		OutCompletion = ELearningAgentsCompletion::Termination;
		UE_LOG(LogLearning, Verbose, TEXT("Agent %d got tagged!"), AgentId);
		return;
	}
}

void UTagAITrainingEnvironment::ResetAgentEpisode_Implementation(const int32 AgentId)
{
	// Get the AI agent
	AAITagCharacter* AIAgent = Cast<AAITagCharacter>(GetAgent(AgentId));
	if (!AIAgent)
	{
		return;
	}

	float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
	float RandomRadius = FMath::FRandRange(500.0f, 2000.0f);

	float RadAngle = FMath::DegreesToRadians(RandomAngle);
	float X = FMath::Cos(RadAngle) * RandomRadius;
	float Y = FMath::Sin(RadAngle) * RandomRadius;
	float Z = 100.0f;  

	FVector SpawnLocation(X, Y, Z);
	FRotator RandomRotation(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);

	// Teleport AI to random location
	AIAgent->SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::ResetPhysics);
	AIAgent->SetActorRotation(RandomRotation);

	AIAgent->GetCharacterMovement()->Velocity = FVector::ZeroVector;

	bool bShouldBeTagger = FMath::RandBool();
	AIAgent->SetTaggerStatus(bShouldBeTagger);

	UE_LOG(LogLearning, Verbose, TEXT("Agent %d reset to location %s, tagger=%d"),
		AgentId, *SpawnLocation.ToString(), bShouldBeTagger);
}

AInputCharacter* UTagAITrainingEnvironment::FindPlayerCharacter()
{
	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (PC)
		{
			return Cast<AInputCharacter>(PC->GetPawn());
		}
	}
	return nullptr;
}
