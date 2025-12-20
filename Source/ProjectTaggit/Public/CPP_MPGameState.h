#pragma once
#include "CoreMinimal.h"
#include "CPP_BaseGameState.h"
#include "CPP_MPGameState.generated.h"


UCLASS()
class PROJECTTAGGIT_API ACPP_MPGameState : public ACPP_BaseGameState
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Spawning")
	TSubclassOf<class AAITagCharacter> AICharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Spawning")
	float AISpawnInterval = 10.0f;

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
	int32 AISpawnCount = 0;

	TArray<FVector> SpawnLocations;

	float RetagTimeRemaining = 0.0f;
	float MaxRetagTime = 25.0f;
	bool bPlayerMustRetag = false;

	bool bPlayerWasTaggerLastFrame = false;

	bool bGameEnded = false;
	bool bPlayerWon = false;

	void FindPlayer();

	void SpawnAI();

	void CheckPlayerTagged();

	void ShowGameResult(bool bWin);

	void GenerateSpawnLocations();

};