// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LearningAgentsTrainingEnvironment.h"
#include "TagAITrainingEnvironment.generated.h"

class AAITagCharacter;
class AInputCharacter;


UCLASS()
class PROJECTTAGGIT_API UTagAITrainingEnvironment : public ULearningAgentsTrainingEnvironment
{
	GENERATED_BODY()

protected:
	// Cached player reference
	UPROPERTY()
	AInputCharacter* PlayerCharacter;


	virtual void GatherAgentReward_Implementation(float& OutReward, const int32 AgentId) override;
	virtual void GatherAgentCompletion_Implementation(ELearningAgentsCompletion& OutCompletion, const int32 AgentId) override;
	virtual void ResetAgentEpisode_Implementation(const int32 AgentId) override;


private:
	// Helper to find player character
	AInputCharacter* FindPlayerCharacter();
};
