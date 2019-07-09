#include <gui/detaillist.hpp>
#include <gui/defaults.hpp>
#include <gui/drawing.hpp>
#include <dev/keyboard.h>

#include <cctype>
#include <algorithm>

#include <util/tinyformat.hpp>

namespace btos_api{
namespace gui{
	
const auto scrollbarSize = 17;
const auto checkSize = 17;

DetailList::DetailList(const gds::Rect &r, const std::vector<std::string> &c, bool sH, size_t is, bool mS) : 
	outerRect(r), 
	rect(sH ? gds::Rect{r.x, r.y, r.w - scrollbarSize, r.h - scrollbarSize} : gds::Rect{r.x, r.y, r.w - scrollbarSize, r.h}),
	cols(c), iconsize(is), scrollHoriz(sH), multiSelect(mS){
	auto info = fonts::GetDetailListFont().Info();
	fontHeight = (info.maxH * fonts::GetDetailListTextSize()) / info.scale;
	if(multiSelect && fontHeight < checkSize) fontHeight = checkSize;
	
	vscroll.reset(new Scrollbar({outerRect.x + (int32_t)outerRect.w - scrollbarSize, outerRect.y, scrollbarSize, outerRect.h - (scrollHoriz ? scrollbarSize : 0)}, 1, 1, 1, 1, false));

	vscroll->OnChange([this] (uint32_t v) {
		if(v != vOffset) update = true;
		vOffset = v;
	});
	
	UpdateDisplayState();
	
	if(scrollHoriz){
		CalculateColumnWidths();
		hscroll.reset(new Scrollbar({outerRect.x, outerRect.y + (int32_t)outerRect.h - scrollbarSize, outerRect.w - scrollbarSize, scrollbarSize}, 1, 1, 1, 1, true));
		
		hscroll->OnChange([this] (uint32_t v) {
			if((int32_t)v != hOffset + (multiSelect ? checkSize : 0)) update = true;
			hOffset = v - (multiSelect ? checkSize : 0);
		});
	}
	if(multiSelect) hOffset = -checkSize;
}

void DetailList::CalculateColumnWidths(){
	colWidths.resize(cols.size());
	for(size_t i = 0; i < cols.size(); ++i){
		uint32_t maxW = colItems[i].measures.w;
		for(const auto &di : drawItems){
			if(di.size() > i && di[i].measures.w > maxW) maxW = di[i].measures.w;
		}
		colWidths[i] = maxW;
	}
	while(std::accumulate(colWidths.begin(), colWidths.end(), colWidths.size()) > rect.w){
		auto maxIt = std::max_element(colWidths.begin(), colWidths.end());
		--*maxIt;
	}
}

void DetailList::UpdateDisplayState(bool changePos){
	if(!surf) surf.reset(new gds::Surface(gds_SurfaceType::Vector, rect.w, rect.h, 100, gds_ColourType::True));
	
	visibleItems = (rect.h / fontHeight) - 1;
	if(changePos){
		if(selectedItem < vOffset) vOffset = selectedItem;
		if(selectedItem >= vOffset + (visibleItems - 1)) vOffset = std::max<int32_t>(0, (int32_t)(selectedItem - (visibleItems - 1)));
	}
	
	drawItems.resize(items.size());
	for(size_t i = 0; i < items.size(); ++i){
		drawItems[i].resize(items[i].size());
		for(size_t j = 0; j < items[i].size(); ++j){
			auto &di = drawItems[i][j];
			auto &it = items[i][j];
			if(di.text != it || !di.measures.w){
				di.text = it;
				di.measures = surf->MeasureText(it, fonts::GetDetailListFont(), fonts::GetDetailListTextSize());
				di.fittedText = "";
				di.fittedWidth = 0;
			}
		}
	}
	
	colItems.resize(cols.size());
	for(size_t i = 0; i < cols.size(); ++i){
		auto &di = colItems[i];
		auto &ci = cols[i];
		if(di.text != ci || !di.measures.w){
			di.text = ci;
			di.measures = surf->MeasureText(ci, fonts::GetDetailListFont(), fonts::GetDetailListTextSize());
			di.fittedText = "";
			di.fittedWidth = 0;
		}
	}
	
	if(vscroll){
		if(visibleItems < items.size()){
			vscroll->Enable();
			vscroll->SetLines(std::max<int32_t>((int32_t)items.size() - visibleItems, 1));
			vscroll->SetPage(visibleItems);
			vscroll->SetValue(vOffset);
		}else{
			vscroll->Disable();
			vscroll->SetLines(1);
			vscroll->SetPage(1);
			vscroll->SetValue(0);
			vOffset = 0;
		}
	}
	
	if(hscroll){
		auto overallWidth = std::accumulate(colWidths.begin(), colWidths.end(), colWidths.size());
		if(overallWidth > rect.w - 2){
			hscroll->Enable();
		 	hscroll->SetLines(overallWidth - (rect.w - 2));
		 	hscroll->SetPage(rect.w - 2);
		 	hscroll->SetValue(hOffset + (multiSelect ? checkSize : 0));
		 	hscroll->SetStep(fonts::GetDetailListTextSize() / 2);
		}else{
			hscroll->Disable();
			hscroll->SetLines(1);
			hscroll->SetPage(1);
			hscroll->SetValue(0);
			hOffset = (multiSelect ? -checkSize : 0);
		}
	}else{
		CalculateColumnWidths();
	}
}

std::string DetailList::FitTextToCol(DrawItem &item, size_t colIndex){
	auto width = colWidths[colIndex];
	if(item.fittedWidth == width) return item.fittedText;
	if(item.measures.w < width){
		item.fittedWidth = width;
		item.fittedText = item.text;
		return item.text;	
	}
	
	std::string suffix = "\xE2\x80\xA6";
	if(width < surf->MeasureText(suffix, fonts::GetDetailListFont(), fonts::GetDetailListTextSize()).w) suffix = "";
	std::string text = item.text;
	while(surf->MeasureText(text + suffix, fonts::GetDetailListFont(), fonts::GetDetailListTextSize()).w > width){
		text.pop_back();
	}
	
	item.fittedText = text + suffix;
	item.fittedWidth = width;
	return item.fittedText;
}

EventResponse DetailList::HandleEvent(const wm_Event &e){
	bool handled = false;
	if(e.type == wm_EventType::Keyboard && !(e.Key.code & KeyFlags::KeyUp)){
		uint16_t code = KB_code(e.Key.code);
		if(code == (KeyFlags::NonASCII | KeyCodes::DownArrow) && selectedItem < items.size() - 1){
			++selectedItem;
			update = true;
			handled = true;
			UpdateDisplayState();
		}else if(code == (KeyFlags::NonASCII | KeyCodes::UpArrow) && selectedItem > 0){
			--selectedItem;
			update = true;
			handled = true;
			UpdateDisplayState();
		}else if(code == (KeyFlags::NonASCII | KeyCodes::PageUp)){
			if(selectedItem != 0){
				if(selectedItem > visibleItems) selectedItem -= visibleItems;
				else selectedItem = 0;
				update = true;
				UpdateDisplayState();
			}
			handled = true;
		}else if(code == (KeyFlags::NonASCII | KeyCodes::PageDown)){
			if(selectedItem < items.size() - 1){
				if(selectedItem + visibleItems < items.size()) selectedItem += visibleItems;
				else selectedItem = items.size() - 1;
				update = true;
				UpdateDisplayState();
			}
			handled = true;
		}else if(code == (KeyFlags::NonASCII | KeyCodes::Home)){
			if(selectedItem != 0){
				selectedItem = 0;
				update = true;
				UpdateDisplayState();
			}
			handled = true;
		}else if(code == (KeyFlags::NonASCII | KeyCodes::End)){
			if(selectedItem < items.size() - 1){
				selectedItem = items.size() - 1;
				update = true;
				UpdateDisplayState();
			}
			handled = true;
		}else if(!(code & KeyFlags::NonASCII)){
			auto oldSelectedItem = selectedItem;
			char c = KB_char(e.Key.code);
			if(multiSelect && (c == ' ' || c == '\n')){
				multiSelection[selectedItem] = !multiSelection[selectedItem];
				update = true;
			}else{
				c = std::tolower(c);
				auto finder = [&](const std::vector<std::string> &item){
					return !item.empty() && !item.front().empty() && std::tolower(item.front().front()) == c;
					
				};
				auto it = std::find_if(begin(items) + selectedItem + 1, end(items), finder);
				if(it == end(items)) it = std::find_if(begin(items), end(items), finder);
				if(it != end(items)) selectedItem = it - begin(items);
				update = oldSelectedItem != selectedItem;
			}
			handled = true;
			UpdateDisplayState();
		}
	}else if(e.type & wm_PointerEvents){
		if(InRect(e.Pointer.x, e.Pointer.y, rect)){
			if(e.type == wm_EventType::PointerButtonUp){
				auto oldSelectedItem = selectedItem;
				selectedItem = ((e.Pointer.y - outerRect.y) / fontHeight) + vOffset - 1;
				if(selectedItem < items.size()) update = oldSelectedItem != selectedItem;
				if(multiSelect && (e.Pointer.x - outerRect.x) < checkSize + 1){
					multiSelection[selectedItem] = !multiSelection[selectedItem];
					update = true;
				}
				handled = true;
				UpdateDisplayState();
			}
		}else if(hscroll && hscroll->IsEnabled() && InRect(e.Pointer.x, e.Pointer.y, hscroll->GetInteractRect()) && (e.type & hscroll->GetSubscribed())){
			auto ret = hscroll->HandleEvent(e);
			update = ret.IsFinishedProcessing();
			handled = handled || update;
		}else if(vscroll && vscroll->IsEnabled() && InRect(e.Pointer.x, e.Pointer.y, vscroll->GetInteractRect()) && (e.type & vscroll->GetSubscribed())){
			auto ret = vscroll->HandleEvent(e);
			update = ret.IsFinishedProcessing();
			handled = handled || update;
		}
	}
	if(update) IControl::Paint(outerRect);
	return {handled};
}

void DetailList::Paint(gds::Surface &s){
	if(update || !surf){
		UpdateDisplayState(false);
		
		uint32_t inW = rect.w - 1;
		uint32_t inH = rect.h - 1;
		
		auto bkgCol = colours::GetBackground().Fix(*surf);
		auto txtCol = colours::GetDetailListText().Fix(*surf);
		auto border = colours::GetBorder().Fix(*surf);
		auto selCol = colours::GetSelection().Fix(*surf);
		auto hdrCol = colours::GetDetailListHeader().Fix(*surf);
		
		auto topLeft = colours::GetDetailListLowLight().Fix(*surf);
		auto bottomRight = colours::GetDetailListHiLight().Fix(*surf);
		surf->Clear();
		surf->BeginQueue();
		surf->Box({0, 0, rect.w, rect.h}, bkgCol, bkgCol, 1, gds_LineStyle::Solid, gds_FillStyle::Filled);

		surf->Box({1, 1, inW, fontHeight}, hdrCol, hdrCol, 1, gds_LineStyle::Solid, gds_FillStyle::Filled);
		
		for(size_t i = 0; i < cols.size(); ++i){
			auto &cItem = colItems[i];
			
			auto colOffset = std::accumulate(colWidths.begin(), colWidths.begin() + i, i + iconsize + 1);
			auto textX = ((int32_t)colOffset + 2) - (int32_t)hOffset;
			auto textY = std::max<int32_t>(((fontHeight + cItem.measures.h) / 2), 0);
			if((int32_t)colOffset + (int32_t)colWidths[i] < 0) continue;
			
			auto text = FitTextToCol(cItem, i);
			
			surf->Text({textX, textY}, text, fonts::GetDetailListFont(), fonts::GetDetailListTextSize(), txtCol);
		}
		surf->Line({1, (int32_t)fontHeight}, {(int32_t)inW, (int32_t)fontHeight}, border);
		
		auto visibleItems = (inH / fontHeight);
		
		for(auto i = vOffset; i < items.size() && i < vOffset + visibleItems; ++i){
			if(i == selectedItem){
				int32_t selY = fontHeight * ((i + 1) - vOffset);
				auto selFocus = colours::GetSelectionFocus().Fix(*surf);
				if(hasFocus){
					surf->Box({0, selY, inW, fontHeight}, selCol, selCol, 1, gds_LineStyle::Solid, gds_FillStyle::Filled);
				}
				surf->Box({0, selY, inW, fontHeight}, selFocus, selFocus, 1, gds_LineStyle::Solid);
			}
			if(hOffset < (int32_t)iconsize && (icons[i] || defaultIcon)){
				int32_t iconY = fontHeight * ((i + 1) - vOffset);
				auto icon = icons[i] ? icons[i] : defaultIcon;
				surf->Blit(*icon, {0, 0, iconsize, iconsize}, {1 - (int32_t)hOffset, iconY, iconsize, iconsize});
			}
			for(size_t j = 0; j < items[i].size(); ++j){
				if(j > cols.size()) continue;
				auto &cItem = drawItems[i][j];
				auto colOffset = std::accumulate(colWidths.begin(), colWidths.begin() + j, j + iconsize + 1);
				if((int32_t)colOffset + (int32_t)cItem.measures.w < hOffset) continue;
				
				auto textX = ((int32_t)colOffset + 2) - (int32_t)hOffset;
				auto textY = std::max<int32_t>(((fontHeight + cItem.measures.h) / 2), 0);
				textY += ((i + 1) - vOffset) * fontHeight;
				
				auto text = FitTextToCol(cItem, j);
				
				surf->Text({textX, textY}, text, fonts::GetDetailListFont(), fonts::GetDetailListTextSize(), txtCol);
			}
			if(multiSelect){
				int32_t chkY = fontHeight * ((i + 1) - vOffset);
				surf->Box({1, chkY, checkSize, checkSize}, bkgCol, bkgCol, 1, gds_LineStyle::Solid, gds_FillStyle::Filled);
				drawing::BevelBox(*surf, {2, chkY + 1, checkSize - 2, checkSize - 2}, topLeft, bottomRight);
				if(multiSelection[i]){
					surf->Line({5, chkY + 5}, {checkSize - 3, (chkY + checkSize) - 4}, txtCol, 2);
					surf->Line({5,  (chkY + checkSize) - 4}, {checkSize - 3, chkY + 5}, txtCol, 2);
				}
			}
		}
		drawing::Border(*surf, {0, 0, inW, inH}, border);
		drawing::BevelBox(*surf, {1, 1, inW - 2, inH - 2}, topLeft, bottomRight);
		
		surf->CommitQueue();
		update = false;
	}
	s.Blit(*surf, {0, 0, rect.w, rect.h}, rect);

	if(hscroll) hscroll->Paint(s);
	if(vscroll) vscroll->Paint(s);
	
	if(!enabled){
		auto cast = colours::GetDisabledCast().Fix(s);
		s.Box(outerRect, cast, cast, 1, gds_LineStyle::Solid, gds_FillStyle::Filled);
	}
}

gds::Rect DetailList::GetPaintRect(){
	return outerRect;
}

gds::Rect DetailList::GetInteractRect(){
	return outerRect;
}

uint32_t DetailList::GetSubscribed(){
	auto ret = wm_KeyboardEvents | wm_EventType::PointerButtonUp | wm_EventType::PointerButtonDown;
	if(hscroll) ret |= hscroll->GetSubscribed();
	if(vscroll) ret |= vscroll->GetSubscribed();
	return ret;
}

void DetailList::Focus(){
	if(!hasFocus){
		update = true;
		hasFocus = true;
		IControl::Paint(outerRect);
	}
}

void DetailList::Blur(){
	if(hasFocus){
		update = true;
		hasFocus = false;
		IControl::Paint(outerRect);
	}
}

uint32_t DetailList::GetFlags(){
	return 0;
}

void DetailList::Enable(){
	if(!enabled){
		enabled = true;
		IControl::Paint(rect);
	}
}

void DetailList::Disable(){
	if(enabled){
		enabled = false;
		IControl::Paint(rect);
	}
}

bool DetailList::IsEnabled(){
	return enabled;
}

void DetailList::SetPosition(const gds::Rect &r){
	outerRect = r;
	rect = scrollHoriz ? gds::Rect{r.x, r.y, r.w - scrollbarSize, r.h - scrollbarSize} : gds::Rect{r.x, r.y, r.w - scrollbarSize, r.h};
	if(vscroll) vscroll->SetPosition({outerRect.x + (int32_t)outerRect.w - scrollbarSize, outerRect.y, scrollbarSize, outerRect.h - (scrollHoriz ? scrollbarSize : 0)});
	if(hscroll) hscroll->SetPosition({outerRect.x, outerRect.y + (int32_t)outerRect.h - scrollbarSize, outerRect.w - scrollbarSize, scrollbarSize});
	update = true;
	surf.reset();
}

size_t DetailList::GetValue(){
	return selectedItem;
}

void DetailList::SetValue(size_t idx){
	if(selectedItem == idx) return;
	selectedItem = idx;
	update = true;
}

std::vector<bool> &DetailList::MulitSelections(){
	return multiSelection;
}

std::vector<std::vector<std::string>> &DetailList::Items(){
	return items;
}

std::vector<std::string> &DetailList::Columns(){
	return cols;
}

std::vector<uint32_t> &DetailList::ColumnWidths(){
	return colWidths;
}

void DetailList::Refresh(){
	update = true;
	IControl::Paint(outerRect);
	colWidths.resize(cols.size());
	icons.resize(items.size());
	multiSelection.resize(items.size());
}

void DetailList::SetDefaultIcon(std::shared_ptr<gds::Surface> img){
	defaultIcon = img;
}

void DetailList::SetItemIcon(size_t idx, std::shared_ptr<gds::Surface> img){
	icons[idx] = img;
}

void DetailList::ClearItemIcons(){
	icons.clear();
	icons.resize(items.size());
}

}
}