#pragma once
#include "CoreMinimal.h"
#include "CPP_BaseGameState.h"
#include "CPP_AIGameState.generated.h"

UCLASS()
class PROJECTTAGGIT_API ACPP_AIGameState : public ACPP_BaseGameState
{
	GENERATED_BODY()

protected:

	ACPP_AIGameState();

	virtual void BeginPlay() override;

	virtual bool winConditionCheck() override;

	virtual void Tick(float DeltaTime) override;

private:

	// References to player and AI
	UPROPERTY()
	class AInputCharacter* PlayerCharacter = nullptr;

	UPROPERTY()
	class AAITagCharacter* AICharacter = nullptr;

	// Retag timer
	float RetagTimeRemaining = 0.0f;
	float MaxRetagTime = 25.0f;
	bool bPlayerMustRetag = false;

	bool bPlayerWasTaggerLastFrame = false;

	// Game result flags
	bool bGameEnded = false;
	bool bPlayerWon = false;


	void FindCharacters();

	void CheckPlayerTagged();

	void ShowGameResult(bool bWin);

};