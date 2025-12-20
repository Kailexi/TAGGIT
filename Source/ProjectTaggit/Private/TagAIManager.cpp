// Copyright Epic Games, Inc. All Rights Reserved.

#include "TagAIManager.h"
#include "TagAIInteractor.h"
#include "TagAITrainingEnvironment.h"
#include "LearningAgentsManager.h"
#include "LearningAgentsPolicy.h"
#include "LearningAgentsCritic.h"
#include "LearningAgentsTrainer.h"
#include "LearningAgentsNeuralNetwork.h"
#include "LearningAgentsSharedMemoryTrainingProcess.h"
#include "LearningAgentsSharedMemoryCommunicator.h"

ATagAIManager::ATagAIManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	Manager = CreateDefaultSubobject<ULearningAgentsManager>(TEXT("LearningAgentsManager"));

	Tags.Add(FName("LearningAgentsManager"));
}

void ATagAIManager::BeginPlay()
{
	Super::BeginPlay();

	SetupManager();
	SetupInteractor();
	SetupPolicy();
	SetupCritic();
	SetupTrainingEnvironment();

	if (!bRunInference)
	{
		SetupTrainer();
	}
	else
	{
		ResetAllAgentsIfInference();
	}

	UE_LOG(LogLearning, Log, TEXT("TagAIManager: Setup complete. RunInference=%d"), bRunInference);
}

void ATagAIManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Manager || !Trainer)
	{
		return;
	}

	if (bRunInference)
	{
		if (Interactor && Policy)
		{
			Interactor->GatherObservations();
			Policy->EvaluatePolicy();
			Interactor->PerformActions();
		}
	}
	else
	{
		if (Trainer)
		{
			Trainer->RunTraining();
		}
	}
}

void ATagAIManager::SetupManager()
{
	if (!Manager)
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Manager component is null!"));
		return;
	}

	Manager->SetMaxAgentNum(MaxAgents);
	UE_LOG(LogLearning, Log, TEXT("TagAIManager: Manager setup with max %d agents"), MaxAgents);
}

void ATagAIManager::SetupInteractor()
{
	if (!Manager)
	{
		return;
	}

	Interactor = ULearningAgentsInteractor::MakeInteractor(
		Manager,
		UTagAIInteractor::StaticClass(),
		TEXT("TagAIInteractor")
	);

	if (Interactor)
	{
		UE_LOG(LogLearning, Log, TEXT("TagAIManager: Interactor created successfully"));
	}
	else
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Failed to create Interactor"));
	}
}

void ATagAIManager::SetupPolicy()
{
	if (!Interactor)
	{
		return;
	}

	FLearningAgentsPolicySettings PolicySettings;
	PolicySettings.bReinitializeEncoderNetwork = bReinitializeNetworks;
	PolicySettings.bReinitializePolicyNetwork = bReinitializeNetworks;
	PolicySettings.bReinitializeDecoderNetwork = bReinitializeNetworks;
	PolicySettings.RandomSeed = RandomSeed;

	// Create policy
	Policy = ULearningAgentsPolicy::MakePolicy(
		Manager,
		Interactor,
		ULearningAgentsPolicy::StaticClass(),
		TEXT("TagAIPolicy"),
		EncoderNetwork,
		PolicyNetwork,
		DecoderNetwork,
		PolicySettings
	);

	if (Policy)
	{
		UE_LOG(LogLearning, Log, TEXT("TagAIManager: Policy created successfully"));
	}
	else
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Failed to create Policy"));
	}
}

void ATagAIManager::SetupCritic()
{
	if (!Interactor)
	{
		return;
	}

	FLearningAgentsCriticSettings CriticSettings;
	CriticSettings.bReinitializeCriticNetwork = bReinitializeNetworks;
	CriticSettings.RandomSeed = RandomSeed;
	Critic = ULearningAgentsCritic::MakeCritic(
		Manager,
		Interactor,
		ULearningAgentsCritic::StaticClass(),
		TEXT("TagAICritic"),
		CriticNetwork,
		CriticSettings
	);

	if (Critic)
	{
		UE_LOG(LogLearning, Log, TEXT("TagAIManager: Critic created successfully"));
	}
	else
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Failed to create Critic"));
	}
}

void ATagAIManager::SetupTrainingEnvironment()
{
	if (!Manager)
	{
		return;
	}

	// Create training environment
	TrainingEnvironment = ULearningAgentsTrainingEnvironment::MakeTrainingEnvironment(
		Manager,
		UTagAITrainingEnvironment::StaticClass(),
		TEXT("TagAITrainingEnvironment")
	);

	if (TrainingEnvironment)
	{
		UE_LOG(LogLearning, Log, TEXT("TagAIManager: Training Environment created successfully"));
	}
	else
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Failed to create Training Environment"));
	}
}

void ATagAIManager::SetupTrainer()
{
	if (!Policy || !Critic || !TrainingEnvironment)
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Cannot setup trainer - missing components"));
		return;
	}

	// Create shared memory training process
	FLearningAgentsTrainingProcessSettings ProcessSettings;
	ProcessSettings.TimeoutSeconds = 10.0f;

	TrainingProcess = ULearningAgentsSharedMemoryTrainingProcess::MakeSharedMemoryTrainingProcess(
		Manager,
		ULearningAgentsSharedMemoryTrainingProcess::StaticClass(),
		TEXT("TagAITrainingProcess"),
		ProcessSettings
	);

	if (!TrainingProcess)
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Failed to create Training Process"));
		return;
	}

	// Create communicator
	FLearningAgentsSharedMemoryCommunicatorSettings CommunicatorSettings;

	Communicator = ULearningAgentsSharedMemoryCommunicator::MakeSharedMemoryCommunicator(
		Manager,
		ULearningAgentsSharedMemoryCommunicator::StaticClass(),
		TEXT("TagAICommunicator"),
		TrainingProcess,
		CommunicatorSettings
	);

	if (!Communicator)
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Failed to create Communicator"));
		return;
	}

	// Create trainer
	FLearningAgentsTrainerSettings TrainerSettings;
	TrainerSettings.MaximumRecordedEpisodesPerIteration = 100;
	TrainerSettings.MaximumRecordedStepsPerIteration = 10000;

	FLearningAgentsTrainerTrainingSettings TrainingSettings;
	TrainingSettings.NumberOfIterations = 100000;  // Train for a long time
	TrainingSettings.LearningRatePolicy = 0.0001f;
	TrainingSettings.LearningRateCritic = 0.0001f;
	TrainingSettings.IterationsPerGather = 1;
	TrainingSettings.NumberOfStepsPerIteration = 2048;

	FLearningAgentsTrainerGameSettings GameSettings;
	GameSettings.EpisodeLengthsToRecord.Add(MaxEpisodeLength);

	Trainer = ULearningAgentsTrainer::MakeTrainer(
		Manager,
		Interactor,
		Policy,
		Critic,
		TrainingEnvironment,
		ULearningAgentsTrainer::StaticClass(),
		TEXT("TagAITrainer"),
		Communicator,
		TrainerSettings,
		TrainingSettings,
		GameSettings
	);

	if (Trainer)
	{
		UE_LOG(LogLearning, Log, TEXT("TagAIManager: Trainer created successfully"));
		UE_LOG(LogLearning, Log, TEXT("TagAIManager: Training started - this will take a while..."));
	}
	else
	{
		UE_LOG(LogLearning, Error, TEXT("TagAIManager: Failed to create Trainer"));
	}
}

void ATagAIManager::ResetAllAgentsIfInference()
{
	if (!Manager || !TrainingEnvironment)
	{
		return;
	}

	// Reset all agents to random positions
	TArray<int32> AgentIds = Manager->GetAllAgentIds();
	for (int32 AgentId : AgentIds)
	{
		TrainingEnvironment->ResetAgentEpisode(AgentId);
	}

	UE_LOG(LogLearning, Log, TEXT("TagAIManager: Reset %d agents for inference mode"), AgentIds.Num());
}
