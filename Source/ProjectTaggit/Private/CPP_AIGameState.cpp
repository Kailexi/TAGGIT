#include "CPP_AIGameState.h"
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "ProjectTaggit/Public/AITagCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACPP_AIGameState::ACPP_AIGameState()
{
	roundNumber = 1;
	startSeconds = 420;  // 7 minutes
	seconds = 0;
	PrimaryActorTick.bCanEverTick = true;
}

void ACPP_AIGameState::BeginPlay()
{
	Super::BeginPlay();
	initializeRound(false);

	FindCharacters();

	if (PlayerCharacter && AICharacter)
	{
		PlayerCharacter->SetTaggerStatus(false);  // Player is Hider
		AICharacter->SetTaggerStatus(true);       // AI is Tagger

		bPlayerWasTaggerLastFrame = false;

		UE_LOG(LogTemp, Warning, TEXT("AI Game Mode: Player is HIDER, AI is TAGGER. Survive 7 minutes to win!"));
	}
}

void ACPP_AIGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bGameEnded)
		return;

	if (!PlayerCharacter || !AICharacter)
	{
		FindCharacters();
		return;
	}

	CheckPlayerTagged();

	if (bPlayerMustRetag)
	{
		RetagTimeRemaining -= DeltaTime;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1,
				DeltaTime,
				FColor::Red,
				FString::Printf(TEXT("RETAG THE AI IN: %.1f seconds!"), RetagTimeRemaining)
			);
		}

		if (RetagTimeRemaining <= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player FAILED to retag in time! AI WINS!"));
			ShowGameResult(false);
			bGameEnded = true;
			bPlayerWon = false;
			return;
		}
	}

	if (winConditionCheck())
	{
		ShowGameResult(true);
		bGameEnded = true;
		bPlayerWon = true;
	}
}

bool ACPP_AIGameState::winConditionCheck()
{
	// Player wins if:
	// 1. Timer reaches 0 (7 minutes passed)
	// 2. Player is still a Hider (not the tagger)
	if (seconds <= 0 && PlayerCharacter && !PlayerCharacter->IsTagger())
	{
		UE_LOG(LogTemp, Warning, TEXT("Player survived 7 minutes as HIDER! Player WINS!"));
		return true;
	}

	return false;
}

void ACPP_AIGameState::FindCharacters()
{
	if (!PlayerCharacter)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		PlayerCharacter = Cast<AInputCharacter>(PlayerPawn);
	}

	if (!AICharacter)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAITagCharacter::StaticClass(), FoundActors);

		if (FoundActors.Num() > 0)
		{
			AICharacter = Cast<AAITagCharacter>(FoundActors[0]);
		}
	}
}

void ACPP_AIGameState::CheckPlayerTagged()
{
	if (!PlayerCharacter || !AICharacter)
		return;

	bool bPlayerIsCurrentlyTagger = PlayerCharacter->IsTagger();

	if (bPlayerIsCurrentlyTagger && !bPlayerWasTaggerLastFrame)
	{
		bPlayerMustRetag = true;
		RetagTimeRemaining = MaxRetagTime;

		UE_LOG(LogTemp, Warning, TEXT("Player got TAGGED! Must retag AI within 25 seconds or LOSE!"));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Red,
				TEXT("YOU GOT TAGGED! RETAG THE AI WITHIN 25 SECONDS!")
			);
		}
	}
	else if (!bPlayerIsCurrentlyTagger && bPlayerWasTaggerLastFrame)
	{
		bPlayerMustRetag = false;
		RetagTimeRemaining = 0.0f;

		UE_LOG(LogTemp, Warning, TEXT("Player successfully RETAGGED the AI! Continue surviving!"));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Green,
				TEXT("Good job! You retagged the AI! Keep surviving!")
			);
		}
	}

	bPlayerWasTaggerLastFrame = bPlayerIsCurrentlyTagger;
}

void ACPP_AIGameState::ShowGameResult(bool bWin)
{
	if (GEngine)
	{
		FColor Color = bWin ? FColor::Green : FColor::Red;
		FString Message = bWin ? TEXT("YOU WIN! You survived 7 minutes!") : TEXT("YOU LOSE! Failed to retag in time!");

		GEngine->AddOnScreenDebugMessage(
			-1,
			10.0f,
			Color,
			Message,
			true,
			FVector2D(2.0f, 2.0f)
		);
	}

	setTimeState(false);
}