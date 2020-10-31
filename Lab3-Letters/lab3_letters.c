#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

int main(int argc, char *argv[]){
    FILE *fpt, *temp_fpt;
    unsigned char *image;
    unsigned char *Template;
    unsigned char *MSF_Image;
    unsigned char *BinaryImage;
    float *ZeroMeanTemplate;
    float *pixelvalue;
    char header[80], header_temp[80];
    int TEMP_COLS, TEMP_ROWS, TEMP_BYTES;
    int COLS,ROWS,BYTES;
    float sum;
    int r,c,iter_row=7,iter_col=4,dr,dc,i;
    int final, initial, num_values, step;
    int *ThresholdValues;
    int r2,c2,flag;
    unsigned char *Binary_Inflated;

    if (argc!=4){
        printf("Error - Pass Arguments: Executable file | Image File | Template Image File | Ground Truth Text File \n");
        exit(0);
    }

    /* read input image */
    if ((fpt=fopen(argv[1],"r")) == NULL)
    {
    printf("Unable to open parenthood.ppm for reading\n");
    exit(0);
    }
    i=fscanf(fpt,"%s %d %d %d ",header,&COLS,&ROWS,&BYTES);

    if (i != 4  ||  strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
    printf(" Parenthood Not a greyscale 8-bit PPM image\n");
    exit(0);
    }

    image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    fread(image,sizeof(unsigned char),COLS*ROWS,fpt);
    fclose(fpt);

    /* Read template image*/
    if ((temp_fpt=fopen(argv[2],"r")) == NULL)
    {
    printf("Unable to open parenthood_e_template.ppm for reading\n");
    exit(0);
    }

    i=fscanf(temp_fpt,"%s %d %d %d ",header_temp,&TEMP_COLS,&TEMP_ROWS,&TEMP_BYTES);
        
    if (i != 4 || strcmp(header_temp,"P5") != 0 ||  TEMP_BYTES != 255 )
    {
    printf("Parenthood template Not a greyscale 8-bit PPM image\n");
    exit(0);
    }

    Template=(unsigned char *)calloc(TEMP_ROWS*TEMP_COLS,sizeof(unsigned char));
    // header_temp[0]=fgetc(temp_fpt);	/* read white-space character that separates header */
    fread(Template,1,TEMP_COLS*TEMP_ROWS,temp_fpt);
    fclose(temp_fpt);

    MSF_Image = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    pixelvalue = (float *)calloc(ROWS*COLS, sizeof(float));
    ZeroMeanTemplate = (float *)calloc(TEMP_ROWS*TEMP_COLS,sizeof(float));

    // Zero Mean Center Template
    float sum_template = 0.0;
    float mean_template = 0.0;

    for(int m=0; m<TEMP_ROWS*TEMP_COLS;m++){
        sum_template+= Template[m];
    }
    mean_template = sum_template/(TEMP_COLS*TEMP_ROWS);

    for(int m=0; m<TEMP_ROWS*TEMP_COLS;m++){
        ZeroMeanTemplate[m] = Template[m] - mean_template;
    }
    // printf("%f \n", mean_template);

    // Computing Cross Correlation
    float minPixelValue = 99999999999999;
    float maxPixelValue = 0;
    for(r=0; r<ROWS; r++){
        if((r-iter_row)>=0 && (r+iter_row)<ROWS){
            for(c=0;c<COLS; c++){
                sum=0.0;
                if((c-iter_col)>=0 && (c+iter_col)<COLS){
                    for(dr=-TEMP_ROWS/2; dr<=TEMP_ROWS/2; dr++){
                        for(dc=-TEMP_COLS/2; dc<=TEMP_COLS/2; dc++){
                            sum+= (image[(r+dr)*COLS+(c+dc)]*ZeroMeanTemplate[(dr+TEMP_ROWS/2)*TEMP_COLS+(dc+TEMP_COLS/2)]);
                        }
                    }
                    pixelvalue[r*COLS+c] = sum;
                    // printf("%f  ",pixelvalue[r*COLS+c]);
                    if(sum > maxPixelValue){
                        maxPixelValue=sum;
                    }
                    if(sum < minPixelValue){
                        minPixelValue=sum;
                    }
                }
            }
        }
    }

    // printf("%f %f", maxPixelValue,minPixelValue);

    // Normalization
    float PixelRange=maxPixelValue-minPixelValue;
    float newmax=255.0;
    float newmin=0.0;
    float newrange = newmax - newmin;
    float normalized;
    float factor;

    for(r=0; r<ROWS;r++){
        for(c=0; c<COLS; c++){
                factor = newrange / PixelRange;
                normalized = (pixelvalue[r*COLS+c] - minPixelValue)*factor;
                int f = round(normalized);
                MSF_Image[r*COLS+c] = f;
                // printf("%f ", round((pixelvalue[r*COLS+c] - minPixelValue)*factor));
        }
    }

    // Get array of threshold values
    initial = 0;
    final = 255;
    num_values = 255;
    ThresholdValues = (int *)calloc(num_values,sizeof(int));
    step = (final - initial) / num_values;
    int j = 0;
    int ct = 0;
    ThresholdValues[j] = initial;
    for(j = 1; j<=num_values; j++){
        ThresholdValues[j] = ThresholdValues[j-1] + step;
        // printf("%d  ", ThresholdValues[j]);
        ct+=1;
    }
    // printf("%d \n",ct);

    // ROC Curves
    FILE *gt_fpt;
    unsigned char *marked_image;
    int GT_R,GT_C;
    char letter;
    int detected;
    int not_detected;
    float TPR;
    float FPR;
    float *TPR_Array = (float *)calloc(num_values+1,sizeof(float));
    float *FPR_Array = (float *)calloc(num_values+1,sizeof(float));
    int temp_image;
    int edge_to_non_edge;
    int ddr,ddc,dddr,dddc;
    int count;
    int edge_neighbors;
    int c1;
    int pixels_left;
    int bp; int ep;
    unsigned char *Thinned_Image;
    int TP;int FP;int TN; int FN;

    for (int k=0; k<=255;k++){
        TP=0;
        FP=0;
        TN=0;
        FN=0;
        TPR=0.0;
        FPR=0.0;

        /* Read Ground Truth*/
        gt_fpt = fopen(argv[3],"r");
        if (gt_fpt == NULL)
        {
        printf("Unable to open ocr.txt for reading\n");
        exit(0);
        }
        i=fscanf(gt_fpt,"%s %d %d",&letter,&GT_C,&GT_R);
        while(i==3){    
            detected=0;
            not_detected=0;  
            // I am opening the template file again because the number of columns in the template image becomes
            // zero when the loop enters second iteration. So instead of 9 columns, I get 0 columns all along.
            if ((temp_fpt=fopen(argv[2],"r")) == NULL)
            {
            printf("Unable to open parenthood_e_template.ppm for reading\n");
            exit(0);
            }
            fscanf(temp_fpt,"%s %d %d %d ",header_temp,&TEMP_COLS,&TEMP_ROWS,&TEMP_BYTES);
            fclose(temp_fpt);  

            for(dr=-TEMP_ROWS/2; dr<=TEMP_ROWS/2; dr++){
                if (detected==0){
                    for(dc=-TEMP_COLS/2; dc<=TEMP_COLS/2; dc++){
                                // printf("%d ",COLS);
                            if(MSF_Image[(GT_R+dr)*COLS+(GT_C+dc)] > ThresholdValues[k]){
                                detected=1;
                            }
                            else if(MSF_Image[(GT_R+dr)*COLS+(GT_C+dc)] <= ThresholdValues[k]){
                                not_detected=1;
                            }
                        }
                    }
                }
            
            if(detected==1){
                detected=0;
                not_detected=0;


                Thinned_Image = (unsigned char *)calloc(TEMP_COLS*TEMP_ROWS,sizeof(unsigned char));
                BinaryImage = (unsigned char *)calloc(TEMP_COLS*TEMP_ROWS,sizeof(unsigned char));
                Binary_Inflated = (unsigned char *)calloc((TEMP_COLS+2)*(TEMP_ROWS+2),sizeof(unsigned char));
                flag = 0;
                for(dr=-TEMP_ROWS/2; dr<=TEMP_ROWS/2; dr++){
                    for(dc=-TEMP_COLS/2; dc<=TEMP_COLS/2; dc++){
                        if(image[(GT_R+dr)*COLS+(GT_C+dc)] > 128){
                            // printf("%d %d ",TEMP_COLS,TEMP_ROWS);
                            BinaryImage[flag] = 0;
                            flag++; 
                        }
                        else if(image[(GT_R+dr)*COLS+(GT_C+dc)] <= 128){
                            BinaryImage[flag] = 255;
                            flag++; 
                        }
                    }
                }
                
                //Creating binary inflated image

                for(r=1;r<=TEMP_ROWS;r++){  
                    for(c=1;c<=TEMP_COLS;c++){
                        temp_image = round(BinaryImage[(r-1)*TEMP_COLS+(c-1)]);
                        Binary_Inflated[(r)*(TEMP_COLS+2)+(c)] = temp_image;
                    }
                }

                // Thinning operation
                pixels_left = 1;
                while(pixels_left!=0){
                    marked_image = (unsigned char *)calloc(TEMP_ROWS*TEMP_COLS,sizeof(unsigned char));
                    for(r=1;r<=TEMP_ROWS;r++){
                        for(c=1;c<=TEMP_COLS;c++){
                            if(Binary_Inflated[r*(TEMP_COLS+2)+c]==255){
                                edge_to_non_edge = 0;
                                dr=-1;
                                for(dc=-1;dc<=1;dc++){ //traverse the first row,first column of window
                                    if (dc==1){ //reached last column,now traverse vertically down
                                        for(ddr=-1;ddr<=1;ddr++){
                                            if(ddr<1){
                                                if(Binary_Inflated[(r+ddr)*(TEMP_COLS+2)+(c+dc)]==255 && Binary_Inflated[(r+1+ddr)*(TEMP_COLS+2)+(c+dc)]==0){
                                                    edge_to_non_edge+=1;
                                                }
                                            } //ddr<1
                                            else if(ddr==1){ //reached last row of window,now traverse horizontally backwards
                                                for(ddc=1;ddc>=-1;ddc--){
                                                    if(ddc==-1){ //reached first column of window, now traverse vertically back up. 
                                                        for(dddr=1;dddr>-1;dddr--){
                                                            if(Binary_Inflated[(r+dddr)*(TEMP_COLS+2)+(c+ddc)]==255 && Binary_Inflated[(r-1+dddr)*(TEMP_COLS+2)+(c+ddc)]==0){
                                                                edge_to_non_edge+=1;
                                                            }
                                                        } //dddr
                                                    } //ddc==-1
                                                    else if (ddc!=-1){
                                                        if(Binary_Inflated[(r+ddr)*(TEMP_COLS+2)+(c+ddc)]==255 && Binary_Inflated[(r+ddr)*(TEMP_COLS+2)+(c+ddc-1)]==0){
                                                            edge_to_non_edge+=1;
                                                        }
                                                    } //ddc!=-1
                                                } //ddc
                                            } //ddr==1
                                        } //ddr
                                    } //dc==1
                                    else{
                                        if(Binary_Inflated[(r+dr)*(TEMP_COLS+2)+(c+dc)]==255 && Binary_Inflated[(r+dr)*(TEMP_COLS+2)+(c+dc+1)]==0){
                                        edge_to_non_edge+=1;
                                        }
                                    } //dc<1
                                } //dc
                                if(edge_to_non_edge==1){
                                    edge_neighbors=0;
                                    for(dddr=-1;dddr<=1;dddr++){
                                        for(dddc=-1;dddc<=1;dddc++){
                                            if(dddr!=0 || dddc!=0){
                                                if(Binary_Inflated[(r+dddr)*(TEMP_COLS+2)+(c+dddc)] == 255){
                                                    edge_neighbors=edge_neighbors+1;
                                                }            
                                            }
                                        }
                                    }
                                    if(edge_neighbors>=2 && edge_neighbors<=6){
                                        c1=0;
                                        if(Binary_Inflated[(r-1)*(TEMP_COLS+2)+c] ==0 || Binary_Inflated[r*(TEMP_COLS+2)+(c+1)] ==0 || (Binary_Inflated[r*(TEMP_COLS+2)+(c-1)] ==0 && Binary_Inflated[(r+1)*(TEMP_COLS+2)+c]==0)){
                                            c1=1;
                                        }
                                        if(c1==1){
                                            marked_image[(r-1)*TEMP_COLS+(c-1)] = 255;
                                            // printf("%d ",c1);
                                        }
                                        else if(c1==0){
                                            marked_image[(r-1)*TEMP_COLS+(c-1)] = 0;
                                        }
                                    }
                                } //edge_to_non_edge==1
                            }//bin_image == 0
                        } //loop through binary inflated cols
                    } //loop through binary inflated rows
                    pixels_left=0;

                    for(r=0;r<TEMP_ROWS;r++){
                        for(c=0;c<TEMP_COLS;c++){
                            if(marked_image[r*TEMP_COLS+c]==255){
                                Binary_Inflated[(r+1)*(TEMP_COLS+2)+(c+1)] = 0;
                                pixels_left = 1;
                            }
                        }
                    }
                } //end pixels left loop
                

                ep=0;bp=0;
                for(r=1;r<=TEMP_ROWS;r++){
                    for(c=1;c<=TEMP_COLS;c++){
                        if(Binary_Inflated[r*(TEMP_COLS+2)+c]==255){  
                            edge_to_non_edge=0;                 
                            dr=-1;
                            for(dc=-1;dc<=1;dc++){ //traverse the first row,first column of window
                                if (dc==1){ //reached last column,now traverse vertically down
                                    for(ddr=-1;ddr<=1;ddr++){
                                        if(ddr<1){
                                            if(Binary_Inflated[(r+ddr)*(TEMP_COLS+2)+(c+dc)]==255 && Binary_Inflated[(r+1+ddr)*(TEMP_COLS+2)+(c+dc)]==0){
                                                edge_to_non_edge+=1;
                                            }
                                        } //ddr<1
                                        else if(ddr==1){ //reached last row of window,now traverse horizontally backwards
                                            for(ddc=1;ddc>=-1;ddc--){
                                                if(ddc==-1){ //reached first column of window, now traverse vertically back up. 
                                                    for(dddr=1;dddr>-1;dddr--){
                                                        if(Binary_Inflated[(r+dddr)*(TEMP_COLS+2)+(c+ddc)]==255 && Binary_Inflated[(r-1+dddr)*(TEMP_COLS+2)+(c+ddc)]==0){
                                                            edge_to_non_edge+=1;
                                                        }
                                                    } //dddr
                                                } //ddc==-1
                                                else if (ddc!=-1){
                                                    if(Binary_Inflated[(r+ddr)*(TEMP_COLS+2)+(c+ddc)]==255 && Binary_Inflated[(r+ddr)*(TEMP_COLS+2)+(c+ddc-1)]==0){
                                                        edge_to_non_edge+=1;
                                                    }
                                                } //ddc<-1
                                            } //ddc
                                        } //ddr==1
                                    } //ddr
                                } //dc==1
                                else{
                                    if(Binary_Inflated[(r+dr)*(TEMP_COLS+2)+(c+dc)]==255 && Binary_Inflated[(r+dr)*(TEMP_COLS+2)+(c+dc+1)]==0){
                                    edge_to_non_edge+=1;
                                    }
                                } //dc<1
                            } //dc  
                            if(edge_to_non_edge==1){
                                ep=ep+1;
                            } 
                            else if(edge_to_non_edge>2){
                                bp=bp+1;
                            }
                        } //thinned image==0                       
                    }
                }
                if(ep==1 && bp==1){
                    detected=1;
                }
                else if(ep!=1 || bp!=1){
                    not_detected=1;
                }
            // printf("ep %d|bp %d|d %d|nd %d\n",ep,bp,detected,not_detected);
            } // if detected ==1  
            if(detected==1 && letter=='e'){
                TP++;
            }
            else if(detected==1 && letter!='e'){
                FP++;
            }
            else if(not_detected==1 && letter!='e'){
                TN++;
            }
            else if(not_detected==1 && letter=='e'){
                FN++;
            }          
        i=fscanf(gt_fpt,"%s %d %d",&letter,&GT_C,&GT_R);
        } //end while read file
        fclose(gt_fpt); 
        TPR = TP/(float)(TP+FN);
        FPR = FP/(float)(FP+TN);
        TPR_Array[k] = TPR;
        FPR_Array[k] = FPR;
        // printf("\n True Positive Rate|%f \n", TPR);   
        // printf("False Positive Rate|%f \n", FPR);     
        // printf("T|%d True Positive|%d False Positive|%d True Negative|%.d False Negative|%d \n",k,TP,FP,TN,FN);
    }//loop threshold values

    fpt=fopen("TPR.txt","w");
    if (fpt == NULL)
    {
    printf("Unable to open TPR.txt for writing\n");
    exit(0);
    }
    for(int t=initial; t<=final;t++){
        fprintf(fpt,"%f\n",TPR_Array[t]);
        // printf("%f  ", TPR_Array[t]);
    }
    fclose(fpt);

    fpt=fopen("FPR.txt","w");
    if (fpt == NULL)
    {
    printf("Unable to open FPR.txt for writing\n");
    exit(0);
    }
    for(int t=initial; t<=final;t++){
        fprintf(fpt,"%f\n",FPR_Array[t]);
        // printf("%f  ", FPR_Array[t]);
    }
    fclose(fpt);


    // fpt=fopen("BinaryImage.ppm","w");
    // if (fpt == NULL)
    // {
    // printf("Unable to open Binary.ppm for writing\n");
    // exit(0);
    // }
    // fprintf(fpt,"P5 %d %d 255\n",TEMP_COLS,TEMP_ROWS);
    // fwrite(BinaryImage,1,TEMP_COLS*TEMP_ROWS,fpt);
    // fclose(fpt);

    // fpt=fopen("BinaryInflImage.ppm","w");
    // if (fpt == NULL)
    // {
    // printf("Unable to open Binary_Inflated.ppm for writing\n");
    // exit(0);
    // }
    // fprintf(fpt,"P5 %d %d 255\n",(TEMP_COLS+2),(TEMP_ROWS+2));
    // fwrite(Binary_Inflated,1,(TEMP_COLS+2)*(TEMP_ROWS+2),fpt);
    // fclose(fpt);
}