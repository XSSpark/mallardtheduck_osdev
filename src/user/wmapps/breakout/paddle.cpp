#include <gds/libgds.h>
#include <wm/libwm.h>
#include <dev/keyboard.h>
#include "paddle.hpp"
#include "game.hpp"

using namespace gds;

void Paddle::Draw(){
	Colour colour = surf->GetColour(128, 128, 128);
	surf->Box({(int32_t)x, (int32_t)y, width, height}, colour, colour, 1, gds_LineStyle::Solid, gds_FillStyle::Filled);
}

void Paddle::Left(){
	dx = -speed;
}

void Paddle::Right(){
	dx = speed;
}

bool Paddle::Step(){
	auto ballRect = ball->GetBoundingRect();
	auto paddleRect = GetBoundingRect();
	if(Overlaps(ballRect, paddleRect)){
		int32_t paddleCentre = paddleRect.x + (paddleRect.w / 2);
		int32_t ballCentre = ballRect.x + (ballRect.w / 2);
		int32_t diff = ballCentre - paddleCentre;
		float angle = (float)diff / ((float)paddleRect.w / 2.0);
		ball->BounceAngle(angle);
	}
	if(dx){
		uint32_t lx = x;
		x += dx;
		if(dx < 0 && x > lx) x = 0;
		if(x > 320 - width) x = 320 - width;
		dx = 0;
		return true;
	}
	return false;
}

void Paddle::Input(const wm_Event &e){
	if(e.type == wm_EventType::Keyboard){
		uint16_t keycode = KB_code(e.Key.code);
		if(keycode == (KeyFlags::NonASCII | KeyCodes::LeftArrow)){
			Left();
		}else if(keycode == (KeyFlags::NonASCII | KeyCodes::RightArrow)){
			Right();
		}
	}
}

Rect Paddle::GetBoundingRect(){
	return {(int32_t)x, (int32_t)y, width, height};
}

uint32_t Paddle::GetZOrder(){
	return 1;
}
