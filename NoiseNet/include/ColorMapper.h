#pragma once

#include "cinder/app/App.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class ColorMapper
{
public:
	ColorMapper();
	~ColorMapper();

	int getR(ivec2 pos);
	int getG(ivec2 pos);
	int getB(ivec2 pos);

	int getAvgR();
	int getAvgG();
	int getAvgB();

	void computeAvgColor(Surface *surface, Area area);
	void computeAvgMotionColor(Surface *referenceSurface, Surface::Iter iter);

private:
	int avgR = -1;
	int avgG = -1;
	int avgB = -1;
};

