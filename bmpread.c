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

typedef struct {
    int ss_tlx;
    int ss_tly;
    int ss_w;
    int ss_h;
    unsigned char avg;
    double mse;
} SubSect;

typedef struct {
    SubSect *ChildNW;
    SubSect *ChildNE;
    SubSect *ChildSW;
    SubSect *ChildSE;
} ChildSect;

typedef struct Node{
    SubSect *Image;
    struct Node *ChildNW;
    struct Node *ChildNE;
    struct Node *ChildSW;
    struct Node *ChildSE;
    struct Node *parent;
} QNode;

unsigned char *bmphdrvalues;

unsigned char *arr;
unsigned char *uncomparr;
int imagewidth;
double threshold=1500.0;
QNode root;


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

void ReadFile(FILE *fp,int width,int height){
    int i=0; 
    int j=0;

    imagewidth = width;
    arr = (unsigned char *) malloc(sizeof(unsigned char)*height*width);
    fread (arr, sizeof(unsigned char), height*width, fp);
}

void print_array(unsigned char* arr,int width,int height){
     for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            printf("%x ", (unsigned char) arr[i*width+j]);
        }
        printf("\n");
    }

}

void DisplaySubSect (SubSect *Image, FILE *ofp) {

    if (ofp != NULL) {
        fprintf(ofp, "%d,%d,%d,%d,%x\n", Image->ss_tlx, Image->ss_tly, Image->ss_w, Image->ss_h, (unsigned char) Image->avg);
    }
    printf("TLX = %d, TLY = %d, Width = %d, Height = %d \n", Image->ss_tlx, Image->ss_tly, Image->ss_w, Image->ss_h);
    for (int i = 0; i < Image->ss_h; i++) {
        for (int j = 0; j < Image->ss_w; j++) {
            int indl = (Image->ss_tly+i)*imagewidth+j+Image->ss_tlx;
            printf("%x ", (unsigned char) arr[indl]);
         }
        printf("\n");
    }
    printf("\nAverage Color %x\n", (unsigned char) Image->avg);
    printf("MSE Value %lf\n", Image->mse);
}

void SetAverageMSESubSect (SubSect *Image) {

    unsigned long int sum=0;
    unsigned long int sumsq=0;
    double sumdiff=0;
    int n = Image->ss_w*Image->ss_h;

    for (int i = 0; i < Image->ss_h; i++) {
        for (int j = 0; j < Image->ss_w; j++) {
            int indl = (Image->ss_tly+i)*imagewidth+j+Image->ss_tlx;
            sum += (unsigned long int) arr[indl];
            sumsq += (unsigned long int) (arr[indl]*arr[indl]);
         }
    }
    double avg = (sum*1.0)/n;
    Image->avg = (unsigned char) round(avg);

    double sqavg = (sumsq*1.0)/n;
    Image->mse = sqavg - (avg*avg);
}

void Split4 (SubSect *Image, ChildSect *Child) {

    SubSect *Node1 = (SubSect *) malloc (sizeof (SubSect));
    Node1->ss_tlx = Image->ss_tlx;
    Node1->ss_tly = Image->ss_tly;
    Node1->ss_w = Image->ss_w/2;
    Node1->ss_h = Image->ss_h/2;
    Child->ChildNW = Node1;

    SubSect *Node2 = (SubSect *) malloc (sizeof (SubSect));
    Node2->ss_tlx = Image->ss_tlx+(Image->ss_w/2);
    Node2->ss_tly = Image->ss_tly;
    Node2->ss_w = Image->ss_w/2;
    Node2->ss_h = Image->ss_h/2;
    Child->ChildNE = Node2;

    SubSect *Node3 = (SubSect *) malloc (sizeof (SubSect));
    Node3->ss_tlx = Image->ss_tlx;
    Node3->ss_tly = Image->ss_tly+(Image->ss_h/2);
    Node3->ss_w = Image->ss_w/2;
    Node3->ss_h = Image->ss_h/2;
    Child->ChildSW = Node3;

    SubSect *Node4 = (SubSect *) malloc (sizeof (SubSect));
    Node4->ss_tlx = Image->ss_tlx+(Image->ss_w/2);
    Node4->ss_tly = Image->ss_tly+(Image->ss_h/2);
    Node4->ss_w = Image->ss_w/2;
    Node4->ss_h = Image->ss_h/2;
    Child->ChildSE = Node4;
}

void Divideinto4 (QNode *QN) {
   ChildSect CS;
   Split4 (QN->Image, &CS);

   QNode *Node1 = (QNode *) malloc (sizeof( QNode));
   Node1->parent = QN;
   Node1->ChildNW = (QNode *) NULL;
   Node1->ChildNE = (QNode *) NULL;
   Node1->ChildSW = (QNode *) NULL;
   Node1->ChildSE = (QNode *) NULL;

   QN->ChildNW = Node1;
   Node1->Image = CS.ChildNW;
   SetAverageMSESubSect (CS.ChildNW);
   if (Node1->Image->mse > threshold) {
       Divideinto4(Node1);
   }

   QNode *Node2 = (QNode *) malloc (sizeof( QNode));
   Node2->parent = QN;
   Node2->ChildNW = (QNode *) NULL;
   Node2->ChildNE = (QNode *) NULL;
   Node2->ChildSW = (QNode *) NULL;
   Node2->ChildSE = (QNode *) NULL;

   QN->ChildNE = Node2;
   Node2->Image = CS.ChildNE;
   SetAverageMSESubSect (CS.ChildNE);
   if (Node2->Image->mse > threshold) {
       Divideinto4(Node2);
   }

   QNode *Node3 = (QNode *) malloc (sizeof( QNode));
   Node3->parent = QN;
   Node3->ChildNW = (QNode *) NULL;
   Node3->ChildNE = (QNode *) NULL;
   Node3->ChildSW = (QNode *) NULL;
   Node3->ChildSE = (QNode *) NULL;

   QN->ChildSW = Node3;
   Node3->Image = CS.ChildSW;
   SetAverageMSESubSect (CS.ChildSW);
   if (Node3->Image->mse > threshold) {
       Divideinto4(Node3);
   }

   QNode *Node4 = (QNode *) malloc (sizeof( QNode));
   Node4->parent = QN;
   Node4->ChildNW = (QNode *) NULL;
   Node4->ChildNE = (QNode *) NULL;
   Node4->ChildSW = (QNode *) NULL;
   Node4->ChildSE = (QNode *) NULL;

   QN->ChildSE = Node4;
   Node4->Image = CS.ChildSE;
   SetAverageMSESubSect (CS.ChildSE);
   if (Node4->Image->mse > threshold) {
       Divideinto4(Node4);
   }

}

void Print_QTree_PostOrder (QNode *QN, int level, FILE *ofp) {

    // printf("Level %d\n", level);
    if (QN == (QNode *) NULL) 
        return;
    if ((QN->ChildNW == (QNode *) NULL)
        && (QN->ChildNE == (QNode *) NULL)
        && (QN->ChildSW == (QNode *) NULL)
        && (QN->ChildSE == (QNode *) NULL)
        ) {
        // Leaf Node - Print the contents of the QNode.
        printf("========%d===========\n", level);
        DisplaySubSect(QN -> Image, ofp);
        printf("*******************\n");
        return;
    }
    if (QN->ChildNW != (QNode *) NULL){
        Print_QTree_PostOrder(QN->ChildNW, level+1, ofp);
    }
    if (QN->ChildNE != (QNode *) NULL){
        Print_QTree_PostOrder(QN->ChildNE, level+1, ofp);
    }
    if (QN->ChildSW != (QNode *) NULL){
        Print_QTree_PostOrder(QN->ChildSW, level+1, ofp);
    }
    if (QN->ChildSE != (QNode *) NULL){
        Print_QTree_PostOrder(QN->ChildSE, level+1, ofp);
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
        printf ("Command line requires file name and threshold value\n");
        return (-1);
    }
    fp = fopen(argv[1], "rb" );
    if(fp == NULL)    {
        printf("The file %s cannot be opened.\n", argv[1]);
        return (-1);
    }
    threshold = (double)atof(argv[2]);
    printf("Threshold value set as %lf\n", threshold);
    
   ReadBMPHeader (&hdrdata, sizeof(hdrdata), fp, &filesize, &fileoffset); 
   printf("FileSize = %d\n", filesize);
   printf("FileOffset = %d\n", fileoffset);

  /* 
   height = (int) sqrt(filesize-fileoffset);
   width = height;
  */
   
   dibheader dibdata;
   ReadDIBHeader (&dibdata, sizeof(dibdata), fp, &dibhdrlen, &width, &height); 
   printf("Height = %d, Width = %d\n", height, width);

   // Read the BMP Header region - without the rawdata.
   fseek (fp, (long int) 0,  SEEK_SET);
   bmphdrvalues = (unsigned char *) malloc(sizeof(unsigned char)*fileoffset);
   fread (bmphdrvalues, sizeof(unsigned char), fileoffset, fp);

   ReadFile(fp,width,height);
   fclose(fp);
   
   FILE *bmphdrwrite = fopen ("BMPHeader.bin", "w");
   fwrite(bmphdrvalues, sizeof(unsigned char), fileoffset, bmphdrwrite);
   fclose(bmphdrwrite);


   // print_array (arr, width, height);
   SubSect *FullImage = malloc(sizeof(SubSect));
   FullImage->ss_tlx = 0;
   FullImage->ss_tly = 0;
   FullImage->ss_w = width;
   FullImage->ss_h = height;
   SetAverageMSESubSect (FullImage);
   DisplaySubSect (FullImage, NULL);

   root.Image = FullImage;
   root.ChildNW = (QNode *) NULL;
   root.ChildNE = (QNode *) NULL;
   root.ChildSW = (QNode *) NULL;
   root.ChildSE = (QNode *) NULL;
   root.parent = (QNode *) NULL;

   if (root.Image->mse > threshold) {
       Divideinto4 (&root);
   }
   
   FILE *ofp = fopen("Out.qtree", "w");
   fprintf(ofp,"%d,%d\n",width,height);
   Print_QTree_PostOrder (&root, 0, ofp);
   fclose(ofp);

   return 0;
}
