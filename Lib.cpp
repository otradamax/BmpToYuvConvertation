#include "lib.h"

BMP_Image::BMP_Image(char const* filename){
    // Reading bmp image file
    FILE* bmp = fopen(filename, "rb");

    if (bmp == NULL){
        std::cout << "Cannot open file.\n";
        exit(1);
    }

    // Reading image's width and height
    fseek(bmp, 18, SEEK_SET);
    fread(&this->width, sizeof(int), 1, bmp);
    fseek(bmp, 22, SEEK_SET);
    fread(&this->height, sizeof(int), 1, bmp);

    //Allocating memory
    this->R = new unsigned char* [this->height];
    this->G = new unsigned char* [this->height];
    this->B = new unsigned char* [this->height];

    for(size_t i = 0; i < this->height; i++){
        this->R[i] = new unsigned char [this->width];
        this->G[i] = new unsigned char [this->width];
        this->B[i] = new unsigned char [this->width];
    }

    //Filling with data
    fseek(bmp, 54, SEEK_SET);
	unsigned char buffer[4];
	int restBytes = (this->width % 4);
    for(size_t i = (this->height - 1); (i >= 0) && (i < this->height); i--){  
        for(size_t j = 0; j < this->width; j++){
            fread(&this->B[i][j], sizeof(unsigned char), 1, bmp);
            fread(&this->G[i][j], sizeof(unsigned char), 1, bmp);
            fread(&this->R[i][j], sizeof(unsigned char), 1, bmp);
        }
		fread(buffer, sizeof(unsigned char), restBytes, bmp);
    }
    
    fclose(bmp);
}

int BMP_Image::GetWidth() const {
    return this->width;
}

int BMP_Image::GetHeight() const {
    return this->height;
}

unsigned char BMP_Image::GetR(size_t i, size_t j) const {
    if ((i < this->height) && (j < this->width)) {
        return this->R[i][j];
    } else {
        std::cout << "No such element.\n";
        return -1;
    }
}

unsigned char BMP_Image::GetG(size_t i, size_t j) const {
    if ((i < this->height) && (j < this->width)) {
        return this->G[i][j];
    } else {
        std::cout << "No such element.\n";
        return -1;
    }
}

unsigned char BMP_Image::GetB(size_t i, size_t j) const {
    if ((i < this->height) && (j < this->width)) {
        return this->B[i][j];
    } else {
        std::cout << "No such element.\n";
        return -1;
    }
}

BMP_Image::~BMP_Image(){
    for(size_t i = 0; i < this->height; i++){
        delete[] this->R[i];
        delete[] this->G[i];
        delete[] this->B[i];
    }
    delete[] this->R;
    delete[] this->G;
    delete[] this->B;
}

void YUV_Image::AlgorithmForConvertation(BMP_Image const& bmp, int threadNumber, int rowThread, bool lastThread){

	//Using SIMD math operations will be conducted by four variables
	uint8_t array[16]__attribute__((aligned(16)));
	bool isComplete = 0;
	for (size_t i = threadNumber * rowThread; i < (lastThread ? this->height : (threadNumber + 1) * rowThread); i++){
		for (size_t j = 0; j < this->width; j += 4) {
			//Checking possibility of using four variables
			int sub = this->width - j;
			if ((sub < 2) || (sub > 4)) {
				//Convertation math formula using SIMD instructions
				//this->Y[i][j + k] = (unsigned char)(this->Kred * bmp.GetR(i, j + k) + this->Kgreen * bmp.GetG(i, j + k) + this->Kblue * bmp.GetB(i, j + k));
				__m128 mul1 = _mm_mul_ps(_mm_set1_ps(this->Kred), _mm_cvtepi32_ps(_mm_setr_epi32(bmp.GetR(i, j), bmp.GetR(i, j + 1), bmp.GetR(i, j + 2), bmp.GetR(i, j + 3))));
				__m128 mul2 = _mm_mul_ps(_mm_set1_ps(this->Kgreen), _mm_cvtepi32_ps(_mm_setr_epi32(bmp.GetG(i, j), bmp.GetG(i, j + 1), bmp.GetG(i, j + 2), bmp.GetG(i, j + 3))));
				__m128 mul3 = _mm_mul_ps(_mm_set1_ps(this->Kblue), _mm_cvtepi32_ps(_mm_setr_epi32(bmp.GetB(i, j), bmp.GetB(i, j + 1), bmp.GetB(i, j + 2), bmp.GetB(i, j + 3))));
				__m128 sum_ps = _mm_add_ps(_mm_add_ps(mul1, mul2), mul3);
				__m128i sum = _mm_cvttps_epi32(sum_ps);
				_mm_storeu_si128((__m128i*)array, sum);
				for (size_t k = 0; k < 4; k++) {
					this->Y[i][j + k] = array[k * 4];
				}
			}
			else {
				//Usage of ordinary formula for last one-three variables in row
				for (size_t k = 0; k < sub; k++) {
					this->Y[i][j + k] = (unsigned char)(this->Kred * bmp.GetR(i, j + k) + this->Kgreen * bmp.GetG(i, j + k) + this->Kblue * bmp.GetB(i, j + k));
				}
			}

			if ((i % 2 == 0) && ( sub > 4)) {
				//Filling U and V arrays every eight variables (Evaluting U and V arrays only on even elements of Y array)
				if (isComplete) {
					//Convertation math formula using SIMD instructions
					//this->U[i / 2][(j + k) / 2] = (unsigned char)((bmp.GetB(i, j + k) - this->Y[i][j + k]) / (2 * (1 - this->Kblue))) + 128;
					__m128 sub1 = _mm_sub_ps(_mm_cvtepi32_ps(_mm_setr_epi32(bmp.GetB(i, j - 4), bmp.GetB(i, j - 2), bmp.GetB(i, j), bmp.GetB(i, j + 2))), _mm_cvtepi32_ps(_mm_setr_epi32(this->Y[i][j - 4], this->Y[i][j - 2], this->Y[i][j], this->Y[i][j + 2])));
					__m128 div1 = _mm_div_ps(sub1, _mm_set1_ps(2 * (1 - this->Kblue)));
					__m128i res1 = _mm_cvttps_epi32(div1);
					_mm_storeu_si128((__m128i*)array, res1);
					for (size_t k = 0; k < 4; k++) {
						this->U[i / 2][(j - 4) / 2 + k] = array[k * 4] + 128;
					}
					//Convertation math formula using SIMD instructions
					//this->V[i / 2][(j + k) / 2] = (unsigned char)((bmp.GetR(i, j + k) - this->Y[i][j + k]) / (2 * (1 - this->Kred))) + 128;
					__m128 sub2 = _mm_sub_ps(_mm_cvtepi32_ps(_mm_setr_epi32(bmp.GetR(i, j - 4), bmp.GetR(i, j - 2), bmp.GetR(i, j), bmp.GetR(i, j + 2))), _mm_cvtepi32_ps(_mm_setr_epi32(this->Y[i][j - 4], this->Y[i][j - 2], this->Y[i][j], this->Y[i][j + 2])));
					__m128 div2 = _mm_div_ps(sub2, _mm_set1_ps(2 * (1 - this->Kred)));
					__m128i res2 = _mm_cvttps_epi32(div2);
					_mm_storeu_si128((__m128i*)array, res2);
					for (size_t k = 0; k < 4; k++) {
						this->V[i / 2][(j - 4) / 2 + k] = array[k * 4] + 128;
					}
					isComplete = 0;
				}
				else if(sub != 1) {
					isComplete = 1;
				}
			}
			else if ((i % 2 == 0) && (!isComplete)) {
				//Usage of ordinary formula for last one-seven variables in row
				for (int k = (isComplete ?  -4 : 0); k < sub; k += 2) {
					this->U[i / 2][(j + k) / 2] = (unsigned char)((bmp.GetB(i, j + k) - this->Y[i][j + k]) / (2 * (1 - this->Kblue))) + 128;
					this->V[i / 2][(j + k) / 2] = (unsigned char)((bmp.GetR(i, j + k) - this->Y[i][j + k]) / (2 * (1 - this->Kred))) + 128;
				}
				isComplete = 0;
			}
		}
	}
}

YUV_Image::YUV_Image(BMP_Image const& bmp): width(bmp.GetWidth()), height(bmp.GetHeight()), halfWidth(bmp.GetWidth() % 2 == 0 ? bmp.GetWidth() / 2 : bmp.GetWidth() / 2 + 1), halfHeight(bmp.GetHeight() % 2 == 0 ? bmp.GetHeight() / 2 : bmp.GetHeight() / 2 + 1) {

    //Allocating memory
    this->Y = new unsigned char* [this->height];
    this->U = new unsigned char* [this->halfHeight];
    this->V = new unsigned char* [this->halfHeight];

    for(size_t i = 0; i < this->height; i++){
        this->Y[i] = new unsigned char [this->width];
    }
    for(size_t i = 0; i< this->halfHeight; i++){
        this->U[i] = new unsigned char [this->halfWidth];
        this->V[i] = new unsigned char [this->halfWidth];
    }

	//Available threads
    int threadAmount = std::thread::hardware_concurrency();
	if (threadAmount == 0){
		std::cout << "Can't evaluate an amount of threads";
        exit(1);
	}

    //Case of small yuv image
	if (this->height / 2 < threadAmount){
		if (this->height % 2 == 0) {
            threadAmount = this->height / 2;
        } else {
            threadAmount = this->height / 2 + 1;
        }
	}

	//Number of rows for thread
	int rowThread = this->height / threadAmount;
	if (rowThread % 2 == 1){
		rowThread--;
	}

	// Lambda function for threads
	std::function<void(int)> func = [&](int threadNumber){
		AlgorithmForConvertation(bmp,threadNumber,rowThread,0);
	};

	// Vector of threads
	std::vector<std::thread> th;
	for (size_t i = 0; i < threadAmount - 1; i++){
		th.push_back(std::thread(func, i));
	}

	// The last thread
	AlgorithmForConvertation(bmp,threadAmount - 1,rowThread,1);
    
	for (size_t i = 0; i < threadAmount - 1; i++){
		th[i].join();
	}

}

YUV_Image::~YUV_Image(){
    for(size_t i = 0; i < this->height; i++){
        delete[] this->Y[i];
    }
    for(size_t i = 0; i< this->halfHeight; i++){
        delete[] this->U[i];
        delete[] this->V[i];
    }
    delete[] this->Y;
    delete[] this->U;
    delete[] this->V;
}

YUV_Video::YUV_Video(char const* filename, int width, int height): width(width), height(height), halfWidth(width % 2 == 0 ? width / 2 : width / 2 + 1), halfHeight(height % 2 == 0 ? height / 2 : height / 2 + 1), filename(filename){
    //Reading yuv video file
	FILE* yuvVideo = fopen(filename, "rb");

    if (yuvVideo == NULL){
        std::cout << "Cannot open file.\n";
        exit(1);
    }

    //Evaluating size and number of frames
    fseek(yuvVideo,0,SEEK_END);
    this->frameSize = (this->width * this->height + 2 * this->halfWidth * this->halfHeight);
    this->frameNumber = ftell(yuvVideo) / (this->frameSize);
    fclose(yuvVideo); 
}

void addImageToVideo(YUV_Video& video, YUV_Image& image, char const* newFilename){
    if ((image.width <= video.width) && (image.height <= video.height)){
        FILE* yuvVideo = fopen(video.filename,"rb");
        FILE* yuvFinalVideo = fopen(newFilename,"wb");

        int U_offset = video.width * video.height;
        int V_offset = U_offset + video.halfWidth * video.halfHeight;

        //Creating buffer for writing a frame
        unsigned char * buffer = new unsigned char [video.frameSize];

        for (size_t i = 0; i < video.frameNumber; i++){
			fread(buffer, sizeof(unsigned char), video.frameSize, yuvVideo);
			for (size_t j = 0; j < image.height; j++){
				for (size_t k = 0; k < image.width; k++){
					buffer[j * video.width + k] = image.Y[j][k];
				}
			}
			for (size_t j = 0; j < image.halfHeight; j++){
				for (size_t k = 0; k < image.halfWidth; k++){
					buffer[U_offset + j * video.halfWidth + k] = image.U[j][k];
				}
			}
			for (size_t j = 0; j < image.halfHeight; j++){
				for (size_t k = 0; k < image.halfWidth; k++){
					buffer[V_offset + j * video.halfWidth + k] = image.V[j][k];
				}
			}
			fwrite(buffer, sizeof(unsigned char), video.frameSize, yuvFinalVideo);
		}
		fclose(yuvVideo);
		fclose(yuvFinalVideo);

        delete[] buffer;
    }
}