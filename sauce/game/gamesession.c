#include "gamesession.h"

#include "../about.h"
#include "../constants.h"
#include "../deviceinfo.h"
#include "drawhelper.h"
#include "gameActionLogic.h"
#include "hexgrid.h"
#include "mathIsFun.h"
#include "minimap.h"
#include "movement.h"
#include "pawnActionMenuViewModel.h"
#include "viewport.h"

#define WARPINITIALTIME 0.4

static void gameSession_resetHighlightTiles();
static void gameSession_moveCameraToPawn(Pawn *pawn);
static void gameSession_updateViewPortOffset(Boolean forceUpdateActivePawn);
static Boolean gameSession_restoreGameState();
static void gameSession_clearSavedGameState();
static void gameSession_startTurn();

Faction gameSession_factionWithRandomizedCPUProfile() {
    Faction faction;
    faction.human = false;
    faction.profile.defendBasePriority = random(-70, 70);
    faction.profile.captureFlagPriority = random(-70, 70);
    faction.profile.attackPriority = random(-70, 70);
    return faction;
}

static NewGameConfig gameSession_defaultNewGameConfig() {
    NewGameConfig config;
    int i;
    for (i = 0; i < MAXPLAYERCOUNT; i++) {
        config.playerConfig[i].active = true;
        config.playerConfig[i].isHuman = i == 0;
    }
    config.placementStrategy = PLAYERPLACEMENTSTRATEGY_CORNERS;
    config.shipCount = 2;
    return config;
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
    gameSession.secondaryHighlightTiles = NULL;
    gameSession.secondaryHighlightTileCount = 0;

    gameSession.attackAnimation = NULL;
    gameSession.movement = NULL;

    gameSession.targetSelectionType = TARGETSELECTIONTYPE_MOVE;

    gameSession.viewportOffset = (Coordinate){0, 0};
    level_destroy(&gameSession.level);

    if (newGame) {
        gameSession_loadStartMenu();  // TODO: implement new game logic
    } else {
        if (gameSession_restoreGameState()) {
            gameSession.menuScreenType = MENUSCREEN_GAME;
            gameSession.activePawn = &gameSession.level.pawns[0];
            gameSession_updateViewPortOffset(true);
            gameSession_startTurn();
        } else {
            gameSession_loadStartMenu();
        }
    }
}

static void gameSession_clearSavedGameState() {
    FtrUnregister(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA);
    FtrUnregister(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS);
}

static void gameSession_saveGameState() {
    void *sessionDataPtr;
    void *pawnDataPtr;
    int i;
    GameRestorableSessionData sessionData;
    gameSession_clearSavedGameState();
    if (gameSession.menuScreenType != MENUSCREEN_GAME) {
        return;
    }
    sessionData.currentTurn = gameSession.currentTurn;
    sessionData.pawnCount = gameSession.level.pawnCount;
    sessionData.factionCount = gameSession.factionCount;
    sessionData.factionTurn = gameSession.factionTurn;
    for (i = 0; i < MAXPLAYERCOUNT; i++) {
        sessionData.factions[i] = gameSession.factions[i];
    }

    FtrPtrNew(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA, sizeof(GameRestorableSessionData), &sessionDataPtr);
    FtrPtrNew(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS, sizeof(Pawn) * gameSession.level.pawnCount, &pawnDataPtr);
    DmWrite(sessionDataPtr, 0, &sessionData, sizeof(GameRestorableSessionData));
    DmWrite(pawnDataPtr, 0, gameSession.level.pawns, sizeof(Pawn) * gameSession.level.pawnCount);
}

static Boolean gameSession_restoreGameState() {
    UInt32 sessionDataPtr, pawnDataPtr;
    GameRestorableSessionData *sessionData;
    Pawn *pawnData;
    int i;
    if (FtrGet(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_SESSIONDATA, &sessionDataPtr) != errNone || FtrGet(APP_CREATOR_ID, FEATUREMEM_SAVESTATE_PAWNS, &pawnDataPtr) != errNone) {
        gameSession_clearSavedGameState();
        return false;
    }

    sessionData = (GameRestorableSessionData *)sessionDataPtr;
    pawnData = (Pawn *)pawnDataPtr;

    gameSession.currentTurn = sessionData->currentTurn;
    gameSession.factionCount = sessionData->factionCount;
    gameSession.factionTurn = sessionData->factionTurn;
    for (i = 0; i < MAXPLAYERCOUNT; i++) {
        gameSession.factions[i] = sessionData->factions[i];
    }

    gameSession.level.pawnCount = sessionData->pawnCount;
    gameSession.level.pawns = (Pawn *)MemPtrNew(sizeof(Pawn) * sessionData->pawnCount);
    MemSet(gameSession.level.pawns, sizeof(Pawn) * sessionData->pawnCount, 0);
    for (i = 0; i < sessionData->pawnCount; i++) {
        MemMove(&gameSession.level.pawns[i], &pawnData[i], sizeof(Pawn));
    }
    gameSession_clearSavedGameState();
    return true;
}

void gameSession_cleanup() {
    gameSession_saveGameState();
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

static Pawn *gameSession_pawnAtTile(Coordinate tile) {
    int i;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (gameSession.level.pawns[i].position.x == tile.x && gameSession.level.pawns[i].position.y == tile.y && !isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            return &gameSession.level.pawns[i];
        }
    }
    return NULL;
}

static void gameSession_updateValidPawnPositionsForMovement(Coordinate currentPosition, TargetSelectionType targetSelectionType) {
    int i;
    int maxTileRange = gameActionLogic_maxRange(targetSelectionType);
    Coordinate *coordinates = NULL;
    int coordinatesCount = 0;
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
            movement_findTilesInRange(currentPosition, maxTileRange, coordinates, coordinatesCount, &gameSession.highlightTiles, &gameSession.highlightTileCount);
            break;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            movement_findTilesInRange(currentPosition, maxTileRange, NULL, 0, &gameSession.secondaryHighlightTiles, &gameSession.secondaryHighlightTileCount);
            gameSession.highlightTiles = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.secondaryHighlightTileCount);
            for (i = 0; i < gameSession.secondaryHighlightTileCount; i++) {
                Pawn *pawnAtPosition = gameSession_pawnAtTile(gameSession.secondaryHighlightTiles[i]);
                if (pawnAtPosition == NULL || pawnAtPosition->faction == gameSession.activePawn->faction) {
                    continue;
                }
                gameSession.highlightTiles[coordinatesCount] = pawnAtPosition->position;
                coordinatesCount++;
            }
            coordinatesCount++;
            gameSession.highlightTileCount = coordinatesCount;
            MemPtrResize(gameSession.highlightTiles, sizeof(Coordinate) * coordinatesCount);
            break;
    }

    if (coordinates != NULL) {
        MemPtrFree(coordinates);
    }
    gameSession.drawingState.shouldRedrawOverlay = true;
}

static void gameSession_showPawnActions() {
    if (!gameSession.factions[gameSession.activePawn->faction].human) {
        gameSession.drawingState.shouldRedrawOverlay = true;
        return;
    }
    if (gameSession.activePawn->turnComplete) {
        FrmCustomAlert(GAME_ALERT_NOMOREACTIONS, NULL, NULL, NULL);
        return;
    }
    pawnActionMenuViewModel_setupMenuForPawn(gameSession.activePawn, &gameSession.displayButtons, &gameSession.displayButtonCount, gameSession.currentTurn);
    gameSession.drawingState.shouldRedrawOverlay = true;
    gameSession.state = GAMESTATE_CHOOSEPAWNACTION;
}

static void gameSession_enableActionsForFaction(int faction) {
    int i;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (gameSession.level.pawns[i].faction == faction && gameSession.level.pawns[i].type == PAWNTYPE_SHIP && !isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            gameSession.level.pawns[i].turnComplete = false;
        }
    }
}

static Boolean gameSession_movesLeftForFaction(int faction) {
    int i;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (gameSession.level.pawns[i].faction == faction && gameSession.level.pawns[i].type == PAWNTYPE_SHIP && !gameSession.level.pawns[i].turnComplete && !isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            return true;
        }
    }
    return false;
}

static int gameSession_nextAvailableFaction(int currentFaction) {
    int nextFaction = (currentFaction + 1) % gameSession.factionCount;
    while (!gameSession_movesLeftForFaction(nextFaction)) {
        nextFaction = (nextFaction + 1) % gameSession.factionCount;
    }
    return nextFaction;
}

static Pawn *gameSession_nextPawn() {
    Pawn *firstPawn = NULL;
    Pawn *currentPawn = gameSession.activePawn->faction == gameSession.factionTurn ? gameSession.activePawn : NULL;
    int i;
    int startMatching = false;
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        if (gameSession.level.pawns[i].faction == gameSession.factionTurn && gameSession.level.pawns[i].type == PAWNTYPE_SHIP && !isInvalidCoordinate(gameSession.level.pawns[i].position)) {
            if (startMatching) {
                return &gameSession.level.pawns[i];
            }
            if (firstPawn == NULL) {
                firstPawn = &gameSession.level.pawns[i];
            }
            if (currentPawn == &gameSession.level.pawns[i]) {
                startMatching = true;
            }
        }
    }
    return firstPawn;
}

static void gameSession_moveCameraToPawn(Pawn *pawn) {
    gameSession.cameraPawn = (Pawn){PAWNTYPE_SHIP, gameSession.activePawn->position, (Inventory){-1, 0, 0, false}, 0, 0, false, false};
    gameActionLogic_scheduleMovement(&gameSession.cameraPawn, NULL, pawn->position);
}

static void gameSession_startTurn() {
    Pawn *nextPawn;
    gameSession.drawingState.shouldDrawButtons = gameSession.factions[gameSession.factionTurn].human;
    nextPawn = gameSession_nextPawn();
    gameSession_moveCameraToPawn(nextPawn);
    gameSession.activePawn = nextPawn;
    gameSession.lastPenInput.wasUpdatedFlag = false;
    gameSession.drawingState.shouldRedrawOverlay = true;
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
        gameSession.activePawn = gameSession_nextPawn();
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
        case MENUSCREEN_GAME:
            return 0;
    }
    return 0;
}

DmResID gameSession_menuBottomTitleResource() {
    switch (gameSession.menuScreenType) {
        case MENUSCREEN_START:
            return STRING_STARGRID;
        case MENUSCREEN_PLAYERCONFIG:
            return STRING_CONFIG;
        case MENUSCREEN_GAME:
            return 0;
    }
    return 0;
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
                    level_addPlayerConfigPawns(&gameSession.level, gameSession_defaultNewGameConfig());
                    gameSession.activePawn = &gameSession.level.pawns[0];
                    gameActionLogic_scheduleMovement(gameSession.activePawn, NULL, (Coordinate){STARTSCREEN_NAVIGATIONSHIPOFFSETRIGHT, gameSession.activePawn->position.y});
                    break;
                case STRING_ABOUT:
                    about_show();
                    break;
            }
            return true;
        }
    }
}

static void gameSession_launchGame(NewGameConfig config) {
    int faction, previousFaction = 0;
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
    gameSession_startTurn();
}

static Boolean gameSession_handlePlayerConfigTap(Coordinate selectedTile) {
    int i;
    for (i = 0; i < gameSession.level.actionTileCount; i++) {
        if (isEqualCoordinate(gameSession.level.actionTiles[i].position, selectedTile)) {
            NewGameConfig config = level_getNewGameConfig(&gameSession.level, gameSession_defaultNewGameConfig());
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
    }
    return false;
}

static Boolean gameSession_handleTileTap() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    Pawn *selectedPawn = gameSession_pawnAtTile(selectedTile);
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
        if (gameSession.highlightTiles[i].x == coordinate.x && gameSession.highlightTiles[i].y == coordinate.y) {
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
    if (gameSession.secondaryHighlightTiles != NULL) {
        MemPtrFree(gameSession.secondaryHighlightTiles);
        gameSession.secondaryHighlightTiles = NULL;
        gameSession.secondaryHighlightTileCount = 0;
    }
}

static void gameSession_handleTargetSelection() {
    Coordinate convertedPoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchCoordinate);
    Coordinate selectedTile = hexgrid_tileAtPixel(convertedPoint.x, convertedPoint.y);
    Pawn *selectedPawn = gameSession_pawnAtTile(selectedTile);
    if (!gameSession_highlightTilesContains(selectedTile)) {
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
            homeBase = movement_homeBase(gameSession.activePawn, gameSession.level.pawns, gameSession.level.pawnCount);
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
            gameActionLogic_scheduleShockwave(gameSession.activePawn);
            gameSession.activePawn->inventory.baseActionLastActionTurn = gameSession.currentTurn;
        case MenuActionTypeBuildShip:
            // TODO: implement build ship action
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

AppColor gameSession_hightlightTilesColor() {
    switch (gameSession.targetSelectionType) {
        case TARGETSELECTIONTYPE_MOVE:
            return EMERALD;
        case TARGETSELECTIONTYPE_PHASER:
        case TARGETSELECTIONTYPE_TORPEDO:
            return ALIZARIN;
    }
}

static Coordinate gameSession_getBoxCoordinate(Coordinate center, float t, int boxSize) {
    int halfSize = boxSize / 2;
    int perimeter = 4 * boxSize;
    int pos = (int)(t * perimeter) % perimeter;

    Coordinate result = {center.x, center.y};

    if (pos < boxSize) {
        result.x = center.x - halfSize + pos;
        result.y = center.y - halfSize;
    } else if (pos < 2 * boxSize) {
        result.x = center.x + halfSize;
        result.y = center.y - halfSize + (pos - boxSize);
    } else if (pos < 3 * boxSize) {
        result.x = center.x + halfSize - (pos - 2 * boxSize);
        result.y = center.y + halfSize;
    } else {
        result.x = center.x - halfSize;
        result.y = center.y + halfSize - (pos - 3 * boxSize);
    }

    return result;
}

static void gameSession_progressUpdateExplosion() {
    Int32 timeSinceLaunch;
    float timePassedScale;
    gameSession.drawingState.shouldRedrawOverlay = true;
    timeSinceLaunch = TimGetTicks() - gameSession.attackAnimation->explosionTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * gameSession.attackAnimation->explosionDurationSeconds);
    if (timePassedScale >= 1) {
        gameActionLogic_clearAttack();
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
            attackLine = (Line){hexgrid_tileCenterPosition(gameSession.activePawn->position), gameSession_getBoxCoordinate(targetCenter, timePassedScale, HEXTILE_PAWNSIZE / 3)};
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
        if (gameSession.targetSelectionType == TARGETSELECTIONTYPE_TORPEDO || isInvalidCoordinate(gameSession.attackAnimation->targetPawn->position)) {  // show explosion when destroyed or always when torpedo is used
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
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * 2.0);
    if (timePassedScale > 0.2) {
        for (i = 0; i < gameSession.shockWaveAnimation->affectedPawnCount; i++) {
        int nextOrientation = ((i + (int)(timePassedScale * 2 * GFX_FRAMECOUNT_SHIPA)) % GFX_FRAMECOUNT_SHIPA);
        if (i % 2 == 0) {
            nextOrientation = GFX_FRAMECOUNT_SHIPA - 1 - nextOrientation;
        }
        gameSession.level.pawns[gameSession.shockWaveAnimation->affectedPawnIndices[i]].orientation = nextOrientation;
    }
    }
    
    for (i = 0; i < WARPCIRCLECOUNT; i++) {
        if (timePassedScale < 0.6) {
            gameSession.shockWaveAnimation->circleDiameter[i] = (float)(timePassedScale) * (float)(WARPCIRCLECOUNT - i) * 30.0;
        } else {
            gameSession.shockWaveAnimation->circleDiameter[i] = (float)(1.0 - timePassedScale) * (float)(WARPCIRCLECOUNT - i) * 30.0;
        }
    }

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
    if (gameSession.movement == NULL) {
        return;
    }
    gameSession.drawingState.shouldRedrawOverlay = true;

    timeSinceLaunch = TimGetTicks() - gameSession.movement->launchTimestamp;
    timePassedScale = (float)timeSinceLaunch / ((float)SysTicksPerSecond() * ((float)gameSession.movement->trajectory.tileCount - 1) / 1.7);
    gameSession.movement->pawnPosition = movement_coordinateAtPercentageOfTrajectory(gameSession.movement->trajectory, timePassedScale, &gameSession.movement->pawn->orientation);
    gameSession_updateViewPortOffset(false);

    if (timePassedScale >= 1) {
        if (!gameActionLogic_afterMove()) {
            gameSession_updateViewPortOffset(true);
        }
    }
}

Boolean gameSession_animating() {
    return gameSession.attackAnimation != NULL || gameSession.movement != NULL || gameSession.warpAnimation.isWarping;
}

static void gameSession_cpuTurn() {
    int i;
    if (gameSession_animating() || gameSession.state != GAMESTATE_DEFAULT || gameSession.factions[gameSession.factionTurn].human) {
        return;
    }
    for (i = 0; i < gameSession.level.pawnCount; i++) {
        UInt16 textId;
        char *text;
        MemHandle resourceHandle;
        Coordinate closestTile;
        Pawn *targetPawn;
        CPUStrategyResult strategy;
        Pawn *homeBase;
        Pawn *pawn = &gameSession.level.pawns[i];
        if (pawn->faction != gameSession.factionTurn || gameSession.factions[pawn->faction].human || pawn->type != PAWNTYPE_SHIP || pawn->turnComplete || isInvalidCoordinate(pawn->position)) {
            continue;
        }
        // move camera to active pawn
        if (!gameSession_isViewPortOffsetToPawn(pawn)) {
            gameSession_moveCameraToPawn(pawn);
            gameSession.activePawn = pawn;
            return;
        }
        strategy = cpuLogic_getStrategy(pawn, gameSession.level.pawns, gameSession.level.pawnCount, gameSession.factions[pawn->faction].profile);
        pawn->turnComplete = true;
        gameSession.activePawn = pawn;
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
                homeBase = movement_homeBase(pawn, gameSession.level.pawns, gameSession.level.pawnCount);
                pawn->warped = true;
                closestTile = movement_closestTileToTargetInRange(homeBase, homeBase->position, gameSession.level.pawns, gameSession.level.pawnCount, false);
                textId = STRING_WARP;
                gameActionLogic_scheduleWarp(pawn, closestTile);
                pawn->position = closestTile;
                break;
            case CPUACTION_NONE:
                textId = STRING_NOACTION;
                gameSession.drawingState.requiresPauseAfterLayout = true;
                break;
        }
        resourceHandle = DmGetResource(strRsc, textId);
        text = (char *)MemHandleLock(resourceHandle);
        StrCopy(gameSession.cpuActionText, text);
        MemHandleUnlock(resourceHandle);
        DmReleaseResource(resourceHandle);
        return;
    }
    if (!gameSession_movesLeftForFaction(gameSession.factionTurn)) {
        gameSession_startTurnForNextFaction();
    }
}

Boolean gameSession_handleMenu(UInt16 menuItemID) {
    switch (menuItemID) {
        case GAME_MENUITEM_EXIT:
            gameSession_reset(false);
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

void gameSession_progressLogic() {
    if (!gameSession_animating()) {
        if (!gameSession.factions[gameSession.factionTurn].human) {
            gameSession_cpuTurn();
        } else {
            if (!gameSession_movesLeftForFaction(gameSession.factionTurn)) {
                gameSession_startTurnForNextFaction();
                return;
            }
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
                if (gameSession.lastPenInput.moving && gameSession.state == GAMESTATE_DEFAULT && gameSession_handleMiniMapTap()) {
                    gameSession.drawingState.awaitingEndMiniMapScrolling = true;
                } else {
                    if ((gameSession.state == GAMESTATE_SELECTTARGET && gameSession.lastPenInput.moving || isInvalidCoordinate(gameSession.lastPenInput.touchCoordinate))) {
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
