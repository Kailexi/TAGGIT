// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_AIGameState.h"

ACPP_AIGameState::ACPP_AIGameState()
{
	roundNumber = 1;
	startSeconds = 420;
	seconds = 0;
}

void ACPP_AIGameState::BeginPlay()
{
	initializeRound(false);
}

bool ACPP_AIGameState::winConditionCheck()
{
	return false;
}


