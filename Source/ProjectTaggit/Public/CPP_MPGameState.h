
#pragma once

#include "CoreMinimal.h"
#include "CPP_BaseGameState.h"
#include "CPP_MPGameState.generated.h"


UCLASS()
class PROJECTTAGGIT_API ACPP_MPGameState : public ACPP_BaseGameState
{
	GENERATED_BODY()

protected:

	ACPP_MPGameState();

	virtual void BeginPlay() override;

	virtual bool winConditionCheck() override;

	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY()
	class AInputCharacter* PlayerCharacter = nullptr;

	UPROPERTY()
	TArray<class AAITagCharacter*> AICharacters;

	// AI spawning
	float AISpawnTimer = 0.0f;
	float AISpawnInterval = 60.0f;  
	int32 AISpawnCount = 0;

	TArray<FVector> SpawnLocations;

	// Retag timer (when player gets tagged)
	float RetagTimeRemaining = 0.0f;
	float MaxRetagTime = 25.0f;
	bool bPlayerMustRetag = false;

	bool bPlayerWasTaggerLastFrame = false;

	bool bGameEnded = false;
	bool bPlayerWon = false;

	UPROPERTY(EditDefaultsOnly, Category = "AI Spawning")
	TSubclassOf<class AAITagCharacter> AICharacterClass;

	void FindPlayer();

	void SpawnAI();

	void CheckPlayerTagged();

	void ShowGameResult(bool bWin);

	void GenerateSpawnLocations();

};