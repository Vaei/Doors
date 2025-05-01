// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "DoorBoxComponent.generated.h"


/**
 * A high-precision box collision component designed to suit doors
 * Allows physics subticks for accurate collision detection
 * Allows actually detecting collision for rotation which is not supported from the engine with BP-exposed functions for components
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DOORS_API UDoorBoxComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	/** Simulation frequency in Hz (e.g. 60 = 60 simulation steps per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Door, meta=(ClampMin="1", UIMin="1", UIMax="120", Delta="1"))
	float SimulationFrequency = 60.f;

	/** If true, we cannot tick slower than our frame rate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Door)
	bool bLimitByFrameRate;
	
protected:
	float ProgressAlpha = 0.f;
	float TargetAlpha = 0.f;
	
public:
	UDoorBoxComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Assigns the hinge to move based on this component's overlap detection */
	UFUNCTION(BlueprintCallable, Category=Door)
	void SetTarget(USceneComponent* InUpdatedComponent, float InTargetAlpha, FRotator InInwardDelta, FRotator InOutwardDelta);

	/** Performs the movement in small steps to simulate high-precision interaction */
	UFUNCTION(BlueprintCallable, Category=Door)
	void TickBox(float DeltaTime);

private:
	TWeakObjectPtr<USceneComponent> UpdatedComponent;
	FRotator TargetRotation;
	FVector TargetLocation;

	FRotator InwardRotationDelta = FRotator::ZeroRotator;
	FRotator OutwardRotationDelta = FRotator::ZeroRotator;

	float SimulatedTimeAccumulator = 0.f;
	
	/** Minimum delta time considered when ticking. Delta times below this are not considered. This is a very small non-zero value to avoid potential divide-by-zero in simulation code. */
	static const float MIN_TICK_TIME;
};
