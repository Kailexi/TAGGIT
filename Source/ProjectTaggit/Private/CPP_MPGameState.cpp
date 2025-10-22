// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_MPGameState.h"

ACPP_MPGameState::ACPP_MPGameState()
{
	roundNumber = 1;
	startSeconds = 600;
	seconds = 0;
}

void ACPP_MPGameState::BeginPlay()
{
	initializeRound(false);
}

bool ACPP_MPGameState::winConditionCheck()
{
	return false;
}