#define NCURSES_MOUSE_VERSION 2
#define SCREEN_WIDTH 99 // should be a number where (width-59) % 8 = 0 for proper spacing
#define SCREEN_HEIGHT 45
#define AI_MISSILES 12 // maximum number of AI missiles firing simultaneously
#define PLAYER_MISSILES 5 // maximum number of player missiles firing simultaneously
#define ENEMIES_PER_ROUND 15
#define MISSILES_PER_BASE 7
#define RED 1 // colour definitions
#define YELLOW 3
#define BLUE 4
#define CYAN 6
#define WHITE 7
#include <curses.h>
#include <math.h> // to calculate the path taken for missiles
#include <time.h> // used t create a delay function
#include <stdlib.h> // random number generation

struct missile // missiles fired by the player
{
    int active; // determines whether the missile is currently in motion
    int currX; // the current position of the missile
    int currY;
    int startX; // the initial location of the missile
    int startY;
    int destX; // the destination of the missile
    int destY;
    int prevX; // the asterisk in the trail previously drawn
    int prevY;
    int trailCoords[SCREEN_WIDTH][SCREEN_HEIGHT]; // marks the locations of the trail
    int explosionStage; // determines the animation stage of the missile's explosion event
    int enableSplit;
};
void myDelay(int secs) // causes the program to temporarily halt
{
    const time_t start = clock();
    time_t current;
    do{ current = clock(); }
    while((double)(current - start)/CLOCKS_PER_SEC < secs);
}
int baseStatus(int baseMissiles, int baseSurvived)
{
    return baseMissiles && baseSurvived;
}
void updateScore(int* score)
{
    attron(COLOR_PAIR(RED));
    mvprintw(1, 1, "%d", *score);
}
int reachedDestination(struct missile m) // returns whether a missile has hit the intended target. 1 means successful.
{
    if(m.currX == m.destX && m.currY == m.destY) return 1;
    else return 0;
}
int endRound(int* baseMissiles, int* baseSurvived, int* cityStatus, struct missile* pMissiles)
{
    for(int i = 0; i < PLAYER_MISSILES; i++)
        if(pMissiles[i].active == 1) return 0;

    if(!baseStatus(baseMissiles[0], baseSurvived[0]) && !baseStatus(baseMissiles[1], baseSurvived[1]) && !baseStatus(baseMissiles[2], baseSurvived[2])) return 1; // speeds up the missiles if the bases have been destroyed

    if(!cityStatus[0] && !cityStatus[1] && !cityStatus[2] && !cityStatus[3] && !cityStatus[4] && !cityStatus[5]) return 1;
     // speeds up the missiles if the cities have been destroyed
    return 0;
}
void printResults(int result, int* score, int* baseSurvived, int* missilesRemaining, int missilesDestroyed, int roundNumber)
{
    int totalRemaining = missilesRemaining[0] + missilesRemaining[1] + missilesRemaining[2];
    int totalBases = baseSurvived[0] + baseSurvived[1] + baseSurvived[2];
    *score += 100 * totalBases + 5 * totalRemaining;
    if(result == 1)
    {
        mvprintw(SCREEN_HEIGHT/2 - 5, SCREEN_WIDTH/2 - 6,             "ROUND %d CLEARED"             , roundNumber);

        mvprintw(SCREEN_HEIGHT/2 - 3, SCREEN_WIDTH/2 - 11,        "CIVILISATION HAS SURVIVED"          );

        mvprintw(SCREEN_HEIGHT/2 - 1, SCREEN_WIDTH/2 - 4,                 "SCORE: %d"                 , *score);

        mvprintw(SCREEN_HEIGHT/2 + 1, SCREEN_WIDTH/2 - 17, "MISSILES INTERCEPTED:  %02d X 25 POINTS"  , missilesDestroyed);
        mvprintw(SCREEN_HEIGHT/2 + 2, SCREEN_WIDTH/2 - 17, "BASES SURVIVED:        %d X 100 POINTS"  , totalBases);
        mvprintw(SCREEN_HEIGHT/2 + 3, SCREEN_WIDTH/2 - 17, "MISSILES REMAINING:     %02d X 5 POINTS"  , totalRemaining);
    }
    else
    {
        mvprintw(SCREEN_HEIGHT/2 - 1, SCREEN_WIDTH/2 - 1, "THE");
        mvprintw(SCREEN_HEIGHT/2 + 1, SCREEN_WIDTH/2 - 1, "END");
    }
    refresh();
    myDelay(8);
}
int checkGameOver(int* cityStatus, int totalEnemies, struct missile* AIMissiles, struct missile* pMissiles, int total, int total2)
{
    int citiesSurvived = 0;
    int activeAIMissile = 0;
    int activePMissile = 0;
    int activeMissile = 0;

    for(int i = 0; i < 6; i++)
    {
        if(cityStatus[i] == 1)
        {
            citiesSurvived = 1;
            break;
        }
    }
    for(int i = 0; i < total; i++)
    {
        if(AIMissiles[i].active == 1)
        {
            activeAIMissile = 1;
            break;
        }
    }
    for(int i = 0; i < total2; i++)
    {
        if(pMissiles[i].active == 1)
        {
            activePMissile = 1;
            break;
        }
    }
    activeMissile = activePMissile | activeAIMissile;

    if(!activeMissile && !citiesSurvived) return -1;
    else if(!activeMissile && totalEnemies == ENEMIES_PER_ROUND) return 1;
    else return 0;
}
void deactivateAssets(struct missile* missiles, int total, int* baseOffset, int* cityOffset, int* baseSurvived, int* cityStatus)
{ // sets the status of bases or cities to zero, if the asset has been hit with a missile
    for(int i = 0; i < total; i++)
    {
        if(missiles[i].active == 1 && reachedDestination(missiles[i]))
        {
            for(int j = 0; j < 9; j++)
            {
                if(j < 3 && ((baseOffset[j] + 4) == missiles[i].destX)) // decides which asset was hit by the enemy missile (base)
                {
                    baseSurvived[j] = 0;
                    break;
                }
                if(j >= 3 && ((cityOffset[j - 3] + 2) == missiles[i].destX)) // decides which asset was hit by the enemy missile (city)
                {
                    cityStatus[j - 3] = 0;
                    break;
                }
            }
        }
    }
}
void formatMissiles(struct missile* missiles, int total) // formats missile values once initialised
{
    for(int i = 0; i < total; i++)
    {
        missiles[i].active = 0;
        missiles[i].currX = -1;
        missiles[i].currY = -1;
        missiles[i].startX = -1;
        missiles[i].startY = -1;
        missiles[i].destX = -1;
        missiles[i].destY = -1;
        missiles[i].prevX = -1;
        missiles[i].prevY = -1;
        missiles[i].explosionStage = 1;
        missiles[i].enableSplit = 1;

        for(int col = 0; col < SCREEN_HEIGHT; col++)
            for(int row = 0; row < SCREEN_WIDTH; row++)
                missiles[i].trailCoords[row][col] = 0;
    }
}
void ammoCheck(int missiles, int* baseMissiles, int index) // deactivates a base that has no ammunition to fire
{
     if(missiles <= 0) baseMissiles[index] = 0;
}
void removeTrail(struct missile* m, struct missile* missiles, int total)
{
   for(int j = 0; j < SCREEN_WIDTH; j++)
    {
        for(int k = 0; k < SCREEN_HEIGHT; k++)
        {
            if((*m).trailCoords[j][k] == 1)
            {
                int count = 0;

                for(int l = 0; l < total; l++)
                    if(missiles[l].trailCoords[j][k] == 1) count++;

                (*m).trailCoords[j][k] = 0;

                int charRead = mvinch(k, j);

                if(count == 1 && !(((A_CHARTEXT & charRead) == '*') && ((A_COLOR & charRead) == 0x07000000))) mvprintw(k, j, " ");
            }
        }
    }
}
void explode(int x, int y, int stage) // explosion animation for the player missiles
{
    if(stage == 1)
    {
        attron(COLOR_PAIR(RED));
        mvprintw(y, x,  "*");
    }
    else if(stage == 2)
    {
        attron(COLOR_PAIR(YELLOW));
        mvprintw(y - 1, x,  "*");
        mvprintw(y, x - 1, "***");
        mvprintw(y + 1, x,  "*");
    }
    else if(stage == 3)
    {
        attron(COLOR_PAIR(RED));
        mvprintw(y - 2, x - 2, "* * *");
        mvprintw(y - 1, x - 1,  "***");
        mvprintw(y, x - 3,    "*******");
        mvprintw(y + 1, x - 2, " *** ");
        mvprintw(y + 2, x - 2, "* * *");
    }
    else if(stage == 4)
    {
        attron(COLOR_PAIR(YELLOW));
        mvprintw(y - 2, x - 2, "     ");
        mvprintw(y - 1, x - 1,  " * ");
        mvprintw(y, x - 3,    "  ***  ");
        mvprintw(y + 1, x - 1,  " * ");
        mvprintw(y + 2, x - 2, "     ");
    }
    else if(stage == 5)
    {
        attron(COLOR_PAIR(RED));
        mvprintw(y - 1, x,  " ");
        mvprintw(y, x - 1, " * ");
        mvprintw(y + 1, x,  " ");
    }
    else mvprintw(y, x, " ");
}
void testCollisions(struct missile* AIMissiles, int total, int* score, int* missilesDestroyed)
{ // destroys AI missiles that have been hit with the player's missiles.
    for(int i = 0; i < total; i++)
    {
        if(!reachedDestination(AIMissiles[i]))
        {
            int head = mvinch(AIMissiles[i].currY, AIMissiles[i].currX);

            if((AIMissiles[i].active == 1) && !((((head & A_CHARTEXT) == '*') && ((head & A_COLOR) == 0x07000000)) || ((head & A_CHARTEXT) == 'X'))) // removes obscured missile as long as it isn't obscured by a white cross
            {
                *score += 25;
                *missilesDestroyed += 1;
                AIMissiles[i].active = 0;
                removeTrail(&AIMissiles[i], AIMissiles, AI_MISSILES);
            }
        }
    }
}
struct missile createPlayerMissile(int x, int y, int* missilesRemaining, int* baseOffset, int* baseMissiles, int* baseSurvived)
{
    struct missile m;

    m.active = 1;
    m.destX = x;
    m.destY = y;
    m.prevX = -1; // not a necessary property for player missiles
    m.prevY = -1;
    m.startY = SCREEN_HEIGHT - 8;
    m.explosionStage = 1;
    m.enableSplit = 0;
    attron(COLOR_PAIR(BLUE));

    for(int i = 0; i < SCREEN_WIDTH; i++)
    {
        for(int j = 0; j < SCREEN_HEIGHT; j++)
        {
            m.trailCoords[i][j] = 0;
        }
    }

	int i;
    if(m.destX <= (SCREEN_WIDTH - 2) / 3 + 1)  // click in the left of the viewport
    {
        for(i = 0; i < 3; i++) // base preference: 0, 1, 2
        {
            if(baseStatus(baseMissiles[i], baseSurvived[i]) == 1)
           {
               m.startX = baseOffset[i] + 4;
               missilesRemaining[i]--;
               ammoCheck(missilesRemaining[i], baseMissiles, i);
               break;
           }
        }
        if(i == 3) // unable to fire a missile
        {
            m.active = 0;
            return m;
        }
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]); // updates ammunition count in the selected base
    }
    else if(x >= (SCREEN_WIDTH - 2) / 3 * 2 + 1) // click in the right of the viewport
    {
        for(i = 2; i > -1; i--) // base preference: 2, 1, 0
        {
            if(baseStatus(baseMissiles[i], baseSurvived[i]))
            {
               m.startX = baseOffset[i] + 4;
               missilesRemaining[i]--;
               ammoCheck(missilesRemaining[i], baseMissiles, i);
               break;
           }
        }
        if(i == -1) // unable to fire a missile
        {
            m.active = 0;
            return m;
        }
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]); // updates ammunition count in the selected base
    }
    else // click in the centre of the viewport
    {
        i = 1;
        if(!baseStatus(baseMissiles[1], baseSurvived[1]))
        {
            if(x <= (((SCREEN_WIDTH - 2) / 2) + 1))
            {
                if(baseStatus(baseMissiles[0], baseSurvived[0]) == 1) i = 0;
                else if(baseStatus(baseMissiles[2], baseSurvived[2]) == 1) i = 2;
                else
                {
                    m.active = 0;
                    return m;
                }
            }
            else
            {
                if(baseStatus(baseMissiles[2], baseSurvived[2]) == 1) i = 2;
                else if(baseStatus(baseMissiles[0], baseSurvived[0]) == 1) i = 0;
                else
                {
                    m.active = 0;
                    return m;
                }
            }
        }
        m.startX = baseOffset[i] + 4;
        missilesRemaining[i]--;
        ammoCheck(missilesRemaining[i], baseMissiles, i);
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]); // updates ammunition count in the selected base
    }

    m.currX = m.startX;
    m.currY = m.startY;
    m.trailCoords[m.startX][m.startY] = 1;

    mvprintw(m.currY, m.currX, "*");
    attron(COLOR_PAIR(WHITE));
    mvprintw(m.destY, m.destX, "X");

    return m;
}
int canSplit(struct missile* AIMissiles)
{
    for(int i = 0; i < AI_MISSILES; i++)
    {
        if((AIMissiles[i].currY == SCREEN_HEIGHT/2) && (AIMissiles[i].prevY == (AIMissiles[i].currY - 1)) && (AIMissiles[i].active == 1) && (AIMissiles[i].enableSplit == 1))
        {
            AIMissiles[i].enableSplit = 0;
            return i;
        }
    }
    return -1;
}
void updateMissiles(struct missile* missiles, int total, int clock, int endRound, int roundNumber)
{
    int missileSpeedAI = 2000 - (200 * (roundNumber - 1)); // sets the speed of the AI missile, the lower the value; the faster it goes
    if(missileSpeedAI < 500) missileSpeedAI = 500;
    int explosionSpeed = 750; // sets the speed of the explosion animation, the lower the value; the faster it goes
    int missileSpeedPlayer = 100; // sets the speed of the missile, the lower the value; the faster it goes

    if(endRound)
    {
        missileSpeedPlayer = 100;
        missileSpeedAI = 100;
    }
    for(int i = 0; i < total; i++)
    {
        if(missiles[i].active == 1)
        {
            int flightX = missiles[i].destX - missiles[i].startX;
            int flightY = abs(missiles[i].startY - missiles[i].destY);

            if(reachedDestination(missiles[i])) // if the missile has reached its destination
            {
                if(missiles[i].explosionStage == 1) removeTrail(&missiles[i], missiles, total);

                if(clock % explosionSpeed == 0) // sets the speed of the explosion animation
                {
                    explode(missiles[i].destX, missiles[i].destY, missiles[i].explosionStage);

                    if(missiles[i].explosionStage == 6)
                    {
                        missiles[i].explosionStage = 1;
                        missiles[i].active = 0;
                    }
                    else missiles[i].explosionStage++;

                    continue;
                }
            }
           else if(((total == PLAYER_MISSILES) && (clock % missileSpeedPlayer == 0)) || ((total == AI_MISSILES)  && (clock % missileSpeedAI == 0))) // adjust the number to affect enemy missile speed. The lower; the faster
            {
                missiles[i].prevY = missiles[i].currY;
                missiles[i].prevX = missiles[i].currX;
                if(total == AI_MISSILES && (missiles[i].prevX != -1 && missiles[i].prevY != -1))
                { // prints the previous asterisk in the AI missile trial to red
                    attron(COLOR_PAIR(RED));
                    mvprintw(missiles[i].prevY, missiles[i].prevX, "*");
                }
                if(missiles[i].currX == missiles[i].destX) // moves missile vertically up or down
                {
                    if(total == AI_MISSILES) missiles[i].currY++;
                    else missiles[i].currY--;
                }
                else if(abs(flightX) >= flightY) // moves the missile on trajectories with greater horizontal components using trigonometry
                {
                    double theta = atan2(flightY, abs(flightX));

                    if(total == AI_MISSILES) missiles[i].currY = missiles[i].startY + round(tan(theta) * (abs(missiles[i].currX - missiles[i].startX) + 1));
                    else missiles[i].currY = missiles[i].startY - round(tan(theta) * (abs(missiles[i].currX - missiles[i].startX) + 1));

                    if(flightX > 0) missiles[i].currX++;
                    else missiles[i].currX--;
                }
                else // moves the missile on trajectories with a greater vertical component using trigonometry
                {
                    double theta = atan2(abs(flightX), flightY);

                    if(total == AI_MISSILES) missiles[i].currY++;
                    else missiles[i].currY--;

                    if(flightX > 0) missiles[i].currX = missiles[i].startX + round(tan(theta) * abs(missiles[i].startY - missiles[i].currY));
                    else missiles[i].currX = missiles[i].startX - round(tan(theta) * abs(missiles[i].startY - missiles[i].currY));
                }
                if(total == PLAYER_MISSILES) attron(COLOR_PAIR(BLUE));
                else attron(COLOR_PAIR(WHITE));

                missiles[i].trailCoords[missiles[i].currX][missiles[i].currY] = 1;

                int charRead = mvinch(missiles[i].currY, missiles[i].currX);  // does not delete front of enemy missile if player missile passes over
                if(!(((charRead & A_CHARTEXT) == '*') && ((A_COLOR & charRead) == 0x07000000))) mvprintw(missiles[i].currY, missiles[i].currX, "*");
            }
        }
    }
}
struct missile createAIMissile(int* baseOffset, int* baseMissiles, int* baseSurvived, int* cityOffset, int* cityStatus, int splitIndex, struct missile* AIMissiles)
{
    int totalTargets = 0; // spawning missiles will aim for a target that has not been deactivated
    for(int i = 0; i < 9; i++)
    {
        if(i < 3 && baseStatus(baseMissiles[i], baseSurvived[i]) == 1) totalTargets++;

        if((i >= 3) && (cityStatus[i - 3] == 1)) totalTargets++;
    }
    if(totalTargets == 0) totalTargets = 9;

    int posTargetY[totalTargets];
    int posTargetX[totalTargets];

    if(totalTargets < 9) // populates arrays with possible X, Y targets if some targets have been deactivated
    {
        int index = 0;
        for(int i = 0; i < 9; i++)
        {
            if(i < 3 && baseStatus(baseMissiles[i], baseSurvived[i]) == 1)
            {
                posTargetX[index] = baseOffset[i] + 4;
                index++;
            }
            if(i >= 3 && cityStatus[i - 3] == 1)
            {
                posTargetX[index] = cityOffset[i - 3] + 2;
                index++;
            }
            if(index == totalTargets) break;
        }
    }
    else // populates arrays with all targets since nothing has been deactivated
    {
        for(int i = 0; i < 9; i++)
        {
            if(i < 3) posTargetX[i] = baseOffset[i] + 4;

            if(i >= 3) posTargetX[i] = cityOffset[i - 3] + 2;
        }
    }

    struct missile m;
    m.active = 1;
    m.explosionStage = 1;
    m.enableSplit = 1;

    if(splitIndex == -1) // default values
    {
        m.currY = 1;
        m.startX = rand() % (SCREEN_WIDTH-2) + 1;
        m.currX = m.startX;
        m.startY = 1;
    }
    else
    {
        m.startY = AIMissiles[splitIndex].currY;
        m.startX = AIMissiles[splitIndex].currX;
        m.currX = m.startX;
        m.currY = m.startY;
    }

    m.prevX = m.currX;
    m.prevY = m.currY;

    for(int i = 0; i < SCREEN_WIDTH; i++)
        for(int j = 0; j < SCREEN_HEIGHT; j++)
            m.trailCoords[i][j] = 0;

    int randTarget = rand() % totalTargets; // generates a random integer for

    m.destY = SCREEN_HEIGHT - 7; // selects a random target from the array of possible targets
    m.destX = posTargetX[randTarget];

    attron(COLOR_PAIR(WHITE));
    mvprintw(m.startY,  m.startX, "*");
    m.trailCoords[m.startX][m.startY] = 1;
    return m;
}
void drawBox()
{
    attron(COLOR_PAIR(WHITE));

    printw("+"); // prints top row
    for(int i = 0; i < SCREEN_WIDTH - 2; i++) printw("-");
    printw("+");

    int i; // prints rows 2-101
    for(i = 1; i < SCREEN_HEIGHT - 1; i++)
    {
        mvprintw(i, 0, "|");
        mvprintw(i, SCREEN_WIDTH - 1, "|");
    }

    mvprintw(SCREEN_HEIGHT - 1, 0, "+"); // prints bottom row
    for(int i = 1; i < SCREEN_WIDTH - 1; i++) mvprintw(SCREEN_HEIGHT - 1, i, "-");
    mvprintw(SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, "+");
}
void drawLandscape(int* baseOffset, int* cityOffset, int* missilesRemaining)
{
	attron(COLOR_PAIR(YELLOW));

    for(int i = 0; i < 3; i++)
    {
        for(int j = 1; j < SCREEN_WIDTH - 1; j++) mvprintw(SCREEN_HEIGHT - (2 + i), j, "X"); // prints the ground

        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i], "/XX"); // prints missile bases
        attron(COLOR_PAIR(BLUE));
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]);
        attron(COLOR_PAIR(YELLOW));
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 6, "XX\\");
        mvprintw(SCREEN_HEIGHT - 6, baseOffset[i] + 1, "/XXXXX\\");
        mvprintw(SCREEN_HEIGHT - 7, baseOffset[i] + 2, ".^_^.");
    }

    attron(COLOR_PAIR(CYAN));
    for(int i = 0; i < 6; i++)
    {
        mvprintw(SCREEN_HEIGHT - 5, cityOffset[i], "||[]|"); // prints cities
        mvprintw(SCREEN_HEIGHT - 6, cityOffset[i], "/MMM\\");
    }
}
int main()
{
    int ch;
    MEVENT event;
    int missilesRemaining[] = {MISSILES_PER_BASE, MISSILES_PER_BASE, MISSILES_PER_BASE}; // 7 missiles per base
    int baseOffset[] = {1, SCREEN_WIDTH/2-4, SCREEN_WIDTH-10}; // horizontal offsets for missile bases
    int padding = (SCREEN_WIDTH - (2 + (3 * 9) + (6 * 5))) / 8;
    int cityOffset[] = {baseOffset[0] + 9 + padding, baseOffset[0] + padding * 2 + 14, baseOffset[1] - padding - 5, baseOffset[1] + 9 + padding, baseOffset[1] + padding * 2 + 14, baseOffset[2] - padding - 5}; // horizontal offsets for cities
    int baseMissiles[] = {1, 1, 1}; // sets whether a base is capable of firing
    int baseSurvived[] = {1, 1, 1};
    int cityStatus[] = {1, 1, 1, 1, 1, 1}; // sets whether a city has been hit

    struct missile pMissiles[PLAYER_MISSILES];
    formatMissiles(pMissiles, PLAYER_MISSILES);
    struct missile AIMissiles[AI_MISSILES];
    formatMissiles(AIMissiles, AI_MISSILES);

	initscr();
	resize_term(SCREEN_HEIGHT, SCREEN_WIDTH); // sets terminal dimensions
    cbreak();
	keypad(stdscr, TRUE); // enables button presses (including mouse)
	curs_set(0); // removes the cursor from the terminal
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL); // allows for mouse clicking interrupts
    noecho();

    start_color();
    init_pair(RED, COLOR_RED, COLOR_BLACK);
    init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);

    nodelay(stdscr, TRUE); // allows for character polling without halting the program
    drawBox();
    drawLandscape(baseOffset, cityOffset, missilesRemaining);

    srand(time(0)); // generates a pseudo-random seed for pseudo-random value generation

    attron(COLOR_PAIR(RED));
    int score = 0;
    mvprintw(1, 1, "%d", score);

    int clock = 0; // coordinates timing events within the program
    int totalEnemies = 0; // counts the number of enemies that have spawned in a round
    int missilesDestroyed = 0; // counts the number of AI missiles intercepted in a round
    int result = 0; // determines the state of the game upon the completion of a round
    int roundNumber = 1;

    while(1)
    {
        ch = getch();

        int split = canSplit(AIMissiles);
        if(!(clock %= 12000) || split != -1) // rate at which AI missiles spawn, the lower the value; the faster rate they spawn
        {
            for(int i = 0; i < AI_MISSILES; i++)
            {
                if(AIMissiles[i].active == 0 && totalEnemies < ENEMIES_PER_ROUND)
                {
                   AIMissiles[i] = createAIMissile(baseOffset, baseMissiles, baseSurvived, cityOffset, cityStatus, split, AIMissiles);
                   totalEnemies++;
                   break;
                }
            }
        }
        clock++;

        if(ch == KEY_MOUSE)
            if(getmouse(&event) == OK)
                if(event.bstate & BUTTON1_CLICKED)
				    if(event.x >= 4 && event.x <= SCREEN_WIDTH - 5 && event.y >= 3 && event.y <= SCREEN_HEIGHT - 10)
                        for(int i = 0; i < PLAYER_MISSILES; i++)
                            if(pMissiles[i].active == 0)
                            {
                                pMissiles[i] = createPlayerMissile(event.x, event.y, missilesRemaining, baseOffset, baseMissiles, baseSurvived);
                                break;
                            }

        updateMissiles(pMissiles, PLAYER_MISSILES, clock, endRound(baseMissiles, baseSurvived, cityStatus, pMissiles), roundNumber);
        updateMissiles(AIMissiles, AI_MISSILES, clock, endRound(baseMissiles, baseSurvived, cityStatus, pMissiles), roundNumber);
        deactivateAssets(AIMissiles, AI_MISSILES, baseOffset, cityOffset, baseSurvived, cityStatus);
        testCollisions(AIMissiles, AI_MISSILES, &score, &missilesDestroyed);
        updateScore(&score);

        result = checkGameOver(cityStatus, totalEnemies, AIMissiles, pMissiles, AI_MISSILES, PLAYER_MISSILES);
        if(result != 0)
        {
            printResults(result, &score, baseSurvived, missilesRemaining, missilesDestroyed, roundNumber);
            if(result == -1)
            {
                 clock = 1;
                 ch = 0;
                 while(1)
                 {
                    ;
                 }
            }

           if(result == 1)
           {
                formatMissiles(pMissiles, PLAYER_MISSILES); // resets variables in preparation for the next round
                formatMissiles(AIMissiles, AI_MISSILES);
                clock = 0;
                totalEnemies = 0;
                missilesDestroyed = 0;
                result = 0;
                roundNumber++;

                for(int i = 0; i < 3; i++)
                {
                    baseMissiles[i] = 1;
                    baseSurvived[i] = 1;
                    missilesRemaining[i] = MISSILES_PER_BASE;

                    attron(COLOR_PAIR(YELLOW));
                    mvprintw(SCREEN_HEIGHT - 5, baseOffset[i], "/XX"); // prints missile bases
                    attron(COLOR_PAIR(BLUE));
                    mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]);
                    attron(COLOR_PAIR(YELLOW));
                    mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 6, "XX\\");
                    mvprintw(SCREEN_HEIGHT - 6, baseOffset[i] + 1, "/XXXXX\\");
                    mvprintw(SCREEN_HEIGHT - 7, baseOffset[i] + 2, ".^_^.");
                }

                for(int i = 0; i < 9; i++)
                {
                    if(i == 0 || i == 2 || i == 4 || i >= 6)
                    {
                        for(int j = 1; j < SCREEN_WIDTH - 1; j++)
                        {
                            mvprintw(SCREEN_HEIGHT/2 - 5 + i, j, " ");
                        }
                    }
                }
            }
        }
    }
	endwin();
	return 0;
}
