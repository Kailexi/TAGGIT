// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_TestGameState.h"

ACPP_TestGameState::ACPP_TestGameState()
{
	roundNumber = 1;
	startSeconds = 6000;
	seconds = 0;
}

void ACPP_TestGameState::BeginPlay()
{
	initializeRound(false);
}

bool ACPP_TestGameState::winConditionCheck()
{
	return false;
}
