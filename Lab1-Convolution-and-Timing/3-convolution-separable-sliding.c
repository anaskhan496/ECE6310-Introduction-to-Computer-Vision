
	/*
	** This program reads bridge.ppm, a 512 x 512 PPM image.
	** It smooths it using a standard 3x3 mean filter.
	** The program also demonstrates how to time a piece of code.
	**
	** To compile, must link using -lrt  (man clock_gettime() function).
	*/

#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <time.h>
#include<math.h>

int main()

{
FILE		*fpt;
unsigned char	*image;
unsigned char	*smoothed;
double	*smoothed_col;
char		header[320];
int		ROWS,COLS,BYTES;
int		r,c,d,iter=3;
double sum_row, sum_column;
int count = 0;
struct timespec	tp1,tp2;

	/* read image */
if ((fpt=fopen("bridge.ppm","rb")) == NULL)
  {
  printf("Unable to open bridge.ppm for reading\n");
  exit(0);
  }
fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
if (strcmp(header,"P5") != 0  ||  BYTES != 255)
  {
  printf("Not a greyscale 8-bit PPM image\n");
  exit(0);
  }
image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
header[0]=fgetc(fpt);	/* read white-space character that separates header */
fread(image,1,COLS*ROWS,fpt);
fclose(fpt);

	/* allocate memory for smoothed version of image */
smoothed=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
// smoothed_col=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
smoothed_col=(double *)calloc(ROWS*COLS,sizeof(double));
	/* query timer */
clock_gettime(CLOCK_REALTIME,&tp1);
printf("%ld %ld\n",(long int)tp1.tv_sec,tp1.tv_nsec);

/* smooth image */
//sliding window
//iter is 3
for (r=0; r<ROWS; r++){
  sum_column = 0.0;
    for (c=0; c<COLS; c++){
        if((c-iter) >=0 && (c+iter) < COLS){
          if(sum_column==0.0){
            for (d=-3; d<=3; d++){
                sum_column+= image[r*COLS+(c+d)];
            }
        }
        else {
               sum_column+= -image[r*COLS+(c-4)] + image[r*COLS+(c+3)];
        }
        smoothed_col[r*COLS+c] = sum_column/7.0;
      }
    }
}


//iter is 3
for (c=0; c<COLS; c++){
  sum_row = 0.0;
    for (r=0; r<ROWS; r++){
        if((r-iter) >= 0 && (r+iter) < ROWS){
         if(sum_row==0.0){
            for (d=-3; d<=3; d++){
                sum_row+= smoothed_col[(r+d)*COLS+c];
            }
        }
        else{
                sum_row+= -smoothed_col[(r-4)*COLS+c] + smoothed_col[(r+3)*COLS+c];
        }
        int inter = round(sum_row/7.0);
        smoothed[r*COLS+c] = inter;
        }
    }
  }

	/* query timer */
clock_gettime(CLOCK_REALTIME,&tp2);
printf("%ld %ld\n",(long int)tp2.tv_sec,tp2.tv_nsec);

	/* report how long it took to smooth */
printf("%ld\n",tp2.tv_nsec-tp1.tv_nsec);

	/* write out smoothed image to see result */
fpt=fopen("4-smoothed-7-sep-sliding.ppm","wb");
fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
fwrite(smoothed,COLS*ROWS,1,fpt);
fclose(fpt);
}
