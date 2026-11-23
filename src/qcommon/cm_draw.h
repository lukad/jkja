///////////////////////////////////////////////////////////////////////////////
// CDraw32 Class Interface
//
// Basic drawing routines for 32-bit per pixel buffer
///////////////////////////////////////////////////////////////////////////////

#if !defined(CM_DRAW_H_INC)
#define CM_DRAW_H_INC

// calc offset into image array for a pixel at (x,y)
#define PIXPOS(x,y,stride) (((y)*(stride))+(x))

#ifndef MIN
// handy macros
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x)<0 ? -(x):(x))
#define SIGN(x)  (((x) < 0) ? -1 : (((x) > 0) ? 1 : 0))
#endif

#ifndef CLAMP
#define SWAP(a,b) { a^=b; b^=a; a^=b; }
#define SQR(a)  ((a)*(a))
#define CLAMP(v,l,h)    ((v)<(l) ? (l) : (v) > (h) ? (h) : (v))
#define LERP(t, a, b) (((b)-(a))*(t) + (a))

// round a to nearest integer towards 0
#define FLOOR(a)        ((a)>0 ? (int)(a) : -(int)(-a))

// round a to nearest integer away from 0
#define CEILING(a) \
((a)==(int)(a) ? (a) : (a)>0 ? 1+(int)(a) : -(1+(int)(-a)))

#include <stdlib.h>
#endif

class CPixel32
{
public:
	byte r;
	byte g;
	byte b;
	byte a;

	CPixel32(byte R = 0, byte G = 0, byte B = 0, byte A = 255) : r(R), g(G), b(B), a(A) {}
	CPixel32(int l) {r = (l >> 24) & 0xff; g = (l >> 16) & 0xff; b = (l >> 8) & 0xff; a = l & 0xff;};

	~CPixel32()
	{}
};

#define PIX32_SIZE sizeof(CPixel32)

// standard image operator macros
#define IMAGE_SIZE(width,height) ((width)*(height)*(PIX32_SIZE))


inline CPixel32 AVE_PIX (CPixel32 x, CPixel32 y) 
	{ CPixel32 t; t.r = (byte)(((int)x.r + (int)y.r)>>1); 
				  t.g = (byte)(((int)x.g + (int)y.g)>>1); 
				  t.b = (byte)(((int)x.b + (int)y.b)>>1); 
				  t.a = (byte)(((int)x.a + (int)y.a)>>1); return t;}

inline CPixel32 ALPHA_PIX (CPixel32 x, CPixel32 y, int alpha, int inv_alpha) 
	{ CPixel32 t; t.r = (byte)((x.r*alpha + y.r*inv_alpha)>>8); 
				  t.g = (byte)((x.g*alpha + y.g*inv_alpha)>>8); 
				  t.b = (byte)((x.b*alpha + y.b*inv_alpha)>>8); 
//				  t.a = (byte)((x.a*alpha + y.a*inv_alpha)>>8);  return t;}
				  t.a = y.a;  return t;}

inline CPixel32 LIGHT_PIX (CPixel32 p, int light) 
{ CPixel32 t; 
  t.r = (byte)CLAMP(((p.r * light)>>10) + p.r, 0, 255); 
  t.g = (byte)CLAMP(((p.g * light)>>10) + p.g, 0, 255); 
  t.b = (byte)CLAMP(((p.b * light)>>10) + p.b, 0, 255); 
  t.a = p.a;  return t;}

// Colors are 32-bit RGBA

// a vertex of DrawPolygon
struct drawPoint_t
{
	int x;
	int y;
};

// draw class
class CDraw32
{
public: // static drawing context - static so we set only ONCE for many draw calls
	static	CPixel32*	buffer;	   			// pointer to pixel buffer (one active)
	static	int	buf_width;				// size of buffer
	static	int	buf_height;				// size of buffer
	static	int	stride;					// stride of buffer in pixels
	static	int	clip_min_x;				// clip bounds
	static	int	clip_min_y;				// clip bounds
	static	int	clip_max_x;				// clip bounds
	static	int	clip_max_y;				// clip bounds
	static	int*	row_off;				// Table for quick Y calculations

private:
	void BlitClip(int& dstX, int& dstY, 
		 		  int& width, int& height, 
				  int& srcX, int& srcY);

protected:
public:
	CDraw32();						// constructor
	~CDraw32();						// destructor

	// set the rect to clip drawing functions to
	static void	SetClip(int min_x, int min_y,int max_x, int max_y)
						{clip_min_x = MAX(min_x,0); clip_max_x = MIN(max_x,buf_width-1);
						 clip_min_y = MAX(min_y,0); clip_max_y = MIN(max_y,buf_height-1);}

	static void GetClip(int& min_x, int& min_y,int& max_x, int& max_y)
						{min_x = clip_min_x; min_y = clip_min_y;
						 max_x = clip_max_x; max_y = clip_max_y; }

	// set the buffer to use for drawing off-screen
	static void	SetBuffer(CPixel32* buf) {buffer = buf;};

	// set the dimensions of the off-screen buffer
	static bool	SetBufferSize(int width,int height,int stride_len);

	// call this to free the table for quick y calcs before the program ends
	static void CleanUp(void) 
		{if (row_off) delete [] row_off; row_off=NULL; buf_width=0; buf_height=0;}

	// set a pixel at (x,y) to color (no clipping)
	void				PutPixNC(int x, int y, CPixel32 color)
		{buffer[row_off[y] + x] = color;}

	// set a pixel at (x,y) to color
	void				PutPix(int x, int y, CPixel32 color)
		{	// clipping check
			if (x < clip_min_x || x > clip_max_x ||
			    y < clip_min_y || y > clip_max_y)
	  			return;
			PutPixNC(x,y,color);
		}

	// get the color of a pixel at (x,y)
	CPixel32			GetPix(int x, int y)
		{return buffer[row_off[y] + x];}

	// set a pixel at (x,y) with 50% translucency (no clip)
	void				PutPixAveNC(int x, int y, CPixel32 color)
		{	PutPixNC(x,y,AVE_PIX(GetPix(x, y), color)); }

	// set a pixel at (x,y) with 50% translucency
	void				PutPixAve(int x, int y, CPixel32 color)
		{	// clipping check
			if (x < clip_min_x || x > clip_max_x ||
				y < clip_min_y || y > clip_max_y)
		  		return;
			PutPixNC(x,y,AVE_PIX(GetPix(x, y), color));
		}

	// set a pixel at (x,y) with translucency level (no clip)
	void				PutPixAlphaNC(int x, int y, CPixel32 color)
		{	PutPixNC(x,y,ALPHA_PIX(color, GetPix(x, y), color.a, 256-color.a));}

	// set a pixel at (x,y) with translucency level
	void				PutPixAlpha(int x, int y, CPixel32 color)
		{	// clipping check
		if (x < clip_min_x || x > clip_max_x ||
		    y < clip_min_y || y > clip_max_y)
  		return;
		PutPixNC(x,y,ALPHA_PIX(color, GetPix(x, y), color.a, 256-color.a));}

	// clear screen buffer to color from start to end line
	void				ClearLines(CPixel32 color,int start,int end);

	// clear screen buffer to color provided
	void				ClearBuffer(CPixel32 color)
		{ClearLines(color,0,buf_height-1);};

	// fill buffer alpha from start to end line
	void				SetAlphaLines(byte alpha,int start,int end);

	// clear screen buffer to color provided
	void				SetAlphaBuffer(byte alpha)
		{SetAlphaLines(alpha,0,buf_height-1);};

	// clip a line segment to the clip rect
	bool				ClipLine(int& x1, int& y1, int& x2, int& y2);

	// draw a solid colored line, no clipping
	void				DrawLineNC(int x1, int y1, int x2, int y2, CPixel32 color);
	
	// draw a solid color line
	void				DrawLine(int x1, int y1, int x2, int y2, CPixel32 color)
		{	if (ClipLine(x1,y1,x2,y2)) DrawLineNC(x1,y1,x2,y2,color);}

	void				DrawLineAveNC(int x1, int y1, int x2, int y2, CPixel32 color);

	// draw a translucent solid color line
	void				DrawLineAve(int x1, int y1, int x2, int y2, CPixel32 color)
		{	if (ClipLine(x1,y1,x2,y2)) DrawLineAveNC(x1,y1,x2,y2,color);}

	// draw an anti-aliased line, no clipping
	void				DrawLineAANC(int x0, int y0, int x1, int y1, CPixel32 color);

	// draw an anti-aliased line
	void				DrawLineAA(int x1, int y1, int x2, int y2, CPixel32 color)
		{	if (ClipLine(x1,y1,x2,y2)) DrawLineAANC(x1,y1,x2,y2,color);}

	// draw a filled rectangle, no clipping
	void				DrawRectNC(int ulx, int uly, int width, int height,CPixel32 color);
	
	// draw a filled rectangle
	void				DrawRect(int ulx, int uly, int width, int height, CPixel32 color);

	// draw a filled rectangle
	void				DrawRectAve(int ulx, int uly, int width, int height,CPixel32 color);

	// draw a box (unfilled rectangle) no clip
	void				DrawBoxNC(int ulx, int uly, int width, int height, CPixel32 color);

	// draw a box (unfilled rectangle)
	void				DrawBox(int ulx, int uly, int width, int height, CPixel32 color);

	// draw a box (unfilled rectangle)
	void				DrawBoxAve(int ulx, int uly, int width, int height, CPixel32 color);

	// draw a circle with fill and edge colors 
	void				DrawCircle(int xc, int yc, int r, CPixel32 edge, CPixel32 fill);

	// draw a circle with fill and edge colors averaged with dest 
	void				DrawCircleAve(int xc, int yc, int r, CPixel32 edge, CPixel32 fill);

	// draw a polygon (complex) with fill and edge colors 
	void				DrawPolygon(int nvert, drawPoint_t *point, CPixel32 edge, CPixel32 fill);

	// simple blit function
	void				BlitNC(int dstX, int dstY, int dstWidth, int dstHeight, 
							   CPixel32* srcImage, int srcX, int srcY, int srcStride);

	void				Blit(int dstX, int dstY, int dstWidth, int dstHeight, 
							   CPixel32* srcImage, int srcX, int srcY, int srcStride);

	// blit image times color
	void				BlitColor(int dstX, int dstY, int dstWidth, int dstHeight, 
								  CPixel32* srcImage, int srcX, int srcY, int srcStride, CPixel32 color);

	void				Emboss(int dstX, int dstY, int width, int height, 
							    CPixel32* clrImage, int clrX, int clrY, int clrStride);
};

///////////////////////////////////////////////////////////////////////////////
#endif
