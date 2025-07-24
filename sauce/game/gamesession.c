#include "gamesession.h"

#include "../about.h"
#include "../constants.h"
#include "../deviceinfo.h"
#include "../storage.h"
#include "Form.h"
#include "drawhelper.h"
#include "gameActionLogic.h"
#include "hexgrid.h"
#include "level.h"
#include "mathIsFun.h"
#include "minimap.h"
#include "models.h"
#include "movement.h"
#include "pawn.h"
#include "pawnActionMenuViewModel.h"
#include "viewport.h"

#define WARPINITIALTIME 0.4

static void gameSession_resetHighlightTiles();
static void gameSession_moveCameraToPawn(Pawn *pawn);
static void gameSession_updateViewPortOffset(Boolean forceUpdateActivePawn);
static void gameSession_startTurn();

Faction gameSession_factionWithRandomizedCPUProfile() {
    Faction faction;
    faction.human = false;
    faction.profile.defendBasePriority = random(-70, 70);
    faction.profile.captureFlagPriority = random(-70, 70);
    faction.profile.attackPriority = random(-70, 70);
    return faction;
}

static void gameSession_loadStartMenu() {
    gameSession.menuScreenType = MENUSCREEN_START;
    gameSession.level = level_startLevel();
    gameSession.factions[0] = (Faction){(CPUFactionProfile){0, 0, 0}, true};
    gameSession.factionCount = 1;
    gameSession.factionTurn = 0;
    gameSession.activePawn = &gameSession.level.pawns[0];
    gameSession_updateViewPortOffset(true);
    gameActionLogic_scheduleMovement(gameSession.activePawn, NULL, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETLEFT, gameSession.activePawn->position.y});
    gameSession.drawingState.shouldRedrawBackground = true;
    gameSession.drawingState.shouldRedrawHeader = true;
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_launchGame(NewGameConfig config) {
    int faction;
    level_destroy(&gameSession.level);
    gameSession.menuScreenType = MENUSCREEN_GAME;
    gameSession.level = level_create(config);
    gameSession.factionCount = level_factionCount(config);
    // setup factions
    for (faction = 0; faction < gameSession.factionCount; faction++) {
        if (!config.playerConfig[faction].active) {
            continue;
        }
        if (config.playerConfig[faction].isHuman) {
            gameSession.factionTurn = faction;
            gameSession.factions[faction] = (Faction){(CPUFactionProfile){0, 0, 0}, true};
        } else {
            gameSession.factions[faction] = gameSession_factionWithRandomizedCPUProfile();
        }
    }
    gameSession.drawingState.shouldDrawButtons = gameSession.factions[gameSession.factionTurn].human;
    gameSession.drawingState.shouldRedrawBackground = true;
    gameSession.drawingState.shouldRedrawOverlay = true;
    gameSession.continueCPUPlay = !gameActionLogic_humanShipsLeft(&gameSession.level);
    gameSession_startTurn();
}

void gameSession_reset(Boolean newGame) {
    gameActionLogic_clearAttack();
    gameActionLogic_clearMovement();
    gameActionLogic_clearShockwave();
    gameSession.diaSupport = deviceinfo_diaSupported();
    gameSession.colorSupport = deviceinfo_colorSupported();

    gameSession.state = GAMESTATE_DEFAULT;
    MemSet(&gameSession.lastPenInput, sizeof(InputPen), 0);

    gameSession.level.pawns = NULL;
    gameSession.activePawn = NULL;
    gameSession.continueCPUPlay = false;
    gameSession.paused = false;
    gameSession.currentTurn = 0;

    gameSession.drawingState = (DrawingState){true, true, false, true, false, false, (Coordinate){0, 0}, (Coordinate){0, 0}};

    gameSession.highlightTiles = NULL;
    gameSession.highlightTileCount = 0;

    gameSession.attackAnimation = NULL;
    gameSession.movement = NULL;

    gameSession.targetSelectionType = TARGETSELECTIONTYPE_MOVE;

    gameSession.viewportOffset = (Coordinate){0, 0};
    level_destroy(&gameSession.level);

    if (newGame) {
        NewGameConfig config = level_defaultNewGameConfig(scoring_rankValue(scoring_loadSavedScore()));
        gameSession_launchGame(config);
    } else {
        if (storage_restoreGameState(&gameSession.currentTurn, &gameSession.level.pawnCount, &gameSession.factionCount, &gameSession.factionTurn, &gameSession.level.pawns, gameSession.factions, gameSession.level.scores)) {
            gameSession.menuScreenType = MENUSCREEN_GAME;
            gameSession.activePawn = &gameSession.level.pawns[0];
            gameSession_updateViewPortOffset(true);
            gameSession_startTurn();
        } else {
            gameSession_loadStartMenu();
        }
    }
}

void gameSession_cleanup() {
    if (gameSession.menuScreenType == MENUSCREEN_GAME) {
        storage_saveGameState(
            gameSession.currentTurn,
            gameSession.level.pawnCount,
            gameSession.factionCount,
            gameSession.factionTurn,
            gameSession.level.pawns,
            gameSession.factions,
            gameSession.level.scores);
    }
    level_destroy(&gameSession.level);
    gameActionLogic_clearAttack();
    gameActionLogic_clearMovement();
    gameActionLogic_clearShockwave();
    gameSession_resetHighlightTiles();
    spriteLibrary_clean();
}

void gameSession_registerPenInput(EventPtr eventptr) {
    if (gameSession.paused) {
        return;
    }
    inputPen_updateEventDetails(&gameSession.lastPenInput, eventptr);
}

static Coordinate gameSession_validViewportOffset(Coordinate position) {
    Coordinate newOffset;
    Coordinate screenSize = deviceinfo_screenSize();
    int gameWindowHeight = screenSize.y - BOTTOMMENU_HEIGHT;
    Coordinate gridSize = hexgrid_size();
    newOffset.x = fmin(gridSize.x - screenSize.x + 1, fmax(0, position.x - screenSize.x / 2));
    newOffset.y = fmin(gridSize.y - gameWindowHeight + 1, fmax(0, position.y - gameWindowHeight / 2));
    return newOffset;
}

static Boolean gameSession_isViewPortOffsetToPawn(Pawn *pawn) {
    if (pawn != NULL) {
        return (isEqualCoordinate(gameSession.viewportOffset, gameSession_validViewportOffset(hexgrid_tileCenterPosition(pawn->position))));
    }
    return false;
}

static void gameSession_updateViewPortOffset(Boolean forceUpdateActivePawn) {
    Coordinate position;
    if (gameSession.activePawn != NULL && forceUpdateActivePawn) {
        position = hexgrid_tileCenterPosition(gameSession.activePawn->position);
    } else if (gameSession.movement != NULL) {
        position = gameSession.movement->pawnPosition;
    } else {
        return;
    }
    gameSession.viewportOffset = gameSession_validViewportOffset(position);
}

static AppColor gameSession_hightlightTilesColor() {
    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            return EMERALD;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            return ALIZARIN;
    }
}

static void gameSession_updateValidPawnPositionsForMovement(Coordinate currentPosition, TargetSelectionType targetSelectionType) {
    int i, j;
    int maxTileRange = gameActionLogic_maxRange(targetSelectionType);
    Coordinate *coordinates = NULL;
    int coordinatesCount = 0;
    int maxIteration;
    AppColor color = gameSession_hightlightTilesColor();
    gameSession_resetHighlightTiles();
    switch (targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            coordinates = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.level.pawnCount);
            for (i = 0; i < gameSession.level.pawnCount; i++) {
                if (gameSession.level.pawns[i].type != PAWNTYPE_BASE && !isInvalidCoordinate(gameSession.level.pawns[i].position)) {
                    coordinates[coordinatesCount] = gameSession.level.pawns[i].position;
                    coordinatesCount++;
                }
            }
            MemPtrResize(coordinates, sizeof(Coordinate) * coordinatesCount);
            movement_findTilesInRange(currentPosition, maxTileRange, coordinates, coordinatesCount, &gameSession.highlightTiles, &gameSession.highlightTileCount, color, true);
            for (i = 0; i < gameSession.highlightTileCount; i++) {
                HighlightTile *tile = &gameSession.highlightTiles[i];
                for (j = 0; j < gameSession.level.pawnCount; j++) {
                    Pawn *pawnAtPosition = &gameSession.level.pawns[j];
                    if (pawnAtPosition->type == PAWNTYPE_SHIP && pawnAtPosition->faction != gameSession.activePawn->faction && movement_distance(pawnAtPosition->position, tile->position) <= GAMEMECHANICS_MAXTILETORPEDORANGE) {
                        tile->color = SUNFLOWER;
                    }
                }
            }
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            movement_findTilesInRange(currentPosition, maxTileRange, NULL, 0, &gameSession.highlightTiles, &gameSession.highlightTileCount, color, false);
            for (i = 0; i < gameSession.highlightTileCount; i++) {
                Pawn *pawnAtPosition = level_pawnAtTile(gameSession.highlightTiles[i].position, &gameSession.level);
                if (pawnAtPosition == NULL || pawnAtPosition->faction == gameSession.activePawn->faction) {
                    continue;
                }
                gameSession.highlightTiles[i].filled = true;
            }

            maxIteration = gameSession.highlightTileCount;
            // move the filled tiles to the end so they don't get overdrawn
            for (i = 0; i < maxIteration; i++) {
                if (gameSession.highlightTiles[i].filled) {
                    HighlightTile temp = gameSession.highlightTiles[i];
                    gameSession.highlightTiles[i] = gameSession.highlightTiles[gameSession.highlightTileCount - 1];
                    gameSession.highlightTiles[gameSession.highlightTileCount - 1] = temp;
                    maxIteration--;
                    i--;
                }
            }
            break;
    }

    if (coordinates != NULL) {
        MemPtrFree(coordinates);
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_showPawnActions() {
    int baseTurnsLeft;
    Boolean isCurrentPlayer = gameSession.factionTurn == gameSession.activePawn->faction && gameSession.factions[gameSession.activePawn->faction].human; 
    if (!isCurrentPlayer) {
        gameSession.drawingState.shouldRedrawOverlay = true;
        return;
    }
    if (gameSession.activePawn->turnComplete) {
        FrmCustomAlert(GAME_ALERT_NOMOREACTIONS, NULL, NULL, NULL);
        return;
    }
    baseTurnsLeft = pawn_baseTurnsLeft(gameSession.currentTurn, gameSession.activePawn->inventory.baseActionLastActionTurn, gameSession.activePawn->inventory.lastBaseAction);
    if (gameSession.activePawn->type == PAWNTYPE_BASE && gameSession.activePawn->inventory.lastBaseAction == BASEACTION_BUILD_SHIP && baseTurnsLeft > 0) {
        char baseTurnsLeftText[4];
        StrIToA(baseTurnsLeftText, baseTurnsLeft);
        FrmCustomAlert(GAME_ALERT_SHIPBUILDINPROGRESS, baseTurnsLeftText, NULL, NULL);
        return;
    }
    pawnActionMenuViewModel_setupMenuForPawn(gameSession.activePawn, &gameSession.displayButtons, &gameSession.displayButtonCount, gameSession.currentTurn);
    gameSession.drawingState.shouldRedrawOverlay = true;
    gameSession.state = GAMESTATE_CHOOSEPAWNACTION;
}

static void gameSession_enableActionsForFaction(int faction) {
    int i;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (gameSession.level.pawns[i].faction == faction && !isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            gameSession.level.pawns[i].turnComplete = false;
        }
    }
}

static int gameSession_nextAvailableFaction(int currentFaction) {
    int nextFaction = (currentFaction + 1) % gameSession.factionCount;
    while (!level_movesLeftForFaction(nextFaction, gameSession.currentTurn, &gameSession.level)) {
        nextFaction = (nextFaction + 1) % gameSession.factionCount;
    }
    return nextFaction;
}

static Pawn *gameSession_nextPawn(Boolean allPawns, Boolean onlyWithAvailableActions) {
    return level_nextPawn(gameSession.activePawn, allPawns, onlyWithAvailableActions, gameSession.factionTurn, gameSession.currentTurn, &gameSession.level);
}

static void gameSession_moveCameraToPawn(Pawn *pawn) {
    gameSession.cameraPawn = (Pawn){PAWNTYPE_SHIP, gameSession.activePawn->position, (Inventory){-1, 0, 0, BASEACTION_NONE, false}, 0, 0, false, false};
    gameActionLogic_scheduleMovement(&gameSession.cameraPawn, NULL, pawn->position);
}

static void gameSession_buildShip(Pawn *homeBase) {
    Coordinate oldActivePawnPosition = gameSession.activePawn->position;
    Coordinate closestTile = movement_closestTileToTargetInRange(homeBase, homeBase->position, gameSession.level.pawns, gameSession.level.pawnCount, false);
    level_addPawn(
        (Pawn){PAWNTYPE_SHIP, closestTile, (Inventory){GAMEMECHANICS_MAXSHIPHEALTH, 0, GAMEMECHANICS_MAXTORPEDOCOUNT, 0, BASEACTION_NONE, false}, 0, homeBase->faction, false, true},
        &gameSession.level);
    gameSession.activePawn = level_pawnAtTile(oldActivePawnPosition, &gameSession.level);
}

static void gameSession_startTurn() {
    Pawn *nextPawn;
    Pawn *homeBase;
    Coordinate oldActivePawnPosition = gameSession.activePawn->position;
    level_reorderPawnsByDistance(&gameSession.level);
    gameSession.activePawn = level_pawnAtTile(oldActivePawnPosition, &gameSession.level);
    gameSession.drawingState.shouldDrawButtons = gameSession.factions[gameSession.factionTurn].human;
    nextPawn = gameSession_nextPawn(false, true);
    homeBase = movement_homeBase(gameSession.factionTurn, gameSession.level.pawns, gameSession.level.pawnCount);
    if (homeBase != NULL && homeBase->inventory.lastBaseAction == BASEACTION_BUILD_SHIP && pawn_baseTurnsLeft(gameSession.currentTurn, homeBase->inventory.baseActionLastActionTurn, homeBase->inventory.lastBaseAction) == 0) {
        homeBase->inventory.lastBaseAction = BASEACTION_NONE;
        gameSession_buildShip(homeBase);
        nextPawn = movement_homeBase(gameSession.factionTurn, gameSession.level.pawns, gameSession.level.pawnCount);
        homeBase = nextPawn;
        if (gameSession.factions[homeBase->faction].human) {
            FrmCustomAlert(GAME_ALERT_SHIPBUILDFINISHED, NULL, NULL, NULL);
        }
    }
    gameSession_moveCameraToPawn(nextPawn);
    gameSession.activePawn = nextPawn;
    gameSession.lastPenInput.wasUpdatedFlag = false;
    gameSession.drawingState.shouldRedrawOverlay = true;
    gameSession.disableAutoMoveToNextPawn = false;
}

static void gameSession_startTurnForNextFaction() {
    gameSession_enableActionsForFaction(gameSession.factionTurn);
    gameSession.factionTurn = gameSession_nextAvailableFaction(gameSession.factionTurn);
    if (gameSession.factionTurn == 0) {
        gameSession.currentTurn++;
    }
    gameSession_startTurn();
}

static Boolean gameSession_handleBarButtonsTap() {
    if (gameSession.lastPenInput.touchCoordinate.x > gameSession.drawingState.barButtonPositions[0].x && gameSession.lastPenInput.touchCoordinate.y > gameSession.drawingState.barButtonPositions[0].y && gameSession.lastPenInput.touchCoordinate.y < gameSession.drawingState.barButtonPositions[0].y + gameSession.drawingState.barButtonHeight) {  // Next button
        gameSession.activePawn = gameSession_nextPawn(true, false);
        gameSession.disableAutoMoveToNextPawn = gameSession.activePawn->turnComplete;
        gameSession_updateViewPortOffset(true);
        gameSession.drawingState.shouldRedrawOverlay = true;
        inputPen_temporarylyBlockPenInput(&gameSession.lastPenInput);
        return true;
    } else if (gameSession.lastPenInput.touchCoordinate.x > gameSession.drawingState.barButtonPositions[1].x && gameSession.lastPenInput.touchCoordinate.y > gameSession.drawingState.barButtonPositions[1].y && gameSession.lastPenInput.touchCoordinate.y < gameSession.drawingState.barButtonPositions[1].y + gameSession.drawingState.barButtonHeight) {  // end turn button
        inputPen_temporarylyBlockPenInput(&gameSession.lastPenInput);
        gameSession_startTurnForNextFaction();
        return true;
    }
    return false;
}

DmResID gameSession_menuTopTitleResource() {
    switch (gameSession.menuScreenType) {
        case MENUSCREEN_START:
            return STRING_CAPTAINS;
        case MENUSCREEN_PLAYERCONFIG:
            return STRING_PLAYERCPU;
        case MENUSCREEN_SCORE:
            return STRING_LEVELSCORE;
        case MENUSCREEN_RANK:
        case MENUSCREEN_RANK_AFTERGAME:
            return scoring_rankForScore(scoring_loadSavedScore());
        case MENUSCREEN_GAME:
            return 0;
    }
    return 0;
}

Boolean gameSession_useValueForBottomTitle() {
    return gameSession.menuScreenType == MENUSCREEN_SCORE;
}

int gameSession_valueForBottomTitle() {
    return gameSession.menuScreenType == MENUSCREEN_SCORE ? scoring_levelScoreValue(gameSession.level.scores, gameSession.activePawn->faction) * GAMEMECHANICS_SCOREBOOST : -1;
}

DmResID gameSession_menuBottomTitleResource() {
    switch (gameSession.menuScreenType) {
        case MENUSCREEN_START:
            return STRING_STARGRID;
        case MENUSCREEN_PLAYERCONFIG:
            return STRING_CONFIG;
        case MENUSCREEN_RANK:
        case MENUSCREEN_RANK_AFTERGAME:
        case MENUSCREEN_SCORE:
        case MENUSCREEN_GAME:
            return 0;
    }
    return 0;
}

static Boolean gameSession_handleScoreMenuTap(Coordinate selectedTile) {
    int i;
    for (i = 0; i < gameSession.level.actionTileCount; i++) {
        if (isEqualCoordinate(gameSession.level.actionTiles[i].position, selectedTile)) {
            Pawn oldPawn;
            switch (gameSession.level.actionTiles[i].identifier) {
                case ACTIONTILEIDENTIFIER_HUMANPLAYER:
                case ACTIONTILEIDENTIFIER_CPUPLAYER:
                case ACTIONTILEIDENTIFIER_LAUNCHGAME:
                case ACTIONTILEIDENTIFIER_TWOPLAYERS:
                case ACTIONTILEIDENTIFIER_THREEPLAYERS:
                case ACTIONTILEIDENTIFIER_FOURPLAYERS:
                    break;
                case ACTIONTILEIDENTIFIER_ENDGAME:
                    if (gameSession.menuScreenType == MENUSCREEN_RANK_AFTERGAME && FrmCustomAlert(GAME_ALERT_ENDOFGAME, NULL, NULL, NULL) == 0) {  // new game
                        gameSession_reset(true);
                    } else {
                        gameSession_reset(false);
                    }
                    return true;
                case ACTIONTILEIDENTIFIER_SHOWENDGAMEOPTIONS:
                    gameSession.drawingState.shouldRedrawHeader = true;
                    gameSession.menuScreenType = MENUSCREEN_RANK_AFTERGAME;

                    oldPawn = *gameSession.activePawn;
                    level_addRank(&gameSession.level, scoring_loadSavedScore());
                    gameSession.activePawn = level_pawnAtTile(oldPawn.position, &gameSession.level);
                    if (gameSession.activePawn == NULL) {
                        level_addPawn(oldPawn, &gameSession.level);
                        gameSession.activePawn = level_pawnAtTile(oldPawn.position, &gameSession.level);
                    }
                    gameActionLogic_scheduleMovement(gameSession.activePawn, NULL, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETRIGHT, 0});
                    return true;
            }
        }
    }
    return false;
}

static Boolean gameSession_handleStartMenuTap(Coordinate selectedTile) {
    int i;
    for (i = 0; i < gameSession.level.gridTextCount; i++) {
        if (gameSession.level.gridTexts[i].position.y == selectedTile.y) {
            switch (gameSession.level.gridTexts[i].textResource) {
                case STRING_NEW:
                    gameSession.level.gridTexts[i].alternateColor = true;
                    gameSession.drawingState.shouldRedrawOverlay = true;
                    gameSession.drawingState.shouldRedrawHeader = true;
                    gameSession.menuScreenType = MENUSCREEN_PLAYERCONFIG;
                    level_addPlayerConfigPawns(&gameSession.level, level_defaultNewGameConfig(0));
                    gameSession.activePawn = &gameSession.level.pawns[0];
                    gameActionLogic_scheduleMovement(gameSession.activePawn, NULL, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETRIGHT, gameSession.activePawn->position.y});
                    break;
                case STRING_ABOUT:
                    about_show();
                    break;
                case STRING_RANK:
                    gameSession.level.gridTexts[i].alternateColor = true;
                    gameSession.drawingState.shouldRedrawOverlay = true;
                    gameSession.drawingState.shouldRedrawHeader = true;
                    gameSession.menuScreenType = MENUSCREEN_RANK;
                    level_addRank(&gameSession.level, scoring_loadSavedScore());
                    gameSession.activePawn->type = PAWNTYPE_SHIP;
                    gameActionLogic_scheduleMovement(gameSession.activePawn, NULL, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETRIGHT, 0});
            }
            return true;
        }
    }
}

static Boolean gameSession_handlePlayerConfigTap(Coordinate selectedTile) {
    int i;
    for (i = 0; i < gameSession.level.actionTileCount; i++) {
        if (isEqualCoordinate(gameSession.level.actionTiles[i].position, selectedTile)) {
            NewGameConfig config = level_getNewGameConfig(&gameSession.level, level_defaultNewGameConfig(scoring_rankValue(scoring_loadSavedScore())));
            gameSession.level.actionTiles[i].selected = true;
            gameSession.drawingState.shouldRedrawOverlay = true;
            switch (gameSession.level.actionTiles[i].identifier) {
                case ACTIONTILEIDENTIFIER_HUMANPLAYER:
                    config.playerConfig[gameSession.level.actionTiles[i].tag].isHuman = true;
                    break;
                case ACTIONTILEIDENTIFIER_CPUPLAYER:
                    config.playerConfig[gameSession.level.actionTiles[i].tag].isHuman = false;
                    break;
                case ACTIONTILEIDENTIFIER_LAUNCHGAME:
                    gameSession_launchGame(config);
                    return true;
                    break;
                case ACTIONTILEIDENTIFIER_TWOPLAYERS:
                    config.playerConfig[2].active = false;
                    config.playerConfig[3].active = false;
                    break;
                case ACTIONTILEIDENTIFIER_THREEPLAYERS:
                    config.playerConfig[2].active = true;
                    config.playerConfig[3].active = false;
                    break;
                case ACTIONTILEIDENTIFIER_FOURPLAYERS:
                    config.playerConfig[2].active = true;
                    config.playerConfig[3].active = true;
                    break;
                case ACTIONTILEIDENTIFIER_SHOWENDGAMEOPTIONS:
                case ACTIONTILEIDENTIFIER_ENDGAME:
                    break;
            }
            level_applyNewGameConfig(config, &gameSession.level);
            return true;
        }
    }
}

static Boolean gameSession_handleNonGameMenuTap(Coordinate selectedTile) {
    selectedTile.y -= 2;  // offset for the top bar
    switch (gameSession.menuScreenType) {
        case MENUSCREEN_START:
            return gameSession_handleStartMenuTap(selectedTile);
            break;
        case MENUSCREEN_PLAYERCONFIG:
            return gameSession_handlePlayerConfigTap(selectedTile);
        case MENUSCREEN_GAME:
            break;
        case MENUSCREEN_RANK:
        case MENUSCREEN_RANK_AFTERGAME:
        case MENUSCREEN_SCORE:
            return gameSession_handleScoreMenuTap(selectedTile);
    }
    return false;
}

static Boolean gameSession_handleTileTap() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    Pawn *selectedPawn = level_pawnAtTile(selectedTile, &gameSession.level);
    if (gameSession.menuScreenType != MENUSCREEN_GAME) {
        return gameSession_handleNonGameMenuTap(selectedTile);
    }
    if (selectedPawn != NULL) {
        gameSession.activePawn = selectedPawn;
        gameSession_updateViewPortOffset(true);
        gameSession_showPawnActions();
        return true;
    }
    return false;
}

static Boolean gameSession_handleMiniMapTap() {
    Coordinate viewportOffset = minimap_viewportOffsetForTap(gameSession.lastPenInput.touchCoordinate, gameSession.drawingState.miniMapDrawPosition, gameSession.drawingState.miniMapSize);
    if (isInvalidCoordinate(viewportOffset)) {
        return false;
    }
    gameSession.viewportOffset = gameSession_validViewportOffset(viewportOffset);
    gameSession.drawingState.shouldRedrawOverlay = true;
    return true;
}

static Boolean gameSession_highlightTilesContains(Coordinate coordinate) {
    int i;
    for (i = 0; i < gameSession.highlightTileCount; i++) {
        if (isEqualCoordinate(gameSession.highlightTiles[i].position, coordinate)) {
            return true;
        }
    }
    return false;
}

static void gameSession_resetHighlightTiles() {
    if (gameSession.highlightTiles != NULL) {
        MemPtrFree(gameSession.highlightTiles);
        gameSession.highlightTiles = NULL;
        gameSession.highlightTileCount = 0;
    }
}

static void gameSession_handleTargetSelection() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    Pawn *selectedPawn = level_pawnAtTile(selectedTile, &gameSession.level);
    Boolean invalidTile = gameSession.targetSelectionType != TARGETSELECTIONTYPE_MOVE && selectedPawn == NULL;
    if (!gameSession_highlightTilesContains(selectedTile) || invalidTile) {
        gameSession_resetHighlightTiles();
        gameSession.state = GAMESTATE_DEFAULT;
        gameSession.drawingState.shouldRedrawOverlay = true;
        return;
    }

    gameSession.activePawn->turnComplete = true;
    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            gameActionLogic_scheduleMovement(gameSession.activePawn, selectedPawn, selectedTile);
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            gameActionLogic_scheduleAttack(selectedPawn, selectedTile, gameSession.targetSelectionType);
            break;
    }

    gameSession_resetHighlightTiles();
    gameSession.drawingState.shouldRedrawOverlay = true;
}

Boolean gameSession_shouldShowHealthBar() {
    return gameSession.state == GAMESTATE_SELECTTARGET &&
           (gameSession.targetSelectionType == TARGETSELECTIONTYPE_PHASER || gameSession.targetSelectionType == TARGETSELECTIONTYPE_TORPEDO) &&
           gameSession.movement == NULL;
}

static void gameSession_handlePawnActionButtonSelection() {
    int i;
    int selectedIndex = bottomMenu_selectedIndex(gameSession.lastPenInput.touchCoordinate);
    Pawn *homeBase;
    Coordinate closestTile;

    if (selectedIndex >= gameSession.displayButtonCount) {
        return;
    }

    if (gameSession.displayButtons[selectedIndex].disabled) {
        return;
    }

    inputPen_temporarylyBlockPenInput(&gameSession.lastPenInput);

    switch (pawnActionMenuViewModel_actionAtIndex(selectedIndex, gameSession.activePawn)) {
        case MenuActionTypeCancel:
            gameSession.state = GAMESTATE_DEFAULT;
            break;
        case MenuActionTypeWarp:
            gameSession.activePawn->turnComplete = true;
            homeBase = movement_homeBase(gameSession.activePawn->faction, gameSession.level.pawns, gameSession.level.pawnCount);
            gameSession.activePawn->warped = true;
            closestTile = movement_closestTileToTargetInRange(homeBase, homeBase->position, gameSession.level.pawns, gameSession.level.pawnCount, false);
            gameSession.state = GAMESTATE_DEFAULT;
            gameActionLogic_scheduleWarp(gameSession.activePawn, closestTile);
            gameSession.activePawn->position = closestTile;
            break;
        case MenuActionTypeTorpedo:
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_TORPEDO;
            break;
        case MenuActionTypePhaser:
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_PHASER;
            break;
        case MenuActionTypeMove:
            gameSession.state = GAMESTATE_SELECTTARGET;
            gameSession.targetSelectionType = TARGETSELECTIONTYPE_MOVE;
            break;
        case MenuActionTypeShockwave:
            gameSession.activePawn->turnComplete = true;
            gameActionLogic_scheduleShockwave(gameSession.activePawn);
            gameSession.activePawn->inventory.baseActionLastActionTurn = gameSession.currentTurn;
            gameSession.activePawn->inventory.lastBaseAction = BASEACTION_SHOCKWAVE;
            break;
        case MenuActionTypeBuildShip:
            gameSession.activePawn->turnComplete = true;
            gameSession.activePawn->inventory.baseActionLastActionTurn = gameSession.currentTurn;
            gameSession.activePawn->inventory.lastBaseAction = BASEACTION_BUILD_SHIP;
            gameSession.state = GAMESTATE_DEFAULT;
            break;
    }

    for (i = 0; i < gameSession.displayButtonCount; i++) {
        MemPtrFree(gameSession.displayButtons[i].text);
    }
    MemPtrFree(gameSession.displayButtons);
    gameSession.displayButtons = NULL;
    gameSession.displayButtonCount = 0;

    if (gameSession.state == GAMESTATE_SELECTTARGET) {
        gameSession_updateValidPawnPositionsForMovement(gameSession.activePawn->position, gameSession.targetSelectionType);
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_progressUpdateExplosion() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    gameSession.drawingState.shouldRedrawOverlay = true;
    timeSinceLaunch = TimGetTicks() - gameSession.attackAnimation->explosionTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * gameSession.attackAnimation->explosionDurationSeconds);
    if (timePassedScale >= 1) {
        gameActionLogic_clearAttack();
        gameActionLogic_afterExplosion();
        gameSession.state = GAMESTATE_DEFAULT;
    }
}

static void gameSession_progressUpdateAttack() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    Coordinate targetCenter, sourceCenter;
    Line attackLine;
    if (gameSession.attackAnimation == NULL) {
        return;
    }
    if (!isInvalidCoordinate(gameSession.attackAnimation->explosionPosition)) {
        gameSession_progressUpdateExplosion();
        return;
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
    timeSinceLaunch = TimGetTicks() - gameSession.attackAnimation->launchTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * gameSession.attackAnimation->durationSeconds);

    if (gameSession.attackAnimation->lines != NULL) {
        MemPtrFree(gameSession.attackAnimation->lines);
    }

    targetCenter = hexgrid_tileCenterPosition(gameSession.attackAnimation->target);
    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            break;
        case TARGETSELECTIONTYPE_PHASER:
            attackLine = (Line){hexgrid_tileCenterPosition(gameSession.activePawn->position), movement_getBoxCoordinate(targetCenter, timePassedScale, HEXTILE_PAWNSIZE / 3)};
            gameSession.attackAnimation->lines = (Line *)MemPtrNew(sizeof(Line) * 3);
            gameSession.attackAnimation->lines[0] = (Line){attackLine.startpoint, movement_coordinateAtPercentageOfLine(attackLine, remapToMax(timePassedScale * 2.4, 1))};
            attackLine.startpoint = gameSession.attackAnimation->lines[0].endpoint;
            gameSession.attackAnimation->lines[1] = (Line){gameSession.attackAnimation->lines[0].endpoint, movement_coordinateAtPercentageOfLine(attackLine, 0.7)};
            gameSession.attackAnimation->lines[2] = (Line){gameSession.attackAnimation->lines[1].endpoint, attackLine.endpoint};
            gameSession.attackAnimation->lineCount = 3;
            gameSession.attackAnimation->torpedoPosition = (Coordinate){-1, -1};
            break;
        case TARGETSELECTIONTYPE_TORPEDO:
            sourceCenter = hexgrid_tileCenterPosition(gameSession.activePawn->position);
            gameSession.attackAnimation->torpedoPosition = movement_coordinateAtPercentageOfLine((Line){sourceCenter.x, sourceCenter.y, targetCenter.x, targetCenter.y}, timePassedScale);
            break;
    }

    if (timePassedScale >= 1) {
        gameActionLogic_afterAttack();
        if (gameSession.targetSelectionType == TARGETSELECTIONTYPE_TORPEDO || gameSession.attackAnimation->targetPawn == NULL) {  // show explosion when destroyed or always when torpedo is used
            gameSession.attackAnimation->explosionPosition = targetCenter;
            gameSession.attackAnimation->explosionTimestamp = TimGetTicks();
            gameSession.attackAnimation->explosionDurationSeconds = 0.5;
        } else {
            gameActionLogic_clearAttack();
            gameSession.state = GAMESTATE_DEFAULT;
        }
    }
}

static void gameSession_progressUpdateShockWave() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    int i;
    if (gameSession.shockWaveAnimation == NULL) {
        return;
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
    timeSinceLaunch = TimGetTicks() - gameSession.shockWaveAnimation->launchTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * 1.7);
    if (timePassedScale > 0.2) {
        for (i = 0; i < gameSession.shockWaveAnimation->affectedPawnCount; i++) {
            int nextOrientation = ((i + (int)(timePassedScale * 2 * GFX_FRAMECOUNT_SHIPA)) % GFX_FRAMECOUNT_SHIPA);
            if (i % 2 == 0) {
                nextOrientation = GFX_FRAMECOUNT_SHIPA - 1 - nextOrientation;
            }
            gameSession.level.pawns[gameSession.shockWaveAnimation->affectedPawnIndices[i]].orientation = nextOrientation;
        }
    }

    for (i = 0; i < gameSession.shockWaveAnimation->affectedPawnCount; i++) {
        Coordinate pawnOriginalPosition = hexgrid_tileCenterPosition(gameSession.shockWaveAnimation->pawnOriginalPositions[i]);
        Pawn *pawn = &gameSession.level.pawns[gameSession.shockWaveAnimation->affectedPawnIndices[i]];
        gameSession.shockWaveAnimation->pawnIntermediatePositions[i] = movement_coordinateAtPercentageOfLine((Line){pawnOriginalPosition, hexgrid_tileCenterPosition(pawn->position)}, timePassedScale);
    }

    for (i = 0; i < WARPCIRCLECOUNT; i++) {
        gameSession.shockWaveAnimation->circleDiameter[i] = (float)(timePassedScale) * (float)(WARPCIRCLECOUNT - i) * 30.0;
    }
    gameSession.shockWaveAnimation->maskCircleDiameter = timePassedScale * timePassedScale * WARPCIRCLECOUNT * 30;

    if (timePassedScale >= 1) {
        gameActionLogic_clearShockwave();
        gameSession.state = GAMESTATE_DEFAULT;
    }
}

static void gameSession_progressUpdateWarp() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    int i;
    if (gameSession.warpAnimation.isWarping) {
        gameSession.drawingState.shouldRedrawOverlay = true;
        timeSinceLaunch = TimGetTicks() - gameSession.warpAnimation.launchTimestamp;
        timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * 1.2);
        gameSession.warpAnimation.pawn->orientation = (int)(timePassedScale * 1.5 * GFX_FRAMECOUNT_SHIPA) % GFX_FRAMECOUNT_SHIPA;
        gameSession.warpAnimation.shipVisible = timePassedScale < WARPINITIALTIME || timePassedScale > WARPINITIALTIME + 0.4;
        for (i = 0; i < WARPCIRCLECOUNT; i++) {
            if (timePassedScale < WARPINITIALTIME) {
                gameSession.warpAnimation.circleDiameter[i] = (1 - timePassedScale / 2) * (WARPCIRCLECOUNT - i) * 4;
            } else {
                gameSession.warpAnimation.circleDiameter[i] = (timePassedScale - WARPINITIALTIME) * (WARPCIRCLECOUNT - i) * 6;
                gameSession.warpAnimation.currentPosition = gameSession.warpAnimation.endPosition;
            }
        }
        if (timePassedScale > WARPINITIALTIME) {
            gameSession_updateViewPortOffset(true);
        }
        if (timePassedScale >= 1) {
            gameSession.warpAnimation.isWarping = false;
            gameSession.state = GAMESTATE_DEFAULT;
        }
    }
}

static void gameSession_progressUpdateMovement() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    float totalAnimationTime = 1.7;
    if (gameSession.movement == NULL) {
        return;
    }
    if (gameSession.menuScreenType != MENUSCREEN_GAME || gameSession.continueCPUPlay) {
        totalAnimationTime = 2.8;
    }
    gameSession.drawingState.shouldRedrawOverlay = true;

    timeSinceLaunch = TimGetTicks() - gameSession.movement->launchTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * ((float)gameSession.movement->trajectory.tileCount - 1) / totalAnimationTime);
    gameSession.movement->pawnPosition = movement_coordinateAtPercentageOfTrajectory(gameSession.movement->trajectory, timePassedScale, &gameSession.movement->pawn->orientation);
    gameSession_updateViewPortOffset(false);

    if (timePassedScale >= 1) {
        if (!gameActionLogic_afterMove()) {
            gameSession_updateViewPortOffset(true);
        }
    }
}

Boolean gameSession_animating() {
    return gameSession.attackAnimation != NULL || gameSession.movement != NULL || gameSession.warpAnimation.isWarping || gameSession.shockWaveAnimation != NULL;
}

static void gameSession_cpuTurn() {
    UInt16 textId;
    char *text;
    MemHandle resourceHandle;
    Coordinate closestTile;
    Pawn *targetPawn;
    CPUStrategyResult strategy;
    Pawn *homeBase;
    Pawn *pawn = gameSession.activePawn;
    if (gameSession_animating() || gameSession.state != GAMESTATE_DEFAULT || gameSession.menuScreenType != MENUSCREEN_GAME || gameSession.factions[gameSession.factionTurn].human || pawn->turnComplete) {
        return;
    }

    strategy = cpuLogic_getStrategy(pawn, gameSession.level.pawns, gameSession.level.pawnCount, gameSession.currentTurn, gameSession.factions[pawn->faction].profile, !gameActionLogic_humanShipsLeft());
    pawn->turnComplete = true;
    switch (strategy.CPUAction) {
        case CPUACTION_MOVE:
            closestTile = strategy.targetPosition;
            if (isEqualCoordinate(closestTile, strategy.target->position)) {
                targetPawn = strategy.target;
            } else {
                targetPawn = NULL;
            }
            gameActionLogic_scheduleMovement(gameSession.activePawn, targetPawn, closestTile);
            textId = STRING_MOVING;
            break;
        case CPUACTION_PHASERATTACK:
        case CPUACTION_TORPEDOATTACK:
            gameActionLogic_scheduleAttack(strategy.target, strategy.target->position, strategy.CPUAction == CPUACTION_PHASERATTACK ? TARGETSELECTIONTYPE_PHASER : TARGETSELECTIONTYPE_TORPEDO);
            textId = STRING_ATTACKING;
            break;
        case CPUACTION_WARP:
            homeBase = movement_homeBase(pawn->faction, gameSession.level.pawns, gameSession.level.pawnCount);
            pawn->warped = true;
            closestTile = movement_closestTileToTargetInRange(homeBase, homeBase->position, gameSession.level.pawns, gameSession.level.pawnCount, false);
            textId = STRING_WARP;
            gameActionLogic_scheduleWarp(pawn, closestTile);
            pawn->position = closestTile;
            break;
        case CPUACTION_BASE_SHOCKWAVE:
            gameActionLogic_scheduleShockwave(pawn);
            pawn->inventory.baseActionLastActionTurn = gameSession.currentTurn;
            pawn->inventory.lastBaseAction = BASEACTION_SHOCKWAVE;
            textId = STRING_SHOCKWAVE;
            break;
        case CPUACTION_BASE_BUILDSHIP:
            pawn->inventory.baseActionLastActionTurn = gameSession.currentTurn;
            pawn->inventory.lastBaseAction = BASEACTION_BUILD_SHIP;
            textId = STRING_BUILDSHIP;
            gameSession.drawingState.requiresPauseAfterLayout = true;
            break;
        case CPUACTION_NONE:
            textId = STRING_NOACTION;
            gameSession.drawingState.requiresPauseAfterLayout = pawn->type == PAWNTYPE_SHIP;
            break;
    }
    resourceHandle = DmGetResource(strRsc, textId);
    text = (char *)MemHandleLock(resourceHandle);
    StrCopy(gameSession.cpuActionText, text);
    MemHandleUnlock(resourceHandle);
    DmReleaseResource(resourceHandle);
}

Boolean gameSession_handleMenu(UInt16 menuItemID) {
    switch (menuItemID) {
        case GAME_MENUITEM_EXIT:
            gameSession_reset(false);
            return true;
        case GAME_MENUITEM_RESETRANK:
            if (FrmCustomAlert(GAME_ALERT_RESETRANKCONFIRMATION, NULL, NULL, NULL) == 0) {
                scoring_reset();
                gameSession_reset(false);
            }
            return true;
        case GAME_MENUITEM_ABOUT:
            about_show();
            return true;
    }
    return false;
}

Boolean gameSession_handleFormButtonTap(UInt16 buttonID) {
    return about_buttonHandler(buttonID);
}

static Boolean moveToNextPawnIfNeeded() {
#ifdef DEBUG
    int i;
#endif
    Pawn *pawn = gameSession.activePawn;
    if (!pawn->turnComplete || gameSession.disableAutoMoveToNextPawn || gameSession.menuScreenType != MENUSCREEN_GAME) {
        return false;
    }

    while (pawn->turnComplete) {
#ifdef DEBUG
        drawhelper_drawTextWithValue("", i++, (Coordinate){140, 0});
#endif
        pawn = gameSession_nextPawn(false, true);
        if (pawn == NULL || !level_movesLeftForFaction(gameSession.factionTurn, gameSession.currentTurn, &gameSession.level)) {
            gameSession_startTurnForNextFaction();
            return true;
        }
    }

    // move camera to active pawn
    if (!gameSession_isViewPortOffsetToPawn(pawn)) {
        gameSession_moveCameraToPawn(pawn);
        gameSession.activePawn = pawn;
    } else {
        gameSession.activePawn = pawn;
        gameSession.drawingState.shouldRedrawOverlay = true;
    }

    return true;
}

void gameSession_progressLogic() {
    if (gameSession_animating()) {
        // disregard all input while animating
        gameSession.lastPenInput.wasUpdatedFlag = false;
    } else {
        if (moveToNextPawnIfNeeded()) {
            return;
        }
        if (!gameSession.factions[gameSession.factionTurn].human) {
            gameSession_cpuTurn();
        } else {
            if (gameSession.drawingState.awaitingEndMiniMapScrolling && gameSession.lastPenInput.penUpOccured) {
                gameSession.lastPenInput.penUpOccured = false;
                gameSession.drawingState.awaitingEndMiniMapScrolling = false;
                gameSession.drawingState.shouldRedrawOverlay = true;
                return;
            }
            gameSession.lastPenInput.penUpOccured = false;
            if (gameSession.lastPenInput.wasUpdatedFlag) {  // handle user actions
                // Handle pen input
                gameSession.lastPenInput.wasUpdatedFlag = false;
#ifdef DEBUG
                drawhelper_drawTextWithValue("X:", gameSession.lastPenInput.touchCoordinate.x, (Coordinate){gameSession.lastPenInput.touchCoordinate.x, gameSession.lastPenInput.touchCoordinate.y});
                drawhelper_drawTextWithValue("Y:", gameSession.lastPenInput.touchCoordinate.y, (Coordinate){gameSession.lastPenInput.touchCoordinate.x, gameSession.lastPenInput.touchCoordinate.y + 10});
#endif
                if (gameSession.lastPenInput.moving && gameSession.state == GAMESTATE_DEFAULT && gameSession.menuScreenType == MENUSCREEN_GAME && gameSession_handleMiniMapTap()) {
                    gameSession.drawingState.awaitingEndMiniMapScrolling = true;
                } else {
                    if ((gameSession.state != GAMESTATE_DEFAULT && gameSession.lastPenInput.moving || isInvalidCoordinate(gameSession.lastPenInput.touchCoordinate))) {
                        // note for later: if this gives unresponsive tap issues, only process the initial tap (when gamestate not default) and disregard all further taps until pen up
                        gameSession.lastPenInput.wasUpdatedFlag = false;
                        return;
                    }

                    switch (gameSession.state) {
                        case GAMESTATE_DEFAULT:
                            if (gameSession_handleMiniMapTap()) break;
                            if (gameSession_handleTileTap()) return;
                            if (gameSession_handleBarButtonsTap()) break;
                            break;
                        case GAMESTATE_CHOOSEPAWNACTION:
                            gameSession_handlePawnActionButtonSelection();
                            break;
                        case GAMESTATE_SELECTTARGET:
                            gameSession_handleTargetSelection();
                            break;
                    }
                }
            }
        }
    }

    gameSession_progressUpdateMovement();
    gameSession_progressUpdateAttack();
    gameSession_progressUpdateWarp();
    gameSession_progressUpdateShockWave();
}
