// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_TestingGameState.h"

ACPP_TestingGameState::ACPP_TestingGameState()
{
	roundNumber = 1;
	startSeconds = 6000;
	seconds = 0;
}

void ACPP_TestingGameState::BeginPlay()
{
	initializeRound(false);
}

bool ACPP_TestingGameState::winConditionCheck()
{
	return false;
}
