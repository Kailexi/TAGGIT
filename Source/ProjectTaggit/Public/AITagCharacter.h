
#pragma once
#include "CoreMinimal.h"
#include "ProjectTaggit/InputPlayer/InputCharacter.h"
#include "AITagCharacter.generated.h"


UENUM(BlueprintType)
enum class EAIDifficulty : uint8
{
	Easy UMETA(DisplayName = "Easy"),
	Medium UMETA(DisplayName = "Medium"),
	Hard UMETA(DisplayName = "Hard")
};


UCLASS()
class PROJECTTAGGIT_API AAITagCharacter : public AInputCharacter
{
	GENERATED_BODY()

public:
	AAITagCharacter();

	// AI Difficulty Setting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Settings")
	EAIDifficulty Difficulty = EAIDifficulty::Medium;

	// AI Behavior Parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	float ChaseRadius = 2000.0f;  // (20m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	float DashRadius = 400.0f;    // (4m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	float SprintRadius = 1000.0f; //  (10m)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	float PatrolRadius = 500.0f;  //  (5m)

	// AI Movement Functions
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void AIMoveToward(FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void AIPerformTagDash(FVector Direction);

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void AIStartSprint();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void AIEndSprint();

	// AI State Queries
	UFUNCTION(BlueprintCallable, Category = "AI|State")
	float GetDistanceToPlayer() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	AInputCharacter* GetPlayerCharacter() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	bool CanUseDash() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// Cached player reference
	UPROPERTY()
	AInputCharacter* CachedPlayer = nullptr;

	//timers
	float UpdatePlayerCacheTimer = 0.0f;
	float UpdatePlayerCacheInterval = 1.0f;
};