#include "PerlinImpl.h"

PerlinImpl::PerlinImpl() {
	for (int i = 0; i < netSize; i++)
	{
		vector<Anim<Color>> v;
		for (int j = 0; j < netSize; j++)
		{
			pointMatrix[i][j].x = i;
			pointMatrix[i][j].y = j;
			Anim<Color> c;
			c.value().set(CM_HSV, vec3(242.0 / 360.0, randomBetween(.5, .85), randomBetween(.8, 1.0)));
			v.push_back(c);
			colorsHSV[i][j].set(CM_HSV,  c.value().get(CM_HSV));
		}
		animColors.push_back(v);
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

void PerlinImpl::setColorHSV(int x, int y, Color color) {
	vec3 newCol = rgbToHsv(color);
	newCol.y = colorsHSV[x][y].get(CM_HSV).y;
	newCol.z = colorsHSV[x][y].get(CM_HSV).z;

	colorsHSV[x][y].set(CM_HSV, newCol);
	//animColors[x][y].value().set(CM_HSV, newCol);
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

Anim<Color> PerlinImpl::getAnimColor(int x, int y)
{
	return animColors[x][y];
}

void PerlinImpl::setAnimColor(int x, int y, Color c, int t)
{
	float ct = (float)t / (float)1000;
	Color newCol = Color(CM_HSV, vec3(c.get(CM_HSV).x, colorsHSV[x][y].get(CM_HSV).y, colorsHSV[x][y].get(CM_HSV).z));
	//animColors[x][y] = newCol;
	timeline().apply(&animColors[x][y], newCol, ct, EaseInCubic());
}
