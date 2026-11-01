#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/**
 * GameMode per la mappa MenuPrincipale: spawna AMainMenuPawn con puntatori laser VR.
 */
UCLASS()
class THE_SKALDS_HALL_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();
};