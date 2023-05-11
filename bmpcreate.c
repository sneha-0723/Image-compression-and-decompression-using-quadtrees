#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct
{
unsigned char fileMarker1; /* 'B' */
unsigned char fileMarker2; /* 'M' */
unsigned char bfSize[4];
unsigned short unused1;
unsigned short unused2;
unsigned char imageDataOffset[4]; /* Offset to the start of image data */
} bmpheader;

typedef struct {
    unsigned char hdrlen[4];
    unsigned char width[4];
    unsigned char height[4];
} dibheader;

unsigned char *bmphdrvalues;

unsigned char *uncomparr;
int imagewidth;


unsigned int HextoDec(unsigned char c) {
    unsigned int dec;
    // printf("H2D %d %d \n", (c>>4)*16,(c & 15));
    dec = ((c>>4)*16)+(c & 15);
    // printf("Returning Dec %d\n", dec);
    return(dec);
}

int FourBytesHextoNum (unsigned char *c) {
    int num = HextoDec(c[3])*(1<<24)+HextoDec(c[2])*(1<<16)+HextoDec(c[1])*(1<<8)+HextoDec(c[0]);
    return (num);

}

void ReadBMPHeader (bmpheader *hdrdata, int len, FILE *fp, int *filesize, int *offset) {
    fread(hdrdata, sizeof(unsigned char), len, fp);
    printf("fileMarker1 = %c\n", hdrdata->fileMarker1);
    printf("fileMarker2 = %c\n", hdrdata->fileMarker2);
    // *filesize = HextoDec(hdrdata->bfSize[3])*(1<<24)+HextoDec(hdrdata->bfSize[2])*(1<<16)+HextoDec(hdrdata->bfSize[1])*(1<<8)+HextoDec(hdrdata->bfSize[0]);
    *filesize = FourBytesHextoNum (hdrdata->bfSize);
    printf("unused 1 = %d\n", hdrdata->unused1);
    printf("unused 2 = %d\n", hdrdata->unused2); 
    // *offset = HextoDec(hdrdata->imageDataOffset[3])*(1<<24)+HextoDec(hdrdata->imageDataOffset[2])*(1<<16)+HextoDec(hdrdata->imageDataOffset[1])*(1<<8)+HextoDec(hdrdata->imageDataOffset[0]);
    *offset = FourBytesHextoNum (hdrdata->imageDataOffset);
}

void ReadDIBHeader (dibheader *dibdata, int len, FILE *fp, int *hdrlen, int *w, int *h) {
    fread(dibdata, sizeof(unsigned char), len, fp);
    // *hdrlen = HextoDec(dibdata->hdrlen[3])*(1<<24)+HextoDec(dibdata->hdrlen[2])*(1<<16)+HextoDec(dibdata->hdrlen[1])*(1<<8)+HextoDec(dibdata->hdrlen[0]);
    *hdrlen = FourBytesHextoNum (dibdata->hdrlen);
    printf("DIB Header Len = %d\n", *hdrlen); 
    *w = FourBytesHextoNum (dibdata->width);
    printf("Image Width = %d\n", *w); 
    *h = FourBytesHextoNum (dibdata->height);
    printf("Image Height = %d\n", *h); 
}

void CreateSubSect (int ss_tlx, int ss_tly, int ss_w, int ss_h, unsigned int val, int width) {

    for (int i = 0; i < ss_h; i++) {
        for (int j = 0; j < ss_w; j++) {
            int indl = (ss_tly+i)*imagewidth+j+ss_tlx;
            uncomparr[indl] = (unsigned char) val;
        }
    }
}

void print_array(unsigned char* arr,int width,int height){
     for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            printf("%x ", (unsigned char) arr[i*width+j]);
        }
        printf("\n");
    }

}

int main ( int argc, char *argv[] ) {
    char filename[256];
    FILE *fp;
    bmpheader hdrdata;
    int filesize;
    int fileoffset;
    double avg=0.0;
    double mse=0.0;
    int dibhdrlen;
    int width;
    int height;

    // strcpy (filename, "256color16x16.bmp");
    if (argc != 3) {
        printf ("Command line requires Header File (bin) and Output file name \n");
        return (-1);
    }
    // Read the Header file (bin).
    fp = fopen(argv[1], "rb" );
    if(fp == NULL)    {
        printf("The file %s cannot be opened.\n", argv[1]);
        return (-1);
    }
    
   ReadBMPHeader (&hdrdata, sizeof(hdrdata), fp, &filesize, &fileoffset); 
   printf("FileSize = %d\n", filesize);
   printf("FileOffset = %d\n", fileoffset);

   dibheader dibdata;
   ReadDIBHeader (&dibdata, sizeof(dibdata), fp, &dibhdrlen, &width, &height); 
   printf("Height = %d, Width = %d\n", height, width);

   // Read the BMP Header region - without the rawdata.
   fseek (fp, (long int) 0,  SEEK_SET);
   bmphdrvalues = (unsigned char *) malloc(sizeof(unsigned char)*fileoffset);
   fread (bmphdrvalues, sizeof(unsigned char), fileoffset, fp);
   fclose (fp);

   FILE *ofp = fopen (argv[2], "wb");
   fwrite(bmphdrvalues, sizeof(unsigned char), fileoffset, ofp);
   
   FILE *qfp = fopen("Out.qtree", "r");
   int w, h;
   fscanf(qfp,"%d,%d",&w,&h);
   if ((w != width) || (h != height)) {
       printf("Mismatch between bin file and QTree file\n");
       return (-1);
   }

   uncomparr = (unsigned char *) malloc(sizeof(unsigned char)*height*width);

   int tlx, tly, ss_w, ss_h;
   unsigned int val;

   while (fscanf (qfp, "%d,%d,%d,%d,%x", &tlx, &tly, &ss_w, &ss_h, &val) != EOF) {
       printf("Read - %d %d %d %d %x\n", tlx, tly, ss_w, ss_h, val);
       for (int i = 0; i < ss_h; i++) {
           for (int j = 0; j < ss_w; j++) {
               int indl = (tly+i)*width+j+tlx;
               uncomparr[indl] = (unsigned char) val;
           }
       }
   }
   fclose(qfp);

   print_array(uncomparr, width, height);
   fwrite(uncomparr, sizeof(unsigned char), height*width, ofp);
   fclose(ofp);
   return 0;
}
