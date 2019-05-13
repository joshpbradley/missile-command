#define NCURSES_MOUSE_VERSION 2
#define SCREEN_WIDTH 101
#define SCREEN_HEIGHT 42
#include <curses.h>
#include <math.h>
#include <unistd.h>

struct playerMissile // missiles fired by the player
{
    int active; // determines whether the missile is currently in motion
    int currX; // the current position of the missile
    int currY;
    int startX; // the initial location of the missile
    int startY;
    int destX; // the destination of the missile
    int destY;
    int trailCoords[SCREEN_WIDTH][SCREEN_HEIGHT]; // marks the locations of the trail
    int explosionStage; // determines the animation stage of the missile's explosion event
};
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
        mvprintw(y - 1, x,  "*");
        mvprintw(y, x - 1, "***");
        mvprintw(y + 1, x,  "*");
    }
    else if(stage == 4)
    {
        attron(COLOR_PAIR(3));
        mvprintw(y - 2, x - 2, "* * *");
        mvprintw(y - 1, x - 1,  "***");
        mvprintw(y, x - 2,     "*****");
        mvprintw(y + 1, x - 1,  "***");
        mvprintw(y + 2, x - 2, "* * *");
    }
    else if(stage == 5)
    {
        attron(COLOR_PAIR(1));
        mvprintw(y - 2, x - 2, "     ");
        mvprintw(y - 1, x - 1,  " * ");
        mvprintw(y, x - 2,     " *** ");
        mvprintw(y + 1, x - 1,  " * ");
        mvprintw(y + 2, x - 2, "     ");
    }
    else
    {
        mvprintw(y - 1, x,  " ");
        mvprintw(y, x - 1, "   ");
        mvprintw(y + 1, x,  " ");
    }
    usleep(65000);
}
struct playerMissile createPlayerMissile(int x, int y, int* missilesRemaining, int* baseOffset, int* baseStatus)
{
    struct playerMissile m;

    m.active = 1;
    m.destX = x;
    m.destY = y;
    m.startY = SCREEN_HEIGHT - 8;
    m.explosionStage = 1;

    for(int i = 0; i < SCREEN_WIDTH; i++)
        for(int j = 0; j < SCREEN_HEIGHT; j++)
            m.trailCoords[i][j] = 0;

    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
	attron(COLOR_PAIR(1));

	int i;
    if(m.destX <= (SCREEN_WIDTH - 2) / 3 + 1)  // selects base for the portion of the screen clicked
    {
        for(i = 0; i < 3; i++)
        {
            if(baseStatus[i] == 1)
           {
               m.startX = baseOffset[i] + 4;
               missilesRemaining[i]--;
               break;
           }
        }
        if(i == 3) // unable to fire a missile
        {
            m.active = 0;
            return m;
        }

       if(missilesRemaining[i] <= 0)
            baseStatus[i] = 0;

        attron(COLOR_PAIR(2));
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]);
        attron(COLOR_PAIR(1));
    }
    else if(x >= (SCREEN_WIDTH - 2) / 3 * 2 + 1)
    {
        for(i = 2; i > -1; i--)
        {
            if(baseStatus[i] == 1)
            {
               m.startX = baseOffset[i] + 4;
               missilesRemaining[i]--;
               break;
           }
        }
        if(i == -1)
        {
            m.active = 0;
            return m;
        }

       if(missilesRemaining[i] <= 0)
            baseStatus[i] = 0;

        attron(COLOR_PAIR(2));
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[i] + 3, "%03d", missilesRemaining[i]);
        attron(COLOR_PAIR(1));
    }
    else
    {
        int j;

        for(i = 0; i < 3; i++)
        {
            j = (i + 1) % 3;
            if(baseStatus[j] == 1)
           {
               m.startX = baseOffset[j] + 4;
               missilesRemaining[j]--;
               break;
           }
        }
        if(i == 3)
        {
            m.active = 0;
            return m;
        }

        if(missilesRemaining[j] <= 0)
            baseStatus[j] = 0;

        attron(COLOR_PAIR(2));
        mvprintw(SCREEN_HEIGHT - 5, baseOffset[j] + 3, "%03d", missilesRemaining[j]);
        attron(COLOR_PAIR(1));
    }
    attroff(COLOR_PAIR(1));

    m.currX = m.startX;
    m.currY = m.startY;

    mvprintw(m.destY, m.destX, "X");
    attron(COLOR_PAIR(2));
    mvprintw(m.currY, m.currX, "*");
    m.trailCoords[m.startX][m.startY] = 1;
    m.trailCoords[m.currX][m.currY] = 1;
    usleep(12000);

    return m;
}
void updatePlayerMissiles(struct playerMissile* pMissiles)
{
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    attron(COLOR_PAIR(2));
    for(int i = 0; i < 5; i++)
    {
        if(pMissiles[i].active == 1)
        {
            int flightX = pMissiles[i].destX - pMissiles[i].startX;
            int flightY = pMissiles[i].startY - pMissiles[i].destY;

            if(pMissiles[i].currX == pMissiles[i].destX && pMissiles[i].currY == pMissiles[i].destY)
            {
                for(int j = 0; j < SCREEN_WIDTH; j++)
                {
                    for(int k = 0; k < SCREEN_HEIGHT; k++)
                    {
                        if(pMissiles[i].trailCoords[j][k] == 1)
                        {
                            mvprintw(k, j, " ");
                            pMissiles[i].trailCoords[j][k] = 0;
                        }
                    }
                }
                explode(pMissiles[i].destX, pMissiles[i].destY, pMissiles[i].explosionStage);

                if(pMissiles[i].explosionStage == 6)
                {
                    pMissiles[i].active = 0;
                    pMissiles[i].explosionStage = 1;
                }
                else
                {
                    pMissiles[i].explosionStage++;
                }
                continue;
            }
            else if(pMissiles[i].currX == pMissiles[i].destX)
            {
                pMissiles[i].currY--;
            }
            else if(abs(flightX) >= flightY)
            {
                double theta = atan2(flightY, abs(flightX));
                pMissiles[i].currY = pMissiles[i].startY - round(tan(theta) * (abs(pMissiles[i].currX - pMissiles[i].startX) + 1));

                if(flightX > 0)
                    pMissiles[i].currX++;
                else
                    pMissiles[i].currX--;
            }
            else
            {
                double theta = atan2(abs(flightX), flightY);
                pMissiles[i].currY--;

                if(flightX > 0)
                   pMissiles[i].currX = pMissiles[i].startX + round(tan(theta) * abs(pMissiles[i].startY - pMissiles[i].currY));
                else
                    pMissiles[i].currX = pMissiles[i].startX - round(tan(theta) * abs(pMissiles[i].startY - pMissiles[i].currY));
            }
            attron(COLOR_PAIR(2));
            mvprintw(pMissiles[i].currY, pMissiles[i].currX, "*");
            pMissiles[i].trailCoords[pMissiles[i].currX][pMissiles[i].currY] = 1;
        }
    }
}
void drawBox()
{
    printw("+");                                    // prints top row
    for(int i = 0; i < SCREEN_WIDTH - 2; i++)
        printw("-");

    printw("+");

    int i;                                          // prints rows 2-101
    for(i = 1; i < SCREEN_HEIGHT - 1; i++)
    {
        mvprintw(i, 0, "|");
        mvprintw(i, SCREEN_WIDTH - 1, "|");
    }

    mvprintw(SCREEN_HEIGHT - 1, 0, "+");           // prints bottom row
    for(int i = 1; i < SCREEN_WIDTH - 1; i++)
        mvprintw(SCREEN_HEIGHT - 1, i, "-");
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
            mvprintw(SCREEN_HEIGHT - (2 + i), j, "X"); // prints the ground

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
}
int main()
{
    int ch;
    MEVENT event;
    int missilesRemaining[] = {10, 10, 10}; // 10 missiles per base
    int baseOffset[] = {1, (SCREEN_WIDTH-2)/2-4, SCREEN_WIDTH-10}; // horizontal offsets for missile bases
    int cityOffset[] = {(SCREEN_WIDTH-2)/8, (SCREEN_WIDTH-2)/4, (SCREEN_WIDTH-2)/8*3, (SCREEN_WIDTH-2)/8*5, (SCREEN_WIDTH-2)/4*3, (SCREEN_WIDTH-2)/8*7}; // horizontal offsets for cities
    int baseStatus[] = {1, 1, 1}; // sets whether a base is capable of firing
    struct playerMissile pMissiles[5] = {
                                            {0, -1, -1, -1, -1, -1, -1},
                                            {0, -1, -1, -1, -1, -1, -1},
                                            {0, -1, -1, -1, -1, -1, -1},
                                            {0, -1, -1, -1, -1, -1, -1},
                                            {0, -1, -1, -1, -1, -1, -1}
                                        }; // 5 missiles capable of being airborne at once

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

    while(1)
    {
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
                        for(int i = 0; i < 5; i++)
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
        updatePlayerMissiles(pMissiles);
        usleep(12000);
    }
	endwin();

	return 0;
}
