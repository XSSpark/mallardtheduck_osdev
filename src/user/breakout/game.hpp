#ifndef _GAME_HPP
#define _GAME_HPP

#include <memory>
#include <wm/libwm.h>
#include "ball.hpp"
#include "paddle.hpp"

extern std::shared_ptr<Ball> ball;
extern std::shared_ptr<Paddle> paddle;

bool Overlaps(const wm_Rect &r1, const wm_Rect &r2);

void DrawBackground();
void DrawTitle();

void InitGame();
void GameEvent(const wm_Event &e);
void GameStep();
void GameDraw();

void AddSprite(Sprite *s);
void RemoveSprite(std::shared_ptr<Sprite> s);

#endif