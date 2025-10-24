// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPP_BaseGameState.h"
#include "CPP_AIGameState.generated.h"

/**
 *
 */
UCLASS()
class PROJECTTAGGIT_API ACPP_AIGameState : public ACPP_BaseGameState
{
	GENERATED_BODY()

protected:

	ACPP_AIGameState();

	virtual void BeginPlay() override;

	virtual bool winConditionCheck() override;

};
