#include <gui/radiobutton.hpp>
#include <gui/defaults.hpp>
#include <gui/drawing.hpp>

namespace btos_api{
namespace gui{
	
const auto checkSize = 15;

RadioButton::RadioButton(const gds::Rect &r, const std::string &t, bool v) : rect(r), text(t), value(v) {}
	
EventResponse RadioButton::HandleEvent(const wm_Event &e){
	if(e.type == wm_EventType::PointerButtonUp){
		if(!value) update = true;
		value = true;
		if(onChange) onChange(value);
		if(getAllRects) return {true, getAllRects()};
		else return {true, rect};
	}
	return {true};
}

void RadioButton::Paint(gds::Surface &s){
	if(!surf || update){
		uint32_t inW = rect.w - 1;
		uint32_t inH = rect.h - 1;
		int32_t chkY = std::max<int32_t>((rect.h - checkSize) / 2, 0);
		
		if(!bkSurf){
			bkSurf.reset(new gds::Surface(gds_SurfaceType::Vector, rect.w, rect.h, 100, gds_ColourType::True));
			bkSurf->BeginQueue();
			
			textMeasures = bkSurf->MeasureText(text, fonts::GetRadioButtonFont(), fonts::GetRadioButtonTextSize());
			auto bkgCol = colours::GetBackground().Fix(*bkSurf);
			auto txtCol = colours::GetRadioButtonText().Fix(*bkSurf);
				
			int32_t textX = checkSize + 3;
			int32_t textY = std::max<int32_t>(((rect.h + textMeasures.h) / 2), 0);
			
			bkSurf->Box({0, 0, rect.w, rect.h}, bkgCol, bkgCol, 1, gds_LineStyle::Solid, gds_FillStyle::Filled);
			bkSurf->Text({textX, textY}, text, fonts::GetRadioButtonFont(), fonts::GetRadioButtonTextSize(), txtCol);
			
			auto cx = (checkSize / 2) + 1;
			auto cy = chkY + (checkSize / 2);
			
			auto topLeft = colours::GetRadioButtonLowLight().Fix(*bkSurf);
			auto bottomRight = colours::GetRadioButtonHiLight().Fix(*bkSurf);
			bkSurf->Arc({cx, cy, checkSize - 1, checkSize - 1}, 135, 315, topLeft, topLeft, 1);
			bkSurf->Arc({cx, cy, checkSize - 1, checkSize - 1}, 315, 135, bottomRight, topLeft, 1);
			
			auto border = colours::GetBorder().Fix(*bkSurf);
			bkSurf->Ellipse({cx, cy, checkSize, checkSize}, border, border);
			
			bkSurf->CommitQueue();
		}
		
		if(!surf) surf.reset(new gds::Surface(gds_SurfaceType::Vector, rect.w, rect.h, 100, gds_ColourType::True));
		else surf->Clear();
		
		surf->BeginQueue();
		
		surf->Blit(*bkSurf, {0, 0, rect.w, rect.h}, {0, 0, rect.w, rect.h});
		
		if(value){
			auto chkCol = colours::GetRadioButtonCheck().Fix(*surf);
			auto cx = (checkSize / 2) + 1;
			auto cy = chkY + (checkSize / 2);
			
			surf->Ellipse({cx, cy, checkSize - 8, checkSize - 8}, chkCol, chkCol, 1, gds_LineStyle::Solid, gds_FillStyle::Filled);
		}
		
		if(focus){
			auto focusCol = colours::GetRadioButtonFocus().Fix(*surf);
			
			int32_t boxTop = std::max(std::min<int32_t>(chkY - 1, (rect.h - textMeasures.h) / 2), 0);
			int32_t boxLeft = 0;
			uint32_t boxWidth = textMeasures.w + checkSize + 3;
			if(boxLeft + boxWidth > inW) boxWidth = inW - boxLeft;
			uint32_t boxHeight = std::max<int32_t>(textMeasures.h, checkSize + 3);
			if(boxTop + boxHeight > inH) boxHeight = inH - boxTop;
			surf->Box({boxLeft, boxTop, boxWidth, boxHeight}, focusCol, focusCol);
		}
		
		surf->CommitQueue();
		
		update = false;
	}
	
	s.Blit(*surf, {0, 0, rect.w, rect.h}, rect);
}

gds::Rect RadioButton::GetPaintRect(){
	return rect;
}

gds::Rect RadioButton::GetInteractRect(){
	if(textMeasures.w){
		uint32_t inW = rect.w - 1;
		uint32_t inH = rect.h - 1;
		int32_t chkY = std::max<int32_t>((rect.h - checkSize) / 2, 0);
		
		int32_t boxTop = std::max(std::min<int32_t>(chkY - 1, (rect.h - textMeasures.h) / 2), 0);
		int32_t boxLeft = 0;
		uint32_t boxWidth = textMeasures.w + checkSize + 3;
		if(boxLeft + boxWidth > inW) boxWidth = inW - boxLeft;
		uint32_t boxHeight = std::max<int32_t>(textMeasures.h, checkSize + 3);
		if(boxTop + boxHeight > inH) boxHeight = inH - boxTop;
		return {rect.x + boxLeft, rect.y + boxTop, boxWidth, boxHeight};
	}else return rect;
}

uint32_t RadioButton::GetSubscribed(){
	return wm_EventType::PointerButtonUp;
}

void RadioButton::Focus(){
	if(!focus) update = true;
	focus = true;
}
void RadioButton::Blur(){
	if(focus) update = true;
	focus = false;
}
	
void RadioButton::SetText(const std::string &t){
	text = t;
	bkSurf.reset();
}

void RadioButton::SetValue(bool v){
	value = v;
	update = true;
}

}
}