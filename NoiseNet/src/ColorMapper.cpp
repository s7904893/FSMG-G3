#include "ColorMapper.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//Perlin perlin;

ColorMapper::ColorMapper()
{
}


ColorMapper::~ColorMapper()
{
}


int ColorMapper::getR(ivec2 pos) {
	return 0;
}
int ColorMapper::getG(ivec2 pos) {
	return 0;
}
int ColorMapper::getB(ivec2 pos) {
	//app::console() << " ### " << perlin.perlin(pos.x, pos.y, 0) << endl;
	//int b = floor(perlin.perlin(pos.x, pos.y, 0) * 256);
	//app::console() << b << endl;
	
	return 0;
}

int ColorMapper::getAvgR() {
	return avgR;
}
int ColorMapper::getAvgG() {
	return avgG;
}
int ColorMapper::getAvgB() {
	return avgB;
}

/* compute the average color if a whole image */
void ColorMapper::computeAvgColor(Surface *surface, Area area) {
	int r = 0, g = 0, b = 0;
	avgR = surface->areaAverage(area).r;
	avgG = surface->areaAverage(area).g;
	avgB = surface->areaAverage(area).b;
	//if (avgR > avgG && avgR > avgB) {
	//	avgR = 255;
	//	avgG = 0;
	//	avgB = 0;
	//}
	//else if (avgG > avgB) {
	//	avgR = 0;
	//	avgG = 255;
	//	avgB = 0;
	//}
	//else {
	//	avgR = 0; 
	//	avgG = 0;
	//	avgB = 255;
	//}
}

/* compute average color of all motion pixels in an image */
void ColorMapper::computeAvgMotionColor(Surface *referenceSurface, Surface::Iter iter) {
	//Area area(0, 0, 640, 480);
	//Surface::Iter iter = tmpSurface.getIter(area);
	int count = 0;

	while (iter.line()) {
		while (iter.pixel()) {
			// get color values from referenceSurface
			uint8_t* r = referenceSurface->getDataRed(iter.getPos());
			uint8_t* g = referenceSurface->getDataGreen(iter.getPos());
			uint8_t* b = referenceSurface->getDataBlue(iter.getPos());

			// mark pixel as motion if color values drift over threshold
			//if (abs(iter.r() - *r) > 40 && abs(iter.g() - *g) > 40 && abs(iter.b() - *b) > 40) {
			//	iter.r() = colorMapper.getR(iter.getPos());
			//	iter.g() = colorMapper.getG(iter.getPos());
			//	iter.b() = colorMapper.getB(iter.getPos());
			//}
			//if (abs(iter.r() - *r) > 40 && abs(iter.g() - *g) > 40 && abs(iter.b() - *b) > 40) {
			//	iter.r() = colorMapper.getAvgR();
			//	iter.g() = colorMapper.getAvgG();
			//	iter.b() = colorMapper.getAvgB();

			//}
			//app::console() << iter.r() << " - " << iter.g() << " - " << iter.b() << endl;
			//app::console() << iter.r()<<" - "<<*r << endl;

			if (abs(iter.r() - *r) > 40 && abs(iter.g() - *g) > 40 && abs(iter.b() - *b) > 40) {
				if (count == 0) {
					avgR = iter.r();
					avgG = iter.g();
					avgB = iter.b();
				}
				else {
					avgR += iter.r();
					avgG += iter.g();
					avgB += iter.b();
				}
				count++;
			}
		}
	}

	// get average color value
	if (count > 0) {
		avgR = abs(avgR / count);
		avgG = abs(avgG / count);
		avgB = abs(avgB / count);
		count = 0;
	}

	// just keep dominant color
	if (avgR > avgG && avgR > avgB) {
		avgG = 0;
		avgB = 0;
	}
	else if (avgG > avgB) {
		avgR = 0;
		avgB = 0;
	}
	else {
		avgR = 0;
		avgG = 0;
	}
}