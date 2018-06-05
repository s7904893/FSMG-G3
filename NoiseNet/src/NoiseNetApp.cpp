#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class NoiseNetApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void NoiseNetApp::setup()
{
}

void NoiseNetApp::mouseDown( MouseEvent event )
{
}

void NoiseNetApp::update()
{
}

void NoiseNetApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( NoiseNetApp, RendererGl )
