#pragma warning( disable : 4996 ) 
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <cmath>
#include <direct.h>
#include "bitmap_image.hpp"

using namespace std;

typedef struct point
{
	float x;
	float y;

} point;

typedef struct line		//line struct, a line from point A to point B
{
	point A;
	point B;
} line;

float a, b, p;
int numOfLines, seqLen;
bool bilinearFlag = false;
bool gaussianFlag = false;

void warp(bitmap_image srcImg, vector<line> srcLines, vector<line> dstLines, string warpFolder, string warpName);
int crossDissolve(string srcWarpFolder, string dstWarpFolder, string morphFolder);

int main(int argc, const char* argv[])
{
	string currentLine;
	string srcImgFile, dstImgFile, srcWarpFolder, dstWarpFolder, morphFolder;
	numOfLines = 0;
	
	vector <line> srcLines;
	vector <line> dstLines;
	
	
	if (argc == 1)
		return -1;

	if (argc == 3) {
		if (strcmp(argv[2], "-b") == 0) {
			bilinearFlag = true;
			cout << "using bilinear interpolation " << endl;
		}
		if (strcmp(argv[2], "-g") == 0) {
			gaussianFlag = true;
			cout << "using gaussian interpolation " << endl;
		}
	}

	ifstream configFile(argv[1]);
    
	if (configFile.is_open())
	{
		//get 5 first lines
		getline(configFile, srcImgFile);
		getline(configFile, dstImgFile);
		getline(configFile, srcWarpFolder);
		getline(configFile, dstWarpFolder);
		getline(configFile, morphFolder);

		//get numeric data for seqlen a b p
		getline(configFile, currentLine);
		sscanf(currentLine.c_str(), "%d %f %f %f", &seqLen, &a, &b, &p);

		//read the lines as <x_src> <y_src> <x_dst> <y_dst>
		while (getline(configFile, currentLine))
		{
			line tmp1;
			line tmp2;
		//	sscanf(currentLine.c_str(), "%f %f %f %f %f %f %f %f", &tmp1.A.x, &tmp1.A.y, &tmp2.A.x, &tmp2.A.y, &tmp1.B.x, &tmp1.B.y, &tmp2.B.x, &tmp2.B.y);
			sscanf(currentLine.c_str(), "%f %f %f %f %f %f %f %f", &tmp1.A.x, &tmp1.A.y, &tmp1.B.x, &tmp1.B.y, &tmp2.A.x, &tmp2.A.y, &tmp2.B.x, &tmp2.B.y);
			srcLines.push_back(tmp1);
			dstLines.push_back(tmp2);
			numOfLines++;
		}
	}

	else
		cout << "unable to open config file!!"<<endl;

	// Creating new directories
	char buffer[256];
	sprintf(buffer, "mkdir %s", srcWarpFolder.c_str());
	system(buffer);
	sprintf(buffer, "mkdir %s", dstWarpFolder.c_str());
	system(buffer);
	sprintf(buffer, "mkdir %s", morphFolder.c_str());
	system(buffer);

	// Reading the bitmap images
	bitmap_image srcImg (srcImgFile);
	if (!srcImg) {
		printf("Error failed to open source image\n");
		return -1;
	}

	bitmap_image dstImg (dstImgFile);
	if (!dstImg) {
		printf("Error failed to open destination image\n");
		return -1;
	}

	warp(srcImg, srcLines, dstLines, srcWarpFolder, "src");
	warp(dstImg, dstLines, srcLines, dstWarpFolder, "dst");
	int error=crossDissolve(srcWarpFolder, dstWarpFolder, morphFolder);
	
/*
    cout << srcImgFile << endl;
	cout << dstImgFile << endl;	
	cout << srcWarpFolder << endl;
	cout << dstWarpFolder << endl;
	cout << morphFolder << endl;
	cout << seqLen << endl;
	cout << a << endl;
	cout << b << endl;
	cout << p << endl;

	
	for (i = 0; i < numOfLines; i++) {
		cout << "L1 srcX " << srcLines[i].A.x << "," << "L1 dstX " << srcLines[i].B.x << endl;
		cout << "L1 srcY " << srcLines[i].A.y << "," << "L1 dstY " << srcLines[i].B.y << endl;
		cout << "L2 srcX " << dstLines[i].A.x << "," << "L2 dstX " << dstLines[i].B.x << endl;
		cout << "L2 srcY " << dstLines[i].A.y << "," << "L2 dstY " << dstLines[i].B.y << endl;
	}
	*/


}

void warp(bitmap_image srcImg, vector<line> srcLines, vector<line> dstLines, string warpFolder, string warpName) {
	
	const unsigned int imgHeight = srcImg.height();
	const unsigned int imgWidth = srcImg.width();
	size_t x_pixel, y_pixel;
	int i; //line index
	int frame; // frame number
	float t, t_prev, u, v,weight,weightSum,length,dist;
	rgb_t color;
	point dsum,X_A, B_A, Btag_Atag, perpB_A, perpBtag_Atag, srcPixel,D;
	line t_dstImgLine, t_srcImgLine;

	float dx, dy;
	int x0, x1, y0, y1, tmp_pixel_x, tmp_pixel_y;
	rgb_t color_x0_y0, color_x1_y0, color_x0_y1, color_x1_y1;
	rgb_t color1_1, color1_2, color1_3, color2_1, color2_2, color2_3, color3_1, color3_2, color3_3;

	char fileName[128];
	
	//save frame0000
	sprintf(fileName, "%s\\%s_%04d.bmp", warpFolder.c_str(), warpName.c_str(), 0);
	cout << "Saving image: " << fileName << endl;
	srcImg.save_image(fileName);

	//frame = 1;
	for (frame = 1; frame <= seqLen; frame++) {

		bitmap_image dstImg(srcImg.width(), srcImg.height());  	// Creating new dst Image for warp stage t
		t = (float)frame / seqLen;
		t_prev = (float)(frame - 1) / seqLen;

		for (y_pixel = 0; y_pixel < imgHeight; y_pixel++) {
			for (x_pixel = 0; x_pixel < imgWidth; x_pixel++) {
				//for each pixel in the destination image
				dsum.x = 0;
				dsum.y = 0;
				weightSum = 0;

				//for each line 
				for (i = 0; i < numOfLines; i++) {
					//set the lines according to the frame number
					t_dstImgLine.A.x = (float)(t*dstLines[i].A.x + (1.0 - t)*srcLines[i].A.x);
					t_dstImgLine.A.y = (float)(t*dstLines[i].A.y + (1.0 - t)*srcLines[i].A.y);
					t_dstImgLine.B.x = (float)(t*dstLines[i].B.x + (1.0 - t)*srcLines[i].B.x);
					t_dstImgLine.B.y = (float)(t*dstLines[i].B.y + (1.0 - t)*srcLines[i].B.y);

					t_srcImgLine.A.x = (float)(t_prev*dstLines[i].A.x + (1.0 - t_prev)*srcLines[i].A.x);
					t_srcImgLine.A.y = (float)(t_prev*dstLines[i].A.y + (1.0 - t_prev)*srcLines[i].A.y);
					t_srcImgLine.B.x = (float)(t_prev*dstLines[i].B.x + (1.0 - t_prev)*srcLines[i].B.x);
					t_srcImgLine.B.y = (float)(t_prev*dstLines[i].B.y + (1.0 - t_prev)*srcLines[i].B.y);

					X_A.x = x_pixel - t_dstImgLine.A.x;
					X_A.y = y_pixel - t_dstImgLine.A.y;
					B_A.x = t_dstImgLine.B.x - t_dstImgLine.A.x;
					B_A.y = t_dstImgLine.B.y - t_dstImgLine.A.y;
					perpB_A.x = -B_A.y;
					perpB_A.y = B_A.x;

					Btag_Atag.x = t_srcImgLine.B.x - t_srcImgLine.A.x;
					Btag_Atag.y = t_srcImgLine.B.y - t_srcImgLine.A.y;
					perpBtag_Atag.x = -Btag_Atag.y;
					perpBtag_Atag.y = Btag_Atag.x;

					u = (X_A.x*B_A.x + X_A.y*B_A.y) / (B_A.x*B_A.x + B_A.y*B_A.y);
					v = (X_A.x*perpB_A.x + X_A.y*perpB_A.y) / (sqrt(B_A.x*B_A.x + B_A.y*B_A.y));

					// find the pixel in the source Image
					srcPixel.x = t_srcImgLine.A.x + (u*Btag_Atag.x) + ((v*perpBtag_Atag.x) / (sqrt(Btag_Atag.x*Btag_Atag.x + Btag_Atag.y*Btag_Atag.y)));
					srcPixel.y = t_srcImgLine.A.y + (u*Btag_Atag.y) + ((v*perpBtag_Atag.y) / (sqrt(Btag_Atag.x*Btag_Atag.x + Btag_Atag.y*Btag_Atag.y)));

					// displacement
					D.x = srcPixel.x - x_pixel;
					D.y = srcPixel.y - y_pixel;

					length = sqrt(powf((t_dstImgLine.B.x - t_dstImgLine.A.x), 2.0) + powf((t_dstImgLine.B.y - t_dstImgLine.A.y), 2.0));

					if (u < 0 || u > 1) {
						if (u < 0)
							dist = sqrt(powf((t_dstImgLine.A.x - x_pixel), 2.0) + powf((t_dstImgLine.A.y - y_pixel), 2.0));
						if (u > 1)
							dist = sqrt(powf((t_dstImgLine.B.x - x_pixel), 2.0) + powf((t_dstImgLine.B.y - y_pixel), 2.0));
					}
					else
						dist = abs(v);

					weight = powf(((powf(length, p)) / (a + dist)), b);

					dsum.x = dsum.x + (D.x *weight);
					dsum.y = dsum.y + (D.y *weight);
					weightSum = weightSum + weight;
				}
				srcPixel.x = x_pixel + (dsum.x / weightSum);
				srcPixel.y = y_pixel + (dsum.y / weightSum);

				if (floorf(srcPixel.x)< 0 || ceilf(srcPixel.x) >= imgWidth || floorf(srcPixel.y) < 0 || ceilf(srcPixel.y) >= imgHeight) {
					if (!bilinearFlag && !gaussianFlag) {		// Check if pixel is out of bounds (Implementation note #2)
						color.red = 0;
						color.green = 0;
						color.blue = 0;
					}
					else {		// Taking nearest neighbor instead of blank pixel in case of "pixel out of bounds" 
							tmp_pixel_x =(int) roundf(srcPixel.x);
							tmp_pixel_y =(int)roundf(srcPixel.y);
							if (floorf(srcPixel.x) < 0)
								tmp_pixel_x = 0;
							if (ceilf(srcPixel.x) >= imgWidth)
								tmp_pixel_x = imgWidth - 1;
							if (floorf(srcPixel.y) < 0)
								tmp_pixel_y = 0;
							if (ceilf(srcPixel.y) >= imgHeight)
								tmp_pixel_y = imgHeight - 1;
							srcImg.get_pixel(tmp_pixel_x, tmp_pixel_y, color);
						}
					}
				else
				{
					if (!bilinearFlag && !gaussianFlag) { // Using roundf function for nearest-neighbor interpolation
						srcImg.get_pixel((int)roundf(srcPixel.x), (int)roundf(srcPixel.y), color);
					}
					else {   
						//using gaussian interpolation
							//	Calculated by 3*3 matrix weights by Gaussian as we saw in calss material (from image filters);
							//	By the following weights:
							//	1 , 2 , 1
							//	2 , 4 , 2	*	(1/16)
							//	1 , 2 , 1
						if (gaussianFlag && roundf(srcPixel.x) > 0 && roundf(srcPixel.x) < (imgWidth - 1) && roundf(srcPixel.y) > 0 && roundf(srcPixel.y) < (imgHeight - 1)) {
							tmp_pixel_x = (int)roundf(srcPixel.x);
							tmp_pixel_y = (int)roundf(srcPixel.y);

							srcImg.get_pixel((tmp_pixel_x - 1), (tmp_pixel_y - 1), color1_1);
							srcImg.get_pixel(tmp_pixel_x, (tmp_pixel_y - 1), color1_2);
							srcImg.get_pixel((tmp_pixel_x + 1), (tmp_pixel_y - 1), color1_3);
							srcImg.get_pixel((tmp_pixel_x - 1), tmp_pixel_y, color2_1);
							srcImg.get_pixel(tmp_pixel_x, tmp_pixel_y, color2_2);						
							srcImg.get_pixel(tmp_pixel_x + 1, tmp_pixel_y, color2_3);
							srcImg.get_pixel(tmp_pixel_x - 1, tmp_pixel_y + 1, color3_1);
							srcImg.get_pixel(tmp_pixel_x, tmp_pixel_y + 1, color3_2);
							srcImg.get_pixel(tmp_pixel_x + 1, tmp_pixel_y + 1, color3_3);
						
							color.red =(char) roundf((float)(color1_1.red*(1.0 / 16) + color1_2.red*(2.0 / 16) + color1_3.red*(1.0 / 16) + color2_1.red*(2.0 / 16) + color2_2.red*(4.0 / 16) + color2_3.red*(2.0 / 16) + color3_1.red*(1.0 / 16) + color3_2.red*(2.0 / 16) + color3_3.red*(1.0 / 16)));
							color.green = (char)roundf((float)(color1_1.green*(1.0 / 16) + color1_2.green*(2.0 / 16) + color1_3.green*(1.0 / 16) + color2_1.green*(2.0 / 16) + color2_2.green*(4.0 / 16) + color2_3.green*(2.0 / 16) + color3_1.green*(1.0 / 16) + color3_2.green*(2.0 / 16) + color3_3.green*(1.0 / 16)));
							color.blue = (char)roundf((float)(color1_1.blue*(1.0 / 16) + color1_2.blue*(2.0 / 16) + color1_3.blue*(1.0 / 16) + color2_1.blue*(2.0 / 16) + color2_2.blue*(4.0 / 16) + color2_3.blue*(2.0 / 16) + color3_1.blue*(1.0 / 16) + color3_2.blue*(2.0 / 16) + color3_3.blue*(1.0 / 16)));
						}
						
						
						else {	//using bilinear interpolation
							x0 = (int)floorf(srcPixel.x);
							x1 = (int)ceilf(srcPixel.x);
							y0 = (int)floorf(srcPixel.y);
							y1 = (int)ceilf(srcPixel.y);
							//cout << x0 << "," << x1 << "," << y0 << "," << y1 << "src" << srcPixel.x << "," << srcPixel.y << endl;
							dx = (float)(srcPixel.x - x0);
							dy = (float)(srcPixel.y - y0);
							srcImg.get_pixel(x0, y0, color_x0_y0);
							srcImg.get_pixel(x0, y1, color_x0_y1);
							srcImg.get_pixel(x1, y0, color_x1_y0);
							srcImg.get_pixel(x1, y1, color_x1_y1);

							color.red = (char)roundf(color_x0_y0.red*(1 - dx)*(1 - dy) + color_x1_y0.red*dx*(1 - dy) + color_x0_y1.red*(1 - dx)*dy + color_x1_y1.red*dx*dy);
							color.blue = (char)roundf(color_x0_y0.blue*(1 - dx)*(1 - dy) + color_x1_y0.blue*dx*(1 - dy) + color_x0_y1.blue*(1 - dx)*dy + color_x1_y1.blue*dx*dy);
							color.green = (char)roundf(color_x0_y0.green*(1 - dx)*(1 - dy) + color_x1_y0.green*dx*(1 - dy) + color_x0_y1.green*(1 - dx)*dy + color_x1_y1.green*dx*dy);
						}
					}
				}
				dstImg.set_pixel(x_pixel, y_pixel, color);
			}
		}
		sprintf(fileName, "%s\\%s_%04d.bmp", warpFolder.c_str(), warpName.c_str(), frame);
		cout << "Saving image: " << fileName << endl;
		dstImg.save_image(fileName);
		srcImg = dstImg;
	}
}

int crossDissolve(string srcWarpFolder, string dstWarpFolder, string morphFolder) {
	
	int frame;
	float t;
	char t_SrcWarpFileName[128];
	char t_DstWarpFileName[128];
	char t_MorphFileName[128];
	rgb_t srcColor, dstColor, morphColor;

	for (frame = 0; frame <= seqLen; frame++) {
		t = (float)frame / seqLen;
		//create file names 
		sprintf(t_SrcWarpFileName, "%s\\src_%04d.bmp", srcWarpFolder.c_str(), frame);
		sprintf(t_DstWarpFileName, "%s\\dst_%04d.bmp", dstWarpFolder.c_str(), (seqLen-frame));

		bitmap_image srcImg(t_SrcWarpFileName);
		if (!srcImg) {
			printf("Error failed to open source image\n");
			return -1;
		}
		
		bitmap_image dstImg(t_DstWarpFileName);
		if (!srcImg) {
			printf("Error failed to open destination image\n");
			return -1;
		}

		bitmap_image morphImg(srcImg.width(), srcImg.height());  	// Creating new morph Image for morph stage t

		for (unsigned	int y=0; y < srcImg.height(); y++) {
			for (unsigned int x = 0; x < srcImg.width(); x++) {
				srcImg.get_pixel(x, y, srcColor);
				dstImg.get_pixel(x, y, dstColor);

				morphColor.blue = (char)roundf((1 - t)*srcColor.blue + t*dstColor.blue);
				morphColor.green = (char)roundf((1 - t)*srcColor.green + t*dstColor.green);
				morphColor.red = (char)roundf((1 - t)*srcColor.red + t*dstColor.red);
				
				morphImg.set_pixel(x, y, morphColor);
			}
		}
		sprintf(t_MorphFileName, "%s\\morph_%04d.bmp", morphFolder.c_str(),frame);
		cout << "Saving image: " << t_MorphFileName << endl;
		morphImg.save_image(t_MorphFileName);
	}
	return 0;
}
