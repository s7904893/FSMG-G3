#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/CameraUi.h"
#include "cinder/Camera.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Shader.h"
#include "cinder/Perlin.h"
#include "cinder/Log.h"
#include <stdlib.h>     
#include <time.h>       
#include <random>
#include <iostream>
#include <math.h>
#include <chrono>
#include "cinder/Capture.h"
#include <fstream>

using namespace ci;
using namespace ci::app;

class NetzeUndRauschen : public App {
 public:
	void    setup() override;
	void    keyDown( KeyEvent event ) override;
	void    draw() override;
	void	setInitialColormatrix();

 private:
	
	CameraPersp		mCam;
	CameraUi		mCamUi;

	
	CaptureRef			mCapture;
	gl::TextureRef		mTexture;
	Surface referenceSurface; // reference surface without any motion
	bool mapCameraToNoiseNet = false;
};

struct ColorHSV
{
	double h = 0;
	double s = 0;
	double v = 0;
};

struct ColorRGB
{
	double r = 0;
	double g = 0;
	double b = 0;
};

float randomBetween(float smallNumber, float bigNumber)
{
	float diff = bigNumber - smallNumber;
	return (((float)rand() / RAND_MAX) * diff) + smallNumber;
}

const int netSize = 12;
ColorHSV colorsHSV[netSize][netSize];
vec2 pointMatrix[netSize][netSize];

// strength of effect
float perlinEffectStrength = 0.5;

//speed of moving perlin
float perlinShiftSpeed = 1.0;

// saturation manipulator
float saturationAddend = 0;

std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

Perlin perlinNoise;


void NetzeUndRauschen::setup()
{

	gl::enableAlphaBlending();
	gl::enableDepthRead();
	gl::enableDepthWrite();    


	mCapture = Capture::create(640, 480);
	mCapture->start();

	mCamUi = CameraUi( &mCam, getWindow() );
	mCam.setNearClip( 1 );
	mCam.setFarClip( 2000 );
	mCam.lookAt(vec3(0, netSize / 2, netSize / 2));
	mCam.setEyePoint(vec3(15, netSize / 2, netSize /  2));


	srand(time(NULL));

	Surface initialCameraScan = *mCapture->getSurface();
	setInitialColormatrix();

	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			pointMatrix[i][j].x = i;
			pointMatrix[i][j].y = j;

			colorsHSV[i][j].h = 242.0 / 360.0;
			//colorsHSV[i][j].s = randomBetween(.5, .85);
			colorsHSV[i][j].s = randomBetween(.3, .65);
			colorsHSV[i][j].v = randomBetween(.8, 1.0);

		}
	}

}


ColorRGB initialColormatrix[netSize][netSize];

float colorDistanceMatrix[netSize][netSize];

void NetzeUndRauschen::setInitialColormatrix()
{
	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			initialColormatrix[i][j].r = 0;
			initialColormatrix[i][j].g = 0;
			initialColormatrix[i][j].b = 0;
		}
	}

	 
	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			for (int k = 0; k < 40; k++)
			{
				for (int l = 0; l < 40; l++)
				{

					initialColormatrix[i][j].r += (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).r / (40 * 40 * 255);
					initialColormatrix[i][j].g += (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).g / (40 * 40 * 255);
					initialColormatrix[i][j].b += (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).b / (40 * 40 * 255);
				
				}
			}
		}
	}


}




void NetzeUndRauschen::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case '+':
			if (perlinShiftSpeed < 8)
			{
				perlinShiftSpeed += .25;
			}
		break;
		case '-':
			if (perlinShiftSpeed > .25)
			{
				perlinShiftSpeed -= .25;
			}
		break;
		case 'w':
		{
			if (perlinEffectStrength < 1.0)
			{
				perlinEffectStrength += .1;
			}
		}
		break;
		case 's':
			if (perlinEffectStrength > 0.2)
			{
				perlinEffectStrength -= .1;
			}
		break;
		case 'e':
			if (saturationAddend < .15)
			{
				saturationAddend += .05;
			}
			break;
		case 'd':
			if (saturationAddend > -.5)
			{
				saturationAddend -= .05;
			}
			break;

		case 'a':
			setInitialColormatrix();
			break;

		case ' ':
			mapCameraToNoiseNet = !mapCameraToNoiseNet;
			break;
	}
}

std::chrono::high_resolution_clock::time_point timePrev;

void NetzeUndRauschen::draw()
{
	ColorRGB colormatrix[netSize][netSize];
		 
	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			colormatrix[i][j].r = 0;
			colormatrix[i][j].g = 0;
			colormatrix[i][j].b = 0;
		}
	}

	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			for (int k = 0; k < 40; k++)
			{
				for (int l = 0; l < 40; l++)
				{
					colormatrix[i][j].r +=  (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).r / (40 * 40 * 255);
					colormatrix[i][j].g += (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).g / (40 * 40 * 255);
					colormatrix[i][j].b += (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).b / (40 * 40 * 255);
					CI_LOG_D((float)mCapture->getSurface()->getPixel(vec2(i * 12 + k, j * 12 + l)).r);
				}
			}
		}
	}

	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			colorDistanceMatrix[i][j] = 3 * sqrt(pow(abs(colormatrix[i][j].r - initialColormatrix[i][j].r), 2) + pow(abs(colormatrix[i][j].g - initialColormatrix[i][j].g), 2) + pow(abs(colormatrix[i][j].b - initialColormatrix[i][j].b), 2)) / sqrt(3);
		}
	}

    gl::clear();

	gl::setMatrices( mCam );
	

	std::chrono::high_resolution_clock::time_point timeNow = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> timeSpan = timeNow - startTime;

	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			pointMatrix[i][j].x = i + perlinEffectStrength * colorDistanceMatrix[netSize - 1 - i][netSize - 1 - j] * perlinNoise.noise((timeSpan.count() * perlinShiftSpeed / 1000 + i), (timeSpan.count() * perlinShiftSpeed / 1000 + j)) -.5;
			pointMatrix[i][j].y = j + perlinEffectStrength * colorDistanceMatrix[netSize - 1 - i][netSize - 1 - j] * perlinNoise.noise((timeSpan.count() * perlinShiftSpeed / 1000 + i) + .5, (timeSpan.count() * perlinShiftSpeed / 1000 + j) + .5) - .5;
	
		}
	}

	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			cinder::Color cameraAverageValue(colormatrix[netSize - 1 - i][netSize - 1 - j].r, colormatrix[netSize - 1 - i][netSize - 1 - j].g, colormatrix[netSize - 1 - i][netSize - 1 - j].b);
			cinder::Color hsvColor(CM_HSV, cameraAverageValue);
			
			//gl::color(cinder::Color(cinder::ColorModel::CM_HSV, colorsHSV[i][j].h, colorsHSV[i][j].s + saturationAddend, colorsHSV[i][j].v));
			
			//if (colorDistanceMatrix[i][j] > .9)
			//{
				//gl::color(cinder::Color(cinder::ColorModel::CM_HSV, hsvColor.get(CM_HSV).x, 1, 1));
			//}
			if (mapCameraToNoiseNet)
			{
				gl::color(cinder::Color(cinder::ColorModel::CM_RGB, vec3(colormatrix[netSize - 1 - i][netSize - 1 - j].r, colormatrix[netSize - 1 - i][netSize - 1 - j].g, colormatrix[netSize - 1 - i][netSize - 1 - j].b)));
			}
			else
			{
				gl::color(cinder::Color(cinder::ColorModel::CM_HSV, colorsHSV[i][j].h, colorsHSV[i][j].s + hsvColor.get(CM_HSV).x * colorDistanceMatrix[netSize - 1 - i][netSize - 1 - j] + saturationAddend, colorsHSV[i][j].v));
			}
			if ((i + 1 < netSize) && (j + 1 < netSize))
			gl::drawSolidTriangle(pointMatrix[i][j], pointMatrix[i][j + 1], pointMatrix[i + 1][j]);


			if ((i - 1 >= 0) && (j - 1 >= 0))
			gl::drawSolidTriangle(pointMatrix[i][j], pointMatrix[i][j - 1], pointMatrix[i - 1][j]);
		}
	}
}


CINDER_APP( NetzeUndRauschen, RendererGl( RendererGl::Options().msaa(4)) )
