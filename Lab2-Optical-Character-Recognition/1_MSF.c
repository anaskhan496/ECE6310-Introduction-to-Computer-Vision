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

    if (argc!=4){
        printf("Error - Pass Arguments: Executable file | Image File | Template Image File | Ground Truth Text File \n");
        exit(0);
    }

    /* read input image */
    if ((fpt=fopen(argv[1],"rb")) == NULL)
    {
    printf("Unable to open parenthood.ppm for reading\n");
    exit(0);
    }
    fscanf(fpt,"%s %d %d %d ",header,&COLS,&ROWS,&BYTES);

    if (strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
    printf(" Parenthood Not a greyscale 8-bit PPM image\n");
    exit(0);
    }

    image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    // header[0]=fgetc(fpt);	/* read white-space character that separates header */
    fread(image,1,COLS*ROWS,fpt);
    fclose(fpt);

    /* Read template image*/
    if ((temp_fpt=fopen(argv[2],"rb")) == NULL)
    {
    printf("Unable to open parenthood_e_template.ppm for reading\n");
    exit(0);
    }

    fscanf(temp_fpt,"%s %d %d %d ",header_temp,&TEMP_COLS,&TEMP_ROWS,&TEMP_BYTES);
    if (strcmp(header_temp,"P5") != 0 ||  TEMP_BYTES != 255 )
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

    // Normalization
    float PixelRange=maxPixelValue-minPixelValue;
    float newmax=255.0;
    float newmin=0.0;
    float newrange = newmax - newmin;
    float normalized;
    float factor;

    for(r=0; r<ROWS;r++){
        for(c=0; c<COLS; c++){
            if(r-iter_row>=0 && r+iter_row<ROWS && c-iter_col>=0 && c+iter_col<COLS){
                factor = newrange / PixelRange;
                normalized = (pixelvalue[r*COLS+c] - minPixelValue)*factor;
                int f = round(normalized);
                MSF_Image[r*COLS+c] = f;
            }
        }
    }

    // Get array of threshold values. Outputs 256 values from 0-to-255
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
    int GT_R,GT_C;
    char letter;
    float TPR=0.0; 
    float FPR=0.0;
    float *TPR_Array = (float *)calloc(num_values+1,sizeof(float));
    float *FPR_Array = (float *)calloc(num_values+1,sizeof(float));
    int *TP_Array = (int *)calloc(num_values+1,sizeof(int));
    int *FP_Array = (int *)calloc(num_values+1,sizeof(int));    
    int *T = (int *)calloc(num_values+1,sizeof(int)); 

    for (int k=initial; k<=final;k++){
        // Create Binary Image
        BinaryImage = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
        for (r=0; r<ROWS; r++){
            for(c=0;c<COLS;c++){
                if(MSF_Image[r*COLS+c]>ThresholdValues[k]){
                    BinaryImage[r*COLS+c] = 255;
                }
                else if(MSF_Image[r*COLS+c]<=ThresholdValues[k]){
                    BinaryImage[r*COLS+c] = 0;
                }
            }
        }
        int TP=0;
        int FP=0;
        int TN=0;
        int FN=0;
        gt_fpt = fopen(argv[3],"rb");
        if (gt_fpt == NULL)
        {
        printf("Unable to open ocr.txt for reading\n");
        exit(0);
        }
        // Calculate TPR and FPR
        while(1){    
            int detected=0;
            int not_detected=0;  
            i=fscanf(gt_fpt,"%s %d %d",&letter,&GT_C,&GT_R);
            if(i!=3){
                break;
            }
            else{
                for(dr=-TEMP_ROWS/2; dr<=TEMP_ROWS/2; dr++){
                // If a detection was already found before finishing looping through the 9*15 window, then it skips the rest of looping
                    if (detected==0){ 
                        for(dc=-TEMP_COLS/2; dc<=TEMP_COLS/2; dc++){
                                if(MSF_Image[(GT_R+dr)*COLS+(GT_C+dc)] > ThresholdValues[k]){
                                    detected=1;
                                }
                            }
                        }
                    }
                // If no detections found
                if(detected==0){
                    not_detected = 1;
                }
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
            }
        }
        fclose(gt_fpt);
        TPR = TP/(float)(TP+FN);
        FPR = FP/(float)(FP+TN);
        TP_Array[k] = TP;
        FP_Array[k] = FP;
        T[k] = k;
        TPR_Array[k] = TPR;
        FPR_Array[k] = FPR;

        // printf("\n True Positive Rate|%f \n", TPR);   
        // printf("False Positive Rate|%f \n", FPR);     
        // printf("T|%d True Positive|%d False Positive|%d True Negative|%.d False Negative|%d \n",k,TP,FP,TN,FN);  

    }

    fpt=fopen("TPR_FPR.txt","wb");
    if (fpt == NULL)
    {
    printf("Unable to open TPR_FPR.txt for writing\n");
    exit(0);
    }
    for(int t=initial; t<=final;t++){
        fprintf(fpt,"%f %f\n",TPR_Array[t], FPR_Array[t]);
        // printf("%f  ", TPR_Array[t]);
    }
    fclose(fpt);


    fpt=fopen("TP_FP.txt","wb");
    if (fpt == NULL)
    {
    printf("Unable to open FP_TP.txt for writing\n");
    exit(0);
    }
    for(int t=initial; t<=final;t++){
        fprintf(fpt,"%d %d %d\n",T[t], TP_Array[t], FP_Array[t]);
        // printf("%f  ", FPR_Array[t]);
    }
    fclose(fpt);

    //MSF Image
    fpt=fopen("MSF_Image.ppm","wb");
    if (fpt == NULL)
    {
    printf("Unable to open temp.ppm for writing\n");
    exit(0);
    }
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(MSF_Image,1,ROWS*COLS,fpt);
    fclose(fpt);
}