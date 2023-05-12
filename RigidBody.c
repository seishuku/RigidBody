#include <windows.h>
#include <ddraw.h>
#include <dsound.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <intrin.h>
#include "math.h"
#include "physics.h"

LPDIRECTDRAW7 lpDD=NULL;
LPDIRECTDRAWSURFACE7 lpDDSFront=NULL;
LPDIRECTDRAWSURFACE7 lpDDSBack=NULL;
HWND hWnd=NULL;

char szAppName[256]="DirectDraw";

int Width=512, Height=512;

bool Done=false, Key[256];
bool MouseDrag=false;

unsigned __int64 Frequency, StartTime, EndTime;
float fTimeStep, fTime=0.0f;

float X=0.0f, Y=0.0f;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Render(void);
int Init(void);
int Create(void);
void Destroy(void);

unsigned __int64 rdtsc(void)
{
	return __rdtsc();
}

unsigned __int64 GetFrequency(void)
{
	unsigned __int64 TimeStart, TimeStop, TimeFreq;
	unsigned __int64 StartTicks, StopTicks;
	volatile unsigned __int64 i;

	QueryPerformanceFrequency((LARGE_INTEGER *)&TimeFreq);

	QueryPerformanceCounter((LARGE_INTEGER *)&TimeStart);
	StartTicks=rdtsc();

	for(i=0;i<1000000;i++);

	StopTicks=rdtsc();
	QueryPerformanceCounter((LARGE_INTEGER *)&TimeStop);

	return (StopTicks-StartTicks)*TimeFreq/(TimeStop-TimeStart);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
	WNDCLASS wc;
	wc.style=CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInstance;
	wc.hIcon=LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor=LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground=GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=szAppName;

	RegisterClass(&wc);

	RECT WindowRect;
	WindowRect.left=0;
	WindowRect.right=Width*2;
	WindowRect.top=0;
	WindowRect.bottom=Height*2;

	AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd=CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);

	if(!Create())
	{
		DestroyWindow(hWnd);
		return -1;
	}

	if(!Init())
	{
		Destroy();
		DestroyWindow(hWnd);
		return -1;
	}

	Frequency=GetFrequency();
	EndTime=rdtsc();

	MSG msg={ 0 };
	while(!Done)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message==WM_QUIT)
				Done=1;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			RECT RectSrc, RectDst;
			POINT Point={ 0, 0 };

			StartTime=EndTime;
			EndTime=rdtsc();

			if(IDirectDrawSurface7_IsLost(lpDDSBack)==DDERR_SURFACELOST)
				IDirectDrawSurface7_Restore(lpDDSBack);

			if(IDirectDrawSurface7_IsLost(lpDDSFront)==DDERR_SURFACELOST)
				IDirectDrawSurface7_Restore(lpDDSFront);

			Render();

			fTimeStep=(float)(EndTime-StartTime)/Frequency;
			fTime+=fTimeStep;

			ClientToScreen(hWnd, &Point);
			GetClientRect(hWnd, &RectDst);
			OffsetRect(&RectDst, Point.x, Point.y);
			SetRect(&RectSrc, 0, 0, Width, Height);

			IDirectDrawSurface7_Blt(lpDDSFront, &RectDst, lpDDSBack, &RectSrc, DDBLT_WAIT, NULL);

			sprintf(szAppName, "FPS: %0.4f", 1.0f/fTimeStep);
			SetWindowText(hWnd, szAppName);
		}
	}

	Destroy();
	DestroyWindow(hWnd);

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static POINT old;
	POINT pos, delta;

	switch(uMsg)
	{
		case WM_CREATE:
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_DESTROY:
			break;

		case WM_SIZE:
			break;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			SetCapture(hWnd);
			ShowCursor(FALSE);

			GetCursorPos(&pos);
			old.x=pos.x;
			old.y=pos.y;

			MouseDrag=true;
			break;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			ShowCursor(TRUE);
			ReleaseCapture();
			MouseDrag=false;
			break;

		case WM_MOUSEMOVE:
			GetCursorPos(&pos);

			if(!wParam)
			{
				old.x=pos.x;
				old.y=pos.y;
				break;
			}

			delta.x=pos.x-old.x;
			delta.y=old.y-pos.y;

			if(!delta.x&&!delta.y)
				break;

			SetCursorPos(old.x, old.y);

			switch(wParam)
			{
				case MK_LBUTTON:
					Bodies[0].position[0]+=(delta.x);
					Bodies[0].position[1]-=(delta.y);
					break;

				case MK_MBUTTON:
					break;

				case MK_RBUTTON:
					break;
			}
			break;

		case WM_KEYDOWN:
			Key[wParam]=true;

			switch(wParam)
			{
				case VK_SPACE:
					explode(Bodies);
					break;

				case VK_ESCAPE:
					PostQuitMessage(0);
					break;

				default:
					break;
			}
			break;

		case WM_KEYUP:
			Key[wParam]=false;
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Clear(LPDIRECTDRAWSURFACE7 lpDDS)
{
	DDBLTFX ddbltfx;

	ddbltfx.dwSize=sizeof(DDBLTFX);
	ddbltfx.dwFillColor=0x00000000;

	if(IDirectDrawSurface7_Blt(lpDDS, NULL, NULL, NULL, DDBLT_COLORFILL, &ddbltfx)!=DD_OK)
		return;
}

uint32_t *fb;

void point(int x, int y, uint32_t color)
{
	int xmin=0, ymin=0, xmax=Width-1, ymax=Height-1;

	if(x<xmin) return;
	if(y<ymin) return;
	if(x>xmax) return;
	if(y>ymax) return;

	fb[y*Width+x]=color;
}

void line(int x0, int y0, int x1, int y1, uint32_t color)
{
	int32_t dx=abs(x1-x0);
	int32_t dy=abs(y1-y0);
	int32_t sx=(x0<x1)?1:-1;
	int32_t sy=(y0<y1)?1:-1;
	int32_t err=dx-dy;

	while(x0!=x1||y0!=y1)
	{
		point(x0, y0, color); // set pixel color

		int e2=2*err;
		if(e2>-dy)
		{
			err-=dy;
			x0+=sx;
		}
		if(e2<dx)
		{
			err+=dx;
			y0+=sy;
		}
	}

	point(x1, y1, color); // set final pixel color
}

void circle(uint32_t x, uint32_t y, uint32_t r, uint32_t color)
{
	int32_t d;
	uint32_t curx, cury;

	if(!r)
		return;

	d=3-(r<<1);
	curx=0;
	cury=r;

	while(curx<=cury)
	{
		point(x+curx, y-cury, color);
		point(x-curx, y-cury, color);
		point(x+cury, y-curx, color);
		point(x-cury, y-curx, color);
		point(x+curx, y+cury, color);
		point(x-curx, y+cury, color);
		point(x+cury, y+curx, color);
		point(x-cury, y+curx, color);

		if(d<0)
			d+=(curx<<2)+6;
		else
		{
			d+=((curx-cury)<<2)+10;
			cury--;
		}

		curx++;
	}
}

inline uint32_t MakeRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return ((uint32_t)a<<24)|((uint32_t)r<<16)|((uint32_t)g<<8)|(uint32_t)b;
}

void Render(void)
{
	DDSURFACEDESC2 ddsd;
	HRESULT ret=DDERR_WASSTILLDRAWING;

	Clear(lpDDSBack);

	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize=sizeof(ddsd);

	while(ret==DDERR_WASSTILLDRAWING)
		ret=IDirectDrawSurface7_Lock(lpDDSBack, NULL, &ddsd, 0, NULL);

	fb=(uint32_t *)ddsd.lpSurface;

	// Loop through objects, integrate and check/resolve collisions
	for(int i=0;i<NUM_BODIES;i++)
	{
		const int subSteps=4;
		const float inv_subSteps=1.0f/(float)subSteps;

		for(int steps=0;steps<subSteps;steps++)
			integrate(&Bodies[i], fTimeStep*inv_subSteps);

		for(int j=i+1;j<NUM_BODIES;j++)
			sphere_sphere_collision(&Bodies[i], &Bodies[j]);

		sphere_aabb_collision(&Bodies[i], &aabb);
	}

	// Draw circles
	for(int i=0;i<NUM_BODIES;i++)
	{
		if(i<3)
			circle((int)Bodies[i].position[0], (int)Bodies[i].position[1], (int)Bodies[i].radius, MakeRGBA(255, 0, 0, 255));
		else
			circle((int)Bodies[i].position[0], (int)Bodies[i].position[1], (int)Bodies[i].radius, MakeRGBA(255, 255, 255, 255));
	}

	// Draw AABB
	line(aabb.position[0]-aabb.size[0]*0.5f,
		 aabb.position[1]-aabb.size[1]*0.5f,
		 aabb.position[0]-aabb.size[0]*0.5f,
		 aabb.position[1]+aabb.size[1]*0.5f, MakeRGBA(255, 255, 255, 255));
	line(aabb.position[0]+aabb.size[0]*0.5f,
		 aabb.position[1]-aabb.size[1]*0.5f,
		 aabb.position[0]+aabb.size[0]*0.5f,
		 aabb.position[1]+aabb.size[1]*0.5f, MakeRGBA(255, 255, 255, 255));
	line(aabb.position[0]-aabb.size[0]*0.5f,
		 aabb.position[1]-aabb.size[1]*0.5f,
		 aabb.position[0]+aabb.size[0]*0.5f,
		 aabb.position[1]-aabb.size[1]*0.5f, MakeRGBA(255, 255, 255, 255));
	line(aabb.position[0]-aabb.size[0]*0.5f,
		 aabb.position[1]+aabb.size[1]*0.5f,
		 aabb.position[0]+aabb.size[0]*0.5f,
		 aabb.position[1]+aabb.size[1]*0.5f, MakeRGBA(255, 255, 255, 255));

	IDirectDrawSurface7_Unlock(lpDDSBack, NULL);
}

int Init(void)
{
	Vec3_Set(aabb.position, (float)Width/2.0f, (float)Height/2.0f, 0.0f);
	Vec3_Set(aabb.velocity, 0.0f, 0.0f, 0.0f);
	Vec3_Set(aabb.size, 50.0f, 50.0f, 1.0f);
	aabb.mass=5000.0f*WORLD_SCALE;
	aabb.inverse_mass=1.0f/aabb.mass;

	// Seed random number generator
	srand(42);

	vec2 Center={ (float)Width/2.0f, (float)Height/2.0f };

	// Initialize fragments with random positions and velocities
	for(int i=0; i<NUM_BODIES; i++)
	{
		memset(&Bodies[i], 0, sizeof(RigidBody_t));

		Bodies[i].position[0]=sinf(((float)i/(NUM_BODIES+1))*2.0f*PI)*100.0f+Center[0];
		Bodies[i].position[1]=cosf(((float)i/(NUM_BODIES+1))*2.0f*PI)*100.0f+Center[1];
		Bodies[i].position[2]=0.0f;

		Bodies[i].velocity[0]=0.0f;
		Bodies[i].velocity[1]=0.0f;
		Bodies[i].velocity[2]=0.0f;

		Bodies[i].radius=10.0f;//((float)rand()/RAND_MAX)*10.0f+1.0f;

		Bodies[i].mass=100.0f*WORLD_SCALE;
		Bodies[i].inverse_mass=1.0f/Bodies[i].mass;
	}

	Bodies[0].mass=5000.0f*WORLD_SCALE;
	Bodies[0].inverse_mass=1.0f/Bodies[0].mass;
	Bodies[1].mass=5000.0f*WORLD_SCALE;
	Bodies[1].inverse_mass=1.0f/Bodies[0].mass;
	Bodies[2].mass=5000.0f*WORLD_SCALE;
	Bodies[2].inverse_mass=1.0f/Bodies[0].mass;

	return 1;
}

BOOL Create(void)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWCLIPPER lpClipper=NULL;

	if(DirectDrawCreateEx(NULL, (void *)&lpDD, &IID_IDirectDraw7, NULL)!=DD_OK)
	{
		MessageBox(hWnd, "DirectDrawCreateEx failed.", "Error", MB_OK);
		return FALSE;
	}

	if(IDirectDraw7_SetCooperativeLevel(lpDD, hWnd, DDSCL_NORMAL)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDraw7_SetCooperativeLevel failed.", "Error", MB_OK);
		return FALSE;
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize=sizeof(ddsd);
	ddsd.dwFlags=DDSD_CAPS;
	ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;

	if(IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDSFront, NULL)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDraw7_CreateSurface (Front) failed.", "Error", MB_OK);
		return FALSE;
	}

	ddsd.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;
	ddsd.dwWidth=Width;
	ddsd.dwHeight=Height;
	ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;

	if(IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDSBack, NULL)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDraw7_CreateSurface (Back) failed.", "Error", MB_OK);
		return FALSE;
	}

	if(IDirectDraw7_CreateClipper(lpDD, 0, &lpClipper, NULL)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDraw7_CreateClipper failed.", "Error", MB_OK);
		return FALSE;
	}

	if(IDirectDrawClipper_SetHWnd(lpClipper, 0, hWnd)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDrawClipper_SetHWnd failed.", "Error", MB_OK);
		return FALSE;
	}

	if(IDirectDrawSurface_SetClipper(lpDDSFront, lpClipper)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDrawSurface_SetClipper failed.", "Error", MB_OK);
		return FALSE;
	}

	if(lpClipper!=NULL)
	{
		IDirectDrawClipper_Release(lpClipper);
		lpClipper=NULL;
	}

	return TRUE;
}

void Destroy(void)
{
	if(lpDDSBack!=NULL)
	{
		IDirectDrawSurface7_Release(lpDDSBack);
		lpDDSBack=NULL;
	}

	if(lpDDSFront!=NULL)
	{
		IDirectDrawSurface7_Release(lpDDSFront);
		lpDDSFront=NULL;
	}

	if(lpDD!=NULL)
	{
		IDirectDraw7_Release(lpDD);
		lpDD=NULL;
	}
}
