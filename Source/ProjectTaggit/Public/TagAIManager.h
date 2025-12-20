// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TagAIManager.generated.h"

class ULearningAgentsManager;
class UTagAIInteractor;
class ULearningAgentsPolicy;
class ULearningAgentsCritic;
class UTagAITrainingEnvironment;
class ULearningAgentsTrainer;
class ULearningAgentsNeuralNetwork;
class ULearningAgentsSharedMemoryTrainingProcess;
class ULearningAgentsSharedMemoryCommunicator;


UCLASS()
class PROJECTTAGGIT_API ATagAIManager : public AActor
{
	GENERATED_BODY()

public:
	ATagAIManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Components we don't own lmaoooooo

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning Agents")
	ULearningAgentsManager* Manager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning Agents")
	UTagAIInteractor* Interactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning Agents")
	ULearningAgentsPolicy* Policy;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning Agents")
	ULearningAgentsCritic* Critic;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning Agents")
	UTagAITrainingEnvironment* TrainingEnvironment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning Agents")
	ULearningAgentsTrainer* Trainer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning Agents")
	ULearningAgentsSharedMemoryTrainingProcess* TrainingProcess;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Learning Agents")
	ULearningAgentsSharedMemoryCommunicator* Communicator;

	// Assets for RNN and LSTM

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Neural Networks")
	ULearningAgentsNeuralNetwork* EncoderNetwork;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Neural Networks")
	ULearningAgentsNeuralNetwork* PolicyNetwork;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Neural Networks")
	ULearningAgentsNeuralNetwork* DecoderNetwork;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Neural Networks")
	ULearningAgentsNeuralNetwork* CriticNetwork;

	// Settings for BPS

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Settings")
	bool bRunInference = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Settings")
	bool bReinitializeNetworks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Settings")
	int32 MaxAgents = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Training")
	int32 MaxEpisodeLength = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Learning Agents|Training")
	int32 RandomSeed = 1234;

private:
	// Setup helpers
	void SetupManager();
	void SetupInteractor();
	void SetupPolicy();
	void SetupCritic();
	void SetupTrainingEnvironment();
	void SetupTrainer();
	void ResetAllAgentsIfInference();
};
