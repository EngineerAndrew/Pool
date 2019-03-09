#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "SDL.h"
#include "SDL_ttf.h"

#include "shapes.h"


const int SCREEN_WIDTH = 1250;
const int SCREEN_HEIGHT = 700;

enum PrimaryState
{
	MENU, NINEBALL, EIGHTBALL
};

enum SecondaryState
{
	SETUP, ACTIVE, WAITING, WIN, RULES
};

//START OF STRUCTURE DEFINITIONS
struct Vector2
{
	double x;
	double y;

	double get_magnitude()
	{
		double result = 0;
		result = sqrt(pow(x, 2) + pow(y, 2));
		return result;
	}
};

struct TrajectoryLine
{
	Point2 InitialPos;
	Vector2 PosDifference;

	void draw(SDL_Renderer* Renderer)
	{
		//NOTE:300 is the max length of the trajectory line
		SDL_SetRenderDrawColor(Renderer, (200*PosDifference.get_magnitude()/300), 200 - (200*PosDifference.get_magnitude()/300), 0, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(Renderer, InitialPos.x, InitialPos.y, InitialPos.x + PosDifference.x, InitialPos.y + PosDifference.y);

		SDL_SetRenderDrawColor(Renderer, 150, 150, 150, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(Renderer, InitialPos.x, InitialPos.y, InitialPos.x - PosDifference.x, InitialPos.y - PosDifference.y);
	}
};

struct Ball
{
	Circle Circ;
	Color Color;
	Vector2 Speed;
	bool pocketed;

	bool is_pocketed(Circle Pockets[])
	{
		bool result = false;

		for (int i = 0; i < 6; i++)
		{
			Vector2 CollisionDistance = { (Circ.Pos.x - Pockets[i].Pos.x), (Circ.Pos.y - Pockets[i].Pos.y) };

			if (CollisionDistance.get_magnitude() < Pockets[i].radius)
			{
				result = true;
			}
		}
		return result;
	}

	void move()
	{
		Circ.Pos.x += Speed.x;
		Circ.Pos.y += Speed.y;

		double acceleration = 0.06;
		double magnitude = Speed.get_magnitude();
		double ratio = 0;

		if (magnitude >= acceleration)
		{
			ratio = (magnitude - acceleration) / magnitude;
			magnitude -= acceleration;
		}
		else
		{
			magnitude = 0;
		}

		Speed.x *= ratio;
		Speed.y *= ratio;
	}

	void draw(SDL_Renderer* Renderer, bool isStriped)
	{
		if (isStriped)
		{
			for (int y = (Circ.Pos.y - Circ.radius); y <= (Circ.Pos.y + Circ.radius); y++)
			{
				if (y > Circ.Pos.y - Circ.radius / 3 && y < Circ.Pos.y + Circ.radius / 3)
				{
					SDL_SetRenderDrawColor(Renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
				}
				else
				{
					SDL_SetRenderDrawColor(Renderer, Color.r, Color.g, Color.b, SDL_ALPHA_OPAQUE);
				}
				for (int x = (Circ.Pos.x - Circ.radius); x <= (Circ.Pos.x + Circ.radius); x++)
				{
					if (pow((x - Circ.Pos.x), 2) + pow((y - Circ.Pos.y), 2) <= pow(Circ.radius, 2))
					{
						SDL_RenderDrawPoint(Renderer, x, y);
					}
				}
			}
		}
		else
		{
			draw_circle(Renderer, Circ, Color);
		}
	}
};

struct Table
{
	Circle Pockets[6];
	Rect Rect;

	void draw(SDL_Renderer* Renderer)
	{
		draw_rect(Renderer, {{(Rect.Pos.x - 30), (Rect.Pos.y - 30)}, (Rect.width + 60), (Rect.height + 60)},  {133, 75, 0});
		draw_rect(Renderer, Rect, {40, 117, 81});

		for (int i = 0; i < 18; i++)
		{
			SDL_SetRenderDrawColor(Renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

			if (i < 10)
			{
				SDL_RenderDrawPoint(Renderer, (Rect.Pos.x - 15 + (i%2)*(Rect.width + 30)), (Rect.Pos.y - 15 + (i%5)*(Rect.height + 30)/4));
			}

			SDL_RenderDrawPoint(Renderer, (Rect.Pos.x - 15 + (i%9)*(Rect.width + 30)/8), (Rect.Pos.y - 15 + (i%2)*(Rect.height + 30)));
		}

		for (int i = 0; i < 6; i++)
		{
			Color PocketColor = {0, 0, 0};
			draw_circle(Renderer, Pockets[i], PocketColor);
		}
	}
};

struct Button
{
	Rect Rect;
	char* text;
	
	bool mouse_in_range(int mouseX, int mouseY)
	{
		int result = false;
		
		if (mouseX > Rect.Pos.x - Rect.width/2 && mouseX < Rect.Pos.x + Rect.width/2 && mouseY > Rect.Pos.y - Rect.height/2 && mouseY < Rect.Pos.y + Rect.height/2)
		{
			result = true;
		}
		
		return result;
	}
	
	void draw(SDL_Renderer* Renderer, TTF_Font* font)
	{
		draw_rect(Renderer, { {Rect.Pos.x - Rect.width / 2, Rect.Pos.y - Rect.height / 2}, Rect.width, Rect.height }, { 255, 255, 255 });
		draw_rect(Renderer, {{Rect.Pos.x - Rect.width/2 + 5, Rect.Pos.y - Rect.height/2 + 5}, Rect.width - 10, Rect.height - 10}, {0, 0, 0});
		draw_text(Renderer, Rect.Pos, text, font, {255, 255, 255});
	}
};

//START OF GLOBAL FUNCTION DEFINITIONS
void reset_nine_ball(Ball* Balls, Point2 Reference)
{
	double r = (double)Balls[1].Circ.radius;//NOTE: since all the balls will have the same radius(minus the cue ball), I make r equal to that value for convenience

	Point2 PositionList[] = { {Reference.x, Reference.y},
							  {Reference.x - 2*r*(double)sqrt(3), Reference.y},
							  {Reference.x - r*(double)sqrt(3), Reference.y + r},
							  {Reference.x - r*(double)sqrt(3), Reference.y - r},
							  {Reference.x - 2*r*(double)sqrt(3), Reference.y + 2*r},
							  {Reference.x - 2*r*(double)sqrt(3), Reference.y - 2*r},
							  {Reference.x - 3*r*(double)sqrt(3), Reference.y + r},
							  {Reference.x - 3*r*(double)sqrt(3), Reference.y - r},
							  {Reference.x - 4*r*(double)sqrt(3), Reference.y} };


	Ball* PlacedBalls[9] = {&Balls[1], &Balls[9]};

	Balls[9].Circ.Pos = PositionList[1];
	Balls[1].Circ.Pos = PositionList[0];
	
	for (int i = 0; i < 7; i++)//NOTE: I go through the loop 7 times since the position of the cue ball, nine ball and one ball are always predetermined
	{
		int randomBall;
		bool alreadyPlaced = false;

		do
		{
			randomBall = 2 + (rand() % 7);//NOTE: Gives a random number from 2 to 8, this makes it so the rest of the balls are given a random position 
			alreadyPlaced = false;

			for (int j = 0; j < sizeof(PlacedBalls) / sizeof(PlacedBalls[0]); j++)
			{
				if (&Balls[randomBall] == PlacedBalls[j])
				{
					alreadyPlaced = true;
				}
				if(j == sizeof(PlacedBalls)/sizeof(PlacedBalls[0]) - 1 && !alreadyPlaced)
				{
					PlacedBalls[i+2] = &Balls[randomBall];
				}
			}
		} while (alreadyPlaced);
		
		Balls[randomBall].Circ.Pos = PositionList[i+2];
	}
}

void reset_eight_ball(Ball* Balls, Point2 Reference)
{
	double r = (double)Balls[1].Circ.radius;

	Point2 PositionList[] = { {Reference.x, Reference.y},
							  {Reference.x - ((double)sqrt(3)*r), Reference.y + r},
							  {Reference.x - ((double)sqrt(3)*r), Reference.y - r},
							  {Reference.x - 2*((double)sqrt(3)*r), Reference.y + 2*r},
							  {Reference.x - 2*((double)sqrt(3)*r), Reference.y - 2*r},
							  {Reference.x - 3*((double)sqrt(3)*r), Reference.y + r},
							  {Reference.x - 3*((double)sqrt(3)*r), Reference.y - r},
							  {Reference.x - 3*((double)sqrt(3)*r), Reference.y + 3*r},
							  {Reference.x - 3*((double)sqrt(3)*r), Reference.y - 3*r},
							  {Reference.x - 4*((double)sqrt(3)*r), Reference.y},
							  {Reference.x - 4*((double)sqrt(3)*r), Reference.y + 2*r},
							  {Reference.x - 4*((double)sqrt(3)*r), Reference.y - 2*r},
							  {Reference.x - 4*((double)sqrt(3)*r), Reference.y + 4*r},
							  {Reference.x - 4*((double)sqrt(3)*r), Reference.y - 4*r},
							  {Reference.x - 2*((double)sqrt(3)*r), Reference.y} };

	Ball* PlacedBalls[15] = {&Balls[1], &Balls[8], &Balls[9]};

	Balls[8].Circ.Pos = PositionList[14];
	Balls[1].Circ.Pos = PositionList[12];
	Balls[9].Circ.Pos = PositionList[13];

	for (int i = 0; i < 12; i++)//NOTE: I go through the loop 12 times since the position of the cue ball, eight ball and two others are always predetermined
	{
		int randomBall;
		bool alreadyPlaced = false;

		do
		{
			randomBall =  2 + (rand() % 14);//NOTE: Gives a random number from 1 to 11, this makes it so the rest of the balls are given a random position 
			alreadyPlaced = false;

			for (int j = 0; j < sizeof(PlacedBalls) / sizeof(PlacedBalls[0]); j++)
			{
				if (&Balls[randomBall] == PlacedBalls[j])
				{
					alreadyPlaced = true;
				}
				if (j == sizeof(PlacedBalls) / sizeof(PlacedBalls[0]) - 1 && !alreadyPlaced)
				{
					PlacedBalls[i + 3] = &Balls[randomBall];
				}
			}
		} while (alreadyPlaced);

		Balls[randomBall].Circ.Pos = PositionList[i];
	}
}

//START OF MAIN FUNCTION
int main(int argc, char* args[])
{
	//INITIAL SETUP FOR MAIN WHILE LOOP
	srand(time(NULL)); //setting the random number generator seed for later

	SDL_Window* window = NULL;
	SDL_Surface* screenSurface = NULL;
	SDL_Renderer* renderer = NULL;

	window = SDL_CreateWindow("Pool", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	screenSurface = SDL_GetWindowSurface(window);
	renderer = SDL_CreateRenderer(window, -1, 0);

	if (TTF_Init() == -1)
	{
		exit(1);
	}

	TTF_Font* font;
	font = TTF_OpenFont("..\\data\\Actor-Regular.ttf", 12);
	if (!font)
	{
		exit(1);
	}


	//CREATING ALL ELEMENTS FOR MENU
	Button NineBallButton = { {{SCREEN_WIDTH / 2, SCREEN_HEIGHT * 3 / 4 - 90}, 120, 60}, "Nine Ball" };
	Button EightBallButton = { {{SCREEN_WIDTH / 2, SCREEN_HEIGHT * 3 / 4}, 120, 60}, "Eight Ball"};
	

	//CREATING ALL ELEMENTS FOR NINEBALL/EIGHTBALL
	bool mouseHeld = false;
	TrajectoryLine Trajectory = {};

	Color BallColors[] = { {255, 255, 255},  {252, 238, 35}, {25, 198, 255}, {255, 20, 20}, {22, 24, 140}, {255, 130, 5}, {28, 138, 0}, {158, 0, 0}, {0, 0, 0}, {132, 0, 158} };
	Ball* Balls;
	int BallsSize = 0;
	Ball* CueBall = nullptr;
	Ball* ObjectiveBall = nullptr;

	Table Table = { {}, {{50, 150}, 1000, 500} };

	bool ballWasPocketed = false;
	int currentTurn = 0;
	bool breakShot = true;
	int playerOneAssignedBall = 0;
	int playerTwoAssignedBall = 0;

	Button MenuButton = { {SCREEN_WIDTH - 85, 45, 130, 50}, "Main Menu" };
	Button RuleButton = { {SCREEN_WIDTH - 85, 110, 130, 50}, "Rules" };
	
	//FINAL SETUP FOR MAIN LOOP
	SDL_Event e; //creating the event handler
	bool quit = false; //main loop flag
	PrimaryState GameState = MENU; //Setting the initial game state
	SecondaryState GamePhase = SETUP; //Setting the initial phase within the game state

	//Framerate setup
	unsigned int currentTime, lastTime;
	lastTime = SDL_GetTicks();

	//START OF MAIN GAME LOOP
	while (!quit)
	{
		//START OF MENU STATE
		if (GameState == MENU)
		{
			if (GamePhase == SETUP)
			{
				if (CueBall != nullptr)
				{
					delete[] Balls;
					CueBall = nullptr;
					ObjectiveBall = nullptr;
				}
				GamePhase = ACTIVE;
			}
			else if(GamePhase == ACTIVE)
			{
				//EVENT HANDLING
				while (SDL_PollEvent(&e) != 0)
				{
					if (e.type == SDL_QUIT)
					{
						quit = true;
					}
					if (e.type == SDL_MOUSEBUTTONDOWN)
					{
						int tempX, tempY;
						SDL_GetMouseState(&tempX, &tempY);

						if (NineBallButton.mouse_in_range(tempX, tempY))
						{
							GameState = NINEBALL;
							GamePhase = SETUP;
						}
						else if (EightBallButton.mouse_in_range(tempX, tempY))
						{
							GameState = EIGHTBALL;
							GamePhase = SETUP;
						}
					}
				}

				//DRAWING
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
				SDL_RenderClear(renderer);
				NineBallButton.draw(renderer, font);
				EightBallButton.draw(renderer, font);
			}
		}
		//START OF NINEBALL AND EIGHTBALL STATE
		else if (GameState == NINEBALL || GameState == EIGHTBALL)
		{
			if (GamePhase == SETUP)
			{
				for (int i = 0; i < 6; i++)
				{
					Table.Pockets[i].radius = 20; //Roughly 1.75 times the ball diameter

					double xOffset = 0, yOffset = 0;

					switch (i)
					{
					case 0:
						xOffset = Table.Pockets[i].radius / sqrt(2);
						yOffset = Table.Pockets[i].radius / sqrt(2);
						break;
					case 2:
						xOffset = -1 * Table.Pockets[i].radius / sqrt(2);
						yOffset = Table.Pockets[i].radius / sqrt(2);
						break;
					case 3:
						xOffset = Table.Pockets[i].radius / sqrt(2);
						yOffset = -1 * Table.Pockets[i].radius / sqrt(2);
						break;
					case 5:
						xOffset = -1 * Table.Pockets[i].radius / sqrt(2);
						yOffset = -1 * Table.Pockets[i].radius / sqrt(2);
						break;
					}
					Table.Pockets[i].Pos = { (Table.Rect.Pos.x + (i % 3)*(Table.Rect.width / 2) + xOffset), (Table.Rect.Pos.y + (i % 2)*Table.Rect.height + yOffset) };
				}

				ballWasPocketed = false;
				currentTurn = 0;
				breakShot = true;
				playerOneAssignedBall = 0;
				playerTwoAssignedBall = 0;

				double xOffset = 15;// NOTE: I define an xoffset based on the tables diamonds to properly place the balls

				if (GameState == NINEBALL)
				{
					BallsSize = 10;
					Balls = new Ball[BallsSize];

					for (int i = 0; i < BallsSize; i++)
					{
						Balls[i].Circ.Pos = {};
						Balls[i].Circ.radius = 11;
						Balls[i].Color = BallColors[i];
						Balls[i].Speed = {};
						Balls[i].pocketed = false;
					}

					reset_nine_ball(Balls, { Table.Rect.Pos.x - xOffset + (2 * xOffset + Table.Rect.width) * 2 / 8, Table.Rect.Pos.y + Table.Rect.height/2 });
					ObjectiveBall = &Balls[9];
				}
				else if (GameState == EIGHTBALL)
				{
					BallsSize = 16;
					Balls = new Ball[BallsSize];

					for (int i = 0; i < BallsSize; i++)
					{
						Balls[i].Circ.Pos = {};
						Balls[i].Circ.radius = 11;
						if (i < 9)
						{
							Balls[i].Color = BallColors[i];
						}
						else
						{
							Balls[i].Color = BallColors[i - 8];
						}
						Balls[i].Speed = {};
						Balls[i].pocketed = false;
					}

					reset_eight_ball(Balls, { Table.Rect.Pos.x - xOffset + (2*xOffset + Table.Rect.width)*2/8, Table.Rect.Pos.y + Table.Rect.height / 2 });
					ObjectiveBall = &Balls[8];
				}

				CueBall = Balls;
				CueBall->Circ.radius = 12;
				CueBall->Circ.Pos.x = Table.Rect.Pos.x - xOffset + (2*xOffset + Table.Rect.width) * 7 / 8;
				CueBall->Circ.Pos.y = Table.Rect.Pos.y + Table.Rect.height / 2;

				MenuButton.Rect.Pos.x = SCREEN_WIDTH - 85;
				MenuButton.Rect.Pos.y = 45;

				GamePhase = ACTIVE;
			}
			else if (GamePhase == ACTIVE || GamePhase == WAITING || GamePhase == WIN || GamePhase == RULES)
			{
				if (GamePhase == ACTIVE)
				{
					//EVENT HANDLING
					while (SDL_PollEvent(&e) != 0)
					{
						if (e.type == SDL_QUIT)
						{
							quit = true;
						}
						if (e.type == SDL_MOUSEBUTTONDOWN)
						{
							int tempX, tempY;
							SDL_GetMouseState(&tempX, &tempY);
							
							if (MenuButton.mouse_in_range(tempX, tempY))
							{
								GameState = MENU;
								GamePhase = SETUP;
							}

							if (RuleButton.mouse_in_range(tempX, tempY))
							{
								GamePhase = RULES;
							}

							if (!CueBall->pocketed)
							{
								mouseHeld = true;

								//Automatically snaps the trajectory line to the center of the cueball if in range
								if (sqrt(pow(tempX - CueBall->Circ.Pos.x, 2) + pow(tempY - CueBall->Circ.Pos.y, 2)) <= CueBall->Circ.radius)
								{
									Trajectory.InitialPos = { CueBall->Circ.Pos.x, CueBall->Circ.Pos.y };
								}
								else
								{
									Trajectory.InitialPos = { (double)tempX, (double)tempY };
								}
								Trajectory.PosDifference = {};
							}
							else
							{
								CueBall->pocketed = false;
							}
						}
						if (e.type == SDL_MOUSEBUTTONUP)
						{
							mouseHeld = false;

							//Sets the speed of the ball as soon as the mouse is released
							if (Trajectory.InitialPos.x == CueBall->Circ.Pos.x && Trajectory.InitialPos.y == CueBall->Circ.Pos.y)
							{
								//NOTE: 300 is the max length of the trajectory line, and 20 is the maximum speed of the ball
								CueBall->Speed.x = Trajectory.PosDifference.x / 300 * -20;
								CueBall->Speed.y = Trajectory.PosDifference.y / 300 * -20;
							}
						}
						if (e.type == SDL_MOUSEMOTION)
						{
							int tempX, tempY;
							SDL_GetMouseState(&tempX, &tempY);

							if (!CueBall->pocketed && mouseHeld)
							{
								Trajectory.PosDifference = { (double)tempX - Trajectory.InitialPos.x, (double)tempY - Trajectory.InitialPos.y };

								//Limiting how large the trajectory line can be to 300 pixels
								if (Trajectory.PosDifference.get_magnitude() > 300)
								{
									//By dividing each coordinate by this ratio the line will appear in the same direction but shorter
									double ratio = Trajectory.PosDifference.get_magnitude() / 300;

									Trajectory.PosDifference.x /= ratio;
									Trajectory.PosDifference.y /= ratio;
								}
							}
							else if (CueBall->pocketed)
							{
								CueBall->Circ.Pos.x = tempX;
								CueBall->Circ.Pos.y = tempY;
							}
						}
					}

					//CHECKING IF ANY OF THE BALLS IS MOVING AND IF SO CHANGING THE PHASE TO WAIT
					for (int i = 0; i < BallsSize; i++)
					{
						if (Balls[i].Speed.get_magnitude() != 0)
						{
							GamePhase = WAITING;
							break;
						}
					}
				}
				
				else if (GamePhase == WAITING)
				{
					//CHECKING IF ALL OF THE BALLS HAVE FINISHED MOVING AND IF SO CHANGE TO ACTIVE PHASE
					for (int i = 0; i < BallsSize; i++)
					{
						GamePhase = ACTIVE;
						if (Balls[i].Speed.get_magnitude() != 0)
						{
							GamePhase = WAITING;
							break;
						}
					}

					if (GamePhase == ACTIVE)
					{
						//disabling the breakshot marker
						if (breakShot)
						{
							breakShot = false;
						}
						//Changing turns if a ball was pocketed
						if (!ballWasPocketed)
						{
							currentTurn = (currentTurn + 1) % 2;
						}
						else
						{
							ballWasPocketed = false;
						}
					}
				}

				else if (GamePhase == WIN)
				{
					MenuButton.Rect.Pos.x = SCREEN_WIDTH/2;
					MenuButton.Rect.Pos.y = SCREEN_HEIGHT/2;
					//EVENT HANDLING
					while (SDL_PollEvent(&e) != 0)
					{
						if (e.type == SDL_QUIT)
						{
							quit = true;
						}
						if (e.type == SDL_MOUSEBUTTONDOWN)
						{
							int tempX, tempY;
							SDL_GetMouseState(&tempX, &tempY);

							if (MenuButton.mouse_in_range(tempX, tempY))
							{
								GameState = MENU;
								GamePhase = SETUP;
							}
						}
					}
				}

				else if (GamePhase == RULES)
				{
					//EVENT HANDLING
					while (SDL_PollEvent(&e) != 0)
					{
						if (e.type == SDL_QUIT)
						{
							quit = true;
						}
						if (e.type == SDL_MOUSEBUTTONDOWN)
						{
							GamePhase = ACTIVE;
						}
					}
				}

				//START OF BALL LOGIC (COLLISION, MOVEMENT, etc)
				for (int i = 0; i < BallsSize; i++)
				{
					//Checking if any ball has been pocketed
					if (Balls[i].is_pocketed(Table.Pockets) && !Balls[i].pocketed)
					{
						ballWasPocketed = true;
						Balls[i].pocketed = true;
						if (i != 0)
						{
							int n = 9;// NOTE: n is the number of balls per player
							if (GameState == NINEBALL)
							{
								Balls[i].Circ.Pos.y = 50;
							}
							else if (GameState == EIGHTBALL)
							{
								if (!breakShot && (playerOneAssignedBall == 0 || playerTwoAssignedBall == 0))
								{
									playerOneAssignedBall = 1 + currentTurn;//using smart math to assign player one the right ball type
									if (i > 8)
									{
										playerOneAssignedBall = (playerOneAssignedBall % 2) + 1;
									}
									playerTwoAssignedBall = (playerOneAssignedBall % 2) + 1;//assigns the other ball type to player two
								}
								n = 8;
								Balls[i].Circ.Pos.y = 51 + 20*((i/n + playerTwoAssignedBall)%2) - 20*((i/n + playerOneAssignedBall)%2);//using more smart math to draw balls in the right spot
							}
							Balls[i].Circ.Pos.x = 320 + ((i - 1) % n) * Balls[i].Circ.radius;
						}
						else if (i == 0)
						{
							currentTurn = (currentTurn + 1) % 2;
						}
						if (i == ObjectiveBall - Balls)
						{
							bool onlyObjectiveBall = true;
							if (GameState == NINEBALL)
							{
								for (int j = 1; j < BallsSize - 1; j++)
								{
									if (!Balls[j].pocketed)
									{
										onlyObjectiveBall = false;
									}
								}

							}
							else if (GameState == EIGHTBALL)
							{
								if ((currentTurn == 0 && playerOneAssignedBall == 1) || (currentTurn == 1 && playerTwoAssignedBall == 1))
								{
									for (int j = 1; j <= 7; j++)
									{
										if (!Balls[j].pocketed)
										{
											onlyObjectiveBall = false;
										}
									}
								}
								else if ((currentTurn == 0 && playerOneAssignedBall == 2) || (currentTurn == 1 && playerTwoAssignedBall == 2))
								{
									for (int j = 9; j <= BallsSize; j++)
									{
										if (!Balls[j].pocketed)
										{
											onlyObjectiveBall = false;
										}
									}
								}
								else if (playerOneAssignedBall == 0 || playerTwoAssignedBall == 0)
								{
									onlyObjectiveBall = false;
								}
							}

							if (!onlyObjectiveBall)
							{
								currentTurn = (currentTurn + 1) % 2;
							}
							GamePhase = WIN;
						}
						Balls[i].Speed.x = 0;
						Balls[i].Speed.y = 0;
					}


					//If a ball is pocketed it skips all checks
					if (!Balls[i].pocketed)
					{
						//Checking collision with Table walls
						if ((Balls[i].Circ.Pos.x + Balls[i].Circ.radius) + Balls[i].Speed.x >= (Table.Rect.Pos.x + Table.Rect.width) ||
							(Balls[i].Circ.Pos.x - Balls[i].Circ.radius) + Balls[i].Speed.x <= Table.Rect.Pos.x)
						{
							//A snapshot of the impact frame is created
							if ((Balls[i].Circ.Pos.x - Balls[i].Circ.radius) + Balls[i].Speed.x <= Table.Rect.Pos.x)
							{
								Balls[i].Circ.Pos.x = Table.Rect.Pos.x + Balls[i].Circ.radius + 1;
							}
							else
							{
								Balls[i].Circ.Pos.x = (Table.Rect.Pos.x + Table.Rect.width) - (Balls[i].Circ.radius + 1);
							}
							Balls[i].Speed.x *= -1;
						}
						if ((Balls[i].Circ.Pos.y + Balls[i].Circ.radius) + Balls[i].Speed.y >= (Table.Rect.Pos.y + Table.Rect.height) ||
							(Balls[i].Circ.Pos.y - Balls[i].Circ.radius) + Balls[i].Speed.y <= Table.Rect.Pos.y)
						{
							//also creating a snapshot of the impact frame
							if ((Balls[i].Circ.Pos.y - Balls[i].Circ.radius) + Balls[i].Speed.y <= Table.Rect.Pos.y)
							{
								Balls[i].Circ.Pos.y = Table.Rect.Pos.y + Balls[i].Circ.radius + 1;
							}
							else
							{
								Balls[i].Circ.Pos.y = (Table.Rect.Pos.y + Table.Rect.height) - (Balls[i].Circ.radius + 1);
							}
							Balls[i].Speed.y *= -1;
						}

						//Checking if the ball collides with any other ball
						for (int j = 0; j < BallsSize; j++)
						{
							//Excluding checking if a ball collides with itself as well as pocketed balls
							if (i != j && !Balls[j].pocketed)
							{
								Vector2 DistanceAway = { Balls[i].Circ.Pos.x - Balls[j].Circ.Pos.x, Balls[i].Circ.Pos.y - Balls[j].Circ.Pos.y };

								if (DistanceAway.get_magnitude() < Balls[i].Circ.radius + Balls[j].Circ.radius)
								{

									Vector2 Normal = { Balls[j].Circ.Pos.x - Balls[i].Circ.Pos.x, Balls[j].Circ.Pos.y - Balls[i].Circ.Pos.y };
									Vector2 UnitNormal = { Normal.x / Normal.get_magnitude(), Normal.y / Normal.get_magnitude() };
									Vector2 UnitTangent = { -1 * UnitNormal.y, UnitNormal.x };

									double v1n = Balls[i].Speed.x*UnitNormal.x + Balls[i].Speed.y*UnitNormal.y;
									double v1t = Balls[i].Speed.x*UnitTangent.x + Balls[i].Speed.y*UnitTangent.y;
									double v2n = Balls[j].Speed.x*UnitNormal.x + Balls[j].Speed.y*UnitNormal.y;
									double v2t = Balls[j].Speed.x*UnitTangent.x + Balls[j].Speed.y*UnitTangent.y;

									double newv1n = (v1n*(Balls[i].Circ.radius - Balls[j].Circ.radius) + 2 * Balls[j].Circ.radius*v2n) / (Balls[i].Circ.radius + Balls[j].Circ.radius);
									double newv2n = (v2n*(Balls[j].Circ.radius - Balls[i].Circ.radius) + 2 * Balls[i].Circ.radius*v1n) / (Balls[i].Circ.radius + Balls[j].Circ.radius);

									//Creating a snapshot of the impact frame by moving the first ball to the edge of the second
									if (DistanceAway.x != 0 && DistanceAway.y != 0)
									{
										Balls[i].Circ.Pos.x = Balls[j].Circ.Pos.x + (Balls[j].Circ.radius + Balls[i].Circ.radius)*(DistanceAway.x / DistanceAway.get_magnitude());
										Balls[i].Circ.Pos.y = Balls[j].Circ.Pos.y + (Balls[j].Circ.radius + Balls[i].Circ.radius)*(DistanceAway.y / DistanceAway.get_magnitude());
									}

									Balls[j].Speed.x = newv2n * UnitNormal.x + v2t * UnitTangent.x;
									Balls[j].Speed.y = newv2n * UnitNormal.y + v2t * UnitTangent.y;

									Balls[i].Speed.x = newv1n * UnitNormal.x + v1t * UnitTangent.x;
									Balls[i].Speed.y = newv1n * UnitNormal.y + v1t * UnitTangent.y;
								}
							}
						}

						Balls[i].move();
					}
				}


				//DRAWING
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
				SDL_RenderClear(renderer);

				if (GameState == NINEBALL)
				{
					draw_rect(renderer, { 300, 30, 140, 40 }, { 200, 200, 200 });
				}
				else if (GameState == EIGHTBALL)
				{
					draw_rect(renderer, { 300, 9, 130, 83 }, { 200, 200, 200 });
					draw_rect(renderer, { 300, 50, 130, 3 }, { 100, 100, 100 });
				}
				if (currentTurn == 0)
				{
					draw_text(renderer, {150, 25}, "Player 1's Turn", font, { 255, 0, 0 });
				}
				else
				{
					draw_text(renderer, {150, 75}, "Player 2's Turn", font, { 0, 0, 255 });
				}

				Table.draw(renderer);

				for (int i = 0; i < BallsSize; i++)
				{
					bool striped = false;
					if (GameState == EIGHTBALL && i >= 9)
					{
						striped = true;
					}
					Balls[i].draw(renderer, striped);
				}

				if (mouseHeld)
				{
					Trajectory.draw(renderer);
				}

				RuleButton.draw(renderer, font);

				if (GamePhase == WIN)
				{
					SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
					draw_rect(renderer, { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, {0, 0, 0, 100});
					SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

					if (currentTurn == 0)
					{
						draw_text(renderer, {SCREEN_WIDTH/2, SCREEN_HEIGHT/4}, "Player 1 Wins", font, {255, 255, 255});
					}
					else if (currentTurn == 1)
					{
						draw_text(renderer, { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4 }, "Player 2 Wins", font, { 255, 255, 255 });
					}
				}
				
				MenuButton.draw(renderer, font);

				if (GamePhase == RULES)
				{
					SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
					draw_rect(renderer, { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }, { 0, 0, 0, 100 });
					SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

					draw_text(renderer, { SCREEN_WIDTH / 2, 50 }, "RULES", font, { 255, 255, 255 });
					draw_text(renderer, { SCREEN_WIDTH/2, 100 }, "1. If you pocket a ball other than the cue ball or eight ball, you get to shoot again", font, { 255, 255, 255 });
					draw_text(renderer, { SCREEN_WIDTH/2, 125 }, "2. If the cue ball is pocketed the other player gets to place the cue ball anywhere on the board", font, { 255, 255, 255 });
					draw_text(renderer, { SCREEN_WIDTH / 2, 150 }, "3. If you pocket the eight ball after all other object balls, you win, otherwise you will lose", font, { 255, 255, 255 });
					if (GameState == EIGHTBALL)
					{
						draw_text(renderer, { SCREEN_WIDTH / 2, 175 }, "4. After a player pockets a ball outside the break shot, they will be assigned that ball type", font, { 255, 255, 255 });
					}
				}
			}
		}

		//FRAMERATE LIMITER
		currentTime = SDL_GetTicks();
		unsigned int difference = currentTime - lastTime;

		if (difference < 16)
		{
			SDL_Delay(16.666f - difference);
		}

		SDL_RenderPresent(renderer);

		lastTime = currentTime;
	}
	//END OF PROGRAM

	SDL_DestroyWindow(window);
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();
	return 0;
}
	


/*	TODO LIST
 *
 *	EXTRA
 *	-make non elastic collisions
 *	-make the UI prettier
 *  -replace balls that were sunk during the breakshot to correct player side after ball types have been decided
 *  -create a free play mode where you can do single player stuff
 *	-make a second option for the pool cue which uses two phases:
 *		->select from which direction to hit the ball
 *		->select the power to use
 */