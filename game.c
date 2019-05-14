#define NCURSES_MOUSE_VERSION 2
#define SCREEN_WIDTH 101
#define SCREEN_HEIGHT 42
#define AI_MISSILES 10
#define PLAYER_MISSILES 5
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
};
void formatMissiles(struct missile* missiles, int total)
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
        missiles[i].prevY = 1;
        missiles[i].explosionStage = 1;
        for(int col = 0; col < SCREEN_HEIGHT; col++)
        {
            for(int row = 0; row < SCREEN_WIDTH; row++)
            {
                missiles[i].trailCoords[row][col] = 0;
            }
        }
    }
}
void ammoCheck(int missiles, int* baseStatus, int index)
{
     if(missiles <= 0)
    {
        baseStatus[index] = 0;
    }
}
void delay(int microSecs) // causes the program to temporarily halt
{
    const time_t start = clock();
    time_t current;
    do
    {
        current = clock();
    }while((double)(current - start)/CLOCKS_PER_SEC*1000000 < microSecs);
}
void explode(int x, int y, int stage) // explosion animation for the player missiles
{
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    if(stage == 1)
    {
        attron(COLOR_PAIR(3));
        mvprintw(y, x,  "*");
    }
    else if(stage == 2)
    {
        attron(COLOR_PAIR(1));
        mvprintw(y - 1, x - 1,  " * ");
        mvprintw(y, x - 1,      "***");
        mvprintw(y + 1, x - 1,  " * ");
    }
    else if(stage == 3)
    {
        attron(COLOR_PAIR(3));
        mvprintw(y - 2, x - 2, "* * *");
        mvprintw(y - 1, x - 2, " *** ");
        mvprintw(y, x - 3,    "*******");
        mvprintw(y + 1, x - 2, " *** ");
        mvprintw(y + 2, x - 2, "* * *");
    }
    else if(stage == 4)
    {
        attron(COLOR_PAIR(1));
        mvprintw(y - 2, x - 2, "     ");
        mvprintw(y - 1, x - 1,  " * ");
        mvprintw(y, x - 3,    "  ***  ");
        mvprintw(y + 1, x - 1,  " * ");
        mvprintw(y + 2, x - 2, "     ");
    }
    else if(stage == 5)
    {
        attron(COLOR_PAIR(3));
        mvprintw(y - 1, x,  " ");
        mvprintw(y, x - 1, " * ");
        mvprintw(y + 1, x,  " ");
    }
    else
    {
        mvprintw(y, x, " ");
    }
}
struct missile createPlayerMissile(int x, int y, int* missilesRemaining, int* baseOffset, int* baseStatus)
{
    struct missile m;

    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    attron(COLOR_PAIR(2));

    m.active = 1;
    m.destX = x;
    m.destY = y;
    m.prevX = -1; // not a necessary property for this missile type
    m.prevY = -1;
    m.startY = SCREEN_HEIGHT - 8;
    m.explosionStage = 1;

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
            if(baseStatus[i] == 1)
           {
               m.startX = baseOffset[i] + 4;
               missilesRemaining[i]--;
               break;
           }
        }
        ammoCheck(missilesRemaining[i], baseStatus, i);
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
            if(baseStatus[i] == 1)
            {
               m.startX = baseOffset[i] + 4;
               missilesRemaining[i]--;
               break;
           }
        }
        ammoCheck(missilesRemaining[i], baseStatus, i);
        if(i == -1) // unable to fire a missile
        {
            m.active = 0;
            return m;
        }

        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]); // updates ammunition count in the selected base
    }
    else // click in the centre of the viewport
    {
        int j;

        for(i = 0; i < 3; i++) // base preference: 1, 2, 0
        {
            j = (i + 1) % 3;
            if(baseStatus[j] == 1)
           {
               m.startX = baseOffset[j] + 4;
               missilesRemaining[j]--;
               break;
           }
        }
        ammoCheck(missilesRemaining[j], baseStatus, j);
        if(i == 3) // unable to fire a missile
        {
            m.active = 0;
            return m;
        }
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[j] + 3, "%03d", missilesRemaining[j]); // updates ammunition count in the selected base
    }

    m.currX = m.startX;
    m.currY = m.startY;
    m.trailCoords[m.startX][m.startY] = 1;

    mvprintw(m.currY, m.currX, "*");
    attroff(COLOR_PAIR(2));
    mvprintw(m.destY, m.destX, "X");

    return m;
}
void updateMissiles(struct missile* missiles, int total, int missileCue)
{
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    for(int i = 0; i < total; i++)
    {
        if(missiles[i].active == 1)
        {
            int flightX = missiles[i].destX - missiles[i].startX;
            int flightY = abs(missiles[i].startY - missiles[i].destY);

            if(missiles[i].currX == missiles[i].destX && missiles[i].currY == missiles[i].destY) // if the missile has reached its destination
            {
                if(missileCue % 10 == 0)
                {
                    if(missiles[i].explosionStage == 1)
                    {
                        for(int j = 0; j < SCREEN_WIDTH; j++)
                        {
                            for(int k = 0; k < SCREEN_HEIGHT; k++)
                            {
                                if(missiles[i].trailCoords[j][k] == 1)
                                {
                                    int count = 0;

                                    for(int l = 0; l < total; l++)
                                    {
                                        if(missiles[l].trailCoords[j][k] == 1)
                                        {
                                            count++;
                                        }
                                    }

                                    missiles[i].trailCoords[j][k] = 0;

                                    if(count == 1) // bug, some counts are equal to 2
                                    {
                                        mvprintw(k, j, " ");
                                    }
                                }
                            }
                        }
                    }

                    explode(missiles[i].destX, missiles[i].destY, missiles[i].explosionStage);

                    if(missiles[i].explosionStage == 6)
                    {
                        missiles[i].active = 0;
                        missiles[i].explosionStage = 1;
                    }
                    else
                    {
                        missiles[i].explosionStage++;
                    }
                    continue;
                }
            }
            else if(total == PLAYER_MISSILES || missileCue % 45 == 0) // adjust the number to affect enemy missile speed. The lower; the faster
            {
                if(total == AI_MISSILES && (missiles[i].prevX != -1 && missiles[i].prevY != -1))
                {
                    attron(COLOR_PAIR(3));
                    mvprintw(missiles[i].prevY, missiles[i].prevX, "*");
                }
                if(missiles[i].currX == missiles[i].destX)
                {
                    if(total == AI_MISSILES)
                    {
                        missiles[i].currY++;
                    }
                    else
                    {
                        missiles[i].currY--;
                    }
                }
                else if(abs(flightX) >= flightY)
                {
                    double theta = atan2(flightY, abs(flightX));

                    if(total == AI_MISSILES)
                    {
                        missiles[i].currY = missiles[i].startY + round(tan(theta) * (abs(missiles[i].currX - missiles[i].startX) + 1));
                    }
                    else
                    {
                        missiles[i].currY = missiles[i].startY - round(tan(theta) * (abs(missiles[i].currX - missiles[i].startX) + 1));
                    }

                    if(flightX > 0)
                    {
                        missiles[i].currX++;
                    }
                    else
                    {
                        missiles[i].currX--;
                    }
                }
                else
                {
                    double theta = atan2(abs(flightX), flightY);

                    if(total == AI_MISSILES)
                    {
                        missiles[i].currY++;
                    }
                    else
                    {
                        missiles[i].currY--;
                    }

                    if(flightX > 0)
                    {
                        missiles[i].currX = missiles[i].startX + round(tan(theta) * abs(missiles[i].startY - missiles[i].currY));
                    }
                    else
                    {
                         missiles[i].currX = missiles[i].startX - round(tan(theta) * abs(missiles[i].startY - missiles[i].currY));
                    }
                }
                if(total == PLAYER_MISSILES)
                {
                    attron(COLOR_PAIR(2));
                }
                else
                {
                    attroff(COLOR_PAIR(1));
                    attroff(COLOR_PAIR(2));
                    attroff(COLOR_PAIR(3));
                    missiles[i].prevY = missiles[i].currY;
                    missiles[i].prevX = missiles[i].currX;
                }

                missiles[i].trailCoords[missiles[i].currX][missiles[i].currY] = 1;
                mvprintw(missiles[i].currY, missiles[i].currX, "*");
            }
        }
    }
}
struct missile createAIMissile(int* baseOffset, int* cityOffset)
{
    int targetX[] = {baseOffset[0] + 4, cityOffset[0] + 2, cityOffset[1] + 2, cityOffset[2] + 2, baseOffset[1] + 4, cityOffset[3] + 2, cityOffset[4] + 2, cityOffset[5] + 2, baseOffset[2] + 4};
    int targetY[] = {SCREEN_HEIGHT - 8, SCREEN_HEIGHT - 7, SCREEN_HEIGHT - 7, SCREEN_HEIGHT - 7, SCREEN_HEIGHT - 8, SCREEN_HEIGHT - 7, SCREEN_HEIGHT - 7, SCREEN_HEIGHT - 7, SCREEN_HEIGHT - 8};
    struct missile m;
    m.active = 1;
    m.currY = 1;
    m.startX = rand() % (SCREEN_WIDTH-2) + 1;
    m.currX = m.startX;
    m.startY = 1;
    m.prevX = m.currX;
    m.prevY = m.currY;
    m.explosionStage = 1;

    for(int i = 0; i < SCREEN_WIDTH; i++)
    {
        for(int j = 0; j < SCREEN_HEIGHT; j++)
        {
            m.trailCoords[i][j] = 0;
        }
    }

    int randTarget = rand() % 9;

    m.destY = targetY[randTarget];
    m.destX = targetX[randTarget];

    attroff(COLOR_PAIR(1));
    attroff(COLOR_PAIR(2));
    mvprintw(m.startY,  m.startX, "*");
    m.trailCoords[m.startX][m.startY] = 1;
    return m;
}
void drawBox()
{
    printw("+");                                    // prints top row
    for(int i = 0; i < SCREEN_WIDTH - 2; i++)
    {
        printw("-");
    }

    printw("+");

    int i;                                          // prints rows 2-101
    for(i = 1; i < SCREEN_HEIGHT - 1; i++)
    {
        mvprintw(i, 0, "|");
        mvprintw(i, SCREEN_WIDTH - 1, "|");
    }

    mvprintw(SCREEN_HEIGHT - 1, 0, "+");           // prints bottom row
    for(int i = 1; i < SCREEN_WIDTH - 1; i++)
    {
        mvprintw(SCREEN_HEIGHT - 1, i, "-");
    }
    mvprintw(SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, "+");
}
void drawLandscape(int* baseOffset, int* cityOffset, int* missilesRemaining)
{
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
	attron(COLOR_PAIR(1));

    for(int i = 0; i < 3; i++)
    {
        for(int j = 1; j < SCREEN_WIDTH - 1; j++)
        {
            mvprintw(SCREEN_HEIGHT - (2 + i), j, "X"); // prints the ground
        }

        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i], "/XX"); // prints missile bases
        attron(COLOR_PAIR(2));
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]);
        attron(COLOR_PAIR(1));
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 6, "XX\\");
        mvprintw(SCREEN_HEIGHT - 6, baseOffset[i] + 1, "/XXXXX\\");
        mvprintw(SCREEN_HEIGHT - 7, baseOffset[i] + 2, ".^_^.");
    }
    for(int i = 0; i < 6; i++)
    {
        mvprintw(SCREEN_HEIGHT - 5, cityOffset[i], "|___|"); // prints cities
        mvprintw(SCREEN_HEIGHT - 6, cityOffset[i], "/'''\\");
    }
    attroff(COLOR_PAIR(1));
}
int main()
{
    int ch;
    MEVENT event;
    int missilesRemaining[] = {10, 10, 10}; // 10 missiles per base
    int baseOffset[] = {1, (SCREEN_WIDTH-2)/2-4, SCREEN_WIDTH-10}; // horizontal offsets for missile bases
    int cityOffset[] = {(SCREEN_WIDTH-2)/8, (SCREEN_WIDTH-2)/4, (SCREEN_WIDTH-2)/8*3, (SCREEN_WIDTH-2)/8*5, (SCREEN_WIDTH-2)/4*3, (SCREEN_WIDTH-2)/8*7}; // horizontal offsets for cities
    int baseStatus[] = {1, 1, 1}; // sets whether a base is capable of firing
    int cityStatus[] = {1, 1, 1, 1, 1, 1}; // sets whether a city has been hit

    struct missile pMissiles[PLAYER_MISSILES]; // 5 missiles capable of being airborne at once; // INITIALISE!
    formatMissiles(pMissiles, PLAYER_MISSILES);
    struct missile AIMissiles[AI_MISSILES];
    formatMissiles(AIMissiles, AI_MISSILES);

	initscr();
	resize_term(SCREEN_HEIGHT, SCREEN_WIDTH); // sets terminal dimensions
    cbreak();
	keypad(stdscr, TRUE); // enables button presses (including mouse)
	curs_set(0); // removes the cursor from the terminal
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    start_color();
    nodelay(stdscr, TRUE); // allows for character polling without halting the program
    drawBox();
    drawLandscape(baseOffset, cityOffset, missilesRemaining);
    srand(time(0));
    int missileCue = 0;

    while(1)
    {
        if(missileCue % 150 == 0)
        {
            for(int i = 0; i < AI_MISSILES; i++)
            {
                if(AIMissiles[i].active == 0)
                {
                   AIMissiles[i] = createAIMissile(baseOffset, cityOffset);
                   break;
                }
            }
            missileCue = 0;
        }
        missileCue++;

        noecho();
        ch = getch();
        if(ch == KEY_MOUSE)
        {
            if(getmouse(&event) == OK)
            {
                if(event.bstate & BUTTON1_CLICKED)
				{
				    if(event.x >= 3 && event.x <= SCREEN_WIDTH - 4 && event.y >= 3 && event.y <= SCREEN_HEIGHT - 10)
                    {
                        for(int i = 0; i < PLAYER_MISSILES; i++)
                        {
                            if(pMissiles[i].active == 0)
                            {
                                pMissiles[i] = createPlayerMissile(event.x, event.y, missilesRemaining, baseOffset, baseStatus);
                                break;
                            }
                        }
                    }
                }
            }
        }
        updateMissiles(pMissiles, PLAYER_MISSILES, missileCue);
        delay(6500);
        updateMissiles(AIMissiles, AI_MISSILES, missileCue);
        delay(6500);
    }
	endwin();
	return 0;
}
