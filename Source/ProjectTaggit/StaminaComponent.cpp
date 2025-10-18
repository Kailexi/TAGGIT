#include "StaminaComponent.h"
#include "Math/UnrealMathUtility.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentStamina = MaxStamina;
	bIsExhausted = false;
	RegenDelayRemaining = 0.0f;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateStamina(DeltaTime);
}

float UStaminaComponent::GetCurrentStamina() const
{
	return CurrentStamina;
}

float UStaminaComponent::GetMaxStamina() const
{
    return MaxStamina;
}

bool UStaminaComponent::CanPerformAction(float Amount) const
{
	return CurrentStamina >= Amount && !bIsExhausted;
}

bool UStaminaComponent::TryConsumeStamina(float Amount)
{
    UE_LOG(LogTemp, Warning, TEXT("Trying to consume stamina: %f"), Amount);
    if (CanPerformAction(Amount))
    {
        CurrentStamina = FMath::Clamp(CurrentStamina - Amount, 0.0f, MaxStamina);
        RegenDelayRemaining = RegenDelay;
        UE_LOG(LogTemp, Warning, TEXT("Stamina after consumption: %f"), CurrentStamina);
        return true;
    }
    return false;
}

void UStaminaComponent::UpdateStamina(float DeltaTime)
{
	if (RegenDelayRemaining > 0.0f)
	{
		RegenDelayRemaining -= DeltaTime;
	}
	else if (CurrentStamina < MaxStamina)
	{
		CurrentStamina += StaminaRegenRate * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
		bIsExhausted = CurrentStamina <= 0.0f;
	}
}


