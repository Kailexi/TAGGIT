// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LearningAgentsInteractor.h"
#include "TagAIInteractor.generated.h"

class AAITagCharacter;
class AInputCharacter;


UCLASS()
class PROJECTTAGGIT_API UTagAIInteractor : public ULearningAgentsInteractor
{
	GENERATED_BODY()

public:
	struct FObservationObjects
	{
		// Self state observations
		FLearningAgentsObservationObjectElement SelfVelocity;
		FLearningAgentsObservationObjectElement IsTagger;
		FLearningAgentsObservationObjectElement StaminaPercent;
		FLearningAgentsObservationObjectElement SelfState;

		// Player tracking observations
		FLearningAgentsObservationObjectElement PlayerRelativeLocation;
		FLearningAgentsObservationObjectElement PlayerVelocity;
		FLearningAgentsObservationObjectElement DistanceToPlayer;
		FLearningAgentsObservationObjectElement PlayerTracking;

		// Root observation
		FLearningAgentsObservationObjectElement AllObservations;
	};

	// Action object handles
	struct FActionObjects
	{
		FLearningAgentsActionObjectElement MoveDirection;
		FLearningAgentsActionObjectElement ShouldSprint;
		FLearningAgentsActionObjectElement MovementAction;
	};

protected:
	// Observation and action object storage
	FObservationObjects ObservationObjects;
	FActionObjects ActionObjects;

	// Cached player reference
	UPROPERTY()
	AInputCharacter* PlayerCharacter;

	virtual void SpecifyAgentObservation_Implementation(FLearningAgentsObservationSchemaElement& OutObservationSchemaElement, ULearningAgentsObservationSchema* InObservationSchema) override;
	virtual void GatherAgentObservation_Implementation(FLearningAgentsObservationElement& OutObservationElement, ULearningAgentsObservationObject* InObservationObject, const int32 AgentId) override;

	virtual void SpecifyAgentAction_Implementation(FLearningAgentsActionSchemaElement& OutActionSchemaElement, ULearningAgentsActionSchema* InActionSchema) override;
	virtual void PerformAgentAction_Implementation(const int32 AgentId, ULearningAgentsActionObject* InActionObject, const FLearningAgentsActionElement& InActionElement) override;

private:
	// Helper to find player character
	AInputCharacter* FindPlayerCharacter();
};
