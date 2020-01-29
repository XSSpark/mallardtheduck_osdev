#include <btos.h>
#include <sstream>
#include <gds/libgds.h>
#include <wm/wm.h>
#include <dev/rtc.h>
#include <gds/screen.hpp>
#include <dev/terminal.hpp>
#include <dev/mouse.h>

#include "window.hpp"
#include "metrics.hpp"
#include "drawing.hpp"
#include "windows.hpp"
#include "menus.hpp"

using namespace std;
using namespace gds;

#define DBG(x) do{std::stringstream dbgss; dbgss << x << std::endl; bt_zero(dbgss.str().c_str());}while(0)

uint32_t GetMinWidth(){
	return GetMetric(ButtonSize) * 8;
}

uint32_t GetMinHeight(){
	return GetMetric(TitleBarSize) * 2;
}

Window::Window(shared_ptr<Surface> c) : content(c){
	gds_info = content->Info();
}

Window::~Window(){
}

void Window::Draw(bool active, bool drawContent, shared_ptr<Surface> target){
	Point pos = this->pos;
	if(target){
		pos.x = 0; pos.y = 0;
	}
	this->active = active;
	Surface *surf = target ? target.get() : &Screen;
	if(drawContent) surf->Blit(*content, {0, 0, gds_info.w, gds_info.h}, {pos.x + GetContentOffset().x, pos.y + GetContentOffset().y, gds_info.w, gds_info.h});
	RefreshTitleBar();
	if(HasTitleBar()) surf->Blit(*titleImage, {0, 0, gds_titleinfo.w, gds_titleinfo.h}, {pos.x, pos.y, gds_titleinfo.w, gds_titleinfo.h});
	if(HasBorder()) DrawBorder(*surf, {pos.x, pos.y, GetWidth(), GetHeight()});
	last_active = active;
}

void Window::Draw(bool active, const Rect &r){
	this->active = active;
	Point cpos = GetContentPosition();
	bool drew = false;
	if(Overlaps(r, {cpos.x, cpos.y, gds_info.w, gds_info.h})){
		Rect contentDst = {max(r.x, cpos.x), max(r.y, cpos.y), min((cpos.x + gds_info.w), (r.x + r.w)) - max(r.x, cpos.x), min((cpos.y + gds_info.h), (r.y + r.h)) - max(r.y, cpos.y)};
		Rect contentSrc = Reoriginate(contentDst, {cpos.x, cpos.y});
		Screen.Blit(*content, contentSrc, contentDst);
		drew = true;
	}
	RefreshTitleBar();
	if(Overlaps(r, {pos.x, pos.y, gds_titleinfo.w, gds_titleinfo.h})){
		Rect titleDst = {max(r.x, pos.x), max(r.y, pos.y), min((pos.x + gds_titleinfo.w), (r.x + r.w)) - max(r.x, pos.x), min((pos.y + gds_titleinfo.h), (r.y + r.h)) - max(r.y, pos.y)};
		Rect titleSrc = Reoriginate(titleDst, {pos.x, pos. y});
		if(HasTitleBar()) Screen.Blit(*titleImage, titleSrc, titleDst);
		drew = true;
	}
	if(HasBorder()) DrawBorder(Screen, {pos, GetWidth(), GetHeight()}, r);
	last_active = active;
	if(!drew){
		stringstream ss;
		ss << "WM: Draw with no overlap!" << endl;
		ss << "WM: Draw rect: (" << r.x << ", " << r.y << ") " << r.w << " x " << r.h << "." << endl;
		Rect bounding = GetBoundingRect();
		ss << "WM: Bounding rect: (" << bounding.x << ", " << bounding.y << ") " << bounding.w << " x " << bounding.h << "." << endl;
		ss << "WM: Content rect: (" << cpos.x << ", " << cpos.y << ") " << gds_info.w << " x " << gds_info.h << "." << endl;
		ss << "WM: Content rect: (" << pos.x << ", " << pos.y << ") " << gds_titleinfo.w << " x " << gds_titleinfo.h << "." << endl;
		bt_zero(ss.str().c_str());
	}
}

void Window::SetPosition(Point p){
	Rect oldrect = GetBoundingRect();
	pos = p;
	Rect newrect = GetBoundingRect();
	if(GetVisible()){
		DrawAndRefreshWindows({oldrect, newrect});
		//DrawAndRefreshWindows(newrect, id);
	}
}

Point Window::GetPosition(){
	return pos;
}

Point Window::GetContentPosition(){
	return pos + GetContentOffset();
}

void Window::SetTitle(string ntitle){
	title=ntitle;
	if(HasTitleBar()){
		RefreshTitleBar(true);
		Rect r = GetBoundingRect();
		r.h = GetMetric(TitleBarSize);
		DrawAndRefreshWindows(r);
	}
}

std::string Window::GetTitle(){
	return title;
}

shared_ptr<Surface> Window::GetSurface(){
	return content;
}

void Window::SetSurface(std::shared_ptr<gds::Surface> surf){
	auto oldRect = GetBoundingRect();
	content = surf;
	gds_info = content->Info();
	UpdateTitleBar(true);
	if(GetVisible()){
		DrawAndRefreshWindows({oldRect, GetBoundingRect()});
	}
}

void Window::SetZOrder(uint32_t zorder, bool update){
	z = zorder;
	if(update && GetVisible()){
		DrawWindows();
		RefreshScreen(GetBoundingRect());
	}
}

uint32_t Window::GetZOrder(){
	return z;
}

Rect Window::GetBoundingRect(){
	Rect ret;
	ret.x = pos.x;
	ret.y = pos.y;
	if(HasBorder()){
		ret.w = gds_info.w + (2 * GetMetric(BorderWidth));
		if(HasTitleBar()){
			ret.h = gds_info.h + GetMetric(BorderWidth) + GetMetric(TitleBarSize);
		}else{
			ret.h = gds_info.h + (2 * GetMetric(BorderWidth));
		}
	}else{
		ret.w = gds_info.w;
		ret.h = gds_info.h;
	}
	return ret;
}

uint32_t Window::GetWidth(){
	return GetBoundingRect().w;
}

uint32_t Window::GetHeight(){
	return GetBoundingRect().h;
}

Point Window::GetContentOffset(){
	if(HasBorder()){
		if(HasTitleBar()){
			return {GetMetric(BorderWidth), GetMetric(TitleBarSize)}; 
		}else{
			return {GetMetric(BorderWidth), GetMetric(BorderWidth)}; 
		}
	}else{
		return {0, 0};
	}
}

bool Window::HasTitleBar(){
	return !(options & wm_WindowOptions::NoTitle);
}

bool Window::HasBorder(){
	return (options & wm_WindowOptions::NoFrame) != wm_WindowOptions::NoFrame;
}

void Window::KeyInput(uint32_t key){
	auto mwin = modal.lock();
	if(mwin){
		BringToFront(mwin);
		return;
	}
	//stringstream ss;
	//ss << "WM: Window '" << title << "' key input:" << key << endl;
	//bt_zero(ss.str().c_str());
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::Keyboard)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::Keyboard;
		e.Key.code = key;
		client->SendEvent(e);
	}
}

void DrawAndRefreshRectEdges(const Rect &r, uint32_t lineWidth){
	if(r.x >= 0) DrawAndRefreshWindows({r.x, r.y, lineWidth, r.h});
	if(r.y >= 0) DrawAndRefreshWindows({r.x, r.y, r.w, lineWidth});
	DrawAndRefreshWindows({r.x + (int32_t)r.w - 1, r.y, lineWidth, r.h});
	DrawAndRefreshWindows({r.x, r.y + (int32_t)r.h - 1, r.w, lineWidth});
}

void RefreshRectEdges(const Rect &r, uint32_t lineWidth){
	if(r.x >= 0) RefreshScreen({r.x, r.y, lineWidth, r.h});
	if(r.y >= 0) RefreshScreen({r.x, r.y, r.w, lineWidth});
	RefreshScreen({r.x + (int32_t)r.w - 1, r.y, lineWidth, r.h});
	RefreshScreen({r.x, r.y + (int32_t)r.h - 1, r.w, lineWidth});
}

void Window::PointerInput(const bt_terminal_pointer_event &pevent){
	auto mwin = modal.lock();
	if(mwin){
		mwin->SetZOrder(z + 1, true);
		return;
	}
	if(dragMode == DragMode::Move){
		Point curpos = Point(pevent.x, pevent.y);
		Point newpos = {curpos.x - dragoffset.x, curpos.y - dragoffset.y};
		if(GetMetric(FullWindowDrag)){
			bool firstFrame = false;
			if(!dragImage){
				dragImage = make_shared<Surface>(gds_SurfaceType::Bitmap, GetBoundingRect().w, GetBoundingRect().h, 100, gds_info.colourType);
				Draw(active, true, dragImage);
				SetVisible(false, true);
				firstFrame = true;
			}
			if(firstFrame || newpos.x != pos.x || newpos.y != pos.y){
				Rect oldrect = GetBoundingRect();
				DrawWindows(oldrect, 0, true);
				auto h = oldrect.h;
				if(HasTitleBar()) h += GetMetric(TitleBarSize);
				Screen.Blit(*dragImage, {0, 0, oldrect.w, h}, {newpos.x, newpos.y, oldrect.w, h});
				pos = newpos;
				Rect newrect = GetBoundingRect();
				RefreshScreen(TileRects(oldrect, newrect));
			}
		}else if(newpos.x != pos.x || newpos.y != pos.y){
			DrawAndRefreshRectEdges({last_drag_pos.x, last_drag_pos.y, GetWidth(), GetHeight()}, GetMetric(BorderWidth));
			if(HasBorder()) DrawBorder(Screen, {newpos.x, newpos.y, GetWidth(), GetHeight()});
			last_drag_pos = newpos;
			RefreshRectEdges({last_drag_pos.x, last_drag_pos.y, GetWidth(), GetHeight()}, GetMetric(BorderWidth));
		}
		if(pevent.type == bt_terminal_pointer_event_type::ButtonUp){
			UnGrab();
			dragMode = DragMode::None;
			auto oldpos = pos;
			SetPosition(newpos);
			if(GetMetric(FullWindowDrag)){
				if(dragImage){
					dragImage.reset();
				}
				SetVisible(true, false);
				DrawAndRefreshWindows(TileRects({oldpos.x, oldpos.y, GetWidth(), GetHeight()}, GetBoundingRect()));
			}else{
				Rect winRect = {last_drag_pos.x, last_drag_pos.y, GetWidth() + 10, GetHeight()};
				DrawWindows(winRect);
				RefreshRectEdges({last_drag_pos.x, last_drag_pos.y, GetWidth(), GetHeight()}, GetMetric(BorderWidth));
			}
			if(newpos.x != pre_move_pos.x || newpos.y != pre_move_pos.y) Move(newpos);
			else PointerInput(pevent);
			last_drag_pos = {0, 0};
		}
	}else if(dragMode == DragMode::Resize){
		if((int32_t)pevent.x > pos.x && (int32_t)pevent.y > pos.y){
			Point newpos = Reoriginate({(int32_t)pevent.x, (int32_t)pevent.y}, pos);
			newpos = Reoriginate(newpos, resize_origin);
			if((int32_t)GetWidth() + newpos.x < (int32_t)GetMinWidth()){
				newpos.x = GetMinWidth() - GetWidth();
			}
			if((int32_t)GetHeight() + newpos.y < (int32_t)GetMinHeight()){
				newpos.y = GetMinHeight() - GetHeight();
			}
			if(newpos.x != last_drag_pos.x || newpos.y != last_drag_pos.y){
				DrawAndRefreshRectEdges({pos.x, pos.y, GetWidth() + last_drag_pos.x, GetHeight() + last_drag_pos.y}, GetMetric(BorderWidth));
				DrawBorder(Screen, {pos.x, pos.y, GetWidth() + newpos.x, GetHeight() + newpos.y});
				RefreshRectEdges({pos.x, pos.y, GetWidth() + newpos.x, GetHeight() + newpos.y}, GetMetric(BorderWidth));
				last_drag_pos = newpos;
			}
		}
		if(pevent.type == bt_terminal_pointer_event_type::ButtonUp){
			UnGrab();
			dragMode = DragMode::None;
			RefreshRectEdges({pos.x, pos.y, GetWidth() + last_drag_pos.x, GetHeight() + last_drag_pos.y}, GetMetric(BorderWidth));
			/*if(last_drag_pos.x || last_drag_pos.y)*/ Resize(gds_info.w + last_drag_pos.x, gds_info.h + last_drag_pos.y);
			//else PointerInput(pevent);
			last_drag_pos = {0, 0};
		}
	}else{
		Point epoint = Reoriginate(Point(pevent.x, pevent.y), pos);
		WindowArea over = GetWindowArea(epoint);
		if(pevent.type == bt_terminal_pointer_event_type::ButtonDown && pevent.button == 1){
			pressed = over;
			if(pressed != WindowArea::Content){
				if(pressed == WindowArea::Title){
					WindowGrab(id);
					dragoffset = epoint;
					last_drag_pos = pos;
					pre_move_pos = pos;
					dragMode = DragMode::Move;
				}else{
					RefreshTitleBar(true);
					Rect r = GetBoundingRect();
					r.h = GetMetric(TitleBarSize);
					DrawAndRefreshWindows(r, id);
				}
			}
		}
		if(pevent.type == bt_terminal_pointer_event_type::ButtonUp && pevent.button == 1 && pressed != WindowArea::None){
			if(pressed == over){
				if(pressed == WindowArea::MenuButton && !(options & wm_WindowOptions::NoMenu)) OpenMenu();
				if(pressed == WindowArea::CloseButton && !(options & wm_WindowOptions::NoClose)) Close();
				if(pressed == WindowArea::HideButton && !(options & wm_WindowOptions::NoHide)) Hide();
				if(pressed == WindowArea::ExpandButton && !(options & wm_WindowOptions::NoExpand)) Expand();
			}
			pressed = WindowArea::None;
			RefreshTitleBar(true);
			Rect r = GetBoundingRect();
			r.h = GetMetric(TitleBarSize);
			DrawAndRefreshWindows(r, id);
		}
		if(over == WindowArea::Content){
			wm_Event e;
			if(pevent.type == bt_terminal_pointer_event_type::ButtonDown) e.type = wm_EventType::PointerButtonDown;
			else if(pevent.type == bt_terminal_pointer_event_type::ButtonUp) e.type = wm_EventType::PointerButtonUp;
			else if(pevent.type == bt_terminal_pointer_event_type::Move) e.type = wm_EventType::PointerMove;
			else return;
			shared_ptr<Client> client = owner.lock();
			if(client && (e.type & event_subs) == (uint32_t)e.type){
				Point cpoint = Reoriginate(epoint, GetContentOffset());
				e.window_id = id;
				e.Pointer.x = cpoint.x;
				e.Pointer.y = cpoint.y;
				e.Pointer.button = pevent.button;
				client->SendEvent(e);
			}
		}
	}
}

void Window::DrawGrabbed(const Rect &r){
	if(!Overlaps(r, GetBoundingRect())) return;
	if(dragMode == DragMode::Move){
		if(GetMetric(FullWindowDrag)){
			Rect dstRect = Intersection(r, GetBoundingRect());
			Rect srcRect = Reoriginate(dstRect, pos);
			DBG("WM: dstRect: (" << dstRect.x << ", " << dstRect.y << ") " << dstRect.w << " x " << dstRect.h);
			DBG("WM: srcRect: (" << srcRect.x << ", " << srcRect.y << ") " << srcRect.w << " x " << srcRect.h);
			if(!dragImage){
				dragImage = make_shared<Surface>(gds_SurfaceType::Bitmap, GetBoundingRect().w, GetBoundingRect().h, 100, gds_info.colourType);
				Draw(active, true, dragImage);
				SetVisible(false, false);
			}
			Screen.Blit(*dragImage, {srcRect.x, srcRect.y, srcRect.w, srcRect.h}, {dstRect.x, dstRect.y, dstRect.w, dstRect.h});
			RefreshScreen(r);
		}else{
			DrawBorder(Screen, {pos.x, pos.y, gds_info.w + GetWidth(), GetHeight()});
			RefreshRectEdges({pos.x, pos.y, GetWidth(), GetHeight()}, GetMetric(BorderWidth));
		}
	}
}

void Window::PointerEnter(){
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::PointerEnter)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::PointerEnter;
		client->SendEvent(e);
	}
}

void Window::PointerLeave(){
	if(pressed != WindowArea::None){
		pressed = WindowArea::None;
		RefreshTitleBar();
	}
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::PointerLeave)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::PointerLeave;
		client->SendEvent(e);
	}
}

void Window::SetVisible(bool v, bool update){
	bool oldvisible = GetVisible();
	if(v) options |= wm_WindowOptions::Visible;
	else options &= ~wm_WindowOptions::Visible;
	if(oldvisible != GetVisible() && update){
		if(v) DrawWindows(GetBoundingRect(), id);
		else DrawWindows(GetBoundingRect());
		RefreshScreen(GetBoundingRect());
	}
}

bool Window::GetVisible(){
	return (options & wm_WindowOptions::Visible);
}

WindowArea Window::GetWindowArea(Point p){
	if(!HasBorder()) return WindowArea::Content;
	gds_SurfaceInfo info = content->Info();
	if(p.x >= GetMetric(BorderWidth) && p.x < GetMetric(BorderWidth) + (int32_t)info.w){
		if(p.y >= GetMetric(BorderWidth)){
			if(p.y >= GetMetric(TitleBarSize) || !HasTitleBar()){
				int32_t h = info.h;
				if(HasTitleBar()) h += GetMetric(TitleBarSize);
				if(p.y < h) return WindowArea::Content;
				else return WindowArea::Border;
			}else{
				if(p.x < GetMetric(MenuButtonWidth) + GetMetric(BorderWidth)) return WindowArea::MenuButton;
				if(p.x > (int32_t)info.w - GetMetric(ButtonSize)) return WindowArea::ExpandButton;
				if(p.x > (int32_t)info.w - (GetMetric(ButtonSize) * 2)) return WindowArea::HideButton;
				if(p.x > (int32_t)info.w - (GetMetric(ButtonSize) * 3)) return WindowArea::CloseButton;
				return WindowArea::Title;
			}
		}else return WindowArea::Border;
	}else{
		return WindowArea::Border;
	}
}

void Window::RefreshTitleBar(bool force){
	if(UpdateTitleBar(force)){
		Rect r = GetBoundingRect();
		r.h = GetMetric(TitleBarSize);
		DrawBorder(Screen, {pos, GetWidth(), GetHeight()}, r);
	}
}

bool Window::UpdateTitleBar(bool force){
	if(!HasTitleBar()) return false;
	if(force || active != last_active){
		titleImage = titlebar.Draw(GetWidth(), title, active, options, pressed);
		gds_titleinfo = titleImage->Info();
		return true;
	}
	return false;
}

void Window::OpenMenu(){
	if(windowMenu){
		auto t = GetWindowMenuTemplate();
		auto m = MergeMenus(t, windowMenu, windowMenu->id);
		::OpenMenu(m, shared_from_this(), {pos.x, pos.y + GetMetric(TitleBarSize)});
	}else{
		auto m = GetDefaultWindowMenu();
		::OpenMenu(m, shared_from_this(), {pos.x, pos.y + GetMetric(TitleBarSize)});
	}	
}

void Window::Close(){
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::Close)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::Close;
		client->SendEvent(e);
	}
}

void Window::Hide(){
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::Hide)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::Hide;
		client->SendEvent(e);
	}
}

void Window::Expand(){
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::Expand)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::Expand;
		client->SendEvent(e);
	}
}

void Window::MenuAction(uint64_t menu, uint32_t action){
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::MenuSelection)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::MenuSelection;
		e.Menu.menu_id = menu;
		e.Menu.action = action;
		client->SendEvent(e);
	}
}

void Window::Move(Point newpos){
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::Move)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::Move;
		e.MoveResize.x = newpos.x;
		e.MoveResize.y = newpos.y;
		e.MoveResize.w = GetWidth();
		e.MoveResize.h = GetHeight();
		client->SendEvent(e);
	}
}

void Window::Resize(uint32_t w, uint32_t h){
	SetVisible(true, false);
	shared_ptr<Client> client = owner.lock();
	if(client && (event_subs & wm_EventType::Resize)){
		wm_Event e;
		e.window_id = id;
		e.type = wm_EventType::Resize;
		e.MoveResize.x = pos.x;
		e.MoveResize.y = pos.y;
		e.MoveResize.w = w;
		e.MoveResize.h = h;
		client->SendEvent(e);
	}
}

void Window::GlobalEvent(wm_EventType::Enum et, std::shared_ptr<Window> win){
	auto client = owner.lock();
	if(client && (event_subs & et)){
		wm_Event e;
		e.window_id = id;
		e.type = et;
		e.Global.window_id = win->id;
		client->SendEvent(e);
	}
}

void Window::SetOwner(std::weak_ptr<Client> o){
	owner = o;
}

void Window::Subscribe(uint32_t subs){
	event_subs = subs;
}

uint32_t Window::Subscribe(){
	return event_subs;
}

void Window::SetOptions(uint32_t opts){
	if((opts & wm_WindowOptions::Unlisted) && !(options & wm_WindowOptions::Unlisted)) SendGlobalEvent(wm_EventType::GlobalRemove, shared_from_this());
	if(!(opts & wm_WindowOptions::Unlisted) && (options & wm_WindowOptions::Unlisted)) SendGlobalEvent(wm_EventType::GlobalAdd, shared_from_this());
	options = opts;
	if(GetVisible()) DrawWindows(GetBoundingRect(), id);
	else DrawWindows(GetBoundingRect());
	RefreshScreen(GetBoundingRect());
}

uint32_t Window::GetOptions(){
	return options;
}

void Window::SetWindowMenu(shared_ptr<Menu> menu){
	windowMenu = menu;
}

shared_ptr<Menu> Window::GetWindowMenu(){
	return windowMenu;
}

void Window::StartResize(){
	auto info = Terminal().GetPointerInfo();
	if((info.flags & MouseFlags::Button1)){
		Point epoint = Reoriginate(Point(info.x, info.y), pos);
		resize_origin = epoint;
		WindowGrab(id);
		dragoffset = epoint;
		dragMode = DragMode::Resize;
	}
}

void Window::StartDrag(){
	auto info = Terminal().GetPointerInfo();
	if((info.flags & MouseFlags::Button1)){
		Point epoint = Reoriginate(Point(info.x, info.y), pos);
		last_drag_pos = pos;
		pre_move_pos = pos;
		WindowGrab(id);
		dragoffset = epoint;
		dragMode = DragMode::Move;
	}
}

void Window::SetModal(std::weak_ptr<Window> win){
	modal = win;
	auto mwin = modal.lock();
	if(mwin){
		mwin->SetZOrder(z + 1, true);
	}
}

void Window::ClearModal(){
	modal.reset();
}

std::shared_ptr<Client> Window::GetOwner(){
	return owner.lock();
}

void Window::Compress(){
	content->Compress();
}