#include "CPP_MPGameState.h"
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "ProjectTaggit/Public/AITagCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACPP_MPGameState::ACPP_MPGameState()
{
	roundNumber = 1;
	startSeconds = 420;  // 7 minutes
	seconds = 0;
	PrimaryActorTick.bCanEverTick = true;
}

void ACPP_MPGameState::BeginPlay()
{
	Super::BeginPlay();
	initializeRound(false);

	GenerateSpawnLocations();

	FindPlayer();

	SpawnAI();

	if (PlayerCharacter)
	{
		PlayerCharacter->SetTaggerStatus(false);  // Player is Hider
		bPlayerWasTaggerLastFrame = false;

		UE_LOG(LogTemp, Warning, TEXT("MP Survival Mode: Survive 7 minutes! New AI spawns every minute!"));
	}
}

void ACPP_MPGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bGameEnded)
		return;

	if (!PlayerCharacter)
	{
		FindPlayer();
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
				FString::Printf(TEXT("RETAG AN AI IN: %.1f seconds!"), RetagTimeRemaining)
			);
		}

		if (RetagTimeRemaining <= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player FAILED to retag in time! GAME OVER!"));
			ShowGameResult(false);
			bGameEnded = true;
			bPlayerWon = false;
			return;
		}
	}

	AISpawnTimer += DeltaTime;
	if (AISpawnTimer >= AISpawnInterval)
	{
		AISpawnTimer = 0.0f;
		SpawnAI();
	}

	if (GEngine)
	{
		int32 MinutesLeft = seconds / 60;
		int32 SecondsLeft = seconds % 60;
		int32 NextSpawnIn = (int32)(AISpawnInterval - AISpawnTimer);

		GEngine->AddOnScreenDebugMessage(
			2,
			DeltaTime,
			FColor::Yellow,
			FString::Printf(TEXT("Time Left: %d:%02d | AIs: %d | Next AI in: %ds"),
				MinutesLeft, SecondsLeft, AICharacters.Num(), NextSpawnIn)
		);
	}

	// Check win condition
	if (winConditionCheck())
	{
		ShowGameResult(true);
		bGameEnded = true;
		bPlayerWon = true;
	}
}

bool ACPP_MPGameState::winConditionCheck()
{
	// Player wins if:
	// 1. Timer reaches 0 (7 minutes passed)
	// 2. Player survived
	if (seconds <= 0 && PlayerCharacter && !bPlayerMustRetag)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player survived 7 minutes! Player WINS!"));
		return true;
	}

	return false;
}

void ACPP_MPGameState::FindPlayer()
{
	if (!PlayerCharacter)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		PlayerCharacter = Cast<AInputCharacter>(PlayerPawn);
	}
}

void ACPP_MPGameState::SpawnAI()
{
	if (!GetWorld())
		return;

	FVector SpawnLocation = FVector(0, 0, 100);
	if (SpawnLocations.Num() > 0)
	{
		int32 SpawnIndex = AISpawnCount % SpawnLocations.Num();
		SpawnLocation = SpawnLocations[SpawnIndex];
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AAITagCharacter* NewAI = GetWorld()->SpawnActor<AAITagCharacter>(
		AAITagCharacter::StaticClass(),
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewAI)
	{
		bool bIsTagger = (AISpawnCount == 0);
		NewAI->SetTaggerStatus(bIsTagger);

		AICharacters.Add(NewAI);
		AISpawnCount++;

		UE_LOG(LogTemp, Warning, TEXT("Spawned AI #%d at location %s (Tagger: %s)"),
			AISpawnCount, *SpawnLocation.ToString(), bIsTagger ? TEXT("Yes") : TEXT("No"));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Orange,
				FString::Printf(TEXT("New AI Spawned! Total AIs: %d"), AICharacters.Num())
			);
		}
	}
}

void ACPP_MPGameState::CheckPlayerTagged()
{
	if (!PlayerCharacter)
		return;

	bool bPlayerIsCurrentlyTagger = PlayerCharacter->IsTagger();

	if (bPlayerIsCurrentlyTagger && !bPlayerWasTaggerLastFrame)
	{
		bPlayerMustRetag = true;
		RetagTimeRemaining = MaxRetagTime;

		UE_LOG(LogTemp, Warning, TEXT("Player got TAGGED! Must retag an AI within 25 seconds or LOSE!"));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Red,
				TEXT("YOU GOT TAGGED! RETAG AN AI WITHIN 25 SECONDS!"),
				true,
				FVector2D(1.5f, 1.5f)
			);
		}
	}
	else if (!bPlayerIsCurrentlyTagger && bPlayerWasTaggerLastFrame)
	{
		bPlayerMustRetag = false;
		RetagTimeRemaining = 0.0f;

		UE_LOG(LogTemp, Warning, TEXT("Player successfully RETAGGED an AI! Continue surviving!"));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Green,
				TEXT("Good job! You retagged an AI! Keep surviving!")
			);
		}
	}

	bPlayerWasTaggerLastFrame = bPlayerIsCurrentlyTagger;
}

void ACPP_MPGameState::ShowGameResult(bool bWin)
{
	if (GEngine)
	{
		FColor Color = bWin ? FColor::Green : FColor::Red;
		FString Message = bWin ?
			FString::Printf(TEXT("YOU WIN! Survived 7 minutes against %d AIs!"), AICharacters.Num()) :
			TEXT("YOU LOSE! Failed to retag in time!");

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

void ACPP_MPGameState::GenerateSpawnLocations()
{
	// Generate 12 spawn points in a circle around origin
	int32 NumSpawnPoints = 12;
	float Radius = 2000.0f;  
	float AngleStep = 360.0f / NumSpawnPoints;

	for (int32 i = 0; i < NumSpawnPoints; i++)
	{
		float Angle = AngleStep * i;
		float Radians = FMath::DegreesToRadians(Angle);

		float X = FMath::Cos(Radians) * Radius;
		float Y = FMath::Sin(Radians) * Radius;
		float Z = 100.0f;

		SpawnLocations.Add(FVector(X, Y, Z));
	}

	UE_LOG(LogTemp, Log, TEXT("Generated %d spawn locations in a circle"), SpawnLocations.Num());
}