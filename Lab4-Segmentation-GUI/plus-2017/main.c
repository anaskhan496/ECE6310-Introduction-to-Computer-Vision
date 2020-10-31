
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <process.h>	/* needed for multithreading */
#include "resource.h"
#include "globals.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				LPTSTR lpCmdLine, int nCmdShow)

{
MSG			msg;
HWND		hWnd;
WNDCLASS	wc;

wc.style=CS_HREDRAW | CS_VREDRAW;
wc.lpfnWndProc=(WNDPROC)WndProc;
wc.cbClsExtra=0;
wc.cbWndExtra=0;
wc.hInstance=hInstance;
wc.hIcon=LoadIcon(hInstance,"ID_PLUS_ICON");
wc.hCursor=LoadCursor(NULL,IDC_ARROW);
wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
wc.lpszMenuName="ID_MAIN_MENU";
wc.lpszClassName="PLUS";

if (!RegisterClass(&wc))
  return(FALSE);

hWnd=CreateWindow("PLUS","Lab4 - Region Interaction",
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT,0,400,400,NULL,NULL,hInstance,NULL);
if (!hWnd)
  return(FALSE);

ShowScrollBar(hWnd,SB_BOTH,FALSE);
ShowWindow(hWnd,nCmdShow);
UpdateWindow(hWnd);
MainWnd=hWnd;

ShowPixelCoords=0;
GrowRegionPlay = 0;
GrowRegionStep = 0;
GrowRegionRestore = 0;
RedColor = 0;
GreenColor = 0;
BlueColor = 0;
strcpy(filename,"");
OriginalImage=NULL;
ROWS=COLS=0;

InvalidateRect(hWnd,NULL,TRUE);
UpdateWindow(hWnd);

while (GetMessage(&msg,NULL,0,0))
  {
  TranslateMessage(&msg);
  DispatchMessage(&msg);
  }
return(msg.wParam);
}

LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam)

{
HMENU				hMenu;
OPENFILENAME		ofn;
FILE				*fpt;
HDC					hDC;
char				header[320],text[320];
int					BYTES,xPos,yPos;

switch (uMsg)
  {
  case WM_COMMAND:
    switch (LOWORD(wParam))
      {
	  case ID_SHOWPIXELCOORDS:
		ShowPixelCoords=(ShowPixelCoords+1)%2;
		PaintImage();
		break;
	  case ID_REGIONGROW_PLAY:
		  GrowRegionPlay = (GrowRegionPlay + 1) % 2;
		  GrowRegionStep = 0;
		  GrowRegionRestore = 0;
		  break;
	  case ID_REGIONGROW_STEP:
		  GrowRegionStep = (GrowRegionStep + 1) % 2;
		  GrowRegionPlay = 0;
		  GrowRegionRestore = 0;
		  break;
	  case ID_REGIONGROW_RESTORE:
		  GrowRegionRestore = (GrowRegionRestore + 1) % 2;
		  ThreadRunning = 0;
		  GrowRegionPlay = 0;
		  GrowRegionStep = 0;
		  PaintImage();
		  break;
	  case ID_COLORS_GREEN:
		  GreenColor = (GreenColor + 1) % 2;
		  RedColor = 0;
		  BlueColor = 0;
		  Colors = RGB(0, 255, 0);
		  break;
	  case ID_COLORS_BLUE:
		  BlueColor = (BlueColor + 1) % 2;
		  RedColor = 0;
		  GreenColor = 0;
		  Colors = RGB(0, 0, 255);
		  break;
	  case ID_COLORS_RED:
		  RedColor = (RedColor + 1) % 2;
		  BlueColor = 0;
		  GreenColor = 0;
		  Colors = RGB(255, 0, 0);
		  break;
	  case ID_GROWTHPREDICATES:
		  DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), hWnd, AboutDlgProc);
		  break;
	  case ID_FILE_LOAD:
		if (OriginalImage != NULL)
		  {
		  free(OriginalImage);
		  OriginalImage=NULL;
		  }
		memset(&(ofn),0,sizeof(ofn));
		ofn.lStructSize=sizeof(ofn);
		ofn.lpstrFile=filename;
		filename[0]=0;
		ofn.nMaxFile=MAX_FILENAME_CHARS;
		ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY;
		ofn.lpstrFilter = "PPM files\0*.ppm\0All files\0*.*\0\0";
		if (!( GetOpenFileName(&ofn))  ||  filename[0] == '\0')
		  break;		/* user cancelled load */
		if ((fpt=fopen(filename,"rb")) == NULL)
		  {
		  MessageBox(NULL,"Unable to open file",filename,MB_OK | MB_APPLMODAL);
		  break;
		  }
		fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
		if (strcmp(header,"P5") != 0  ||  BYTES != 255)
		  {
		  MessageBox(NULL,"Not a PPM (P5 greyscale) image",filename,MB_OK | MB_APPLMODAL);
		  fclose(fpt);
		  break;
		  }
		OriginalImage=(unsigned char *)calloc(ROWS*COLS,1);
		header[0]=fgetc(fpt);	/* whitespace character after header */
		fread(OriginalImage,1,ROWS*COLS,fpt);
		fclose(fpt);
		SetWindowText(hWnd,filename);
		PaintImage();
		break;

      case ID_FILE_QUIT:
        DestroyWindow(hWnd);
        break;
      }
    break;
  case WM_SIZE:		  /* could be used to detect when window size changes */
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_PAINT:
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_LBUTTONDOWN:case WM_RBUTTONDOWN:
	  if (GrowRegionPlay == 1) {
		  ThreadCol = LOWORD(lParam);
		  ThreadRow = HIWORD(lParam);
		  ThreadRunning = 1;
		  flag = 1;
		  TotalRegions++;
		  _beginthread(Inter_func, 0, MainWnd);
	  }

	  if (GrowRegionStep == 1) {
		  ThreadCol = LOWORD(lParam);
		  ThreadRow = HIWORD(lParam);
		  TotalRegions++;
		  ThreadRunning = 1;
		  flag = 1;
		  _beginthread(Inter_func, 0, MainWnd);
	  }

	  //left and right mouse presses
	  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	  break;
  case WM_MOUSEMOVE:
	if (ShowPixelCoords == 1)
	  {
	  xPos=LOWORD(lParam);
	  yPos=HIWORD(lParam);
	  if (xPos >= 0  &&  xPos < COLS  &&  yPos >= 0  &&  yPos < ROWS)
		{
		sprintf(text,"%d,%d=>%d     ",xPos,yPos,OriginalImage[yPos*COLS+xPos]);
		hDC=GetDC(MainWnd);
		TextOut(hDC,0,0,text,strlen(text));		/* draw text on the window */
		SetPixel(hDC,xPos,yPos,RGB(255,0,0));	/* color the cursor position red */
		ReleaseDC(MainWnd,hDC);
		}
	  }
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_KEYDOWN:
	if (wParam == 's'  ||  wParam == 'S')
	  PostMessage(MainWnd,WM_COMMAND,ID_SHOWPIXELCOORDS,0);	  /* send message to self */
	if (GrowRegionStep == 1) {
		if (wParam == 'j' || wParam == 'J') {
			ThreadRunning = 1;
			flag = 1;
		}
	}


	if ((TCHAR)wParam == '1')
	  {
	  TimerRow=TimerCol=0;
	  SetTimer(MainWnd,TIMER_SECOND,10,NULL);	/* start up 10 ms timer */
	  }
	if ((TCHAR)wParam == '2')
	  {
	  KillTimer(MainWnd,TIMER_SECOND);			/* halt timer, stopping generation of WM_TIME events */
	  PaintImage();								/* redraw original image, erasing animation */
	  }
	if ((TCHAR)wParam == '3')
	  {
	  ThreadRunning=1;
	  _beginthread(AnimationThread,0,MainWnd);	/* start up a child thread to do other work while this thread continues GUI */
	  }
 	if ((TCHAR)wParam == '4')
	  {
	  ThreadRunning=0;							/* this is used to stop the child thread (see its code below) */
	  }
	return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_TIMER:	  /* this event gets triggered every time the timer goes off */
	hDC=GetDC(MainWnd);
	SetPixel(hDC,TimerCol,TimerRow,RGB(0,0,255));	/* color the animation pixel blue */
	ReleaseDC(MainWnd,hDC);
	TimerRow++;
	TimerCol+=2;
	break;
  case WM_HSCROLL:	  /* this event could be used to change what part of the image to draw */
	PaintImage();	  /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_VSCROLL:	  /* this event could be used to change what part of the image to draw */
	PaintImage();
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
	break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return(DefWindowProc(hWnd,uMsg,wParam,lParam));
    break;
  }

hMenu=GetMenu(MainWnd);
if (ShowPixelCoords == 1)
  CheckMenuItem(hMenu,ID_SHOWPIXELCOORDS,MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
else
  CheckMenuItem(hMenu,ID_SHOWPIXELCOORDS,MF_UNCHECKED);

if (GrowRegionPlay == 1)
	CheckMenuItem(hMenu, ID_REGIONGROW_PLAY, MF_CHECKED);
else
	CheckMenuItem(hMenu, ID_REGIONGROW_PLAY, MF_UNCHECKED);

if (GrowRegionStep == 1)
	CheckMenuItem(hMenu, ID_REGIONGROW_STEP, MF_CHECKED);
else
	CheckMenuItem(hMenu, ID_REGIONGROW_STEP, MF_UNCHECKED);

if (GrowRegionRestore == 1) {
	CheckMenuItem(hMenu, ID_REGIONGROW_RESTORE, MF_CHECKED);
}
else if (GrowRegionRestore == 0) {
	CheckMenuItem(hMenu, ID_REGIONGROW_RESTORE, MF_UNCHECKED);
}
	

if (GreenColor == 1)
	CheckMenuItem(hMenu, ID_COLORS_GREEN, MF_CHECKED);
else
	CheckMenuItem(hMenu, ID_COLORS_GREEN, MF_UNCHECKED);

if (RedColor == 1)
	CheckMenuItem(hMenu, ID_COLORS_RED, MF_CHECKED);
else
	CheckMenuItem(hMenu, ID_COLORS_RED, MF_UNCHECKED);

if (BlueColor == 1)
	CheckMenuItem(hMenu, ID_COLORS_BLUE, MF_CHECKED);
else
	CheckMenuItem(hMenu, ID_COLORS_BLUE, MF_UNCHECKED);

DrawMenuBar(hWnd);

return(0L);
}

BOOL fRelative;
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	BOOL* Val1, * Val2;
	switch (Message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			predicate1 = GetDlgItemInt(hDlg, IDC_PREDICATE1, &Val1, fRelative);
			if (!Val1)
			{
				MessageBox(hDlg, "Couldn't retrieve Predicate 1", NULL, MB_OK);
				SendDlgItemMessage(hDlg, IDC_PREDICATE1, EM_SETSEL, 0, -1l);
			}
			predicate2 = GetDlgItemInt(hDlg, IDC_PREDICATE2, &Val2, fRelative);
			if (!Val2)
			{
				MessageBox(hDlg, "Couldn't retrieve Predicate 2", NULL, MB_OK);
				SendDlgItemMessage(hDlg, IDC_PREDICATE1, EM_SETSEL, 0, -1l);
			}
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
	default:
		return FALSE;
	}
	return FALSE;
}


void PaintImage()

{
PAINTSTRUCT			Painter;
HDC					hDC;
BITMAPINFOHEADER	bm_info_header;
BITMAPINFO			*bm_info;
int					i,r,c,DISPLAY_ROWS,DISPLAY_COLS;
unsigned char		*DisplayImage;

if (OriginalImage == NULL)
  return;		/* no image to draw */

/* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
DISPLAY_ROWS=ROWS;
DISPLAY_COLS=COLS;
if (DISPLAY_ROWS % 4 != 0)
  DISPLAY_ROWS=(DISPLAY_ROWS/4+1)*4;
if (DISPLAY_COLS % 4 != 0)
  DISPLAY_COLS=(DISPLAY_COLS/4+1)*4;
DisplayImage=(unsigned char *)calloc(DISPLAY_ROWS*DISPLAY_COLS,1);
for (r=0; r<ROWS; r++)
  for (c=0; c<COLS; c++)
	DisplayImage[r*DISPLAY_COLS+c]=OriginalImage[r*COLS+c];

BeginPaint(MainWnd,&Painter);
hDC=GetDC(MainWnd);
bm_info_header.biSize=sizeof(BITMAPINFOHEADER); 
bm_info_header.biWidth=DISPLAY_COLS;
bm_info_header.biHeight=-DISPLAY_ROWS; 
bm_info_header.biPlanes=1;
bm_info_header.biBitCount=8; 
bm_info_header.biCompression=BI_RGB; 
bm_info_header.biSizeImage=0; 
bm_info_header.biXPelsPerMeter=0; 
bm_info_header.biYPelsPerMeter=0;
bm_info_header.biClrUsed=256;
bm_info_header.biClrImportant=256;
bm_info=(BITMAPINFO *)calloc(1,sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD));
bm_info->bmiHeader=bm_info_header;
for (i=0; i<256; i++)
  {
  bm_info->bmiColors[i].rgbBlue=bm_info->bmiColors[i].rgbGreen=bm_info->bmiColors[i].rgbRed=i;
  bm_info->bmiColors[i].rgbReserved=0;
  } 

SetDIBitsToDevice(hDC,0,0,DISPLAY_COLS,DISPLAY_ROWS,0,0,
			  0, /* first scan line */
			  DISPLAY_ROWS, /* number of scan lines */
			  DisplayImage,bm_info,DIB_RGB_COLORS);
ReleaseDC(MainWnd,hDC);
EndPaint(MainWnd,&Painter);

free(DisplayImage);
free(bm_info);
}


void AnimationThread(HWND AnimationWindowHandle)

{
HDC		hDC;
char	text[300];

ThreadRow=ThreadCol=0;
while (ThreadRunning == 1)
  {
  hDC=GetDC(MainWnd);
  SetPixel(hDC,ThreadCol,ThreadRow,RGB(0,255,0));	/* color the animation pixel green */
  sprintf(text,"%d,%d     ",ThreadRow,ThreadCol);
  TextOut(hDC,300,0,text,strlen(text));		/* draw text on the window */
  ReleaseDC(MainWnd,hDC);
  ThreadRow+=3;
  ThreadCol++;
  Sleep(100);		/* pause 100 ms */
  }
}

void Inter_func(HWND AnimationWindowHandle)
{
	int CenterRow, CenterCol;
	CenterCol = ThreadCol;
	CenterRow = ThreadRow;
	unsigned char* labels;
	int* indices;
	int	RegionSize;

	void RegionGrow();

	/* segmentation image = labels; calloc initializes all labels to 0 */
	labels = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
	/* used to quickly erase small grown regions */
	indices = (int*)calloc(ROWS * COLS, sizeof(int));
			
	RegionGrow(OriginalImage, labels, ROWS, COLS, CenterRow, CenterCol, 0, TotalRegions,
		indices, &RegionSize);
}

void RegionGrow(unsigned char* OriginalImage,	/* image data */
	unsigned char* labels,	/* segmentation labels */
	int ROWS, int COLS,	/* size of image */
	int r, int c,		/* pixel to paint from */
	int paint_over_label,	/* image label to paint over */
	int new_label,		/* image label for painting */
	int* indices,		/* output:  indices of pixels painted */
	int* count)		/* output:  count of pixels painted */
{
	HDC hDC;
	int	r2, c2;
	int	queue[MAX_QUEUE], qh, qt;
	int	average, total;	/* average and total intensity in growing region */
	int distance_centroid_to_pixel;

	*count = 0;
	if (labels[r * COLS + c] != paint_over_label)
		return;
	labels[r * COLS + c] = new_label;
	average = total = (int)OriginalImage[r * COLS + c];
	if (indices != NULL)
		indices[0] = r * COLS + c;
	queue[0] = r * COLS + c;
	qh = 1;	/* queue head */
	qt = 0;	/* queue tail */
	(*count) = 1;
	while (qt != qh && ThreadRunning == 1)
	{
		if (GrowRegionPlay == 1) {
			flag = 1;
		}
		if (flag == 1) {
			if ((*count) % 50 == 0)	/* recalculate average after each 50 pixels join */
			{
				average = total / (*count);
				// printf("new avg=%d\n",average);
			}
			for (r2 = -1; r2 <= 1; r2++)
				for (c2 = -1; c2 <= 1; c2++)
				{
					if (r2 == 0 && c2 == 0)
						continue;
					if ((queue[qt] / COLS + r2) < 0 || (queue[qt] / COLS + r2) >= ROWS ||
						(queue[qt] % COLS + c2) < 0 || (queue[qt] % COLS + c2) >= COLS)
						continue;
					if (labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] != paint_over_label)
						continue;
					/* test criteria to join region */
					if (abs((int)(OriginalImage[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2])
						- average) > predicate1)
						continue;
					distance_centroid_to_pixel = sqrt(pow((r - (queue[qt] / COLS + r2)), 2) + pow((c - (queue[qt] % COLS + c2)), 2));
					if (distance_centroid_to_pixel > predicate2)
						continue;
					hDC = GetDC(MainWnd);
					labels[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] = new_label;
					if (RedColor == 1 || BlueColor == 1 || GreenColor == 1) {
						SetPixel(hDC, queue[qt] % COLS + c2, queue[qt] / COLS + r2, Colors);
					}
					else if (RedColor == 0 && BlueColor == 0 && GreenColor == 0) { //Setting a default color for regiongrow, if user forgets to select the colors
						SetPixel(hDC, queue[qt] % COLS + c2, queue[qt] / COLS + r2, RGB(211, 211, 211));
					}
					ReleaseDC(MainWnd, hDC);
					if (indices != NULL)
						indices[*count] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
					total += OriginalImage[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2];
					(*count)++;
					queue[qh] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
					qh = (qh + 1) % MAX_QUEUE;
					if (qh == qt)
					{
						printf("Max queue size exceeded\n");
						return;
					}
				}
			qt = (qt + 1) % MAX_QUEUE;
			if (GrowRegionStep == 1) {
				flag = 0;
			}
			else if (GrowRegionPlay == 1) {
				Sleep(0.2);
			}
		}

	}

}
