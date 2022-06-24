/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */

#include <cstdint>

#include <SDL.h>

#include "control.h"
#include "engine.h"
#include "engine/cel_sprite.hpp"
#include "engine/dx.h"
#include "engine/load_cel.hpp"
#include "engine/load_pcx.hpp"
#include "engine/pcx_sprite.hpp"
#include "engine/render/cel_render.hpp"
#include "engine/render/pcx_render.hpp"
#include "hwcursor.hpp"
#include "init.h"
#include "loadsave.h"
#include "palette.h"
#include "pfile.h"
#include "plrmsg.h"
#include "utils/sdl_geometry.h"
#include "utils/stdcompat/optional.hpp"

namespace devilution {

namespace {

std::optional<OwnedCelSprite> sgpBackCel;

uint32_t sgdwProgress;
int progress_id;

/** The color used for the progress bar as an index into the palette. */
const BYTE BarColor[3] = { 138, 43, 254 };
/** The screen position of the top left corner of the progress bar. */
const int BarPos[3][2] = { { 53, 37 }, { 53, 421 }, { 53, 37 } };

std::optional<OwnedPcxSprite> ArtCutsceneWidescreen;

Cutscenes PickCutscene(interface_mode uMsg)
{
	switch (uMsg) {
	case WM_DIABLOADGAME:
	case WM_DIABNEWGAME:
		return CutStart;
	case WM_DIABRETOWN:
		return CutTown;
	case WM_DIABNEXTLVL:
	case WM_DIABPREVLVL:
	case WM_DIABTOWNWARP:
	case WM_DIABTWARPUP: {
		int lvl = MyPlayer->plrlevel;
		if (lvl == 1 && uMsg == WM_DIABNEXTLVL)
			return CutTown;
		if (lvl == 16 && uMsg == WM_DIABNEXTLVL)
			return CutGate;

		switch (GetLevelType(lvl)) {
		case DTYPE_TOWN:
			return CutTown;
		case DTYPE_CATHEDRAL:
			return CutLevel1;
		case DTYPE_CATACOMBS:
			return CutLevel2;
		case DTYPE_CAVES:
			return CutLevel3;
		case DTYPE_HELL:
			return CutLevel4;
		case DTYPE_NEST:
			return CutLevel6;
		case DTYPE_CRYPT:
			return CutLevel5;
		default:
			return CutLevel1;
		}
	}
	case WM_DIABWARPLVL:
		return CutPortal;
	case WM_DIABSETLVL:
	case WM_DIABRTNLVL:
		if (setlvlnum == SL_BONECHAMB)
			return CutLevel2;
		if (setlvlnum == SL_VILEBETRAYER)
			return CutPortalRed;
		return CutLevel1;
	default:
		app_fatal("Unknown progress mode");
	}
}

void LoadCutsceneBackground(interface_mode uMsg)
{
	const char *celPath;
	const char *palPath;

	switch (PickCutscene(uMsg)) {
	case CutStart:
		ArtCutsceneWidescreen = LoadPcxAsset("gendata\\cutstartw.pcx");
		celPath = "Gendata\\Cutstart.cel";
		palPath = "Gendata\\Cutstart.pal";
		progress_id = 1;
		break;
	case CutTown:
		celPath = "Gendata\\Cuttt.cel";
		palPath = "Gendata\\Cuttt.pal";
		progress_id = 1;
		break;
	case CutLevel1:
		celPath = "Gendata\\Cutl1d.cel";
		palPath = "Gendata\\Cutl1d.pal";
		progress_id = 0;
		break;
	case CutLevel2:
		celPath = "Gendata\\Cut2.cel";
		palPath = "Gendata\\Cut2.pal";
		progress_id = 2;
		break;
	case CutLevel3:
		celPath = "Gendata\\Cut3.cel";
		palPath = "Gendata\\Cut3.pal";
		progress_id = 1;
		break;
	case CutLevel4:
		celPath = "Gendata\\Cut4.cel";
		palPath = "Gendata\\Cut4.pal";
		progress_id = 1;
		break;
	case CutLevel5:
		celPath = "Nlevels\\Cutl5.cel";
		palPath = "Nlevels\\Cutl5.pal";
		progress_id = 1;
		break;
	case CutLevel6:
		celPath = "Nlevels\\Cutl6.cel";
		palPath = "Nlevels\\Cutl6.pal";
		progress_id = 1;
		break;
	case CutPortal:
		ArtCutsceneWidescreen = LoadPcxAsset("gendata\\cutportlw.pcx");
		celPath = "Gendata\\Cutportl.cel";
		palPath = "Gendata\\Cutportl.pal";
		progress_id = 1;
		break;
	case CutPortalRed:
		ArtCutsceneWidescreen = LoadPcxAsset("gendata\\cutportrw.pcx");
		celPath = "Gendata\\Cutportr.cel";
		palPath = "Gendata\\Cutportr.pal";
		progress_id = 1;
		break;
	case CutGate:
		celPath = "Gendata\\Cutgate.cel";
		palPath = "Gendata\\Cutgate.pal";
		progress_id = 1;
		break;
	}

	assert(!sgpBackCel);
	sgpBackCel = LoadCel(celPath, 640);
	LoadPalette(palPath);

	sgdwProgress = 0;
}

void FreeCutsceneBackground()
{
	sgpBackCel = std::nullopt;
	ArtCutsceneWidescreen = std::nullopt;
}

void DrawCutsceneBackground()
{
	const Rectangle &uiRectangle = GetUIRectangle();
	const Surface &out = GlobalBackBuffer();
	if (ArtCutsceneWidescreen) {
		const PcxSprite sprite { *ArtCutsceneWidescreen };
		RenderPcxSprite(out, sprite, { uiRectangle.position.x - (sprite.width() - uiRectangle.size.width) / 2, uiRectangle.position.y });
	}
	CelDrawTo(out, { uiRectangle.position.x, 480 - 1 + uiRectangle.position.y }, CelSprite { *sgpBackCel }, 0);
}

void DrawCutsceneForeground()
{
	const Rectangle &uiRectangle = GetUIRectangle();
	const Surface &out = GlobalBackBuffer();
	constexpr int ProgressHeight = 22;
	SDL_Rect rect = MakeSdlRect(
	    out.region.x + BarPos[progress_id][0] + uiRectangle.position.x,
	    out.region.y + BarPos[progress_id][1] + uiRectangle.position.y,
	    sgdwProgress,
	    ProgressHeight);
	SDL_FillRect(out.surface, &rect, BarColor[progress_id]);

	if (DiabloUiSurface() == PalSurface)
		BltFast(&rect, &rect);
	RenderPresent();
}

} // namespace

void interface_msg_pump()
{
	tagMSG msg;

	while (FetchMessage(&msg)) {
		if (msg.message != DVL_WM_QUIT) {
			TranslateMessage(&msg);
			PushMessage(&msg);
		}
	}
}

bool IncProgress()
{
	interface_msg_pump();
	sgdwProgress += 23;
	if (sgdwProgress > 534)
		sgdwProgress = 534;
	DrawCutsceneForeground();
	return sgdwProgress >= 534;
}

void ShowProgress(interface_mode uMsg)
{
	WNDPROC saveProc;

	gbSomebodyWonGameKludge = false;
	plrmsg_delay(true);

	assert(ghMainWnd);
	saveProc = SetWindowProc(DisableInputWndProc);

	interface_msg_pump();
	ClearScreenBuffer();
	scrollrt_draw_game_screen();
	BlackPalette();

	// Blit the background once and then free it.
	LoadCutsceneBackground(uMsg);
	DrawCutsceneBackground();
	if (RenderDirectlyToOutputSurface && IsDoubleBuffered()) {
		// Blit twice for triple buffering.
		for (unsigned i = 0; i < 2; ++i) {
			if (DiabloUiSurface() == PalSurface)
				BltFast(nullptr, nullptr);
			RenderPresent();
			DrawCutsceneBackground();
		}
	}
	FreeCutsceneBackground();

	if (IsHardwareCursor())
		SetHardwareCursorVisible(false);

	PaletteFadeIn(8);
	IncProgress();
	sound_init();
	IncProgress();

	Player &myPlayer = *MyPlayer;

	switch (uMsg) {
	case WM_DIABLOADGAME:
		IncProgress();
		IncProgress();
		LoadGame(true);
		IncProgress();
		IncProgress();
		break;
	case WM_DIABNEWGAME:
		myPlayer.pOriginalCathedral = !gbIsHellfire;
		IncProgress();
		FreeGameMem();
		IncProgress();
		pfile_remove_temp_files();
		IncProgress();
		LoadGameLevel(true, ENTRY_MAIN);
		IncProgress();
		break;
	case WM_DIABNEXTLVL:
		IncProgress();
		if (!gbIsMultiplayer) {
			pfile_save_level();
		} else {
			DeltaSaveLevel();
		}
		IncProgress();
		FreeGameMem();
		setlevel = false;
		currlevel = myPlayer.plrlevel;
		leveltype = GetLevelType(currlevel);
		IncProgress();
		LoadGameLevel(false, ENTRY_MAIN);
		IncProgress();
		break;
	case WM_DIABPREVLVL:
		IncProgress();
		if (!gbIsMultiplayer) {
			pfile_save_level();
		} else {
			DeltaSaveLevel();
		}
		IncProgress();
		FreeGameMem();
		currlevel--;
		leveltype = GetLevelType(currlevel);
		assert(myPlayer.isOnActiveLevel());
		IncProgress();
		LoadGameLevel(false, ENTRY_PREV);
		IncProgress();
		break;
	case WM_DIABSETLVL:
		SetReturnLvlPos();
		IncProgress();
		if (!gbIsMultiplayer) {
			pfile_save_level();
		} else {
			DeltaSaveLevel();
		}
		IncProgress();
		setlevel = true;
		leveltype = setlvltype;
		FreeGameMem();
		IncProgress();
		LoadGameLevel(false, ENTRY_SETLVL);
		IncProgress();
		break;
	case WM_DIABRTNLVL:
		IncProgress();
		if (!gbIsMultiplayer) {
			pfile_save_level();
		} else {
			DeltaSaveLevel();
		}
		IncProgress();
		setlevel = false;
		FreeGameMem();
		IncProgress();
		GetReturnLvlPos();
		LoadGameLevel(false, ENTRY_RTNLVL);
		IncProgress();
		break;
	case WM_DIABWARPLVL:
		IncProgress();
		if (!gbIsMultiplayer) {
			pfile_save_level();
		} else {
			DeltaSaveLevel();
		}
		IncProgress();
		FreeGameMem();
		GetPortalLevel();
		IncProgress();
		LoadGameLevel(false, ENTRY_WARPLVL);
		IncProgress();
		break;
	case WM_DIABTOWNWARP:
		IncProgress();
		if (!gbIsMultiplayer) {
			pfile_save_level();
		} else {
			DeltaSaveLevel();
		}
		IncProgress();
		FreeGameMem();
		setlevel = false;
		currlevel = myPlayer.plrlevel;
		leveltype = GetLevelType(currlevel);
		IncProgress();
		LoadGameLevel(false, ENTRY_TWARPDN);
		IncProgress();
		break;
	case WM_DIABTWARPUP:
		IncProgress();
		if (!gbIsMultiplayer) {
			pfile_save_level();
		} else {
			DeltaSaveLevel();
		}
		IncProgress();
		FreeGameMem();
		currlevel = myPlayer.plrlevel;
		leveltype = GetLevelType(currlevel);
		IncProgress();
		LoadGameLevel(false, ENTRY_TWARPUP);
		IncProgress();
		break;
	case WM_DIABRETOWN:
		IncProgress();
		if (!gbIsMultiplayer) {
			pfile_save_level();
		} else {
			DeltaSaveLevel();
		}
		IncProgress();
		FreeGameMem();
		currlevel = myPlayer.plrlevel;
		leveltype = GetLevelType(currlevel);
		IncProgress();
		LoadGameLevel(false, ENTRY_MAIN);
		IncProgress();
		break;
	}

	assert(ghMainWnd);

	PaletteFadeOut(8);

	saveProc = SetWindowProc(saveProc);
	assert(saveProc == DisableInputWndProc);

	NetSendCmdLocParam2(true, CMD_PLAYER_JOINLEVEL, myPlayer.position.tile, myPlayer.plrlevel, myPlayer.plrIsOnSetLevel ? 1 : 0);
	plrmsg_delay(false);

	if (gbSomebodyWonGameKludge && myPlayer.isOnLevel(16)) {
		PrepDoEnding();
	}

	gbSomebodyWonGameKludge = false;
}

} // namespace devilution
