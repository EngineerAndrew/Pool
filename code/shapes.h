#ifndef shapes
#define shapes


#include <math.h>
#include "SDL.h"
#include "SDL_ttf.h"

struct Color
{
	int r;
	int g;
	int b;
	int a;
};

struct Point2
{
	double x;
	double y;
};

struct Circle
{
	Point2 Pos;
	int radius;
};

struct Rect
{
	Point2 Pos;
	int width;
	int height;
};

SDL_Rect to_sdl_rect(Rect Rect)
{
	SDL_Rect rect = { (int)Rect.Pos.x, (int)Rect.Pos.y, Rect.width, Rect.height };
	return rect;
}

void draw_circle(SDL_Renderer* Renderer, Circle Circ, Color Color)
{
	SDL_SetRenderDrawColor(Renderer, Color.r, Color.g, Color.b, Color.a);

	for (int x = (Circ.Pos.x - Circ.radius); x <= (Circ.Pos.x + Circ.radius); x++)
	{
		for (int y = (Circ.Pos.y - Circ.radius); y <= (Circ.Pos.y + Circ.radius); y++)
		{
			if (pow((x - Circ.Pos.x), 2) + pow((y - Circ.Pos.y), 2) <= pow(Circ.radius, 2))
			{
				SDL_RenderDrawPoint(Renderer, x, y);
			}
		}
	}
}

void draw_rect(SDL_Renderer* Renderer, Rect Rect, Color Color)
{
	SDL_SetRenderDrawColor(Renderer, Color.r, Color.g, Color.b, Color.a);

	SDL_RenderFillRect(Renderer, &to_sdl_rect(Rect));
}

void draw_text(SDL_Renderer* Renderer, Point2 Dest, char* text, TTF_Font* font, Color Color)
{
	int width, height;

	TTF_SizeText(font, text, &width, &height);
	SDL_Surface* TextSurface = TTF_RenderText_Blended(font, text, { Uint8(Color.r), Uint8(Color.g), Uint8(Color.b), 255 });
	SDL_Texture* TextTexture = SDL_CreateTextureFromSurface(Renderer, TextSurface);
	
	SDL_RenderCopy(Renderer, TextTexture, NULL, &to_sdl_rect({Dest.x - width, Dest.y - height, width*2, height*2}));

	SDL_DestroyTexture(TextTexture);
	SDL_FreeSurface(TextSurface);
}

#endif