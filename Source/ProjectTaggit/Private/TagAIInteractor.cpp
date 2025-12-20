// Copyright Epic Games, Inc. All Rights Reserved.

#include "TagAIInteractor.h"
#include "AITagCharacter.h"
#include "InputPlayer/InputCharacter.h"
#include "StaminaComponent.h"
#include "LearningAgentsObservations.h"
#include "LearningAgentsActions.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

void UTagAIInteractor::SpecifyAgentObservation_Implementation(FLearningAgentsObservationSchemaElement& OutObservationSchemaElement, ULearningAgentsObservationSchema* InObservationSchema)
{
	//Observation objects

	// velocity
	ObservationObjects.SelfVelocity = ULearningAgentsObservations::SpecifyVelocityObservation(
		InObservationSchema,
		TEXT("SelfVelocity"),
		500.0f
	);

	// Is tagger status
	ObservationObjects.IsTagger = ULearningAgentsObservations::SpecifyBoolObservation(
		InObservationSchema,
		TEXT("IsTagger")
	);

	// Stamina percentage
	ObservationObjects.StaminaPercent = ULearningAgentsObservations::SpecifyFloatObservation(
		InObservationSchema,
		TEXT("StaminaPercent"),
		0.0f,  // Min
		1.0f   // Max
	);

	// Combine for later lul
	ObservationObjects.SelfState = ULearningAgentsObservations::SpecifyStructObservation(
		InObservationSchema,
		TEXT("SelfState")
	);
	ULearningAgentsObservations::MakeStructObservationFromArrayViews(
		InObservationSchema,
		ObservationObjects.SelfState,
		{
			ObservationObjects.SelfVelocity,
			ObservationObjects.IsTagger,
			ObservationObjects.StaminaPercent
		}
	);

	// Track player please

	// Player relative location
	ObservationObjects.PlayerRelativeLocation = ULearningAgentsObservations::SpecifyLocationObservation(
		InObservationSchema,
		TEXT("PlayerRelativeLocation"),
		2000.0f  
	);

	// Player velocity
	ObservationObjects.PlayerVelocity = ULearningAgentsObservations::SpecifyVelocityObservation(
		InObservationSchema,
		TEXT("PlayerVelocity"),
		600.0f
	);

	// Distance to player
	ObservationObjects.DistanceToPlayer = ULearningAgentsObservations::SpecifyFloatObservation(
		InObservationSchema,
		TEXT("DistanceToPlayer"),
		0.0f,     // Min
		3000.0f   // Max (30 meters)
	);


	ObservationObjects.PlayerTracking = ULearningAgentsObservations::SpecifyStructObservation(
		InObservationSchema,
		TEXT("PlayerTracking")
	);
	ULearningAgentsObservations::MakeStructObservationFromArrayViews(
		InObservationSchema,
		ObservationObjects.PlayerTracking,
		{
			ObservationObjects.PlayerRelativeLocation,
			ObservationObjects.PlayerVelocity,
			ObservationObjects.DistanceToPlayer
		}
	);

	// COMBINE ALL OR BLUEPRINTS DON't FUCKING WORK

	ObservationObjects.AllObservations = ULearningAgentsObservations::SpecifyStructObservation(
		InObservationSchema,
		TEXT("AllObservations")
	);
	ULearningAgentsObservations::MakeStructObservationFromArrayViews(
		InObservationSchema,
		ObservationObjects.AllObservations,
		{
			ObservationObjects.SelfState,
			ObservationObjects.PlayerTracking
		}
	);

	OutObservationSchemaElement = ObservationObjects.AllObservations;
}

void UTagAIInteractor::GatherAgentObservation_Implementation(FLearningAgentsObservationElement& OutObservationElement, ULearningAgentsObservationObject* InObservationObject, const int32 AgentId)
{
	AAITagCharacter* AIAgent = Cast<AAITagCharacter>(GetAgent(AgentId));
	if (!AIAgent)
	{
		UE_LOG(LogLearning, Warning, TEXT("TagAIInteractor: Agent %d is not an AITagCharacter"), AgentId);
		return;
	}
	if (!PlayerCharacter)
	{
		PlayerCharacter = FindPlayerCharacter();
		if (!PlayerCharacter)
		{
			UE_LOG(LogLearning, Warning, TEXT("TagAIInteractor: Could not find player character"));
			return;
		}
	}

	// Gather INFO same AS OBSERVATIONS IF U DON"T DO THIS, BLUEPRINTS BREAK

	FLearningAgentsObservationElement SelfVelocityObs = ULearningAgentsObservations::MakeVelocityObservation(
		InObservationObject,
		ObservationObjects.SelfVelocity,
		AIAgent->GetVelocity(),
		FTransform::Identity,  
		500.0f
	);

	FLearningAgentsObservationElement IsTaggerObs = ULearningAgentsObservations::MakeBoolObservation(
		InObservationObject,
		ObservationObjects.IsTagger,
		AIAgent->IsTagger()
	);

	float StaminaPercent = 1.0f;
	UStaminaComponent* StaminaComp = AIAgent->FindComponentByClass<UStaminaComponent>();
	if (StaminaComp)
	{
		StaminaPercent = StaminaComp->GetStaminaPercent();
	}
	FLearningAgentsObservationElement StaminaObs = ULearningAgentsObservations::MakeFloatObservation(
		InObservationObject,
		ObservationObjects.StaminaPercent,
		StaminaPercent
	);

	FLearningAgentsObservationElement SelfStateObs = ULearningAgentsObservations::MakeStructObservationFromArrayViews(
		InObservationObject,
		ObservationObjects.SelfState,
		{ SelfVelocityObs, IsTaggerObs, StaminaObs }
	);

	// Gather Tracking

	FLearningAgentsObservationElement PlayerLocationObs = ULearningAgentsObservations::MakeLocationObservation(
		InObservationObject,
		ObservationObjects.PlayerRelativeLocation,
		PlayerCharacter->GetActorLocation(),
		AIAgent->GetActorTransform(),  // Relative to AI
		2000.0f
	);

	FLearningAgentsObservationElement PlayerVelocityObs = ULearningAgentsObservations::MakeVelocityObservation(
		InObservationObject,
		ObservationObjects.PlayerVelocity,
		PlayerCharacter->GetVelocity(),
		FTransform::Identity,
		600.0f
	);

	float Distance = FVector::Dist(AIAgent->GetActorLocation(), PlayerCharacter->GetActorLocation());
	FLearningAgentsObservationElement DistanceObs = ULearningAgentsObservations::MakeFloatObservation(
		InObservationObject,
		ObservationObjects.DistanceToPlayer,
		Distance
	);


	FLearningAgentsObservationElement PlayerTrackingObs = ULearningAgentsObservations::MakeStructObservationFromArrayViews(
		InObservationObject,
		ObservationObjects.PlayerTracking,
		{ PlayerLocationObs, PlayerVelocityObs, DistanceObs }
	);

	// COMBINE ALL

	OutObservationElement = ULearningAgentsObservations::MakeStructObservationFromArrayViews(
		InObservationObject,
		ObservationObjects.AllObservations,
		{ SelfStateObs, PlayerTrackingObs }
	);
}

void UTagAIInteractor::SpecifyAgentAction_Implementation(FLearningAgentsActionSchemaElement& OutActionSchemaElement, ULearningAgentsActionSchema* InActionSchema)
{
	ActionObjects.MoveDirection = ULearningAgentsActions::SpecifyLocationAction(
		InActionSchema,
		TEXT("MoveDirection"),
		1.0f 
	);

	ActionObjects.ShouldSprint = ULearningAgentsActions::SpecifyBoolAction(
		InActionSchema,
		TEXT("ShouldSprint")
	);

	ActionObjects.MovementAction = ULearningAgentsActions::SpecifyStructAction(
		InActionSchema,
		TEXT("MovementAction")
	);
	ULearningAgentsActions::MakeStructActionFromArrayViews(
		InActionSchema,
		ActionObjects.MovementAction,
		{
			ActionObjects.MoveDirection,
			ActionObjects.ShouldSprint
		}
	);

	OutActionSchemaElement = ActionObjects.MovementAction;
}

void UTagAIInteractor::PerformAgentAction_Implementation(const int32 AgentId, ULearningAgentsActionObject* InActionObject, const FLearningAgentsActionElement& InActionElement)
{
	AAITagCharacter* AIAgent = Cast<AAITagCharacter>(GetAgent(AgentId));
	if (!AIAgent)
	{
		return;
	}

	FVector MoveDirection = ULearningAgentsActions::GetLocationAction(
		InActionObject,
		ActionObjects.MoveDirection,
		InActionElement
	);

	bool bShouldSprint = ULearningAgentsActions::GetBoolAction(
		InActionObject,
		ActionObjects.ShouldSprint,
		InActionElement
	);

	// Normalize and apply movement
	MoveDirection.Normalize();

	MoveDirection.Z = 0.0f;

	if (!MoveDirection.IsNearlyZero())
	{
		AIAgent->AddMovementInput(MoveDirection, 1.0f);
	}

	UCharacterMovementComponent* MovementComp = AIAgent->GetCharacterMovement();
	if (MovementComp)
	{
		if (bShouldSprint)
		{
			AIAgent->SetSprintDesired(true);
		}
		else
		{
			AIAgent->SetSprintDesired(false);
		}
	}
}

AInputCharacter* UTagAIInteractor::FindPlayerCharacter()
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
