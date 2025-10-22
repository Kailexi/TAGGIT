// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CPP_BaseGameState.generated.h"


/**
 * 
 */
UCLASS()
class PROJECTTAGGIT_API ACPP_BaseGameState : public AGameStateBase
{
	GENERATED_BODY()


	void onTimer();

protected:

	FTimerHandle timer;

	int roundNumber;

	int startSeconds;

	int seconds;

	ACPP_BaseGameState();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	virtual bool winConditionCheck();

	UFUNCTION(BlueprintCallable)
	void initializeRound(bool next = true);

public:

	UFUNCTION(BlueprintCallable)
	int getSeconds();

	UFUNCTION(BlueprintCallable)
	void setSeconds(int newSeconds);

	UFUNCTION(BlueprintCallable)
	int getStartSeconds();

	UFUNCTION(BlueprintCallable)
	void setStartSeconds(int newStartSeconds);

	UFUNCTION(BlueprintCallable)
	int getRoundNumber();

	UFUNCTION(BlueprintCallable)
	void setRoundNumber(int newRoundNumber);

	UFUNCTION(BlueprintCallable)
	bool isTimeTicking();

	UFUNCTION(BlueprintCallable)
	void setTimeState(bool timeState);

};
