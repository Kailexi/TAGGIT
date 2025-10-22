// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_BaseGameState.h"

int ACPP_BaseGameState::getSeconds()
{
	return seconds;
}

void ACPP_BaseGameState::setSeconds(int newSeconds)
{
	seconds = newSeconds;
}

int ACPP_BaseGameState::getStartSeconds()
{
	return startSeconds;
}

void ACPP_BaseGameState::setStartSeconds(int newStartSeconds)
{
	startSeconds = newStartSeconds;
}

void ACPP_BaseGameState::setRoundNumber(int newRoundNumber)
{
	roundNumber = newRoundNumber;
}

int ACPP_BaseGameState::getRoundNumber()
{
	return roundNumber;
}

bool ACPP_BaseGameState::isTimeTicking()
{
	return GetWorld()->GetTimerManager().IsTimerActive(timer);
}

void ACPP_BaseGameState::setTimeState(bool timeState)
{
	if (timeState)
		GetWorld()->GetTimerManager().UnPauseTimer(timer);
	else
		GetWorld()->GetTimerManager().PauseTimer(timer);
}

void ACPP_BaseGameState::onTimer()
{
	seconds--;
	
	if (seconds == 0)
		initializeRound();
}

ACPP_BaseGameState::ACPP_BaseGameState()
{
	roundNumber = 1;
	startSeconds = 100;
	seconds = 0;
}

void ACPP_BaseGameState::BeginPlay()
{
	initializeRound(false);
}

bool ACPP_BaseGameState::winConditionCheck()
{
	return false;
}

void ACPP_BaseGameState::initializeRound(bool next)
{
	if (GetWorld()->GetTimerManager().TimerExists(timer))
		GetWorld()->GetTimerManager().ClearTimer(timer);

	if (next)
		roundNumber++;

	seconds = startSeconds;

	GetWorld()->GetTimerManager().SetTimer(timer, this, &ACPP_BaseGameState::onTimer, 1.0f, true);
}