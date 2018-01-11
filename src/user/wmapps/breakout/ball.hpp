#ifndef _BALL_HPP
#define _BALL_HPP

#include "sprite.hpp"

class Ball : public Sprite{
private:
	int32_t x, y;
	int32_t xvel, yvel;
	uint32_t lx = 0, ly = 0;
	bool launched = false;
	bool reset = true;
	bool bounced = false;
	
public:
	static const uint32_t size = 20;
	
	Ball(std::shared_ptr<gds::Surface> s): Sprite(s), x(170), y(120), xvel(0), yvel(0) {}
	~Ball(){}
	
	void Launch();
	void Draw();
	bool Step();
	void Input(const wm_Event &e);
	gds::Rect GetBoundingRect();
	uint32_t GetZOrder();
	
	void Reset();
	void BounceSide();
	void BounceEdge();
	void BounceAngle(float angle);
};

#endif
