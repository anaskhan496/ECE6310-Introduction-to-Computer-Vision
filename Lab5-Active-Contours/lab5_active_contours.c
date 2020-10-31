#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#define SQR(x) ((x)*(x))

int main(int argc, char *argv[]){
    FILE *fpt, *temp_fpt;
    unsigned char *image,*sobel_image,*initial_contour,*final_contour;
    double* sum;
    double *sobel_x,*sobel_y,*sobel_inter,*ext_energy,*ext_energy_temp;
    double avg_dist1_contour_points[100];
    char header[80];
    int COLS,ROWS,BYTES;
    int r,c,iter_row=1,iter_col=1,dr,dc,i,n,total_points,j,window,x,y,move_x,move_y,iterations,movex,movey;
    int	px[100],py[100];
    double temp1[19*19], temp2[19*19],totalenergy[100];
    double	internal_energy1[19*19],internal_energy2[19*19];
    double dist1,dist2;
    int kernel_x[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
    int kernel_y[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};
    float sum_column,sum_row;
    double min_energy;

    if (argc!=3){
        printf("Error - Pass Arguments: Executable file | Hawk Image | Hawk Init Text File |   \n");
        exit(0);
    }

    /* read input image */
    if ((fpt=fopen(argv[1],"rb")) == NULL)
    {
    printf("Unable to open hawk.ppm for reading\n");
    exit(0);
    }
    i=fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);

    if (i != 4  ||  strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
    printf(" Parenthood Not a greyscale 8-bit PPM image\n");
    exit(0);
    }
    image =  (unsigned char*)calloc(ROWS*COLS,sizeof(unsigned char));
    fread(image,sizeof(unsigned char),COLS*ROWS,fpt);
    fclose(fpt);

    initial_contour = (unsigned char*)calloc(ROWS*COLS,sizeof(unsigned char)); // initial contour around hawk
    final_contour = (unsigned char*)calloc(ROWS*COLS,sizeof(unsigned char)); // final contour around hawk

    for (r=0; r<ROWS; r++) {
        for (c=0; c<COLS; c++) {
            initial_contour[r*COLS+c] = image[r*COLS+c];
            final_contour[r*COLS+c] = image[r*COLS+c];
        }
    }

    sobel_x = (double *)calloc(ROWS*COLS, sizeof(double)); //sobel horizontal
    sobel_y = (double *)calloc(ROWS*COLS,sizeof(double)); //sobel vertical
    sobel_inter = (double *)calloc(ROWS*COLS,sizeof(double)); //temp image for sobel
    sobel_image = (unsigned char*)calloc(ROWS*COLS,sizeof(unsigned char)); // final image for sobel
    ext_energy = (double *)calloc(ROWS*COLS, sizeof(double)); // external energy term
    ext_energy_temp = (double *)calloc(ROWS*COLS, sizeof(double)); // invert sobel energy

    for(r=0; r<ROWS;r++){
        for(c=0;c<COLS;c++){
            sum_row=0.0;
            sum_column=0.0;
            if((c-iter_col) >=0 && (c+iter_col) < COLS && (r-iter_row) >=0 && (r+iter_row) < ROWS){
                for(dr=-iter_row; dr<=iter_row; dr++){
                    for(dc=-iter_col;dc<=iter_col;dc++){
                        sum_row+= kernel_y[1+dr][1+dc]*image[(r+dr)*COLS+(c+dc)];
                        sum_column+= kernel_x[1+dr][1+dc]*image[(r+dr)*COLS+(c+dc)];
                    }
                }
                sobel_x[r*COLS+c] = sum_column;
                sobel_y[r*COLS+c] = sum_row;
            }

        }
    }

    float minPixelValue = 99999999999999;
    float maxPixelValue = 0;
    for(r=0; r<ROWS; r++){
        for(c=0;c<COLS;c++){
            sobel_inter[r*COLS+c] = sqrt(SQR(sobel_x[r*COLS+c]) + SQR(sobel_y[r*COLS+c]));
            if(sobel_inter[r*COLS+c] > maxPixelValue){
                maxPixelValue=sobel_inter[r*COLS+c];
            }
            if(sobel_inter[r*COLS+c] < minPixelValue){
                minPixelValue=sobel_inter[r*COLS+c];
            }
        }
    }
    float PixelRange=maxPixelValue-minPixelValue;
    float newmax=255.0;
    float newmin=0.0;
    float newrange = newmax - newmin;
    float normalized = 0.0;
    float factor = 0.0;
    
    float minSobelValue = 9999999999999;
    float maxSobelValue = 0;  
    for(r=0; r<ROWS;r++){
        for(c=0; c<COLS; c++){
                factor = newrange / PixelRange;
                normalized = (sobel_inter[r*COLS+c] - minPixelValue)*factor;
                int f = round(normalized);
                sobel_image[r*COLS+c] = f;
                ext_energy_temp[r*COLS+c] = f*f;
                if(ext_energy_temp[r*COLS+c] > maxSobelValue){
                    maxSobelValue=ext_energy_temp[r*COLS+c];
                }
                if(ext_energy_temp[r*COLS+c] < minSobelValue){
                    minSobelValue=ext_energy_temp[r*COLS+c];
                }
        }
    }

    PixelRange=maxSobelValue-minSobelValue;
    newmax=1.0;
    newmin=0.0;
    newrange = newmax - newmin;
    normalized = 0.0;
    factor = 0.0;

    for(r=0; r<ROWS;r++){
        for(c=0; c<COLS; c++){
                factor = newrange / PixelRange;
                normalized = (ext_energy_temp[r*COLS+c] - minSobelValue)*factor;
                ext_energy[r*COLS+c] = 1-normalized;
        }
    }

    fpt=fopen("Sobel_Image_Hawk.ppm","wb");
    if (fpt == NULL)
    {
    printf("Unable to open Sobel_Image_Hawk.ppm for writing\n");
    exit(0);
    }
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(sobel_image,1,ROWS*COLS,fpt);
    fclose(fpt);

        /* read contour points file */
    if ((fpt=fopen(argv[2],"rb")) == NULL)
    {
    printf("Unable to open %s for reading\n",argv[2]);
    exit(0);
    }

    total_points=0; 
    i=fscanf(fpt, "%d %d", &px[total_points], &py[total_points]);
    while(i==2){
        //Add plus symbols around the hawk
        for(r=py[total_points]-3; r<=py[total_points]+3; r++){
            for(c=px[total_points]-3; c<=px[total_points]+3; c++){
                if(r==py[total_points] || c==px[total_points]){
                    initial_contour[r*COLS+(c)] = 0;
                }
            }
        }
        total_points+=1;
        if(total_points>100){ break;}
        i=fscanf(fpt, "%d %d", &px[total_points], &py[total_points]);

    }
    fclose(fpt);


    n=0;
    window=7;
    iterations = 30;
    // Loop to compute energy terms
    while (n<iterations){
        sum = (double *)calloc(window*window,sizeof(double));
        for(i=0;i<total_points;i++){
            for (y=0; y<window; y++)
            {
                for (x=0; x<window; x++)
                {
                    move_x = px[i] - window/2 + x;
                    move_y = py[i] - window/2 + y;
                    if(i==total_points-1){
                        sum[y*window+x]+= (double)SQR(move_x-px[0]) + SQR(move_y-py[0]); 
                    }
                    else{
                        sum[y*window+x]+= (double)SQR(move_x-px[i+1]) + SQR(move_y-py[i+1]);                    
                    }
                }
            }        
        }
        //Loop to compute average distance between points
        for (y=0; y<window; y++){
            for (x=0; x<window; x++){
                avg_dist1_contour_points[y*window+x] = sqrt(sum[y*window+x])/total_points;
            }
        }

        for(i=0;i<total_points;i++){  
            min_energy = 99999999999999.0;  
            int min_energy_pixel_locx = 0;
            int min_energy_pixel_locy = 0; 
            float minPixelValue1 = 99999999999999;
            float maxPixelValue1 = 0;
            for (y=0; y<window; y++)
            {
                for (x=0; x<window; x++)
                {
                    move_x = px[i] - window/2 + x;
                    move_y = py[i] - window/2 + y;
                    if(i==total_points-1){
                        temp1[y*window+x] = (double)SQR(move_x-px[0]) + SQR(move_y-py[0]); 
                    }
                    else{
                        temp1[y*window+x] = (double)SQR(move_x-px[i+1]) + SQR(move_y-py[i+1]);                   
                    }

                    if(temp1[y*window+x] > maxPixelValue1){
                        maxPixelValue1=temp1[y*window+x];
                    }
                    if(temp1[y*window+x] < minPixelValue1){
                        minPixelValue1=temp1[y*window+x];
                    }
                }
            }

            // Normalization
            float PixelRange1=maxPixelValue1-minPixelValue1;
            newmax=1.0;
            newmin=0.0;
            newrange = newmax - newmin;
            normalized = 0.0;
            factor = 0.0;

            for(y=0; y<window;y++){
                for(x=0; x<window; x++){
                        factor = newrange / PixelRange1;
                        normalized = (temp1[y*window+x] - minPixelValue1)*factor;
                        internal_energy1[y*window+x] = normalized;
                }
            }

            float minPixelValue2 = 99999999999999;
            float maxPixelValue2 = 0;
            for (y=0; y<window; y++)
            {
                for (x=0; x<window; x++)
                {
                    move_x = px[i] - window/2 + x;
                    move_y = py[i] - window/2 + y;
                    if(i==total_points-1){
                        temp2[y*window+x] = SQR(avg_dist1_contour_points[y*window+x]) - (SQR(move_x-px[0]) + SQR(move_y-py[0])); 
                    }
                    else{
                        temp2[y*window+x] = SQR(avg_dist1_contour_points[y*window+x]) - (SQR(move_x-px[i+1]) + SQR(move_y-py[i+1]));                    
                    }
                    if(temp2[y*window+x] > maxPixelValue2){
                        maxPixelValue2=temp2[y*window+x];
                    }
                    if(temp2[y*window+x] < minPixelValue2){
                        minPixelValue2=temp2[y*window+x];
                    }

                }
            }

            float PixelRange2=maxPixelValue2-minPixelValue2;
            newmax=1.0;
            newmin=0.0;
            newrange = newmax - newmin;
            normalized = 0.0;
            factor = 0.0;
            for(y=0; y<window;y++){
                for(x=0; x<window; x++){
                        factor = newrange / PixelRange2;
                        normalized = (temp2[y*window+x] - minPixelValue2)*factor;
                        internal_energy2[y*window+x] = normalized;
                }
            }

            for(y=0; y<window;y++){
                for(x=0;x<window;x++){
                    movex = px[i] - window/2 + x;
                    movey = py[i] - window/2 + y; 
                    totalenergy[y*window+x]= 1.085*internal_energy1[y*window+x] + 1.16*internal_energy2[y*window+x] + 0.13*ext_energy[movey*COLS+movex];                                                          
                    if(totalenergy[y*window+x] < min_energy){
                        min_energy=totalenergy[y*window+x];
                        min_energy_pixel_locx = movex;
                        min_energy_pixel_locy = movey;                                             
                    }
                }
            }   
            py[i] = min_energy_pixel_locy;
            px[i] = min_energy_pixel_locx;
        }
        n++;
    }

    //Add plus symbols around the hawk
    for(i=0;i<total_points;i++){
        for(r=py[i]-3; r<=py[i]+3; r++){
            for(c=px[i]-3; c<=px[i]+3; c++){
                if(r==py[i] || c==px[i]){
                    final_contour[r*COLS+(c)] = 0;
                }
            }
        }
    }

    fpt=fopen("Final_Contour.txt","wb");
    if (fpt == NULL)
    {
    printf("Unable to open Initial_Contour.txt for writing\n");
    exit(0);
    }
    for(i=0; i<total_points;i++){
        fprintf(fpt,"%d %d \n",px[i], py[i]);
    }
    fclose(fpt);

    fpt=fopen("Initial_Contours.ppm","wb");
    if (fpt == NULL)
    {
    printf("Unable to open Initial_Contours.ppm for writing\n");
    exit(0);
    }
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(initial_contour,1,ROWS*COLS,fpt);
    fclose(fpt);

    fpt=fopen("Final_Contours.ppm","wb");
    if (fpt == NULL)
    {
    printf("Unable to open Final_Contours.ppm for writing\n");
    exit(0);
    }
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(final_contour,1,ROWS*COLS,fpt);
    fclose(fpt);

}