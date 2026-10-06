#include "common/general.h"
#include "game/game_main.h"

#include <maxmod9.h>

#include "soundbank.h"
#include "soundbank_bin.h"

int main(int argc, char **argv)
{
	DS_InitHardware();
	// keep both screens black until the menu has drawn its first frames: until
	// then they would show whatever VRAM holds (the debug console's font)
	setBrightness(3,-16);
	DS_InitFS(argc, argv);
	
	consoleDemoInit();//DEBUG
	
	#ifdef FATONLY
		sprintf(packPath,"packs/eldpack");
	#else
		sprintf(packPath,"nitro:/dscraft/packs/eldpack");
	#endif
	
	NOGBA("init ! %d",fsMode);
	
	time_t unixTime = time(NULL);
	struct tm* timeStruct = gmtime((const time_t *)&unixTime);
	srand(timeStruct->tm_hour*timeStruct->tm_min*timeStruct->tm_sec*timeStruct->tm_mon*timeStruct->tm_mday*timeStruct->tm_year);
	
	mmInitDefaultMem((mm_addr)soundbank_bin);
		
	mmLoadEffect(SFX_STEP);
	mmLoadEffect(SFX_ADD);
	mmLoadEffect(SFX_REMOVE);
		
	DS_CreateState(&Game_State, (function)&Game_Init, (function)&Game_Frame, (function)&Game_Kill, (function)&Game_VBlank);
	DS_CreateState(&Menu_State, (function)&Menu_Init, (function)&Menu_Frame, (function)&Menu_Kill, (function)&Menu_VBlank);
	
	DS_ChangeState(&Menu_State);
	DS_ApplyState();
	
	while(1)
	{
		CurrentState->Init();
		while(CurrentState->used)CurrentState->Frame();
		CurrentState->Kill();
		DS_ApplyState();
	}
	return 0;
}
