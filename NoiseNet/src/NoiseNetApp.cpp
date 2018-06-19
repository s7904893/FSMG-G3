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
#include <cmath>

using namespace ci;
using namespace ci::app;
using namespace std;

#if defined( CINDER_ANDROID )
#define USE_HW_TEXTURE
#endif    

class NoiseNetApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;

private:
	void printDevices();

	CaptureRef			mCapture;
	gl::TextureRef		mTexture;
	Surface				referenceSurface;	// reference surface without any motion
	clock_t				cTime = 0;			// time of last referenceSurface taken
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
			if (referenceSurface.getData() && clock() - cTime > 500) {
				referenceSurface = *mCapture->getSurface();
				cTime = clock();
			}

			Surface tmpSurface = *mCapture->getSurface();

			/* manipulation start here */
			Area area(0, 0, 640, 480);
			Surface::Iter iter = tmpSurface.getIter(area);
			while (iter.line()) {
				while (iter.pixel()) {
					// get color values from referenceSurface
					uint8_t* r = referenceSurface.getDataRed(iter.getPos());
					uint8_t* g = referenceSurface.getDataGreen(iter.getPos());
					uint8_t* b = referenceSurface.getDataBlue(iter.getPos());

					// mark pixel as motion if color values drift over threshold
					if (abs(iter.r() - *r) > 40 && abs(iter.g() - *g) > 40 && abs(iter.b() - *b) > 40) {
						iter.r() = 0;
						iter.g() = 0;
						iter.b() = 0;
					}
				}
			}
			/* manipulation end here */

			//tmpSurface = tmpSurface.getDataBlue - surface.getDataBlue;
			mTexture->update(tmpSurface);

		}
	}
#endif

}

void NoiseNetApp::draw()
{

	gl::clear();

	if (mTexture) {
		gl::ScopedModelMatrix modelScope;
#if defined( CINDER_COCOA_TOUCH ) || defined( CINDER_ANDROID )
		// change iphone to landscape orientation
		gl::rotate(M_PI / 2);
		gl::translate(0, -getWindowWidth());

		Rectf flippedBounds(0, 0, getWindowHeight(), getWindowWidth());
#if defined( CINDER_ANDROID )
		std::swap(flippedBounds.y1, flippedBounds.y2);
#endif
		gl::draw(mTexture, flippedBounds);
#else
		gl::draw(mTexture);
#endif
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
