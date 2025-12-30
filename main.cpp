#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH (1300 / BMP_SIZE) * BMP_SIZE //1280
#define SCREEN_HEIGHT (700 / BMP_SIZE) * BMP_SIZE //672
#define BMP_SIZE 32
#define BORDER_WIDTH 1
#define GAME_X 16 - BORDER_WIDTH
#define GAME_Y 64 - BORDER_WIDTH
#define GAME_WIDTH SCREEN_WIDTH - 2*GAME_X + 4*BORDER_WIDTH 
#define GAME_HEIGHT SCREEN_HEIGHT - 2*GAME_Y + 4*BORDER_WIDTH
#define SPEEDUP_INTERVAL 10.0f
#define SPEEDUP_FACTOR 10  
#define SPEED_BONUS 30
#define SHORTEN_BONUS 4
#define SPEED 80
#define PROGRESS_BAR_X GAME_X
#define PROGRESS_BAR_Y GAME_Y - 15
#define PROGRESS_BAR_WIDTH GAME_WIDTH
#define PROGRESS_BAR_HEIGHT 10
#define RED_DOT_RANDOM_MIN 5.0
#define PROGRESS_BAR_MAX 5.0
#define RED_DOT_RANDOM_MAX 20.0
#define MIN_SPEED 20

typedef struct {
	int x;
	int y;
} Pair;

enum Direction
{
	up,
	right,
	down,
	left
};

struct Node {
	Node* next;
	Node* prev;
	Pair cords;
	SDL_Surface* sprite;
	Direction direction;
};

typedef struct {
	Node* head;
	Node* tail;
	int speed;
	int xDirection;
	int yDirection;
} Snake;

typedef struct {
	Pair cords;
	SDL_Surface* sprite;
} Dot;

struct RedDot {
	bool isActive;
	Pair cords;
	double timer;
	//double bonusTimer;
	int bonusType;
	SDL_Surface* sprite;
};

struct Progress_Bar {
	Pair cords;
	int progressWidth = PROGRESS_BAR_WIDTH;
	int progressHeight = PROGRESS_BAR_HEIGHT;
	double barTimer;
	double currentProgress = PROGRESS_BAR_MAX;
	double maxProgress = PROGRESS_BAR_MAX;
	int isActive = 0;
	Uint32 outlineColor;
	Uint32 fillColor;
};


/*
 * Primitives drawing functions based on template from basics of programming course from Gdansk University of Technology.
 */

// Narysowanie powierzchni sprite na powierzchni screen w punkcie (x, y)
// (x, y) to punkt úrodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};


// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) 
// bπdü poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


void FillRectangle(int& i, int y, int k, SDL_Surface* screen, int x, int l, const Uint32& fillColor)
{
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
}

// rysowanie prostokπta o d≥ugoúci bokÛw l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, bool isFilled, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	if (isFilled) { FillRectangle(i, y, k, screen, x, l, fillColor); };
}

//Snake

void initSnake(Snake* snake, SDL_Surface* headSurface, SDL_Surface* bodySurface) {
	Node* initialNode = new Node;
	initialNode->cords.x = SCREEN_WIDTH / 2;
	initialNode->cords.y = SCREEN_HEIGHT / 2;
	initialNode->sprite = headSurface;
	initialNode->direction = right;
	initialNode->next = nullptr;
	initialNode->prev = nullptr;

	snake->head = initialNode;
	snake->tail = initialNode;

	for (int i = 1; i < 3; ++i) {
		Node* newNode = new Node;
		newNode->cords.x = SCREEN_WIDTH / 2 - i * 32;
		newNode->cords.y = SCREEN_HEIGHT / 2;
		newNode->sprite = bodySurface;
		newNode->direction = right;
		newNode->next = nullptr;
		newNode->prev = snake->tail;

		snake->tail->next = newNode;
		snake->tail = newNode;
	}
	snake->tail->sprite = bodySurface;
	snake->speed = SPEED;
	snake->xDirection = 1;
	snake->yDirection = 0;
}

void drawSnake(SDL_Surface* screen, Snake* snake, SDL_Surface* headUp, SDL_Surface* headDown, SDL_Surface* headLeft, SDL_Surface* headRight) {
	Node* current = snake->head;
	switch (current->direction)
	{
	case up:
		current->sprite = headUp;
		break;
	case down:
		current->sprite = headDown;
		break;
	case left:
		current->sprite = headLeft;
		break;
	case right:
		current->sprite = headRight;
		break;

	default:
		break;
	}
	while (current) {
		DrawSurface(screen, current->sprite, current->cords.x, current->cords.y);
		current = current->next;
	}
}

void addNodeToSnake(Snake* snake) {
	Node* newNode = new Node();
	newNode->cords = snake->tail->cords;
	newNode->next = nullptr;
	newNode->prev = snake->tail;
	snake->tail->next = newNode;
	newNode->sprite = snake->tail->sprite;
	snake->tail = newNode;

}

void destroySnake(Snake& snake)
{
	Node* current = snake.head;
	while (current) {
		Node* next = current->next;
		delete current;
		current = next;
	}
}

void cutSnakeTail(Snake& snake)
{
	Node* current = snake.tail;
	for (int i = 0; i < SHORTEN_BONUS; i++) {
		Node* prev = current->prev;
		if (prev != snake.head) {
			delete current;
			current = prev;
			current->next = nullptr;
		}
	}
	snake.tail = current;
}

//Bar

void initBar(SDL_Surface* screen, Progress_Bar* bar) {
	bar->cords.x = PROGRESS_BAR_X;
	bar->cords.y = PROGRESS_BAR_Y;
	bar->outlineColor = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
	bar->fillColor = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	bar->isActive = 0;
	bar->barTimer = 0.0;
	bar->currentProgress = PROGRESS_BAR_MAX;
	bar->maxProgress = PROGRESS_BAR_MAX;
}

void drawProgressBar(SDL_Surface* screen, Progress_Bar* bar) {
	int i;
	int filledWidth = (int)((bar->currentProgress * bar->progressWidth) / bar->maxProgress);
	DrawRectangle(screen, false, bar->cords.x, bar->cords.y, bar->progressWidth, bar->progressHeight, bar->outlineColor, 0);
	FillRectangle(i, bar->cords.y, bar->progressHeight, screen, bar->cords.x, filledWidth, bar->fillColor);
}


//Dot

void drawDotCords(Dot* dot, Snake* snake)
{
	dot->cords.x = GAME_X + BMP_SIZE / 2 + 1 + ((rand() % 39) * 32);
	dot->cords.y = GAME_Y + BMP_SIZE / 2 + 1 + ((rand() % 17) * 32);
	Node* current = snake->head;
	while (current) {
		if (dot->cords.x == current->cords.x) {
			dot->cords.x = GAME_X + BMP_SIZE / 2 + 1 + ((rand() % 39) * 32);
		}
		else if (dot->cords.y == current->cords.y) {
			dot->cords.y = GAME_Y + BMP_SIZE / 2 + 1 + ((rand() % 17) * 32);
		}
		current = current->next;
	}
}

void drawDot(SDL_Surface* screen, Dot* dot) {
	DrawSurface(screen, dot->sprite, dot->cords.x, dot->cords.y);
}

void initDot(Dot* dot, SDL_Surface* sprite, Snake* snake) {
	drawDotCords(dot, snake);
	dot->sprite = sprite;
}

void dotCollision(Snake* snake, Dot* dot) {
	addNodeToSnake(snake);
	initDot(dot, dot->sprite, snake);
	// increaseScore();
}

//RedDot

void drawRedDotCords(RedDot* rdot, Snake* snake)
{
	rdot->bonusType = rand() % 2;
	rdot->isActive = 0;
	rdot->timer = ((float)rand() / (float)(RAND_MAX)) * RED_DOT_RANDOM_MAX;
	//rdot->bonusTimer = BONUS_TIME;
	rdot->cords.x = GAME_X + BMP_SIZE / 2 + 1 + ((rand() % 39) * 32);
	rdot->cords.y = GAME_Y + BMP_SIZE / 2 + 1 + ((rand() % 17) * 32);
	Node* current = snake->head;
	while (current) {
		if (rdot->cords.x == current->cords.x) {
			rdot->cords.x = GAME_X + BMP_SIZE / 2 + 1 + ((rand() % 39) * 32); // zamienic liczby na define'y
		}
		else if (rdot->cords.y == current->cords.y) {
			rdot->cords.y = GAME_Y + BMP_SIZE / 2 + 1 + ((rand() % 17) * 32);
		}
		current = current->next;
	}
}

void showRedDot(SDL_Surface* screen, RedDot* rdot) {
	DrawSurface(screen, rdot->sprite, rdot->cords.x, rdot->cords.y);
}

void initRedDot(RedDot* rdot, SDL_Surface* sprite, Snake* snake) {
	drawRedDotCords(rdot, snake);
	rdot->sprite = sprite;
}

void redDotCollision(Snake* snake, RedDot* rdot, Progress_Bar* bar, SDL_Surface* screen) {
	if (rdot->bonusType == 0) {
		snake->speed += SPEED_BONUS;
	}
	else {
		cutSnakeTail(*snake);
	}
	rdot->isActive = 0;
	//initRedDot(rdot, rdot->sprite, snake);
	// increaseScore();
}

//Snake behaviour

void borderCollision(int& xDirection, int nextY, int& yDirection, int nextX)
{
	if (xDirection == 1) {

		if (nextY + BMP_SIZE < GAME_Y + GAME_HEIGHT) {
			xDirection = 0;
			yDirection = 1;
		}
		else {
			xDirection = 0;
			yDirection = -1;
		}
	}
	else if (xDirection == -1) {
		if (nextY - BMP_SIZE >= GAME_Y) {
			xDirection = 0;
			yDirection = -1;
		}
		else {
			xDirection = 0;
			yDirection = 1;
		}
	}
	else if (yDirection == 1) {
		if (nextX - BMP_SIZE >= GAME_X) {
			xDirection = -1;
			yDirection = 0;
		}
		else {
			xDirection = 1;
			yDirection = 0;
		}
	}
	else if (yDirection == -1) {
		if (nextX + BMP_SIZE < GAME_X + GAME_WIDTH) {
			xDirection = 1;
			yDirection = 0;
		}
		else {
			xDirection = -1;
			yDirection = 0;
		}
	}
}

void checkDotsCollision(Dot* dot, Snake* snake, RedDot* rDot, Progress_Bar* bar, SDL_Surface* screen, int& points)
{
	if (dot->cords.x == snake->head->cords.x + snake->xDirection * BMP_SIZE && dot->cords.y == snake->head->cords.y + snake->yDirection * BMP_SIZE) {
		dotCollision(snake, dot);
		points += 50;
	}
	if (rDot->isActive == 1 && rDot->cords.x == snake->head->cords.x + snake->xDirection * BMP_SIZE && rDot->cords.y == snake->head->cords.y + snake->yDirection * BMP_SIZE) {
		redDotCollision(snake, rDot, bar, screen);
		points += 200;
	}
}

void moveSnake(Snake* snake, Dot* dot, RedDot* rDot, SDL_Surface* screen, Progress_Bar* bar, int& moveCounter, int& defeat, int& points) {
	moveCounter++;

	if (moveCounter % snake->speed == 0) {
		int nextX = snake->head->cords.x + snake->xDirection * BMP_SIZE;
		int nextY = snake->head->cords.y + snake->yDirection * BMP_SIZE;

		if (nextX < GAME_X || nextX >= GAME_X + GAME_WIDTH || nextY < GAME_Y || nextY >= GAME_Y + GAME_HEIGHT) {
			borderCollision(snake->xDirection, nextY, snake->yDirection, nextX);
		}
		checkDotsCollision(dot, snake, rDot, bar, screen, points);

		Node* current = snake->tail;
		while (current->prev != nullptr) {
			current->cords = current->prev->cords;
			current = current->prev;
		}

		while (current->next != nullptr)
		{
			if (current->cords.x == snake->head->cords.x + snake->xDirection * BMP_SIZE && current->cords.y == snake->head->cords.y + snake->yDirection * BMP_SIZE) {
				defeat = 1;
			}
			current = current->next;
		}
		snake->head->cords.x += snake->xDirection * BMP_SIZE;
		snake->head->cords.y += snake->yDirection * BMP_SIZE;
	}
}

//EndGame

void gameReset(SDL_Surface* screen, Snake& snake, Progress_Bar* bar, RedDot* rDot,
	double& worldTime, double& distance, int& newGame, int& moveCounter, int& defeat, double& speedTime, double& dotTime)
{
	initSnake(&snake, snake.head->sprite, snake.head->next->sprite);
	initBar(screen, bar);
	initRedDot(rDot, rDot->sprite, &snake);
	worldTime = 0;
	speedTime = 0;
	dotTime = 0;
	distance = 0;
	newGame = 0;
	moveCounter = 0;
	defeat = 0;
}

void drawDefeatScreen(SDL_Surface* screen, SDL_Surface* charset, Snake& snake, int& newGame, int& quit) {
	char text[128];
	DrawRectangle(screen, true, GAME_X, GAME_Y, GAME_WIDTH, GAME_HEIGHT,
		SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00),
		SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC));
	sprintf(text, "GAME OVER! Press N for New Game or ESC to Quit.");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, screen->h / 2, text, charset);
}

//saving

void saveGameState(int moveCounter, double speedTime, double dotTime, int newGame, int defeat, double worldTime,
	double fpsTimer, double fps, int frames, int quit, double distance, int points,
	const Dot& dot, const RedDot& rDot, const Progress_Bar& bar, const Snake& snake) {
	FILE* file = fopen("zapis_gry.txt", "w");
	if (!file) {
		fprintf(stderr, "Nie moøna otworzyÊ pliku do zapisu!\n");
		return;
	}

	// Zapis zmiennych gry
	fprintf(file, "%lf %lf %lf %lf %lf %lf\n", speedTime, dotTime, worldTime, fpsTimer, fps, distance);
	fprintf(file, "%d %d %d %d %d %d\n", moveCounter, newGame, frames, defeat, quit, points);

	// Zapis struktury Dot
	fprintf(file, "%d %d\n", dot.cords.x, dot.cords.y);
	fprintf(file, "%d %d %d %d %.6f \n", rDot.cords.x, rDot.cords.y, rDot.isActive, rDot.bonusType, rDot.timer/*, rDot.bonusTimer*/);

	// Zapis struktury Bar
	/*fprintf(file, "%lf %lf %d\n",bar.barTimer, bar.currentProgress, bar.isActive);*/
	double elapsedTime = (SDL_GetTicks() - bar.barTimer) / 1000.0;
	fprintf(file, "%lf %lf %d\n", elapsedTime, bar.currentProgress, bar.isActive);

	// Zapis struktury Snake
	fprintf(file, "%d %d %d\n", snake.speed, snake.xDirection, snake.yDirection);

	Node* current = snake.head;
	while (current) {
		fprintf(file, "%d %d\n", current->cords.x, current->cords.y);
		current = current->next;
	}
	fclose(file);
}

void loadGameState(int& moveCounter, double& speedTime, double& dotTime, int& newGame, int& defeat, double& worldTime,
	double& fpsTimer, double& fps, int& frames, int& quit, double& distance, int& points,
	Dot& dot, RedDot& rDot, Progress_Bar& bar, Snake& snake, SDL_Surface* snakeSprite) {

	FILE* file = fopen("zapis_gry.txt", "r");
	if (!file) {
		fprintf(stderr, "Nie moøna otworzyÊ pliku do odczytu!\n");
		return;
	}

	fseek(file, 0, SEEK_END);
	if (ftell(file) == 0) {
		fprintf(stderr, "Plik jest pusty!\n");
		fclose(file);
		return;
	}
	rewind(file);

	// Wczytywanie zmiennych gry
	fscanf(file, "%lf %lf %lf %lf %lf %lf", &speedTime, &dotTime, &worldTime, &fpsTimer, &fps, &distance);
	fscanf(file, "%d %d %d %d %d %d", &moveCounter, &newGame, &frames, &defeat, &quit, &points);

	// Wczytywanie struktury Dot
	fscanf(file, "%d %d", &dot.cords.x, &dot.cords.y);

	// Wczytywanie struktury RedDot
	fscanf(file, "%d %d %d %d %lf", &rDot.cords.x, &rDot.cords.y, &rDot.isActive, &rDot.bonusType, &rDot.timer);

	// Wczytywanie struktury Progress_Bar
	double elapsedTime;
	fscanf(file, "%lf %lf %d", &elapsedTime, &bar.currentProgress, &bar.isActive);
	bar.barTimer = SDL_GetTicks() - (elapsedTime * 1000.0);

	// Wczytywanie struktury Snake
	fscanf(file, "%d %d %d", &snake.speed, &snake.xDirection, &snake.yDirection);

	// Wczytywanie wÍz≥Ûw Snake

	destroySnake(snake);

	int x, y;
	fscanf(file, "%d %d", &x, &y);
	Node* current = new Node{ nullptr, nullptr,  {x, y}, snakeSprite };
	snake.head = current;
	current->prev = nullptr;

	while (fscanf(file, "%d %d", &x, &y) == 2) {
		Node* newNode = new Node{ nullptr, nullptr,  {x, y}, snakeSprite };
		current->next = newNode;
		newNode->prev = current;
		current = current->next;
	}
	snake.tail = current;
	current->next = nullptr;

	fclose(file);
}

// rest 

void showMenu(SDL_Surface* screen, int bialy, int czerwony, int niebieski, char  text[128], double worldTime, double fps, Snake& snake, int points, SDL_Surface* charset, SDL_Texture* scrtex, SDL_Renderer* renderer)
{
	DrawRectangle(screen, false, GAME_X, GAME_Y, GAME_WIDTH, GAME_HEIGHT, bialy, NULL);
	DrawRectangle(screen, true, 4, 4, SCREEN_WIDTH - 8, 36, czerwony, niebieski);
	sprintf(text, "Szablon drugiego zadania, czas trwania = %.1lf s  %.0lf klatek / s    speed = %d     points = %d", worldTime, fps, snake.speed, points);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
	sprintf(text, "Esc - wyjscie, n - nowa gra, w - przyspieszenie, s - zwolnienie     PKT:1,2,3,4,A,B,C,D,E");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

	SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
	//		SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, scrtex, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void eventHandling(SDL_Event& event, int& quit, int& newGame, int& moveCounter, double& speedTime, double& dotTime, int& defeat, double& worldTime, double& fpsTimer, double& fps, int& frames, double& distance, int& points, Dot& dot, RedDot& rDot, Progress_Bar& bar, Snake& snake, SDL_Surface* eti)
{
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
			else if (event.key.keysym.sym == SDLK_n) {
				newGame = 1;
			}
			if (event.key.keysym.sym == SDLK_s) {
				saveGameState(moveCounter, speedTime, dotTime, newGame, defeat, worldTime,
					fpsTimer, fps, frames, quit, distance, points, dot, rDot, bar, snake);
			}if (event.key.keysym.sym == SDLK_l) {
				loadGameState(moveCounter, speedTime, dotTime, newGame, defeat, worldTime,
					fpsTimer, fps, frames, quit, distance, points, dot, rDot, bar, snake, eti);
			}
			else if (event.key.keysym.sym == SDLK_UP && !(snake.yDirection > 0)) { snake.yDirection = -1; snake.xDirection = 0; snake.head->direction = up; }
			else if (event.key.keysym.sym == SDLK_DOWN && !(snake.yDirection < 0)) { snake.yDirection = 1; snake.xDirection = 0; snake.head->direction = down; }
			else if (event.key.keysym.sym == SDLK_RIGHT && !(snake.xDirection < 0)) { snake.xDirection = 1; snake.yDirection = 0; snake.head->direction = right; }
			else if (event.key.keysym.sym == SDLK_LEFT && !(snake.xDirection > 0)) { snake.xDirection = -1; snake.yDirection = 0; snake.head->direction = left; }
			break;
		case SDL_KEYUP:
			break;
		case SDL_QUIT:
			quit = 1;
			break;
		};
	};
	frames++;
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	Snake snake;
	Dot dot;
	RedDot rDot;
	Progress_Bar bar;

	int moveCounter = 0;
	double speedTime = 0;
	double dotTime = 0;
	int newGame = 0;
	int defeat = 0;
	int points = 0;

	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps, distance;
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* eti, * etic, *headUp, * headDown, * headLeft, *headRight, *body, *tail;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;

	srand(time(NULL));

	printf("wyjscie printfa trafia do tego okienka\n");
	printf("printf output goes here\n");

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
		&window, &renderer);
	//rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
	//	&window, &renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Konrad Machura");

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_ShowCursor(SDL_DISABLE);

	charset = SDL_LoadBMP("./assets/bmp/cs8x8.bmp");
	if (charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);

	eti = SDL_LoadBMP("./assets/bmp/eti.bmp");
	if (eti == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	etic = SDL_LoadBMP("./assets/bmp/etic.bmp");
	if (etic == NULL) {
		printf("SDL_LoadBMP(etic.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	headUp = SDL_LoadBMP("./assets/bmp/headUp.bmp");
	if (headUp == NULL) {
		printf("SDL_LoadBMP(etic.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	headDown = SDL_LoadBMP("./assets/bmp/headDown.bmp");
	if (headDown == NULL) {
		printf("SDL_LoadBMP(etic.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	headRight = SDL_LoadBMP("./assets/bmp/headRight.bmp");
	if (headRight == NULL) {
		printf("SDL_LoadBMP(etic.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	headLeft = SDL_LoadBMP("./assets/bmp/headLeft.bmp");
	if (headLeft == NULL) {
		printf("SDL_LoadBMP(etic.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	body = SDL_LoadBMP("./assets/bmp/body.bmp");
	if (body == NULL) {
		printf("SDL_LoadBMP(body.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);

	initSnake(&snake, headRight, body);
	initDot(&dot, eti, &snake);
	initRedDot(&rDot, etic, &snake);
	initBar(screen, &bar);

	t1 = SDL_GetTicks();
	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	distance = 0;

	while (!quit) {
		if (newGame == 1) {
			gameReset(screen, snake, &bar, &rDot, worldTime, distance, newGame, moveCounter, defeat, speedTime, dotTime);
		}
		SDL_FillRect(screen, NULL, czarny);
		t2 = SDL_GetTicks();

		delta = (t2 - t1) * 0.001;
		t1 = t2;

		worldTime += delta;
		speedTime += delta;
		dotTime += delta;

		if (speedTime >= SPEEDUP_INTERVAL && snake.speed > MIN_SPEED) {
			snake.speed = (int)(snake.speed - SPEEDUP_FACTOR);
			speedTime = 0.0f;
		}
		distance += snake.speed * delta;

		moveSnake(&snake, &dot, &rDot, screen, &bar, moveCounter, defeat, points);
		if (defeat == 1)
		{
			drawDefeatScreen(screen, charset, snake, newGame, quit);
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			bool waitingForInput = true;
			while (waitingForInput) {
				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_KEYDOWN) {
						if (event.key.keysym.sym == SDLK_ESCAPE) {
							quit = 1;
							waitingForInput = false;
						}
						else if (event.key.keysym.sym == SDLK_n) {
							newGame = 1;
							waitingForInput = false;
							gameReset(screen, snake, &bar, &rDot, worldTime, distance, newGame, moveCounter, defeat, speedTime, dotTime);
							break;
						}
					}
					else if (event.type == SDL_QUIT) {
						quit = 1;
						waitingForInput = false;
					}
				}
			}
		}

		drawSnake(screen, &snake, headUp, headDown, headLeft, headRight);
		drawDot(screen, &dot);
		if (dotTime >= rDot.timer || rDot.isActive == 1) {
			showRedDot(screen, &rDot);
			dotTime = 0.0f;
			rDot.isActive = 1;
			if (bar.isActive == 0) {
				initBar(screen, &bar);
				bar.isActive = 1;
				bar.barTimer = SDL_GetTicks();
			}
		}

		if (bar.isActive == 1 && rDot.isActive == 1) {
			drawProgressBar(screen, &bar);
			double elapsedTime = (SDL_GetTicks() - bar.barTimer) / 1000.0;
			bar.currentProgress = bar.maxProgress - elapsedTime;
		}
		if ((bar.currentProgress <= 0 && rDot.isActive == 1) || (bar.currentProgress > 0 && rDot.isActive == 0)) {
			bar.isActive = 0;
			bar.currentProgress = 0;
			initRedDot(&rDot, rDot.sprite, &snake);
		}

		fpsTimer += delta;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		};

		showMenu(screen, bialy, czerwony, niebieski, text, worldTime, fps, snake, points, charset, scrtex, renderer);
		eventHandling(event, quit, newGame, moveCounter, speedTime, dotTime, defeat, worldTime, fpsTimer, fps, frames, distance, points, dot, rDot, bar, snake, eti);
	};

	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	destroySnake(snake);

	SDL_Quit();
	return 0;
};

