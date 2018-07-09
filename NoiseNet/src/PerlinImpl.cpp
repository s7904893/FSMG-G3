#include "PerlinImpl.h"

PerlinImpl::PerlinImpl() {
	for (int i = 0; i < netSize; i++)
	{
		for (int j = 0; j < netSize; j++)
		{
			pointMatrix[i][j].x = i;
			pointMatrix[i][j].y = j;

			colorsHSV[i][j].set(CM_HSV,  vec3(242.0 / 360.0, randomBetween(.5, .85), randomBetween(.8, 1.0)));
		}
	}
}

PerlinImpl::~PerlinImpl(){}

float PerlinImpl::randomBetween(float smallNumber, float bigNumber)
{
	float diff = bigNumber - smallNumber;
	return (((float)rand() / RAND_MAX) * diff) + smallNumber;
}

int PerlinImpl::getNetSize() {
	return netSize;
}

//PerlinImpl::ColorHSV PerlinImpl::getColorsHSV() {
//	return colorsHSV;
//}
void PerlinImpl::setColorHSV(int x, int y, Color color) {
	//colorsHSV[x][y] = color;
	vec3 newCol = rgbToHsv(color);
	newCol.y = colorsHSV[x][y].get(CM_HSV).y;
	newCol.z = colorsHSV[x][y].get(CM_HSV).z;
	//newCol.y = randomBetween(.5, .85);
	//newCol.z = randomBetween(.8, 1.0);
	
	/*Color c;
	c.set(CM_HSV, newCol);
	Anim<Color> oldColor;
	oldColor = colorsHSV[x][y];
	timeline().apply(&oldColor, c, 1.0f, EaseInCubic()); */
	colorsHSV[x][y].set(CM_HSV, newCol);
}
Color PerlinImpl::getColorHSV(int x, int y) {
	return colorsHSV[x][y];
}

vec2 PerlinImpl::getPointMatrix(int x, int y) {
	return pointMatrix[x][y];
}

void PerlinImpl::setPointMatrix(int x, int y, vec2 val) {
	pointMatrix[x][y] = val;
}

void PerlinImpl::setPerlinEffectStrength(float strength) {
	perlinEffectStrength = strength;
}
float PerlinImpl::getPerlinEffectStrength() {
	return perlinEffectStrength;
}

void PerlinImpl::setPerlinShiftSpeed(float speed) {
	perlinShiftSpeed = speed;
}
float PerlinImpl::getPerlinShiftSpeed() {
	return perlinShiftSpeed;
}

void PerlinImpl::setSaturationAdded(float saturation) {
	saturationAdded = saturation;
}
float PerlinImpl::getSaturationAdded() {
	return saturationAdded;
}

Perlin PerlinImpl::getPerlinNoise() {
	return perlinNoise;
}