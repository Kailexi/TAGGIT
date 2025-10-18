#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTTAGGIT_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UStaminaComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	float CurrentStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float MaxStamina = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float StaminaRegenRate = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float RegenDelay = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	bool bIsExhausted;

private:
	float RegenDelayRemaining;

public:
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool TryConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool CanPerformAction(float Amount = 0.0f) const;

	void UpdateStamina(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	float GetCurrentStamina() const;
};
