#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// Define size and origin for the playing field
#define GAMEWINDOW_X 0
#define GAMEWINDOW_Y 0

#define BOTTOMMENU_HEIGHT 30
#define MINIMAP_HEIGHT 40

#define BACKDROP_STARCOUNT 100

// Hexgrid setup
#define HEXTILE_SIZE 20
#define HEXTILE_SEGMENT_SIZE HEXTILE_SIZE / 4
#define HEXGRID_ROWS 15
#define HEXGRID_COLS 15
#define HEXTILE_PAWNSIZE 18

#define GAMEMECHANICS_MAXTILEMOVERANGE 4
#define GAMEMECHANICS_MAXTILEPHASERRANGE 3
#define GAMEMECHANICS_MAXTILETORPEDORANGE 4
#define GAMEMECHANICS_MAXIMPACTPHASER 49
#define GAMEMECHANICS_MAXIMPACTTORPEDO 60
#define GAMEMECHANICS_MAXBASEHEALTH 250
#define GAMEMECHANICS_MAXSHIPHEALTH 100
#define GAMEMECHANICS_MAXTORPEDOCOUNT 4

// Resource IDs
#define GAME_FORM 1000
#define GAME_ALERT_GAMECOMPLETE_ALLFLAGSCAPTURED 1000
#define GAME_ALERT_GAMECOMPLETE_TOTALDESTRUCTION 1001
#define GAME_ALERT_FLAGCAPTURED 1002
#define GAME_ALERT_BASEDESTROYED 1003

#endif