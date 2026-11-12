#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VRMenuBootstrapSubsystem.generated.h"
UCLASS()
class THE_SKALDS_HALL_API UVRMenuBootstrapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};