/**
 * Author: Josh Bradley
 * Last Updated: 20/04/23
 * Platform: Windows. Tested on Windows 10.0.22621.
 * Description: A recreation of Atari's classic arcade game, Missile Command (1980)
 * Built in C, using the PDCurses library - a Windows port of nCurses.
 *
 * To play, try to earn as many points as possible by efficiently intercepting enemy missiles before they destroy your city and missile base assets.
 * Left click in the viewport to launch a missile towards that position.
 * The game continues until all cities have been destroyed.
 */

#define NCURSES_MOUSE_VERSION 2

// The viewport width should be a number that satisfies: (x-59) % 8 = 0 for correct spacing.
#define VIEWPORT_WIDTH 99
#define VIEWPORT_HEIGHT 45

// Total number of enemy missiles fired in a given round.
#define ENEMY_MISSILES_PER_ROUND 18
#define PLAYER_MISSILES_PER_ROUND (MISSILES_PER_BASE * NUMBER_OF_BASES)

// The maximum number of missiles that can be fired from each base.
#define MISSILES_PER_BASE 9

#define GROUND_HEIGHT 3

#define NUMBER_OF_BASES 3
#define NUMBER_OF_CITIES 6

#define BASE_WIDTH 9
#define CITY_WIDTH 5

// The number of columns between assets.
#define ASSET_PADDING (VIEWPORT_WIDTH - 2 - (NUMBER_OF_BASES * BASE_WIDTH) - (NUMBER_OF_CITIES * CITY_WIDTH)) / 8

/*
 * The number of enemy/player missiles that can be launched simultaneously.
 * The buffer should not exceed ENEMY_MISSILES_PER_ROUND or PLAYER_MISSILES_PER_ROUND.
 */
#define ENEMY_MISSILE_BUFFER 12
#define PLAYER_MISSILE_BUFFER 4

// Defines colour of character and background drawn to viewport.
#define RED 1
#define YELLOW 3
#define BLUE 4
#define CYAN 6
#define WHITE 7

// Allows for ASCII graphics to be displayed.
#include <curses.h>

#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <windows.h>

/**
 * Vector
 * Description: A simple container for 2D Cartesian coordinates.
 * Fields:
 * x - the x coordinate
 * y - the y coordinate
 */
struct Vector {
  short x;
  short y;
};

/**
 * Base
 * Description: A player asset. If all bases are destroyed, or all ammunition has been launched in a given round, then the round is over.
 * Fields:
 * isAlive - determines whether the base has been destroyed. If 1, it is alive, else if 0 it is destroyed.
 * offset - the horizontal distance between the left edge of the viewport and the leftmost character of the city
 * ammoCount - the remaining number of missiles that the base can fire in the current round
 * enemyMissileTarget - the coordinate that enemy missiles set as their destination to target this asset
 * playerMissileSource - the coordinate that player missiles set as their starting position when launched from this base
 */
struct Base {
  int isAlive;
  short offset;
  short ammoCount;
  struct Vector enemyMissileTarget;
  struct Vector playerMissileSource;
};

/**
 * City
 * Description: A player asset. If all cities are destroyed then the game is over.
 * Fields:
 * isAlive - determines whether the city has been destroyed. If 1, it is alive, else if 0 it is destroyed
 * offset - the horizontal distance between the left edge of the viewport and the leftmost character of the city
 * enemyMissileTarget - the coordinate that enemy missiles set as their destination to target this asset
 */
struct City {
  int isAlive;
  unsigned short offset;
  struct Vector enemyMissileTarget;
};

/**
 * Missile
 * Description: A generic missile that represents both enemy and player missiles.
 * Fields:
 * currPos - the current position of the missile
 * startPos - the start position of the missile
 * destPos - the destination position of the missile
 * prevPos - the previous position of the missile
 * trailCoords - stores the location of missile trail. If an element is 1, then the two indexes representing
 * that element represent the coordinate location.
 * explosionTimer - coordinates timing of the missile's explosion animation
 * explosionFrame - the animation frame of the missile's explosion
 * canFragment - determines whether the missile can fragment. If 1, it can fragment; else if 0, it cannot.
 * True for enemy missiles that spawned from the top of the viewport
 * isActive - determines whether the missile is active.  If 1, it is active; else if 0, it is not.
 * colour -  the colour of the trail made by the missile
 */
struct Missile
{
  struct Vector currPos;
  struct Vector startPos;
  struct Vector destPos;
  struct Vector prevPos;
  unsigned short trailCoords[VIEWPORT_WIDTH][VIEWPORT_HEIGHT];
  clock_t explosionTimer;
  short explosionFrame;
  int canFragment;
  int isActive;
  int colour;
};

/**
 * gameStates
 * Description: Defines all possible states that the game can be in.
 *
 * ongoing - normal round execution. The player can fire missiles at will.
 * roundEnding - preparing for the end of the round. Missile speed increases.
 * endOfRound - the end of round screen is being displayed. Showing the results of a survived round.
 * endOfGame - the game over screen is being displayed.
 */
enum gameStates {ongoing, roundEnding, endOfRound, endOfGame};

/**
 * getPlayerMissilesRemaining
 * Description: Gets the number of missiles that the player can fire in the current round.
 * Params:
 * bases - the collection of bases
 * Return: the number of missiles remaining that the player can fire in the current round
 */
int getPlayerMissilesRemaining(struct Base bases[])
{
  int ammoCount = 0;

  for(int i = 0; i < NUMBER_OF_BASES; i++)
  {
    if(bases[i].isAlive)
    {
      ammoCount += bases[i].ammoCount;
    }
  }

  return ammoCount;
}

/**
 * drawScore
 * Description: Draws the player's current score.
 * Params:
 * score - the player's score
 */
void drawScore(int score)
{
  attron(COLOR_PAIR(RED));
  mvprintw(1, 1, "%d", score);
}

/**
 * drawBaseMissileCount
 * Description: Draws the number of missiles remaining in a given missile base.
 * Params:
 * b - the base to draw the missile count for
 */
void drawBaseMissileCount(struct Base* b)
{
  attron(COLOR_PAIR(CYAN));
  mvprintw(VIEWPORT_HEIGHT - GROUND_HEIGHT - 2, b->offset + 4, "%d", b->ammoCount);
}

/**
 * drawBases
 * Description: Draws the bases
 * Params:
 * bases - the collection of bases to draw
 */
void drawBases(struct Base bases[])
{
  for(int i = 0; i < NUMBER_OF_BASES; i++)
  {
    if(bases[i].isAlive)
    {
      attron(COLOR_PAIR(YELLOW));

      mvprintw(VIEWPORT_HEIGHT - GROUND_HEIGHT - 4, bases[i].offset + 2,  "/\\_/\\");
      mvprintw(VIEWPORT_HEIGHT - GROUND_HEIGHT - 3, bases[i].offset + 1, "/XXXXX\\");
      mvprintw(VIEWPORT_HEIGHT - GROUND_HEIGHT - 2, bases[i].offset,    "/XX   XX\\");

      drawBaseMissileCount(&bases[i]);
    }
  }
}

/**
 * drawCities
 * Description: Draws the cities.
 * Params:
 * cities - the collection of cities to draw
 */
void drawCities(struct City cities[])
{
  attron(COLOR_PAIR(CYAN));

  for(int i = 0; i < NUMBER_OF_CITIES; i++)
  {
    if(cities[i].isAlive)
    {
      mvprintw(VIEWPORT_HEIGHT - GROUND_HEIGHT - 3, cities[i].offset, "/MMM\\");
      mvprintw(VIEWPORT_HEIGHT - GROUND_HEIGHT - 2, cities[i].offset, "|[]||");
    }
  }
}

/**
 * drawLandscape
 * Description: Draws the ground, bases and cities.
 * Params:
 * bases - the collection of missile bases
 * cities - the collection of cities
 */
void drawLandscape(struct Base bases[], struct City cities[])
{

  // Draws ground.
  attron(COLOR_PAIR(YELLOW));

  for(int row = 0; row < GROUND_HEIGHT; row++)
  {
    for(int col = 0; col < VIEWPORT_WIDTH - 2; col++)
    {
      mvprintw(VIEWPORT_HEIGHT - 2 - row, col + 1, "X");
    }
  }

  // Draws bases.
  drawBases(bases);

  // Draws cities.
  drawCities(cities);
}

/**
 * drawRoundEnd
 * Description: Draws the text that appears at the end of a survived round.
 * Params:
 * score - the player's score
 * bases - the collection of missile bases
 * cities - the collection of cities
 * enemyMissilesDestroyed - the number of enemy missiles intercepted in the current round
 * roundNumber - the current round number
 */
void drawRoundEnd(int* score, struct Base bases[], struct City cities[], int enemyMissilesDestroyed, int roundNumber)
{
  int basesSurvived = 0;
  int ammoRemaining = getPlayerMissilesRemaining(bases);

  for(int i = 0; i < NUMBER_OF_BASES; i++)
  {
    if(bases[i].isAlive)
    {
      basesSurvived++;
    }
  }

  *score += 100 * basesSurvived + 5 * ammoRemaining;
  drawScore(*score);

  attron(COLOR_PAIR(RED));

  mvprintw(VIEWPORT_HEIGHT/2 - 5, VIEWPORT_WIDTH/2 - 6,             "ROUND %d CLEARED"             , roundNumber);

  mvprintw(VIEWPORT_HEIGHT/2 - 3, VIEWPORT_WIDTH/2 - 11,        "CIVILISATION HAS SURVIVED"        );

  mvprintw(VIEWPORT_HEIGHT/2 - 1, VIEWPORT_WIDTH/2 - 4,                 "SCORE: %d"                , *score);

  mvprintw(VIEWPORT_HEIGHT/2 + 1, VIEWPORT_WIDTH/2 - 17, "MISSILES INTERCEPTED:  %02d X 25 POINTS" , enemyMissilesDestroyed);
  mvprintw(VIEWPORT_HEIGHT/2 + 2, VIEWPORT_WIDTH/2 - 17, "BASES SURVIVED:        %d X 100 POINTS"  , basesSurvived);
  mvprintw(VIEWPORT_HEIGHT/2 + 3, VIEWPORT_WIDTH/2 - 17, "MISSILES REMAINING:     %02d X 5 POINTS" , ammoRemaining);

  /*
   * Redraws the landscape.
   * Corrects any visual damage to surviving bases, cities and the ground.
   */
  drawLandscape(bases, cities);

  refresh();
  Sleep(5000);
}

/**
 * eraseRoundEnd
 * Description: Erases the text that appears at game over.
 */
void eraseRoundEnd()
{
  for(int lineNumber = 0; lineNumber < 9; lineNumber++)
  {
    // Selects the rows that contains text.
    if(!lineNumber || lineNumber == 2 || lineNumber == 4 || lineNumber >= 6)
    {
      // Replaces the row with whitespace.
      for(int i = 1; i < VIEWPORT_WIDTH - 1; i++)
      {
        mvprintw(VIEWPORT_HEIGHT/2 - 5 + lineNumber, i, " ");
      }
    }
  }
}

/**
 * drawGameEnd
 * Description: Draws the text that appears at game over.
 */
void drawGameEnd()
{
  attron(COLOR_PAIR(RED));
  mvprintw(VIEWPORT_HEIGHT/2 - 1, VIEWPORT_WIDTH/2 - 1, "THE");
  mvprintw(VIEWPORT_HEIGHT/2 + 1, VIEWPORT_WIDTH/2 - 1, "END");
  refresh();
}

/**
 * drawExplosion
 * Description: Draws the explosion animation for missiles.
 * Params:
 * m - the missile to draw the explosion animation for
 */
void drawExplosion(struct Missile* m)
{
  // Alternates the colour of the explosion frames.
  if(m->explosionFrame % 2)
  {
     attron(COLOR_PAIR(RED));
  }
  else
  {
    attron(COLOR_PAIR(YELLOW));
  }

  switch(m->explosionFrame)
  {
    case 1:
    {
      mvprintw(m->currPos.y, m->currPos.x,  "*");
      break;
    }
    case 2:
    {
      mvprintw(m->currPos.y - 1, m->currPos.x,  "*");
      mvprintw(m->currPos.y, m->currPos.x - 1, "***");
      mvprintw(m->currPos.y + 1, m->currPos.x,  "*");
      break;
    }
    case 3:
    {
      mvprintw(m->currPos.y - 2, m->currPos.x - 2, "* * *");
      mvprintw(m->currPos.y - 1, m->currPos.x - 1,  "***");
      mvprintw(m->currPos.y, m->currPos.x - 3,    "*******");
      mvprintw(m->currPos.y + 1, m->currPos.x - 2, " *** ");
      mvprintw(m->currPos.y + 2, m->currPos.x- 2,  "* * *");
      break;
    }
    case 4:
    {
      mvprintw(m->currPos.y - 2, m->currPos.x - 2, "     ");
      mvprintw(m->currPos.y - 1, m->currPos.x - 1,  " * ");
      mvprintw(m->currPos.y, m->currPos.x - 3,    "  ***  ");
      mvprintw(m->currPos.y + 1, m->currPos.x - 1,  " * ");
      mvprintw(m->currPos.y + 2, m->currPos.x - 2, "     ");
      break;
    }
    case 5:
    {
      mvprintw(m->currPos.y - 1, m->currPos.x,  " ");
      mvprintw(m->currPos.y, m->currPos.x - 1, " * ");
      mvprintw(m->currPos.y + 1, m->currPos.x,  " ");
      break;
    }
    case 6:
    {
      mvprintw(m->currPos.y, m->currPos.x, " ");
      break;
    }
  }
}

/**
 * initBases
 * Description: Initialises the state for bases.
 * Params:
 * bases - the collection of bases to initialise.
 */
void initBases(struct Base bases[])
{
  // Horizontal offsets for missile bases.
  bases[0].offset = 1;
  bases[1].offset = VIEWPORT_WIDTH/2 - 4;
  bases[2].offset = VIEWPORT_WIDTH - 10;

  for(int i = 0; i < NUMBER_OF_BASES; i++)
  {
    bases[i].isAlive = 1;
    bases[i].ammoCount = MISSILES_PER_BASE;

    bases[i].enemyMissileTarget.x = bases[i].offset + 4;
    bases[i].enemyMissileTarget.y = VIEWPORT_HEIGHT - GROUND_HEIGHT - 4;

    bases[i].playerMissileSource.x = bases[i].offset + 4;
    bases[i].playerMissileSource.y = VIEWPORT_HEIGHT - GROUND_HEIGHT - 5;
  }
}

/**
 * initCities
 * Description: Initialises the state for cities.
 * Params:
 * cities - the collection of cities that are initialised
 * bases - the collection of bases. Provided because cities are positioned relative to the bases.
 */
void initCities(struct City cities[], struct Base bases[])
{
  // Horizontal offsets for cities.
  for(int i = 0; i < NUMBER_OF_CITIES; i++)
  {
    short cityWidthsRequired = i % (NUMBER_OF_CITIES / 2);
    int paddingRequired = cityWidthsRequired + 1;

    cities[i].offset = bases[i >= (NUMBER_OF_CITIES / 2)].offset +
                       BASE_WIDTH +
                      (paddingRequired * ASSET_PADDING) +
                      (cityWidthsRequired * CITY_WIDTH);
  }

  for(int i = 0; i < NUMBER_OF_CITIES; i++)
  {
    cities[i].isAlive = 1;

    cities[i].enemyMissileTarget.x = cities[i].offset + 2;
    cities[i].enemyMissileTarget.y = VIEWPORT_HEIGHT - GROUND_HEIGHT - 4;
  }
}

/**
 * isBaseActive
 * Description: Determines whether the base is capable of firing a missile.
 * Params:
 * base - a pointer to the base that is inspected, to determine if it is active
 * Returns: 1 if base is active, else 0
 */
int isBaseActive(struct Base* base)
{
  return base->ammoCount > 0 && base->isAlive;
}

/**
 * hasMissileReachedDestination
 * Description: Determines whether the provided missile's current location is the same as its destination location.
 * Params:
 * m - the missile that is inspected to determine whether it has reached its destination
 * Returns: 1 if the missile has reached its destination, else 0
 */
int hasMissileReachedDestination(struct Missile* m)
{
  return m->currPos.x == m->destPos.x && m->currPos.y == m->destPos.y;
}

/**
 * checkEndOfRoundPending
 * Description: Determines whether the game state should progress to prepare for the end of the round.
 * Params:
 * gameState - the state of the game. This is progressed to reflect the end of the round or game as required.
 * bases - the collection of bases
 * cities - the collection of cities
 * playerMissiles - the collection of player missiles
 * enemyMissiles - the collection of enemy missiles
 * enemyMissilesFired - the number of enemy missiles that have been created in the current round
 */
void checkEndOfRoundPending(enum gameStates* gameState, struct Base bases[], struct City cities[],
                            struct Missile* playerMissiles, struct Missile* enemyMissiles, int enemyMissilesFired)
{
  if(*gameState != ongoing)
  {
    return;
  }

  int basesActive = 0;

  // The player must have an active base to continue the round.
  for(int i = 0; i <= NUMBER_OF_BASES; i++)
  {
    if(isBaseActive(&bases[i]))
    {
      basesActive = 1;
    }
  }

  int citiesSurvived = 0;

  // The player must have a city that is alive to continue the round.
  for(int i = 0; i < NUMBER_OF_CITIES; i++)
  {
    if(cities[i].isAlive)
    {
      citiesSurvived = 1;
    }
  }

  int enemyMissilesRemaining = 0;

  /*
   * The enemy must have missiles left to fire or missiles
   * currently active to continue the round.
   */
  if(enemyMissilesFired < ENEMY_MISSILES_PER_ROUND)
  {
    enemyMissilesRemaining = 1;
  }
  else
  {
    for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
    {
      if(enemyMissiles[i].isActive)
      {
        enemyMissilesRemaining = 1;
      }
    }
  }

  int playerMissilesRemaining = 0;

  /*
   * The player must have missiles remaining or missiles
   * currently active to continue the round.
   */
  if(getPlayerMissilesRemaining(bases) > 0)
  {
    playerMissilesRemaining = 1;
  }
  else
  {
    for(int i = 0; i < PLAYER_MISSILE_BUFFER; i++)
    {
      if(playerMissiles[i].isActive)
      {
        playerMissilesRemaining = 1;
      }
    }
  }

  if(!citiesSurvived || !basesActive || !enemyMissilesRemaining || !playerMissilesRemaining)
  {
    *gameState = roundEnding;
  }
}

/**
 * checkEndOfRound
 * Description: Determines whether the round has ended, and whether it is a game over state or a next round state.
 * Params:
 * gameState - the state of the game. This is progressed to reflect the end of the round or game as required.
 * cities - the collection of cities
 * playerMissiles - the collection of player missiles
 * enemyMissiles - the collection of enemy missiles
 * enemyMissilesFired - the number of enemy missiles that have been created in the current round
 */
void checkEndOfRound(enum gameStates* gameState, struct City cities[], struct Missile playerMissiles[], struct Missile enemyMissiles[], int enemyMissilesFired)
{
  if(*gameState != roundEnding)
  {
    return;
  }

  // All enemy missiles must have been fired to complete the round.
  if(enemyMissilesFired < ENEMY_MISSILES_PER_ROUND)
  {
    return;
  }

  int activeEnemyMissile = 0;

  for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
  {
    if(enemyMissiles[i].isActive)
    {
      activeEnemyMissile = 1;
      break;
    }
  }

  int activePlayerMissile = 0;

  for(int i = 0; i < PLAYER_MISSILE_BUFFER; i++)
  {
    if(playerMissiles[i].isActive)
    {
      activePlayerMissile = 1;
      break;
    }
  }

  // All missiles must be inactive to complete the round.
  if(!activePlayerMissile && !activeEnemyMissile)
  {
    int citiesSurvived = 0;

    for(int i = 0; i < NUMBER_OF_CITIES; i++)
    {
      if(cities[i].isAlive)
      {
        citiesSurvived = 1;
        break;
      }
    }

    // Game over if all cities have been destroyed, else a new round begins.
    if(!citiesSurvived)
    {
      *gameState = endOfGame;
    }
    else
    {
      *gameState = endOfRound;
    }
  }
}

/**
 * destroyAssets
 * Description: Sets the status of bases or cities to 0, if the asset has been hit with a missile explosion.
 * Params:
 * enemyMissiles - the collection of enemy missiles
 * bases - the collection of missile bases. These are assets that can be destroyed by enemy missiles.
 * cities - the collection of cities. These are assets that can be destroyed by enemy missiles.
 */
void destroyAssets(struct Missile enemyMissiles[], struct Base bases[], struct City cities[])
{
  for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
  {
    if(hasMissileReachedDestination(&enemyMissiles[i]) && enemyMissiles[i].currPos.y == (VIEWPORT_HEIGHT - GROUND_HEIGHT - 4))
    {
      for(int j = 0; j < NUMBER_OF_BASES; j++)
      {
        // Decides which base was hit by the enemy missile.
        if(bases[j].offset + 4 == enemyMissiles[i].destPos.x)
        {
          bases[j].isAlive = 0;
          break;
        }
      }

      for(int j = 0; j < NUMBER_OF_CITIES; j++)
      {
        // Decides which city was hit by the enemy missile.
        if(cities[j].offset + 2 == enemyMissiles[i].destPos.x)
        {
          cities[j].isAlive = 0;
          break;
        }
      }
    }
  }
}

/**
 * initMissile
 * Description:
 * Initialises the provided missile.
 * This is called indirectly at the start of the game, and at the start of a new round through initMissiles().
 * It is also called directly on creating a new missile instance.
 * This function must be called prior to a new round beginning to prevent the missile from wrongly being considered active.
 * Params:
 * m - the missile to initialise
 */
void initMissile(struct Missile* m)
{
  m->isActive = 0;

  m->currPos.x = -1;
  m->currPos.y = -1;

  m->startPos.x = -1;
  m->startPos.y = -1;

  m->destPos.x = -1;
  m->destPos.y = -1;

  m->prevPos.x = -1;
  m->prevPos.y = -1;

  m->explosionTimer = 0;

  m->explosionFrame = 0;

  m->canFragment = 0;

  for(int row = 0; row < VIEWPORT_HEIGHT; row++)
  {
    for(int col = 0; col < VIEWPORT_WIDTH; col++)
    {
      m->trailCoords[col][row] = 0;
    }
  }
}

/**
 * initMissiles
 * Description: Initialises all missiles in the provided missile collection.
 * This must be called performed prior to a new round beginning to prevent missiles wrongly being considered active.
 * Params:
 * missiles - the collection of missiles to initialise
 * size - the size of the missiles[] array
 */
void initMissiles(struct Missile missiles[], size_t size)
{
  for(int i = 0; i < size; i++)
  {
    initMissile(&missiles[i]);
  }
}

/**
 * removeTrail
 * Description: Removes the trail from the provided missile.
 * Params:
 * m - a pointer to the missile that will have its trail removed
 */
void removeTrail(struct Missile* m)
{
  for(int col = 0; col < VIEWPORT_WIDTH; col++)
  {
    for(int row = 0; row < VIEWPORT_HEIGHT; row++)
    {
      if(m->trailCoords[col][row])
      {
        m->trailCoords[col][row] = 0;

        /*
         * Only deletes parts of the trail that match the missiles colour.
         * This prevents the trail deletion potentially triggering missile explosions (from obscuring the missile location).
         */
        if((A_COLOR & mvinch(row, col)) >> 24 == m->colour)
        {
          mvprintw(row, col, " ");
        }
      }
    }
  }
}

/**
 * checkInterceptions
 * Description:
 * Destroys enemy missiles that have been hit with a player missiles.
 * Increases the player's score by 25 for each enemy missile hit.
 *
 * This implementation means that enemy missiles will rarely detonate upon colliding with another enemyMissile, but
 * this issue cannot be trivially solved unless missile head colours do not differ from the trail; and therefore preventing
 * the need to redraw over the previous location.
 * Params:
 * fragmentIndexes - an array that is populated with 1 in all indexes where the same index in enemyMissiles can fragment
 * enemyMissiles - the collection of enemyMissiles
 */
void checkInterceptions(struct Missile enemyMissiles[], int* score, int* enemyMissilesDestroyed)
{
  for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
  {
    if(!hasMissileReachedDestination(&enemyMissiles[i]) && !enemyMissiles[i].explosionFrame)
    {
      chtype head = mvinch(enemyMissiles[i].currPos.y, enemyMissiles[i].currPos.x);

      // Removes obscured missile as long as it is not obscured by a cross (target).
      if(!((head & A_CHARTEXT) == '*' && (head & A_COLOR) >> 24 == WHITE) &&
      (head & A_CHARTEXT) != 'X')
      {
        // Resetting the destination prevents the missile path from continuing once intercepted.
        enemyMissiles[i].destPos.x = enemyMissiles[i].currPos.x;
        enemyMissiles[i].destPos.y = enemyMissiles[i].currPos.y;

        removeTrail(&enemyMissiles[i]);
        enemyMissiles[i].explosionFrame = 1;

        *score += 25;
        *enemyMissilesDestroyed += 1;
      }
    }
  }
}

/**
 * checkFragment
 * Description:
 * Retrieves the indexes of the enemyMissiles that are able to fragment.
 * A fragment is a second missile that forms at the current position of the base missile.
 * Params:
 * fragmentIndexes - an array that is populated with 1 in all indexes where the same index in enemyMissiles can fragment
 * enemyMissiles - the collection of enemyMissiles
 */
void checkFragment(int fragmentIndexes[], struct Missile enemyMissiles[])
{
  for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
  {
    if((enemyMissiles[i].currPos.y == VIEWPORT_HEIGHT / 2)
    && (enemyMissiles[i].prevPos.y == (enemyMissiles[i].currPos.y - 1))
    && (enemyMissiles[i].isActive)
    && (enemyMissiles[i].canFragment))
    {
      fragmentIndexes[i] = 1;
    }
  }
}

double getTimeElapsed(clock_t* lastTimeRecorded)
{
  return ((double)(clock() - *lastTimeRecorded))/CLOCKS_PER_SEC;
}

/**
 * updateAbstractMissile
 * Description: Coordinates the explosion and signals whether a movement update is required for a single missile.
 * Params:
 * index - the index of the missile in missiles[]
 * missiles - the collection of missiles to update
 * gameState - the state of the game
 * Returns: 1 if the missile should have its position updated on the current tick, else 0
 */
int updateAbstractMissile(int index, struct Missile missiles[], enum gameStates* gameState)
{
  if(missiles[index].isActive)
  {
    /*
     * Initiates explosions for missiles that have reached their destination
     * and removes their missile trail.
     */
    if(hasMissileReachedDestination(&missiles[index]))
    {
      if(!missiles[index].explosionFrame)
      {
        removeTrail(&missiles[index]);
        missiles[index].explosionFrame = 1;
      }
    }

    // Attempts to progress the explosion if it has been initiated.
    if(missiles[index].explosionFrame)
    {
      // Determines the speed of the explosion animation.
      double timeBetweenExplosionUpdates = 0.1d;

      if(!missiles[index].explosionTimer || getTimeElapsed(&(missiles[index].explosionTimer)) > timeBetweenExplosionUpdates)
      {
        missiles[index].explosionTimer = clock();

        drawExplosion(&missiles[index]);

        // Deactivates the missile once the explosion has finished.
        if(missiles[index].explosionFrame == 7)
        {
          missiles[index].isActive = 0;
        }
        // Progresses the explosion animation.
        else
        {
          missiles[index].explosionFrame++;
        }
      }
    }

    // If true, signals that the missile movement should be performed.
    return !hasMissileReachedDestination(&missiles[index]);
  }

  return 0;
}

/**
 * updatePlayerMissiles
 * Description:
 * Updates the state of player missiles.
 * Coordinates explosions and signals whether missile movement is required.
 * Also moves player missiles towards their destination.
 * Params:
 * missiles - the collection of player missiles to update
 * gameState - the state of the game
 * timer - coordinates timing for missile movement
 */
void updatePlayerMissiles(struct Missile missiles[], enum gameStates* gameState, clock_t* timer)
{
  // Determines the rate of enemy missile movement updates.
  double timeBetweenMovementUpdate = (*gameState == roundEnding) ? .01d : .015d;

  int updateTimer = (!*timer || getTimeElapsed(timer) > timeBetweenMovementUpdate);

  for(int i = 0; i < PLAYER_MISSILE_BUFFER; i++)
  {
    if(updateAbstractMissile(i, missiles, gameState) && updateTimer)
    {
      struct Vector flightPath = {missiles[i].destPos.x - missiles[i].startPos.x, abs(missiles[i].destPos.y - missiles[i].startPos.y)};

      // Moves missile vertically.
      if(missiles[i].currPos.x == missiles[i].destPos.x)
      {
        missiles[i].currPos.y--;
      }
      // Moves the missile on trajectories with a greater horizontal component.
      else if(abs(flightPath.x) >= flightPath.y)
      {
        double theta = atan2(flightPath.y, abs(flightPath.x));

        missiles[i].currPos.y = missiles[i].startPos.y - round(tan(theta) * (abs(missiles[i].currPos.x - missiles[i].startPos.x) + 1));

        if(flightPath.x > 0)
        {
          missiles[i].currPos.x++;
        }
        else
        {
          missiles[i].currPos.x--;
        }
      }
      // Moves the missile on trajectories with a greater vertical component.
      else
      {
        double theta = atan2(abs(flightPath.x), flightPath.y);

        missiles[i].currPos.y--;

        if(flightPath.x > 0)
        {
          missiles[i].currPos.x = missiles[i].startPos.x + round(tan(theta) * abs(missiles[i].startPos.y - missiles[i].currPos.y));
        }
        else
        {
          missiles[i].currPos.x = missiles[i].startPos.x - round(tan(theta) * abs(missiles[i].startPos.y - missiles[i].currPos.y));
        }
      }

      missiles[i].trailCoords[missiles[i].currPos.x][missiles[i].currPos.y] = 1;

       //Stores the character in the missile's updated location.
      int charRead = mvinch(missiles[i].currPos.y, missiles[i].currPos.x);

      /*
       * Draws the head of the missile.
       *
       * Prevents the missile path destroying enemy missiles.
       * The path cannot draw over white asterisks (the head of enemy missiles).
       */
      if(!(((charRead & A_CHARTEXT) == '*') && ((A_COLOR & charRead) >> 24 == WHITE)))
      {
         attron(COLOR_PAIR(missiles[i].colour));
         mvprintw(missiles[i].currPos.y, missiles[i].currPos.x, "*");
      }
    }
  }

  if(updateTimer)
  {
    *timer = clock();
  }
}

/**
 * updateEnemyMissiles
 * Description:
 * Updates the state of enemy missiles.
 * Coordinates explosions and signals whether missile movement is required.
 * Also moves enemy missiles towards their destination.
 * Params:
 * missiles - the collection of enemy missiles to update
 * gameState - the state of the game
 * timer - coordinates whether missile movement is to be performed on the current tick
 * roundNumber - the current round number that modifies enemy missile speed
 */
void updateEnemyMissiles(struct Missile missiles[], enum gameStates* gameState, clock_t* timer, int roundNumber)
{
  /*
   * Determines the rate of enemy missile movement updates.
   * Missile speed increases as the player survives for more rounds.
   */
  double timeBetweenMovementUpdate = (*gameState == roundEnding) ? 1.d/100 : (1.d/4 - (1.d/30 * (roundNumber - 1)));

  if(timeBetweenMovementUpdate < 1.d/100)
  {
    timeBetweenMovementUpdate = 1.d/100;
  }

  int updateTimer = !(*timer) || getTimeElapsed(timer) > timeBetweenMovementUpdate;

  for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
  {
    if(updateAbstractMissile(i, missiles, gameState) && updateTimer)
    {
      struct Vector flightPath = {missiles[i].destPos.x - missiles[i].startPos.x, missiles[i].destPos.y - missiles[i].startPos.y};

      // Reprints the previous position in the trail, changing its colour to red.
      missiles[i].prevPos.y = missiles[i].currPos.y;
      missiles[i].prevPos.x = missiles[i].currPos.x;

      attron(COLOR_PAIR(missiles[i].colour));
      mvprintw(missiles[i].prevPos.y, missiles[i].prevPos.x, "*");

      // Moves missile vertically.
      if(missiles[i].currPos.x == missiles[i].destPos.x)
      {
        missiles[i].currPos.y++;
      }
      // Moves the missile on trajectories with a greater horizontal component than vertical.
      else if(abs(flightPath.x) >= flightPath.y)
      {
        double theta = atan2(flightPath.y, abs(flightPath.x));

        missiles[i].currPos.y = missiles[i].startPos.y + round(tan(theta) * (abs(missiles[i].currPos.x - missiles[i].startPos.x) + 1));

        if(flightPath.x > 0)
        {
          missiles[i].currPos.x++;
        }
        else
        {
          missiles[i].currPos.x--;
        }
      }
      // Moves the missile on trajectories with a greater vertical component than horizontal.
      else
      {
        double theta = atan2(abs(flightPath.x), flightPath.y);

        missiles[i].currPos.y++;

        if(flightPath.x > 0)
        {
          missiles[i].currPos.x = missiles[i].startPos.x + round(tan(theta) * abs(missiles[i].startPos.y - missiles[i].currPos.y));
        }
        else
        {
          missiles[i].currPos.x = missiles[i].startPos.x - round(tan(theta) * abs(missiles[i].startPos.y - missiles[i].currPos.y));
        }
      }

      missiles[i].trailCoords[missiles[i].currPos.x][missiles[i].currPos.y] = 1;

      /*
       * Draws the head of the missile.
       *
       * Prevents the missile path destroying enemy missiles.
       * The path cannot draw over white asterisks (the head of enemy missiles).
       */
      // Stores the character in the missile's updated location.
      int charRead = mvinch(missiles[i].currPos.y, missiles[i].currPos.x);
      if(!(((charRead & A_CHARTEXT) == '*') && ((A_COLOR & charRead) >> 24 == WHITE)))
      {
        attron(COLOR_PAIR(WHITE));
        mvprintw(missiles[i].currPos.y, missiles[i].currPos.x, "*");
      }
    }
  }

  if(updateTimer)
  {
    *timer = clock();
  }
}

/**
 * createPlayerMissile
 * Description: Spawns a new player missile, targeting the location that was left-clicked on the viewport.
 * Params:
 * target - the X, Y location of the missile's destination in viewport coordinates
 * bases - the collection of missile bases
 * Returns: a new player missile
 */
struct Missile createPlayerMissile(struct Vector target, struct Base bases[])
{
  /*
   * Holds the most appropriate order of base priority, based on their
   * proximity from the missile's target.
   */
  int baseOrder[NUMBER_OF_BASES];

  /*
   * Click event in the left third of the viewport.
   * Ranks the bases in the following priority: LEFT, CENTRE, RIGHT.
   *
   * If a base is not active, the next base is considered.
   */
  if(target.x <= ((VIEWPORT_WIDTH - 2) / 3) + 1)
  {
    baseOrder[0] = 0;
    baseOrder[1] = 1;
    baseOrder[2] = 2;
  }
  /*
   * Click event in the right third of the viewport.
   * Ranks the bases in the following priority: RIGHT, CENTRE, LEFT.
   *
   * If a base is not active, the next base is considered.
   */
  else if(target.x >= (((VIEWPORT_WIDTH - 2) / 3) * 2) + 1)
  {
    baseOrder[0] = 2;
    baseOrder[1] = 1;
    baseOrder[2] = 0;
  }
  else
  {
    /*
     * Click event in the left half of the central third of the viewport.
     * Ranks the bases in the following priority: CENTRE, LEFT, RIGHT.
     *
     * If a base is not active, the next base is considered.
     */
    if(target.x <= VIEWPORT_WIDTH / 2)
    {
      baseOrder[0] = 1;
      baseOrder[1] = 0;
      baseOrder[2] = 2;
    }
    /*
     * Click event in the right half of the central third of the viewport.
     * Ranks the bases in the following priority: CENTRE, RIGHT, LEFT.
     *
     * If a base is not active, the next base is considered.
     */
    else
    {
      baseOrder[0] = 1;
      baseOrder[1] = 2;
      baseOrder[2] = 0;
    }
  }

  // Selects an active base to fire the missile.
  int baseIndex = -1;
  for(int i = 0; i < NUMBER_OF_BASES; i++)
  {
    if(isBaseActive(&bases[baseOrder[i]]))
    {
      bases[baseOrder[i]].ammoCount--;
      drawBaseMissileCount(&bases[baseOrder[i]]);

      baseIndex = baseOrder[i];

      break;
    }
  }

  struct Missile m;
  initMissile(&m);

  // True if a base has been selected.
  if(baseIndex != -1)
  {
    m.isActive = 1;
    m.colour = BLUE;

    m.startPos.x = bases[baseIndex].playerMissileSource.x;
    m.startPos.y = bases[baseIndex].playerMissileSource.y;

    m.currPos.x = m.startPos.x;
    m.currPos.y = m.startPos.y;

    m.destPos.x = target.x;
    m.destPos.y = target.y;

    m.trailCoords[m.startPos.x][m.startPos.y] = 1;

    // Draws the start position.
    attron(COLOR_PAIR(m.colour));
    mvprintw(m.startPos.y, m.startPos.x, "*");

    // Draws the missile target.
    attron(COLOR_PAIR(WHITE));
    mvprintw(m.destPos.y, m.destPos.x, "X");
  }

  return m;
}

/**
 * createEnemyMissile
 * Description: Spawns a new enemy missile, which attempts to target an active missile base or city that is alive.
 * Params:
 * enemyMissiles - the collection of enemy missiles
 * fragmentIndex - the index of the active missile in the enemy missile buffer to fragment from
 * bases - collection of missile bases
 * cities - the collection of cities
 * xPosOfTargetToAvoid - the missile target point of an asset that should not be considered a target for the missile.
 * Returns: a new enemy missile
 */
struct Missile createEnemyMissile(struct Missile enemyMissiles[], int fragmentIndex, struct Base bases[], struct City cities[], short xPosOfTargetToAvoid)
{
  /*
   * Stores whether the target at the given index in targets[] is valid.
   * For example: if targetValidity[4] == 1, then targets[4] is a valid target.
   * else if targetValidity[4] == 0, then targets[4] is not valid.
   */
  int targetValidity[NUMBER_OF_BASES + NUMBER_OF_CITIES] = {0};
  // Stores the Vectors of the missiles potential targets.
  struct Vector targets[NUMBER_OF_BASES + NUMBER_OF_CITIES];
  /*
   * The index (0 - (NUMBER_OF_BASES + NUMBER_OF_CITIES - 1) representing an asset that cannot be targeted.
   * Used to prevent a fragmented missile from targeting the same location as its base missile.
   */
  int targetToAvoid = -1;
  // The number or eligible targets for the missile to target, excluding the missile to avoid (if set).
  int totalTargets = 0;

  // Adds base targets.
  for(int i = 0; i < NUMBER_OF_BASES; i++)
  {
    targets[i] = bases[i].enemyMissileTarget;

    // If true, prevents the base from becoming the missile's target.
    if(bases[i].enemyMissileTarget.x == xPosOfTargetToAvoid)
    {
      targetToAvoid = i;
    }
    else if(isBaseActive(&bases[i]))
    {
      totalTargets++;
      targetValidity[i] = 1;
    }
  }

  // Add city targets.
  for(int i = 0; i < NUMBER_OF_CITIES; i++)
  {
    targets[i + NUMBER_OF_BASES] = cities[i].enemyMissileTarget;

    // If true, prevents the city from becoming the missile's target.
    if(cities[i].enemyMissileTarget.x == xPosOfTargetToAvoid)
    {
      targetToAvoid = i + NUMBER_OF_BASES;
    }
    else if(cities[i].isAlive)
    {
      totalTargets++;
      targetValidity[i + NUMBER_OF_BASES] = 1;
    }
  }

  /*
   * If there is not a viable target, consider all of them to be valid (excluding the target to avoid, if set).
   * Ensures enemy missile barrage completes by providing targets even when none are eligible.
   */
  if(!totalTargets)
  {
    totalTargets = NUMBER_OF_BASES + NUMBER_OF_CITIES;

    // Sets the validity of all targets to 1. Sets the target to avoid to 0 (if set).
    for(int i = 0; i < totalTargets; i++)
    {
      targetValidity[i] = (i != targetToAvoid);
    }

    // Decreases the total targets by 1 if a target to avoid has been specified.
    totalTargets -= (targetToAvoid != -1);
  }

  // Dedicated array storing the indexes of valid targets.
  struct Vector validTargets[totalTargets];

  // Transfers the indexes of missiles eligible to fragment to the dedicated validTargets array.
  int validTargetIndex = 0;
  for(int i = 0; i < NUMBER_OF_BASES + NUMBER_OF_CITIES; i++)
  {
    if(targetValidity[i])
    {
      validTargets[validTargetIndex] = targets[i];
      validTargetIndex++;
    }
  }

  struct Missile m;
  initMissile(&m);

  /*
   * Initialising an enemy missile that is not from a fragment.
   * I.e., it spawns from the top of the viewport.
   */
  if(fragmentIndex == -1)
  {
    m.canFragment = 1;

    // Prevents enemyMissiles from colliding with the score and being destroyed prematurely.
    short scoreOffset = 4;
    m.startPos.x = rand() % (VIEWPORT_WIDTH - 2 - scoreOffset) + 1 + scoreOffset;
    m.startPos.y = 1;
  }
  /*
   * Initialising an enemy missile that is a fragment.
   * Fragments spawns on the midpoint of the viewport's y axis, at another missile's current location.
   */
  else
  {
    m.startPos.x = enemyMissiles[fragmentIndex].currPos.x;
    m.startPos.y = enemyMissiles[fragmentIndex].currPos.y;
  }

  m.isActive = 1;
  m.colour = RED;

  m.currPos.x = m.startPos.x;
  m.currPos.y = m.startPos.y;

  // Samples a random target from the possible targets.
  int randomTarget = rand() % totalTargets;

  m.destPos.x = validTargets[randomTarget].x;
  m.destPos.y = validTargets[randomTarget].y;

  m.trailCoords[m.startPos.x][m.startPos.y] = 1;

  // Draws the head of the missile.
  attron(COLOR_PAIR(WHITE));
  mvprintw(m.startPos.y, m.startPos.x, "*");

  return m;
}

int main(int argc, char *argv[])
{
  /*
   * PDCURSES SETUP.
   */
  // Initialises curses. This must be the first curses function to be called in the program.
  initscr();
  // Sets the dimensions  of the viewport.
  resize_term(VIEWPORT_HEIGHT, VIEWPORT_WIDTH);
  // Sets the input mode for the current terminal to cbreak mode.
  cbreak();
  // Removes the cursor from the viewport.
  curs_set(0);
  // Enables input polling without halting the program.
  nodelay(stdscr, TRUE);
  // Prevents characters from keyboard input being printed to the terminal.
  noecho();
  // Enables keyboard/mouse input.
  keypad(stdscr, TRUE);
  // Enables mouse clicking interrupts.
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

  /*
   * Redefines colour pair combinations.
   * Takes the format: key, fg colour, bg colour.
   */
  start_color();
  init_pair(RED, COLOR_RED, COLOR_BLACK);
  init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);

  struct Base bases[NUMBER_OF_BASES];
  struct City cities[NUMBER_OF_CITIES];
  // Player missile buffer.
  struct Missile playerMissiles[PLAYER_MISSILE_BUFFER];
  // Enemy missile buffer.
  struct Missile enemyMissiles[ENEMY_MISSILE_BUFFER];
  int score = 0;

  /*
   * INITIALISE ROUND VARIABLES.
   */
  // Coordinates timing for enemy missile spawning.
  clock_t enemiesLastSpawnTime = 0;
  // Coordinates timing for enemy missile movement.
  clock_t enemiesLastUpdateTime = 0;
  // Coordinates timing for playerMissileMovement.
  clock_t playersLastUpdateTime = 0;

  // Counts the number of enemies that have spawned in a round.
  int enemyMissilesFired = 0;
  // Counts the number of AI missiles intercepted in a round.
  int enemyMissilesDestroyed = 0;
  int roundNumber = 1;
  enum gameStates gameState = ongoing;

  // Initialise missiles.
  initMissiles(playerMissiles, PLAYER_MISSILE_BUFFER);
  initMissiles(enemyMissiles, ENEMY_MISSILE_BUFFER);

  // Initialise assets.
  initBases(bases);
  initCities(cities, bases);

  // Draws the ground, bases and cities.
  drawLandscape(bases, cities);
  // Draws the score (initially 0).
  drawScore(score);

  // Generates a seed for random value generation.
  srand(time(NULL));

  while(1)
  {
    // Creates a new enemy missile.
    if(enemyMissilesFired < ENEMY_MISSILES_PER_ROUND)
    {
      // Stores the indexes of enemy missiles that can fragment.
      int fragmentIndexes[ENEMY_MISSILE_BUFFER] = {0};
      // The number of missiles that are eligible for fragmentation.
      unsigned short numberOfMissilesCanFragment = 0;

      // Populates the fragmentIndexes array with the indexes of missiles that can fragment.
      checkFragment(fragmentIndexes, enemyMissiles);

      // Calculates the number of missiles eligible for fragmentation.
      for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
      {
        if(fragmentIndexes[i])
        {
          numberOfMissilesCanFragment++;
        }
      }

      if(numberOfMissilesCanFragment)
      {
        // Determines the maximum number of missiles that can be fired on this tick.
        unsigned short maximumSpawns = ENEMY_MISSILES_PER_ROUND - enemyMissilesFired;

        // A dedicated array for storing the indexes of missiles eligible for fragmenting.
        int missilesCanFragment[numberOfMissilesCanFragment];

        // Transfers the indexes of missiles eligible to fragment to the dedicated array.
        int index = 0;
        for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
        {
          if(fragmentIndexes[i])
          {
            missilesCanFragment[index] = i;
            index++;
          }
        }

        // Determines the actual number to fragment. Ensures spawns cannot exceed the buffer.
        short fragmentMissilesToSpawn = (numberOfMissilesCanFragment > maximumSpawns) ? maximumSpawns : numberOfMissilesCanFragment;

        // Spawns missiles from fragmentation.
        for(int i = 0; i < fragmentMissilesToSpawn; i++)
        {
          for(int j = 0; j < ENEMY_MISSILE_BUFFER; j++)
          {
            if(!enemyMissiles[j].isActive)
            {
              // Retrieves an eligible missile for fragmentation.
              int selectedIndexToFragment = missilesCanFragment[i];

              // Prevents the base missile from fragmenting multiple times.
              enemyMissiles[selectedIndexToFragment].canFragment = 0;
              // Creates a new missile, fragmenting from the base missile's current position.
              enemyMissiles[j] = createEnemyMissile(enemyMissiles, selectedIndexToFragment, bases, cities, enemyMissiles[selectedIndexToFragment].destPos.x);
              enemyMissilesFired++;

              break;
            }
          }
        }
      }

      // Determines the rate of enemy missile spawns from the top of the viewport.
      double timeBetweenEnemySpawns = 2.d;

      // Spawns missile from clock timing.
      if((!enemiesLastSpawnTime || getTimeElapsed(&enemiesLastSpawnTime) > timeBetweenEnemySpawns) && enemyMissilesFired < ENEMY_MISSILES_PER_ROUND)
      {
        enemiesLastSpawnTime = clock();

        for(int i = 0; i < ENEMY_MISSILE_BUFFER; i++)
        {
          if(!enemyMissiles[i].isActive)
          {
            // Spawns an enemy missile from the top of the viewport.
            enemyMissiles[i] = createEnemyMissile(enemyMissiles, -1, bases, cities, -1);
            enemyMissilesFired++;

            break;
          }
        }
      }
    }

    MEVENT event;
    int inputEvent = getch();

    /*
     * Create new player missile if a valid input has been registered.
     * Check if the input mouse left-click.
     */
    if(inputEvent == KEY_MOUSE && getmouse(&event) == OK && (event.bstate & BUTTON1_CLICKED))
    {
      // Check if click occurs within clickable bounds.
      if(event.x >= 4 && event.x <= VIEWPORT_WIDTH - 5 && event.y >= 3 && event.y <= VIEWPORT_HEIGHT - 10)
      {
        // Check if the game state allows for missile fire and there are missiles remaining.
        if(gameState == ongoing && getPlayerMissilesRemaining(bases) > 0)
        {
          for(int i = 0; i < PLAYER_MISSILE_BUFFER; i++)
          {
            if(!playerMissiles[i].isActive)
            {
              // Create a missile at the location of the click event.
              struct Vector destination = {event.x, event.y};
              playerMissiles[i] = createPlayerMissile(destination, bases);

              break;
            }
          }
        }
      }
    }
    // Prevents screen resizing.
    else if(inputEvent == KEY_RESIZE)
    {
      resize_term(VIEWPORT_HEIGHT, VIEWPORT_WIDTH);
      /*
       * The amount of sleep affects whether the screen clips when it is resized.
       * I do not know why this behaviour is happening.
       * At the value of 250, the clipping does not seem to appear (Windows 10.0.22621) but that may not be universal.
       */
      Sleep(250);
    }

    updatePlayerMissiles(playerMissiles, &gameState, &playersLastUpdateTime);
    updateEnemyMissiles(enemyMissiles, &gameState, &enemiesLastUpdateTime, roundNumber);

    // Marks the assets that have hit been hit by enemy missiles as not being alive.
    destroyAssets(enemyMissiles, bases, cities);
    // Checks whether any enemy missiles have been intercepted.
    checkInterceptions(enemyMissiles, &score, &enemyMissilesDestroyed);

    drawScore(score);

    // Checks whether the end of the round should initiate.
    checkEndOfRoundPending(&gameState, bases, cities, playerMissiles, enemyMissiles, enemyMissilesFired);
    // Checks whether the round has ended.
    checkEndOfRound(&gameState, cities, playerMissiles, enemyMissiles, enemyMissilesFired);

    // Initiate the end of the round and game.
    if(gameState == endOfGame)
    {
      drawGameEnd();

      // Keeps the end game screen open and prevents screen resizing.
      while(1)
      {
        inputEvent = getch();

        if(inputEvent == KEY_RESIZE)
        {
          resize_term(VIEWPORT_HEIGHT, VIEWPORT_WIDTH);
          /*
           * The amount of sleep affects whether the screen clips when it is resized.
           * I do not know why this behaviour is happening.
           * At the value of 250, the clipping does not seem to appear (Windows 10.0.22621) but that may not be universal.
           */
          Sleep(250);
        }
      }
    }
    // Initiate the end of the round and prepares the next round.
    else if(gameState == endOfRound)
    {
      drawRoundEnd(&score, bases, cities, enemyMissilesDestroyed, roundNumber);

      roundNumber++;

      enemyMissilesFired = 0;
      enemyMissilesDestroyed = 0;
      gameState = ongoing;

      initMissiles(playerMissiles, PLAYER_MISSILE_BUFFER);
      initMissiles(enemyMissiles, ENEMY_MISSILE_BUFFER);

      initBases(bases);
      drawBases(bases);

      enemiesLastSpawnTime = 0;
      enemiesLastUpdateTime = 0;
      playersLastUpdateTime = 0;

      eraseRoundEnd();
    }
  }
}
