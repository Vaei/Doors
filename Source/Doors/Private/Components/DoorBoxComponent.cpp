// Copyright (c) Jared Taylor


#include "Components/DoorBoxComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorBoxComponent)


const float UDoorBoxComponent::MIN_TICK_TIME = 1e-6f;

UDoorBoxComponent::UDoorBoxComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);

	BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetGenerateOverlapEvents(true);
}

void UDoorBoxComponent::SetTarget(USceneComponent* InUpdatedComponent, float InTargetAlpha, FRotator InInwardDelta, FRotator InOutwardDelta)
{
	UpdatedComponent = InUpdatedComponent;
	TargetAlpha = FMath::Clamp(InTargetAlpha, -1.f, 1.f);
	InwardRotationDelta = InInwardDelta;
	OutwardRotationDelta = InOutwardDelta;
}

void UDoorBoxComponent::TickBox(float DeltaTime)
{
	if (!UpdatedComponent.IsValid() || DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (FMath::IsNearlyEqual(ProgressAlpha, TargetAlpha, KINDA_SMALL_NUMBER))
	{
		return;
	}

	auto AlphaToRotator = [&](float Alpha) -> FRotator
	{
		return (Alpha < 0.0f)
			? FMath::Lerp(FRotator::ZeroRotator, InwardRotationDelta, -Alpha)
			: FMath::Lerp(FRotator::ZeroRotator, OutwardRotationDelta, Alpha);
	};

	USceneComponent* Hinge = UpdatedComponent.Get();

	const float StepTime = 1.0f / SimulationFrequency;

	if (bLimitByFrameRate)
	{
		SimulatedTimeAccumulator = StepTime;
	}
	else
	{
		SimulatedTimeAccumulator += DeltaTime;
	}

	while (SimulatedTimeAccumulator >= StepTime)
	{
		SimulatedTimeAccumulator -= StepTime;

		const float RemainingAlpha = TargetAlpha - ProgressAlpha;
		if (FMath::IsNearlyZero(RemainingAlpha))
		{
			break;
		}

		const float StepAlpha = FMath::Clamp(StepTime, 0.f, 1.f) * FMath::Sign(RemainingAlpha);

		float TestAlpha = ProgressAlpha + StepAlpha;

		if (FMath::Abs(TestAlpha - TargetAlpha) > FMath::Abs(RemainingAlpha))
		{
			TestAlpha = TargetAlpha;
		}

		const FRotator TestRot = AlphaToRotator(TestAlpha);
		Hinge->SetRelativeRotation(TestRot, false, nullptr, ETeleportType::None);

		// Overlap check
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(DoorBoxOverlap), false, GetOwner());

		FCollisionObjectQueryParams ObjectParams;
		for (const ECollisionChannel Channel : { ECC_WorldStatic, ECC_WorldDynamic, ECC_Pawn, ECC_Vehicle })
		{
			ObjectParams.AddObjectTypesToQuery(Channel);
		}

		GetWorld()->OverlapMultiByObjectType(
			Overlaps,
			GetComponentLocation(),
			FQuat::Identity,
			ObjectParams,
			GetCollisionShape(),
			Params
		);

		bool bBlock = false;
		const FRotator CurrentRot = AlphaToRotator(ProgressAlpha);

		const FVector Pivot = GetComponentLocation();
		const FVector DeltaYaw = FVector::UpVector * (TestRot.Yaw - CurrentRot.Yaw);

		// Compute "direction of rotation" as change in forward vector
		const FVector ForwardNow = CurrentRot.Vector().GetSafeNormal2D();
		const FVector ForwardNext = TestRot.Vector().GetSafeNormal2D();
		const FVector RotationDir = (ForwardNext - ForwardNow).GetSafeNormal2D();
		
		for (const FOverlapResult& Result : Overlaps)
		{
			const AActor* OverlapActor = Result.GetActor();
			if (!OverlapActor)
			{
				continue;
			}

			const FVector ToActor = OverlapActor->GetActorLocation() - Pivot;
			if (ToActor.IsNearlyZero())
			{
				continue;
			}

			// Tangent direction of rotation at the actor’s position
			const float Dot = FVector::DotProduct(RotationDir, ToActor.GetSafeNormal2D());

			DrawDebugDirectionalArrow(GetWorld(), Pivot, Pivot + RotationDir.GetSafeNormal2D() * 200.f, 10.f, FColor::Red, false, 0.f, 10.f, 1.f);
			DrawDebugDirectionalArrow(GetWorld(), Pivot, Pivot + ToActor.GetSafeNormal2D() * 200.f, 10.f, FColor::Blue, false, 0.f, 10.f, 1.f);

			UE_LOG(LogTemp, Warning, TEXT("Dot: %f"), Dot);
			
			// If Dot is positive, we’re sweeping toward the actor
			if (Dot < 0.f)
			{
				bBlock = true;
				break;
			}
		}

		if (bBlock)
		{
			const FRotator RewindRot = AlphaToRotator(ProgressAlpha);
			Hinge->SetRelativeRotation(RewindRot, false, nullptr, ETeleportType::None);
			break;
		}

		ProgressAlpha = TestAlpha;

		if (FMath::IsNearlyEqual(ProgressAlpha, TargetAlpha, KINDA_SMALL_NUMBER))
		{
			break;
		}
	}
}
