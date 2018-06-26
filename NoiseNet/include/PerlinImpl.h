#pragma once
#include "cinder/app/App.h"
#include <Math.h>
#include <math.h>
#include "cinder/Perlin.h"
#include <stdlib.h>     
#include <time.h>       
#include <random>
#include "cinder/Perlin.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PerlinImpl
{
public:
	PerlinImpl();
	~PerlinImpl();
		
	struct ColorHSV
	{
		double h = 0;
		double s = 0;
		double v = 0;
	};

	float randomBetween(float smallNumber, float bigNumber);

	int getNetSize();
	ColorHSV getColorsHSV();
	ColorHSV getColorHSV(int x, int y);

	vec2 getPointMatrix(int x, int y);
	void setPointMatrix(int x, int y, vec2 val);

	float getPerlinEffectStrength();
	void setPerlinEffectStrength(float strength);
	float getPerlinShiftSpeed();
	void setPerlinShiftSpeed(float speed);
	float getSaturationAdded();
	void setSaturationAdded(float saturation);
	Perlin getPerlinNoise();

private:



	static const int netSize = 12;
	ColorHSV colorsHSV[netSize][netSize];
	vec2 pointMatrix[netSize][netSize];

	// strength of effect
	float perlinEffectStrength = 0.5;

	//speed of moving perlin
	float perlinShiftSpeed = 1.0;

	// saturation manipulator
	float saturationAdded = 0;

	

	Perlin perlinNoise;



};