#include "rect.hpp"

#include <algorithm>

using namespace std;
using namespace gds;

bool InRect(int32_t x, int32_t y, const Rect &r){
	if(x >= r.x && x < (r.x + (int32_t)r.w - 1) && y > r.y && y < (r.y + (int32_t)r.h) - 1) return true;
	else return false;
}

bool InRect(const Point &p, const Rect &r){
	return InRect(p.x, p.y, r);
}

bool Overlaps(const Rect &r1, const Rect &r2){
	return !(r1.x + (int32_t)r1.w - 1 < r2.x || r1.y + (int32_t)r1.h - 1 < r2.y || r1.x > r2.x + (int32_t)r2.w - 1 || r1.y > r2.y + (int32_t)r2.h - 1);
}

bool Contains(const Rect &r1, const Rect &r2){
	if(r1 == r2) return true;
	return (InRect(r2.x, r2.y, r1) && InRect(r2.x + r2.w, r2.y + r2.h, r1));
}

Rect Reoriginate(const Rect &r, const Point &p){
	Rect ret = r;
	ret.x -= p.x;
	ret.y -= p.y;
	return ret;
}

Point Reoriginate(const Point &pr, const Point &po){
	Point ret = pr;
	ret.x -= po.x;
	ret.y -= po.y;
	return ret;
}

vector<Rect> SplitRect(const Rect &r, const Point &p){
	if(!InRect(p, r)) return {r};
	vector<Rect> ret;
	ret.push_back(Rect(r.x, r.y, p.x - r.x, p.y - r.y));
	ret.push_back(Rect(p.x, r.y, (r.x + r.w) - p.x, p.y - r.y));
	ret.push_back(Rect(r.x, p.y, p.x - r.x, (r.y + r.h) - p.y));
	ret.push_back(Rect(p.x, p.y, (r.x + r.w) - p.x, (r.y + r.h) - p.y));
	remove_if(ret.begin(), ret.end(), [](const Rect &r){ return r.w == 0 || r.h == 0; });
	return ret;
}

vector<Rect> SplitUp(const Rect &r1, const Rect &r2){
	vector<Rect> ret;
	if(InRect(r1.x, r1.y, r2)){
		vector<Rect> splits = SplitRect(r2, {r1.x, r1.y});
		ret.insert(ret.end(), splits.begin(), splits.end());
	}
	if(InRect(r1.x + r1.w, r1.y, r2)){
		vector<Rect> splits = SplitRect(r2, {r1.x + (int32_t)r1.w, r1.y});
		ret.insert(ret.end(), splits.begin(), splits.end());
	}
	if(InRect(r1.x, r1.y + r1.h, r2)){
		vector<Rect> splits = SplitRect(r2, {r1.x, r1.y + (int32_t)r1.h});
		ret.insert(ret.end(), splits.begin(), splits.end());
	}
	if(InRect(r1.x + r1.w, r1.y + r1.h, r2)){
		vector<Rect> splits = SplitRect(r2, {r1.x + (int32_t)r1.w, r1.y + (int32_t)r1.h});
		ret.insert(ret.end(), splits.begin(), splits.end());
	}
	sort(ret.begin(), ret.end());
	ret.erase(unique(ret.begin(), ret.end()), ret.end());
	return ret;
}

vector<Rect> TileRects(const Rect &r1, const Rect &r2){
	vector<Rect> ret;
	if(!Overlaps(r1, r2)){
		ret.push_back(r1);
		ret.push_back(r2);
	}else{
		if(Contains(r1, r2)){
			ret.push_back(r1);
		}else if(Contains(r2, r1)){
			ret.push_back(r2);
		}else{
			Rect maxRect = Rect(min(r1.x, r2.x), min(r1.y, r2.y), max(r1.x + r1.w, r2.x + r2.w), max(r1.y + r1.h, r2.y + r2.h));
			maxRect.w -= maxRect.x;
			maxRect.h -= maxRect.y;
			ret.push_back(maxRect);
		}
	}
	return ret;
}

Rect Constrain(Rect r, const Rect &bounds){
	if(r.x + r.w >= bounds.w) r.w = bounds.w - r.x;
	if(r.y + r.h >= bounds.h) r.h = bounds.h - r.y;
	return r;
}

Rect Intersection(const Rect &r1, const Rect &r2){
	if(!Overlaps(r1, r2)) return { 0, 0, 0, 0 };
	return { max(r1.x, r2.x), max(r1.y, r2.y), min(r1.x + r1.w, r2.x + r2.w), min(r1.y + r1.h, r2.y + r2.h) };
}
