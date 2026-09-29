
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisionMenuActor.generated.h"

class UWidgetComponent;

/**
 * Pannello menu fluttuante stile visionOS.
 * Chiamare ToggleMenu() dal VRPawn (input bottone Menu del controller):
 * appare davanti al giocatore, lo segue dolcemente e resta rivolto verso di lui.
 */
UCLASS()
class THE_SKALDS_HALL_API AVisionMenuActor : public AActor
{
	GENERATED_BODY()

public:
	AVisionMenuActor();

	UPROPERTY(VisibleAnywhere, Category = "Menu")
	UWidgetComponent* MenuWidget;

	/** Distanza dal visore in cm. */
	UPROPERTY(EditAnywhere, Category = "Menu")
	float DistanceFromPlayer = 120.f;

	/** Velocita' del follow (piu' alto = piu' reattivo). */
	UPROPERTY(EditAnywhere, Category = "Menu")
	float FollowSpeed = 5.f;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ToggleMenu();

	UFUNCTION(BlueprintPure, Category = "Menu")
	bool IsMenuOpen() const { return bOpen; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bOpen = false;

	bool GetPlayerView(FVector& OutLocation, FRotator& OutRotation) const;
	FVector ComputeTargetLocation() const;
};

