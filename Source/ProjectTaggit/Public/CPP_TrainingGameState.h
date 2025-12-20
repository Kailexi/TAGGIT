// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CPP_BaseGameState.h"
#include "CPP_TrainingGameState.generated.h"

UCLASS()
class PROJECTTAGGIT_API ACPP_TrainingGameState : public ACPP_BaseGameState
{
	GENERATED_BODY()

public:
	// Training configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	bool bInfiniteTime = true;  

protected:
	ACPP_TrainingGameState();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool winConditionCheck() override;

private:
	UPROPERTY()
	class AInputCharacter* PlayerCharacter = nullptr;

	void FindPlayer();
};
