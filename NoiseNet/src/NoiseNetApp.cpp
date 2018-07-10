/*
########## NoiseNet ##########
Project for MG TUD SoSe2018
Author: Philip Ulbrich, Vincent Schmidt, Leon Brandt, Philipp Czyborra, Robert Fuhrmann

camera input taken from cinder sample 'CaptureBasic'
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "ColorMapper.h"
#include "PerlinImpl.h"
#include <cmath>
#include "cinder/CameraUi.h"
#include "cinder/Timeline.h"

using namespace ci;
using namespace ci::app;
using namespace std;

ColorMapper colorMapper;
PerlinImpl perlinImpl;

#if defined( CINDER_ANDROID )
#define USE_HW_TEXTURE
#endif    

class NoiseNetApp : public App {
public:
	void setup() override;
	void update() override;
	void keyDown(KeyEvent event) override;
	void draw() override;

private:
	void printDevices();

	CaptureRef			mCapture;
	gl::TextureRef		mTexture;
	Surface				referenceSurface;	// reference surface without any motion
	clock_t				cTime = 0;			// time of last referenceSurface taken
	int					changeTime = 1000;			// time between color change 

	// perlin
	int					netSize = perlinImpl.getNetSize();
	CameraPersp			mCam;
	CameraUi			mCamUi;
	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
};

void NoiseNetApp::setup()
{
	printDevices();

	try {
		mCapture = Capture::create(640, 480);
		//mCapture = Capture::create(1280, 1024);
		mCapture->start();
	}
	catch (ci::Exception &exc) {
		CI_LOG_EXCEPTION("Failed to init capture ", exc);
	}

	//PERLIN

	gl::enableAlphaBlending();
	gl::enableDepthRead();
	gl::enableDepthWrite();

	mCam = ci::CameraPersp();
	mCam.setEyePoint(vec3(0, 0, 10));
	mCam.lookAt(vec3(0, 0, 0));
	mCamUi = CameraUi(&mCam, getWindow());
	mCam.setNearClip(1);
	mCam.setFarClip(2000);
	
	
	

	srand(time(NULL));

}

void NoiseNetApp::update()
{

#if defined( USE_HW_TEXTURE )
	if (mCapture && mCapture->checkNewFrame()) {
		mTexture = mCapture->getTexture();
	}
#else
	if (mCapture && mCapture->checkNewFrame()) {
		if (!mTexture) {
			// Capture images come back as top-down, and it's more efficient to keep them that way
			mTexture = gl::Texture::create(*mCapture->getSurface(), gl::Texture::Format().loadTopDown());
		}
		else {
			if (!referenceSurface.getData()) {
				referenceSurface = *mCapture->getSurface();
			}
			// save the next img 
			mTexture->update(*mCapture->getSurface());

			// save new referenceSurface every x milliseconds
			// this enbables motion detection instead of object detection (!)
			if (referenceSurface.getData() && clock() - cTime > changeTime) {
				referenceSurface = *mCapture->getSurface();
				cTime = clock();
				for (int i = 0; i < netSize; i++)
				{
					for (int j = 0; j < netSize; j++)
					{
						perlinImpl.setAnimColor(i, j, perlinImpl.getColorHSV(i, j), changeTime);
					}
				}
			}

			Surface tmpSurface = *mCapture->getSurface();

			/* manipulation start here */
			Area area(0, 0, 640, 480);
			Surface::Iter iter = tmpSurface.getIter(area);

			// compute average color once
			// fetch it several times later
			//colorMapper.computeAvgMotionColor(&referenceSurface, iter); // sets black if no motion
			colorMapper.computeAvgColor(&referenceSurface, area);
			//app::console() << colorMapper.getAvgR() << "-" << colorMapper.getAvgG() << "-" << colorMapper.getAvgB() << endl;


			//// show background substraction output
			//// set average color for each pixel
			//// reset iterator for setting image values
			//iter = tmpSurface.getIter(area);
			//while (iter.line()) {
			//	while (iter.pixel()) {
			//		
			//		// get color values from referenceSurface
			//		uint8_t* r = referenceSurface.getDataRed(iter.getPos());
			//		uint8_t* g = referenceSurface.getDataGreen(iter.getPos());
			//		uint8_t* b = referenceSurface.getDataBlue(iter.getPos());
			//		
			//		// set color for motion values
			//		if (abs(iter.r() - *r) > 40 && abs(iter.g() - *g) > 40 && abs(iter.b() - *b) > 40) {
			//			iter.r() = colorMapper.getAvgR();
			//			iter.g() = colorMapper.getAvgG();
			//			iter.b() = colorMapper.getAvgB();
			//		// set non motion values to black (for now)
			//		} else {
			//			iter.r() = 0;
			//			iter.g() = 0;
			//			iter.b() = 0;
			//		}
			//	}
			//}

			//PERLINIMPL
			vec3 color = { colorMapper.getAvgR(), colorMapper.getAvgG(), colorMapper.getAvgB()};
			//app::console() << "0 " << color << endl;
			//app::console() << "0 " << (color / vec3(255, 255, 255)) << endl;
			Color col = Color(CM_RGB, (color / vec3(255,255,255)));

			// set color for each item in net
			// only set Hue, keep old Saturation and Value
			for (int i = 0; i < netSize; i++)
			{
				for (int j = 0; j < netSize; j++)
				{

					//// START item wise color calculation
					//// uncomment to enable
					//int cs = 640 / netSize;
					//int rs = 480 / netSize;
					//Area area(i*cs, j*rs, (i+1)*cs, (j+1)*rs);
					//colorMapper.computeAvgColor(&referenceSurface, area);
					//vec3 color = { colorMapper.getAvgR(), colorMapper.getAvgG(), colorMapper.getAvgB() };
					//Color col = Color(CM_RGB, (color / vec3(255, 255, 255)));
					//vec3 hsv = colorMapper.RGBtoHSV(col);
					//// END item wise color calculation
					perlinImpl.setColorHSV(i, j, col);
					
				}
			}
			
			
			/* manipulation end here */

			//tmpSurface = tmpSurface.getDataBlue - surface.getDataBlue;
			mTexture->update(tmpSurface);

		}
	}
#endif

}

void NoiseNetApp::keyDown(KeyEvent event)
{
	switch (event.getChar()) {
	case '+':
		if (perlinImpl.getPerlinShiftSpeed() < 8)
		{
			float newSpeed = perlinImpl.getPerlinShiftSpeed();
			perlinImpl.setPerlinShiftSpeed(newSpeed += .25);
		}
		break;
	case '-':
		if (perlinImpl.getPerlinShiftSpeed() > .25)
		{
			float newSpeed = perlinImpl.getPerlinShiftSpeed();
			perlinImpl.setPerlinShiftSpeed(newSpeed -= .25);
		}
		break;
	case 'w':
	{
		if (perlinImpl.getPerlinEffectStrength() < 1.0)
		{
			float newStrength = perlinImpl.getPerlinEffectStrength();
			perlinImpl.setPerlinEffectStrength(newStrength += .1);
		}
	}
	break;
	case 's':
		if (perlinImpl.getPerlinEffectStrength() > 0.2)
		{
			float newStrength = perlinImpl.getPerlinEffectStrength();
			perlinImpl.setPerlinEffectStrength(newStrength -= .1);
		}
		break;
	case 'e':
		if (perlinImpl.getSaturationAdded() < .15)
		{
			float newSat = perlinImpl.getSaturationAdded();
			perlinImpl.setSaturationAdded(newSat += .05);
		}
		break;
	case 'd':
		if (perlinImpl.getSaturationAdded() > -.5)
		{
			float newSat = perlinImpl.getSaturationAdded();
			perlinImpl.setSaturationAdded(newSat -= .05);
		}
		break;
	}
}

void NoiseNetApp::draw()
{

//	gl::clear();
//
//	if (mTexture) {
//		gl::ScopedModelMatrix modelScope;
//#if defined( CINDER_COCOA_TOUCH ) || defined( CINDER_ANDROID )
//		// change iphone to landscape orientation
//		gl::rotate(M_PI / 2);
//		gl::translate(0, -getWindowWidth());
//
//		Rectf flippedBounds(0, 0, getWindowHeight(), getWindowWidth());
//#if defined( CINDER_ANDROID )
//		std::swap(flippedBounds.y1, flippedBounds.y2);
//#endif
//		gl::draw(mTexture, flippedBounds);
//#else
//		gl::draw(mTexture);
//#endif
//	}


	//PERLINIMPL
	gl::clear();

	gl::setMatrices(mCam);

	gl::drawCoordinateFrame();

	gl::translate(vec3(-netSize*0.4f, -netSize*0.5f, 0.0f));

	std::chrono::high_resolution_clock::time_point timeNow = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> timeSpan = timeNow - startTime;

	float perlinEffectStrength = perlinImpl.getPerlinEffectStrength();
	float perlinShiftSpeed = perlinImpl.getPerlinShiftSpeed();
	float saturationAdded = perlinImpl.getSaturationAdded();
	//vec2 *pointMatrix = perlinImpl.getPointMatrix();
	Perlin perlinNoise = perlinImpl.getPerlinNoise();


	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			double x = i + perlinEffectStrength * perlinNoise.noise((timeSpan.count() * perlinShiftSpeed / 1000 + i), (timeSpan.count() * perlinShiftSpeed / 1000 + j)) - .5;
			double y = j + perlinEffectStrength * perlinNoise.noise((timeSpan.count() * perlinShiftSpeed / 1000 + i) + .5, (timeSpan.count() * perlinShiftSpeed / 1000 + j) + .5) - .5;
			perlinImpl.setPointMatrix(i, j, vec2(x, y));
			//pointMatrix[i][j].x = i + perlinEffectStrength * perlinNoise.noise((timeSpan.count() * perlinShiftSpeed / 1000 + i), (timeSpan.count() * perlinShiftSpeed / 1000 + j)) - .5;
			//pointMatrix[i][j].y = j + perlinEffectStrength * perlinNoise.noise((timeSpan.count() * perlinShiftSpeed / 1000 + i) + .5, (timeSpan.count() * perlinShiftSpeed / 1000 + j) + .5) - .5;

		}
	}

	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			vec3 newCol = perlinImpl.getAnimColor(i, j).value().get(CM_HSV);
			newCol.y += saturationAdded;

			gl::color(cinder::Color(CM_HSV, newCol));
			//gl::color(cinder::Color(perlinImpl.getAnimColor(i, j)));


			if ((i + 1 < netSize) && (j + 1 < netSize))
				gl::drawSolidTriangle(perlinImpl.getPointMatrix(i,j), perlinImpl.getPointMatrix(i, j+1), perlinImpl.getPointMatrix(i+1, j));


			if ((i - 1 >= 0) && (j - 1 >= 0))
				gl::drawSolidTriangle(perlinImpl.getPointMatrix(i, j), perlinImpl.getPointMatrix(i, j-1), perlinImpl.getPointMatrix(i-1, j));
		}
	}
}

void NoiseNetApp::printDevices()
{
	for (const auto &device : Capture::getDevices()) {
		console() << "Device: " << device->getName() << " "
#if defined( CINDER_COCOA_TOUCH ) || defined( CINDER_ANDROID )
			<< (device->isFrontFacing() ? "Front" : "Rear") << "-facing"
#endif
			<< endl;
	}
}

void prepareSettings(NoiseNetApp::Settings* settings)
{
#if defined( CINDER_ANDROID )
	settings->setKeepScreenOn(true);
#endif
}

CINDER_APP(NoiseNetApp, RendererGl, prepareSettings)
