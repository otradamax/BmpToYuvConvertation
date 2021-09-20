#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <functional>
#include <cstring>
#include <emmintrin.h>
#include <immintrin.h>
#include <intrin.h>

class BMP_Image;
class YUV_Image;
class YUV_Video;

class BMP_Image{
    private:
        int width;
        int height;
        unsigned char** R;
        unsigned char** G;
        unsigned char** B;

    public: 
        BMP_Image(char const* file);

        int GetWidth() const;
        int GetHeight() const;
        unsigned char GetR(size_t i, size_t j) const;
        unsigned char GetB(size_t i, size_t j) const;
        unsigned char GetG(size_t i, size_t j) const;

        ~BMP_Image();
};

class YUV_Image{
    private:
        int width;
        int height;
        int halfWidth;
        int halfHeight;
        unsigned char** Y;
        unsigned char** U;
        unsigned char** V;
        const float Kred = 0.299;
        const float Kgreen = 0.587;
        const float Kblue = 0.114;
    
    public:
        YUV_Image(BMP_Image const& bmp);

        void AlgorithmForConvertation(BMP_Image const& bmp, int threadNumber, int rowThread, bool lastThread);
        //Adding yuv image to yuv video
        friend void addImageToVideo(YUV_Video& video, YUV_Image& image, char const* newFilename);

        ~YUV_Image();
};

class YUV_Video{
    private:
        int width;
        int height;
        int halfWidth;
        int halfHeight;
        int frameNumber;
        int frameSize;
        char const* filename;

    public:
        YUV_Video(char const* filename, int width, int height);

        //Adding yuv image to yuv video
        friend void addImageToVideo(YUV_Video& video, YUV_Image& image, char const* newFilename);
};
