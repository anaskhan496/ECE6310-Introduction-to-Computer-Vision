#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define SQR(x) ((x)*(x))

int main()
{
    FILE	*fpt;
    unsigned char	*image,*labels, *Thresh_image, *RGB_image;
    char header[320];
    int COLS, ROWS, BYTES, COLS2, ROWS2, BYTES2, r, c, thresh, vec_dist;
    double	xtheta,ytheta,dist;
    double	cp[7], *xNormal, *yNormal, *zNormal;
    double	SlantCorrection, ScanDirectionFlag;
    int		RegionSize,*RegionPixels,TotalRegions,*indices;
    double	avg,var;
    int r2,c2,i;
    void RegionGrow();

    /* read range image */
    if ((fpt=fopen("chair-range.ppm","rb")) == NULL)
    {
    printf("Unable to open chair-range.ppm for reading\n");
    exit(0);
    }
    fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
    image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    header[0]=fgetc(fpt);	/* read white-space character that separates header */
    if (image == NULL)
    {
    printf("Unable to allocate %d x %d memory\n",COLS,ROWS);
    exit(0);
    }
    fread(image,1,COLS*ROWS,fpt);
    fclose(fpt);

    cp[0]=1220.7;		/* horizontal mirror angular velocity in rpm */
    cp[1]=32.0;		/* scan time per single pixel in microseconds */
    cp[2]=(COLS/2)-0.5;		/* middle value of columns */
    cp[3]=1220.7/192.0;	/* vertical mirror angular velocity in rpm */
    cp[4]=6.14;		/* scan time (with retrace) per line in milliseconds */
    cp[5]=(ROWS/2)-0.5;		/* middle value of rows */
    cp[6]=10.0;		/* standoff distance in range units (3.66cm per r.u.) */

    cp[0]=cp[0]*3.1415927/30.0;	/* convert rpm to rad/sec */
    cp[3]=cp[3]*3.1415927/30.0;	/* convert rpm to rad/sec */
    cp[0]=2.0*cp[0];		/* beam ang. vel. is twice mirror ang. vel. */
    cp[3]=2.0*cp[3];		/* beam ang. vel. is twice mirror ang. vel. */
    cp[1]/=1000000.0;		/* units are microseconds : 10^-6 */
    cp[4]/=1000.0;			/* units are milliseconds : 10^-3 */


    /* Creating Threshold image */
    thresh = 125;
    Thresh_image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));

    for (r=0; r<ROWS; r++) {
        for (c=0; c<COLS; c++) {
            if (image[r*COLS+c]<=thresh) {
                Thresh_image[r*COLS+c] = 0;
            }
            else {
                Thresh_image[r*COLS+c] = 255;
            }
        }
    }

    double P[3][ROWS*COLS];
    for (r=0; r<ROWS; r++)
    {
        for (c=0; c<COLS; c++)
        {
            SlantCorrection=cp[3]*cp[1]*((double)c-cp[2]);
            xtheta=cp[0]*cp[1]*((double)c-cp[2]);
            ytheta=(cp[3]*cp[4]*(cp[5]-(double)r))+	/* Standard Transform Part */
        SlantCorrection*1;	/*  + slant correction */
            dist=(double)image[r*COLS+c]+cp[6];
            P[2][r*COLS+c]=sqrt((dist*dist)/(1.0+(tan(xtheta)*tan(xtheta))
        +(tan(ytheta)*tan(ytheta))));
            P[0][r*COLS+c]=tan(xtheta)*P[2][r*COLS+c];
            P[1][r*COLS+c]=tan(ytheta)*P[2][r*COLS+c];
        }
    }
    
    double x0,y0,z0,x1,y1,z1,x2,y2,z2;
    double ax,ay,az,bx,by,bz;
    double *dot_product_col = (double *)calloc(ROWS*COLS,sizeof(double ));
    double *dot_product_row = (double *)calloc(ROWS*COLS,sizeof(double ));
    double *theta_col = (double *)calloc(ROWS*COLS,sizeof(double ));
    double *theta_row = (double *)calloc(ROWS*COLS,sizeof(double ));
    double amod,bmod;
    xNormal = (double *)calloc(ROWS*COLS, sizeof(double));
    yNormal = (double *)calloc(ROWS*COLS, sizeof(double));
    zNormal = (double *)calloc(ROWS*COLS, sizeof(double));
    vec_dist = 3;

    for (r=0; r<ROWS; r++) {
        for (c=0; c<COLS; c++) {
            //Only those pixel’s cross-product was calculated which had a value of 0 or lied within the threshold in the above image
            if (Thresh_image[r*COLS+c]==0 && (r-vec_dist >= 0 || c-vec_dist >= 0)) 
            {
                x0 = P[0][r*COLS+c]; y0 = P[1][r*COLS+c]; z0 = P[2][r*COLS+c];
                x1 = P[0][(r-vec_dist)*COLS+(c)]; y1 = P[1][(r-vec_dist)*COLS+(c)]; z1 = P[2][(r-vec_dist)*COLS+(c)];
                x2 = P[0][(r)*COLS+(c-vec_dist)]; y2 = P[1][(r)*COLS+(c-vec_dist)]; z2 = P[2][(r)*COLS+(c-vec_dist)];
                ax = x1-x0; ay = y1-y0; az = z1-z0;
                bx = x2-x0; by = y2-y0; bz = z2-z0;
                xNormal[r*COLS+c] = (ay*bz) - (az*by);
                yNormal[r*COLS+c] = (az*bx) - (ax*bz);
                zNormal[r*COLS+c] = (ax*by) - (ay*bx);
            }
        }
    }

    printf("Creating Outputs.csv file\n");
    fpt=fopen("Outputs.csv","wb");
    for (r=0;r<ROWS*COLS;r++) {
        fprintf(fpt,"%lf,%lf,%lf,%lf,%lf,%lf\n",P[0][r],P[1][r],P[2][r],xNormal[r],yNormal[r],zNormal[r]);
    }
    fclose(fpt);
    printf("Outputs.csv created\n");

    fpt=fopen("threshold.ppm","wb");
    if (fpt == NULL)
    {
    printf("Unable to open threshold.ppm for writing\n");
    exit(0);
    }
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(Thresh_image,1,ROWS*COLS,fpt);
    fclose(fpt);


    RGB_image=(unsigned char *)calloc(ROWS*COLS*3,sizeof(unsigned char)); //RGB_image color image
    labels = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char));
    indices = (int *)calloc(ROWS*COLS*3, sizeof(int));
    TotalRegions = 0;
    int region_loc[2];
    double Average_normal[3][10];
    for (r = 2; r < (ROWS-2); r++)
    {
        for (c = 2; c < (COLS-2); c++)
        {
            if (labels[r*COLS+c] != 0)
            {
		        continue;
		    }
            if (Thresh_image[r*COLS+c] == 0)
            {
                int counter = 0;
                for (int r1 = -2; r1 <= 2; r1++)
                {
                for (int c1 = -2; c1 <= 2; c1++)
                {
                    if (labels[(r+r1)*COLS+(c+c1)] != 0) {counter++;}
                }
                }
                if (counter == 0)
                {
                TotalRegions++;
                if (TotalRegions == 255) {printf("Segmentation incomplete.  Ran out of labels.\n"); break;}
                RegionGrow(Thresh_image,labels,ROWS,COLS,r,c,0,TotalRegions,indices,&RegionSize,xNormal,yNormal,zNormal,Average_normal,region_loc);
                if (RegionSize < 50)
                {
                for (i = 0; i < RegionSize; i++) {labels[indices[i]] = 0;}
                TotalRegions--;
                }
                else
                {
                printf("\n Region labeled %d is %d in size.\n",TotalRegions,RegionSize);
                printf("\n At %d %d, %lf %lf %lf \n",region_loc[1],region_loc[0],Average_normal[0][TotalRegions],Average_normal[1][TotalRegions],Average_normal[2][TotalRegions]);
                }
                }
            } 
            else {continue;}
        }
    }
    printf("%d total regions were found\n", TotalRegions);



    // if ((fpt=fopen("seg.ppm","wb")) == NULL)
    // {
    // printf("Unable to open file for writing\n");
    // exit(0);
    // }
    // fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    // fwrite(labels,1,ROWS*COLS,fpt);
    // fclose(fpt);

    // fpt=fopen("region.ppm","wb");
    // if (fpt == NULL)
    // {
    // printf("Unable to open region.ppm for writing\n");
    // exit(0);
    // }
    // fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    // fwrite(labels,1,ROWS*COLS,fpt);
    // fclose(fpt);

    for(i=0; i<ROWS*COLS;i++){
        RGB_image[i*3+0] = (labels[i]*53)%255;
        RGB_image[i*3+1] = (labels[i]*97)%255;
        RGB_image[i*3+2] = (labels[i]*223)%255;
    }
    if ((fpt=fopen("seg-color.ppm","wb")) == NULL)
    {
    printf("Unable to open file for writing\n");
    exit(0);
    }
    fprintf(fpt,"P6 %d %d 255\n",COLS,ROWS);
    fwrite(RGB_image,1,ROWS*COLS*3,fpt);

    fclose(fpt);
    }


/*
	** Given an image, a starting point, and a label, this routine
	** paint-fills (8-connected) the area with the given new label
	** according to the given criteria.
	*/

#define MAX_QUEUE 10000	/* max perimeter size (pixels) of border wavefront */

void RegionGrow (int *image,	/* image data */
		unsigned char *labels,	/* segmentation labels */
		int ROWS,int COLS,	/* size of image */
		int r,int c,		/* pixel to paint from */
		int paint_over_label,	/* image label to paint over */
		int new_label,		/* image label for painting */
		int *indices,		/* output:  indices of pixels painted */
		int *count,     
    double *xNormal,
    double *yNormal, 
    double *zNormal,
    double Average_normal[3][10],
    int region_loc[2])		
{
int	r2,c2;
int	queue[MAX_QUEUE],qh,qt;
double avgx, avgy, avgz, angle, dot, mag_a, mag_b;
if (indices != NULL)
  indices[0]=r*COLS+c;
queue[0]=r*COLS+c;
qh=1;	/* queue head */
qt=0;	/* queue tail */
(*count) = 0;
avgx = xNormal[(queue[qt]/COLS)*COLS+queue[qt]%COLS]; avgy = yNormal[(queue[qt]/COLS)*COLS+queue[qt]%COLS]; avgz = zNormal[(queue[qt]/COLS)*COLS+queue[qt]%COLS];

while (qt != qh)
  {
    for (r2=-1; r2<=1; r2++)
    {
        for (c2=-1; c2<=1; c2++)
        { 
 
        if (labels[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2]!=paint_over_label)
            continue;

           /* test criteria to join region */
        //The orientation of only those pixels was computed which was within the threshold image and also did not have zero surface normal
        if(xNormal[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2]!=0 )
        {
            //A running average of each pixel within the 5*5 window was computed
            avgx = (avgx + xNormal[(r+r2)*COLS+(c+c2)]) / 2.0;
            avgy = (avgy + yNormal[(r+r2)*COLS+(c+c2)]) / 2.0;
            avgz = (avgz + zNormal[(r+r2)*COLS+(c+c2)]) / 2.0;

            //𝑑𝑜𝑡 product of the average normals (𝑎𝑥,𝑎𝑦,𝑎𝑧) was computed with the normal of the neighboring pixel (𝑏𝑥,𝑏𝑦,𝑏𝑧).
            dot = (avgx*xNormal[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2]) + 
            (avgy*yNormal[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2]) + 
            (avgz*zNormal[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2]);

            //magnitude of the average surface normals (𝑚𝑎𝑔𝑎) and the neighboring pixel (𝑚𝑎𝑔𝑏) was computed.
            mag_a = sqrt(pow(avgx, 2) + pow(avgy, 2) + pow(avgz, 2));

            mag_b = sqrt(pow(xNormal[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2], 2) + 
            pow(yNormal[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2], 2) + 
            pow(zNormal[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2], 2));

            //Using the dot product and the magnitude of the surface normals the average orientation was computed
            angle = acos(dot / (mag_a*mag_b));

            /*The region predicate was such that a pixel can join the region if its orientation is within a threshold 
            of the average orientation of pixels already in the region. */        
            if (angle > 0.174*6.7)  //The angle threshold chosen was 𝟔𝟕°.
            continue;

            labels[(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2]=new_label;                
            if (indices != NULL)
                indices[*count]=(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2;

            (*count)++;
            queue[qh]=(queue[qt]/COLS+r2)*COLS+queue[qt]%COLS+c2;
            qh=(qh+1)%MAX_QUEUE;
            if (qh == qt)
                {
                printf("Max queue size exceeded\n");
                exit(0);
                }
            }
        }
    }
    qt=(qt+1)%MAX_QUEUE;
    }
    region_loc[0] = r;
    region_loc[1] = c;
    Average_normal[0][new_label] = avgx;
    Average_normal[1][new_label] = avgy;
    Average_normal[2][new_label] = avgz; 
}