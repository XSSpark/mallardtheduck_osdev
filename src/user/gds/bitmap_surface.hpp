#ifndef _BITMAP_SURFACE_HPP
#define _BITMAP_SURFACE_HPP

#include "gds.hpp"
#include "surface.hpp"
#include "drawingop.hpp"

class BitmapSurface : public Surface{
protected:
	GD::Image *image;
	uint32_t scale;

public:
	BitmapSurface(size_t w, size_t h, bool indexed, uint32_t scale = 100);

	virtual size_t AddOperation(gds_DrawingOp op);
	virtual void RemoveOperation(size_t id);
	virtual gds_DrawingOp GetOperation(size_t id);
	virtual size_t GetWidth();
	virtual size_t GetHeight();
	virtual size_t GetDepth();
	virtual uint32_t GetScale();
	virtual void SetScale(uint32_t scale);
	virtual gds_SurfaceType::Enum GetType(); 
	virtual uint32_t GetColour(uint32_t r, uint32_t g, uint32_t b);
	
	virtual void Resize(size_t w, size_t h, bool indexed);

	virtual ~BitmapSurface();

public:

};

#endif //_BITMAP_SURFACE_HPP