#include <gui/form.hpp>
#include <gui/defaults.hpp>
#include <gds/libgds.h>

#include <iostream>

using namespace std::placeholders;

namespace btos_api{
namespace gui{
	
gds::Surface &Form::GetSurface(){
	return surf;
}

gds::Rect Form::GetBoundingRect(){
	return {0, 0, rect.w, rect.h};
}

void Form::Update(const gds::Rect &r){
	wm::Window::Update(r);
}

void Form::Update(){
	wm::Window::Update();
}

void Form::SetSubscribed(uint32_t subs){
	wm::Window::SetSubscribed(subs);
}

Form::Form(const gds::Rect &r, uint32_t options, const std::string &title)
	: Window({r.x, r.y}, options, 0, gds::Surface::Wrap(GDS_NewSurface(gds_SurfaceType::Vector, r.w, r.h, 100, gds_ColourType::True), false), title),
	rect(r),
	surf(wm::Window::GetSurface())
	{
	SetEventHandler(std::bind(&Form::HandleEvent, this, _1));
	Paint();
}

}
}
