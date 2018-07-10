#pragma once
#include "cinder/app/App.h"
#include <Math.h>
#include <math.h>
#include "cinder/Perlin.h"
#include <stdlib.h>     
#include <time.h>       
#include <random>
#include "cinder/Perlin.h"
#include "cinder/Timeline.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PerlinImpl
{
public:
	PerlinImpl();
	~PerlinImpl();

	float randomBetween(float smallNumber, float bigNumber);

	int getNetSize();
	Color getColorsHSV();
	Color getColorHSV(int x, int y);
	void setColorHSV(int x, int y, Color color);

	vec2 getPointMatrix(int x, int y);
	void setPointMatrix(int x, int y, vec2 val);

	float getPerlinEffectStrength();
	void setPerlinEffectStrength(float strength);

	float getPerlinShiftSpeed();
	void setPerlinShiftSpeed(float speed);
	
	float getSaturationAdded();
	void setSaturationAdded(float saturation);
	
	Perlin getPerlinNoise();

	Anim<Color> getAnimColor(int x, int y);
	void setAnimColor(int x, int y, Color c, int t);

private:

	static const int netSize = 12;
	Color colorsHSV[netSize][netSize];
	vec2 pointMatrix[netSize][netSize];
	vector<vector<Anim<Color>>> animColors;

	// strength of effect
	float perlinEffectStrength = 0.5;

	//speed of moving perlin
	float perlinShiftSpeed = 0.5;

	// saturation manipulator
	float saturationAdded = 0;

	

	Perlin perlinNoise;



};