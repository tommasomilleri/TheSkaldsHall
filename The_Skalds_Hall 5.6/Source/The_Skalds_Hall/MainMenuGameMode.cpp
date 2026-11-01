#include "MainMenuGameMode.h"
#include "MainMenuPawn.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = AMainMenuPawn::StaticClass();
}