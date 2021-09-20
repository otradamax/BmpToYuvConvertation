#include "Lib.h"
#include <thread>

int main(){
    BMP_Image bmpImage("../test1.bmp");
    YUV_Image yuvImage(bmpImage);
    YUV_Video yuvVideo("../video.yuv",352,288);
    addImageToVideo(yuvVideo,yuvImage,"../newVideo.yuv");
    return 0;
}