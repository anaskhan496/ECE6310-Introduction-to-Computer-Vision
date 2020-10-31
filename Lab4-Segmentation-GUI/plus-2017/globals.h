
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif
#define MAX_QUEUE 10000	/* max perimeter size (pixels) of border wavefront */
#define MAX_FILENAME_CHARS	320

char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

// Display flags
int		ShowPixelCoords, ShowBigDots, GrowRegionPlay, GrowRegionStep, GrowRegionRestore,RedColor, GreenColor, BlueColor;
COLORREF Colors;

// Image data
unsigned char* OriginalImage;
int				ROWS, COLS;

#define TIMER_SECOND	1			/* ID of timer used for animation */

// Drawing flags
int		TimerRow, TimerCol;
int		ThreadRow, ThreadCol;
int		ThreadRunning;
int	TotalRegions = 0;
int flag;
int r2, c2, r, c, g;
int predicate1, predicate2;


INT_PTR CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void PaintImage();
void AnimationThread(void*);		/* passes address of window */
void Inter_func(void*);
