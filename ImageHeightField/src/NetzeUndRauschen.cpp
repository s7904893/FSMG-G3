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
#include <random>
#include <math.h>
#include <chrono>
#include "cinder/Capture.h"
#include "cinder/params/Params.h"

/**
 *Variable, die definiert, ob Protoyp-exkklusive Inhalte (GUI-Overlay) gezeigt werden sollen.
 */
#define PROTOTYPE

using namespace ci;
using namespace ci::app;


/**
 *Erzeugt ein Netz aus Dreiecken verschiedener Farbe auf dem Bildschirm. Je nach Aktivitaet vor der Kamera kann der Nutzer das Verhalten des Netzes aendern. Zuerst wird mit (a) der Hintergrund getraeckt.
 *Dann wird, wenn Veraenderungen im Bild stattfinden, die Geschwindigkeit der Bewegung der Knoten des Netzes erhoeht und die Farbigkeit der Dreiecke in der jeweiligen Bildregion verstaerkt. Zusaetzlich veraendert sich die Grundfarbe des Netzes staendig, aber langsam, sodass der Nutzer entweder lange warten muss, oder die Moeglichkeit hat, nach einiger Zeit ein anderes Bild wahrzunehmen. Der Farbzyklus dauert 6 Minuten.
 */
class NetzeUndRauschen : public App {
 public:
	void    setup() override;
	void    keyDown( KeyEvent event ) override;
	void    draw() override;

	/**
	 *Parameter, die über das Cinder-Interface direkt beeinflusst werden koennen.
	 */
	params::InterfaceGl	mParams;

 private:

	 /**
	  *Kamera, die das Netz im 3D-Raum einfaengt.
	  */
	CameraPersp		mCam;

	/**
	 *Darstellung des Kamerainhaltes auf dem Bildschirm
	 */
	CameraUi		mCamUi;

	void	setInitialColormatrix();
	void	createRandomColorMap();
	void	adaptColor();

	void	adaptCameraEyePoint();

	void	setCameraCaptureColorMatrix();
	void	setColorDistanceMatrix();
	void	applyManipulationsToVertices();
	void	applyColorsAndDrawTriangles();

	void setNumberOfColors(short);
	void toggleSingleColor();

	/**
	 *Referenziert das pixelweise Abbild des Kamerainhaltes der realen Kamera.
	 */
	CaptureRef			mCapture;

	/**
	 *Legt fest, ob fuer die Projektion auf das Netz die Bereiche des Kamerabildes verwendet werden sollen.
	 */
	bool mapCameraToNoiseNet = false;
};

/**
 *Wird fuer zeitbasierte Inhalte verwendet, indem die vergangenge Zeit gemessen wird.
 */
struct TimeElapsed
{
	TimeElapsed(int speed = 1)
	{
		this->speed = speed;
		time1 = std::chrono::high_resolution_clock::now();
	}

	/**
	 *Legt die Geschwindigkeit (Faktor fest).
	 *@param speed Faktor, mit dem vergangene Zeit aufaddiert wird.
	 */
	void setSpeed(float speed)
	{
		this->speed = speed;
	}

	/**
	 *Gibt an, wie viel Zeit insgesamt verstrichen ist.
	 *@return Vergangene Zeit seit erstem Aufruf. Groesser, wenn der Geschwindigkeitsfaktor groeßer ist.
	 */
	unsigned long elapse()
	{
	time2 = std::chrono::high_resolution_clock::now();
	elapsed = time2 - time1;
	totalElapsed += elapsed.count() * this->speed; 
	time1 = std::chrono::high_resolution_clock::now();
	return totalElapsed;
	}

	std::chrono::high_resolution_clock::time_point time1;
	std::chrono::high_resolution_clock::time_point time2;
	std::chrono::duration<double, std::milli> elapsed;

	unsigned long totalElapsed = 0;
	float speed;
};

/**
 *Zeitmesser fuer Perlin-Geschwindigkeit.
 */
TimeElapsed perlinSpeed;

/**
 *Zeitmesser fuer Aenderung der Farbe(n) des Netzes.
 */
TimeElapsed colorChangeSpeed;

/**
 *Enthaelt Farbtn, Saettigung und Farbwert als double.
 */
struct ColorHSV
{
	double h = 0;
	double s = 0;
	double v = 0;
	short hueNumber = 0;
};

/**
 *Enthaelt Rot-, Gruen- und Blauwert als double.
 */
struct ColorRGB
{
	double r = 0;
	double g = 0;
	double b = 0;
};

/**
 *Berechnet eine zufaellige Zahl im gegebenen Bereich.
 *@param smallNumber Startwert. (minimal)
 *@param bigNumber Endwert. (maximal)
 *@return Zufallszahl als Gleitkommazahl.
 */
float randomBetween(float smallNumber, float bigNumber)
{
	float diff = bigNumber - smallNumber;
	return (((float)rand() / RAND_MAX) * diff) + smallNumber;
}

/**
 *Groeße des Netzes in x-Richtung. Anzahl der Bereiche.
 */
const int netSizeX = 16;


/**
 *Groeße des Netzes in y-Richtung. Anzahl der Bereiche.
 */
const int netSizeY = 12;

/**
 *Enthaelt die Farbwerte des Kamerabildes, gemittelt fuer jeden Bereich.
 */
ColorRGB cameraCaptureColorMatrix[netSizeX][netSizeY];

/**
 *Enthaelt die Standardfarben des Netzes (Saettigung und Farbwert) fuer jeden Bereich.
 */
ColorHSV colorsHSV[netSizeX][netSizeY];



/**
 *Matrix, die die gemittelten Hintergrundfarben fuer jeden Bereich des Bildes enthaelt.
 */
ColorRGB backgroundColorMatrix[netSizeX][netSizeY];

/**
 *Matrix, in der fuer jeden Bereich (bestehend aus zwei Dreiecken) die euklidische Distanz im RGB-Raum zwischen Hintergrundbild und aktuellem Kamerabild gespeichert wird.
 */
float colorDistanceMatrix[netSizeX][netSizeY];

/**
 *Matrix der Standard-Vertexpositionen. Werden initialisiert und nie veraendert. Werden ausgelesen und mit Manipulatoren an gl::drawTriangle() uebergeben, um Bewegung zu erzeugen.
 */
vec2 pointPositionsMatrix[netSizeX][netSizeY];

/**
 *Definiert die Staerke des Perlineffektes. Faktor, der mit der Perlin-Auslenkung multipliziert wird. Ist bspw. der Rauschwert an einer Stelle 1 und perlinEffectStrength auch 1, so wird das Dreieck um 1 ausgelenkt. Um Ueberschneidungen von Kanten sicher zu verhindern, darf der Wert nicht 0.5 ueberschreiten.
 */
float perlinEffectStrength = 0.5;

/**
 *Geschwindigkeit, mit der das Perlin-Rauschen ueber das Netz verschoben wird. Bei 1 wird es pro Sekunde ein mal ueber das gesamte Netz verschoben.
 */
float perlinShiftSpeed = 1.0;

/**
 *Addend fuer die Saettigung. Wird auf alle Flaechen aufaddiert, sofern Farben der Palette benutzt werden und nicht das Kamerabild. Kann zur Nachkorrektur sinnvoll sein, falls das Anzeigegeraet die Saettigung nicht wie gewuenscht widergibt.
 */
float saturationAddend = 0;

/**
 *Startwert für den Farbton des Netzes. Blauer Farbton. Kann mittels Cinder-GUI manipuliert werden.
 */
float defaultHue = 242.0;

/**
 *Gibt an, in welchem Verhaeltnis weiß-graue und farbige Flaechen randomisiert erzeugt werden, wenn der singleColorMode aktiviert ist.
 */
float singleColorRatio = 0.2;


/**
 *Perlin-Rauschen, das über das Netz als Manipulator verschoben wird. Ist für beide Achsen identisch, jedoch um 0.5 verschoben, um nicht symmetrisch zu wirken.
 */
Perlin perlinNoise;

/**
 *Legt fest, ob, falls nur eine Farbe benutzt wird, das Netz aufgeteilt wird in Flaechen der gegebenen Farbe und Flaechen mit einem weiß-grauen Farbton.
*/
bool flagSingleColor = false;

/**
 *Anzahl an verschiedenen Farbwerten, derer sich bedient wird, für die Einfärbung des Netzes. Genaue Farbwerte sind von der initialen Position defaultHue abhängig. Die Farbwerte werden auf dem Farbkreis gleichverteilt ausgewählt.
 */
int numberOfHues = 1;

/**
 *Wert, der mittels Cinder-GUI manipuliert werden kann. Um nicht permanent das Netz zu verändern, muss er manuell angewendet werden.
 */
int numberOfHuesNew = numberOfHues;

/**
 *Schaltet den Single-Color-Mode um. Wird dieser verwendet, so wird, falls nur eine Farbe ausgewaehlt ist, ein gewisser Teil des Netzes weiss bis grau gezeichnet. Die Aufteilung wird dabei aus singleColorRatio gezogen.
 */
void NetzeUndRauschen::toggleSingleColor()
{
	flagSingleColor = !flagSingleColor;
	createRandomColorMap();
}

/**
 *Gibt an, wie viele Farben (mit maximalem Farbwert-Kontrast) verwendet werden sollen.
 *@param number Anzahl der Farben.
 */
void NetzeUndRauschen::setNumberOfColors(short number)
{
	numberOfHues = number;
	createRandomColorMap();
}


/**
 *Erzeugt die Farbwerte als (sizeX * sizeY)-Matrix fuer die Dreiecke aus einem bestimmten Bereich von moeglichen Saturation- und Value-Werten. Hue (Farbwert) wird spaeter dynamisch hinzugefuegt, abhaengig u.A. vom gegebenen Parameter.
 */
void NetzeUndRauschen::createRandomColorMap()
{
	//colorsHSV.resize(xRes * resolutionMultiplier, std::vector<ColorHSV>((int)(yRes * resolutionMultiplier)));
	//initialColormatrix.resize(xRes * resolutionMultiplier, std::vector<ColorRGB>((int)(yRes * resolutionMultiplier)));
	//colorDistanceMatrix.resize(xRes * resolutionMultiplier, std::vector<float>((int)(yRes * resolutionMultiplier)));
	numberOfHues = numberOfHuesNew;
		for (int i = 0; i < netSizeX; i++) // TODO
	{
		for (int j = 0; j < netSizeY; j++)
		{
			pointPositionsMatrix[i][j].x = i;
			pointPositionsMatrix[i][j].y = j;

			//colorsHSV[i][j].h = defaultHue / 360; // unnoetig
			//colorsHSV[i][j].s = randomBetween(.5, .85);
			if ((flagSingleColor == false) || (numberOfHues > 1))
			{
				colorsHSV[i][j].s = randomBetween(.3, .65);
				colorsHSV[i][j].v = randomBetween(.8, 1.0);
				colorsHSV[i][j].hueNumber = (rand() * (int)(numberOfHues) / RAND_MAX);
			}
			else
			{
				float randomNumber = randomBetween(0, 1.0);
				if (randomNumber > singleColorRatio)
				{
					colorsHSV[i][j].s = 0;
				}
				else
				{
					colorsHSV[i][j].s = randomBetween(.3, .65);
				}
				colorsHSV[i][j].v = randomBetween(.8, 1.0);
				colorsHSV[i][j].hueNumber = (rand() * (int)(numberOfHues) / RAND_MAX);
			}
		}
	}
}

/*
 *Initialisiert das Interface. Zeichnet Manipulatoren fuer die Parameter. Initialisiert die Kameraposition. Erzeugt das die Matrix mit Zufallsfarben.
 */
void NetzeUndRauschen::setup()
{

	gl::enableAlphaBlending();
	gl::enableDepthRead();
	gl::enableDepthWrite();


	mParams = params::InterfaceGl("Parameters", ivec2(220, 170));

	mParams.addParam("Effect strength", &perlinEffectStrength, "min=0.0 max=2.0 step=0.1 keyIncr=p keyDecr=P");
	mParams.addParam("Perlin shift speed", &perlinShiftSpeed, "min=0.5 max=4.0 step=0.25 keyIncr=p keyDecr=P");
	mParams.addParam("Saturation addend", &saturationAddend, "min=0.0 max=0.5 step=0.02 keyIncr=p keyDecr=P");
	mParams.addParam("Default hue", &defaultHue, "min=0.0 max=360 step=1 keyIncr=p keyDecr=P");
	mParams.addParam("Number of Colors", &numberOfHuesNew, "min=1 max=16 step=1 keyIncr=p keyDecr=P");
	mParams.addButton("Refresh net", [&]() { createRandomColorMap(); });
	mParams.addButton("Single-Color mode", [&]() { toggleSingleColor(); });
	mParams.addParam("Single color ratio", &singleColorRatio, "min=0 max=1 step=0.1 keyIncr=p keyDecr=P");

	mCapture = Capture::create(640, 480);
	mCapture->start();



	mCam = ci::CameraPersp();
	mCamUi = CameraUi( &mCam, getWindow() );
	mCam.setNearClip( 1 );
	mCam.setFarClip( 2000 );
	//mCam.lookAt(vec3(0, netSize / 2, netSize / 2));
	//mCam.setEyePoint(vec3(15, netSize / 2, netSize /  2));

	this->adaptCameraEyePoint();
	mCam.lookAt(vec3((netSizeX - 1) / 2, (netSizeY - 1) / 2, 0));



	srand(time(NULL));

	Surface initialCameraScan = *mCapture->getSurface();
	setInitialColormatrix();

	createRandomColorMap();

}

/**
 *Passt den Augpunkt (Position) der Kamera so an, dass sie immer genau soweit vom Netz entfernt ist, dass sie einen optimalen Bereich des Netzes erfasst, ohne dessen Kanten zu zeigen.
 *Dazu wird, ausgehend vom Kamerawinkel von 35 Grad in y-Richtung, für beide Auslenkungen des Netzes (netSizeX und netSizeY) berechnet, wie groß die Distanz maximal sein darf, damit der Kamerawinkel innerhalb des Bereiches liegt. Die kleinere Distanz wird benutzt, um die Kamera zu positionieren.
 */
void NetzeUndRauschen::adaptCameraEyePoint()
{
	float cameraDistance = std::max((((netSizeX + 1) / 2) / 2) / tan(17.5 * (M_PI / 180)), (((netSizeY + 1) / 2) / 2) / tan(17.5 * (M_PI / 180)));
	mCam.setEyePoint(vec3((netSizeX - 1) / 2, (netSizeY - 1) / 2, cameraDistance));
}

/**
 *Aktualisiert oder setzt die Werte der Matrix, die das Hintergrundbild enthaelt. Dazu werden zuerst alle Werte der Matrix auf 0 gesetzt und im nächsten Schritt für jede Matrixposition alle Werte, die vom Bereich umfasst werden, normiert (also jeweils dividiert durch die gesamte Anzahl der Pixel im jeweiligen Bereich) aufsummiert.
 */
void NetzeUndRauschen::setInitialColormatrix()
{
	//initialColormatrix.resize(xRes * resolutionMultiplier, std::vector<ColorRGB>((int)(yRes * resolutionMultiplier)));
	for (int i = 0; i < netSizeX; i++)
	{
		for (int j = 0; j < netSizeY; j++)
		{
			backgroundColorMatrix[i][j].r = 0;
			backgroundColorMatrix[i][j].g = 0;
			backgroundColorMatrix[i][j].b = 0;
		}
	}

	 // TODO
	for (int i = 0; i < netSizeX; i++)
	{
		for (int j = 0; j < netSizeY; j++)
		{
			for (int k = 0; k < 40; k++)
			{
				for (int l = 0; l < 40; l++)
				{

					backgroundColorMatrix[i][j].r += (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).r / (40 * 40 * 255);
					backgroundColorMatrix[i][j].g += (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).g / (40 * 40 * 255);
					backgroundColorMatrix[i][j].b += (float)mCapture->getSurface()->getPixel(vec2(i * 40 + k, j * 40 + l)).b / (40 * 40 * 255);

				}
			}
		}
	}


}



/**
 *Eventhandler fuer Tasten-Events. Durch Druecken bestimmter Tasten koennen Parameter zur Laufzeit manipuliert werden, ohne dass ein Overlay angezeigt werden muss. Fuer jeden Tastendruck ist eine Aktion festgelegt, die ausgefuehrt wird. Fuer alle numerischen Operationen sind Grenzwerte, die nicht ueberschritten werden koennen, definiert.
 *@param event Parameter fuer die Taste, der auszuwerten ist.
*/
void NetzeUndRauschen::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case '+':
			if (perlinShiftSpeed < 8)
			{
				perlinShiftSpeed += .25;
				perlinSpeed.setSpeed(perlinShiftSpeed);
			}
		break;
		case '-':
			if (perlinShiftSpeed > .25)
			{
				perlinShiftSpeed -= .25;
				perlinSpeed.setSpeed(perlinShiftSpeed);
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


/**
 *Iteriert ueber das Kamerabild, um (netSizeX * netSizeY) Farbwerte aus den Pixelwerten zu mitteln. Farbmittelwerte beziehen sich dabei immer auf einen Rechteckigen Bereich, entsprechen also dem Kamerabereich, der auch repraesentiert werden soll. Ueber die Surface, die von der Kamera erzeugt wird und das Kamerabild enthaelt, zu iterieren,
 *besitzt die Klasse CaptureRef eine Memberfunktion (Surface getSurface()), die wiederum einen Iterator (Iter) initialisieren kann, der ueber jeden Pixel im Kamerabild iterieren kann. mit Iter.line() wird die naechste Zeile aufgerufen, also die y-Koordinate inkrementiert, mit Iter.pixel() wird die y-Koordinate inkrementiert. Beide Methoden geben "false" zurueck, falls die Koordinate den Bereich ueberschreitet.
 *Mithilfe des Iterators werden der colormatrix[netSizeX][netSizeY] werde zugewiesen. Diese werden aus dem Kamerabild bezogen und ueber den jeweiligen Bereich des Kamerabildes normiert (also jeweils dividiert durch die gesamte Anzahl der Pixel im jeweiligen Bereich) aufsummiert.
 */
void NetzeUndRauschen::setCameraCaptureColorMatrix()
{

	for (int i = 0; i < netSizeX; i++)
	{
		for (int j = 0; j < netSizeY; j++)
		{
			cameraCaptureColorMatrix[i][j].r = 0;
			cameraCaptureColorMatrix[i][j].g = 0;
			cameraCaptureColorMatrix[i][j].b = 0;
		}
	}


	Area area(0, 0, 640, 480);

	Surface surface = *mCapture->getSurface();
	Surface::Iter pixelIter = surface.getIter(area);
	vec2 pixelIterPos(0, 0);
	while (pixelIter.line())
	{
		while (pixelIter.pixel())
		{
			float r = pixelIter.r();
			float g = pixelIter.g();
			float b = pixelIter.b();
			pixelIterPos = pixelIter.getPos();

			cameraCaptureColorMatrix[(int)floor(pixelIterPos).x / (640 / netSizeX)][(int)floor(pixelIterPos).y / (480 / netSizeY)].r += r / ((640 / netSizeX) * (480 / netSizeY) * 255);
			cameraCaptureColorMatrix[(int)floor(pixelIterPos).x / (640 / netSizeX)][(int)floor(pixelIterPos).y / (480 / netSizeY)].g += g / ((640 / netSizeX) * (480 / netSizeY) * 255);
			cameraCaptureColorMatrix[(int)floor(pixelIterPos).x / (640 / netSizeX)][(int)floor(pixelIterPos).y / (480 / netSizeY)].b += b / ((640 / netSizeX) * (480 / netSizeY) * 255);
		}
	}

}

/**
 *Berechnet die euklidischen Distanzen des aktuellen Bildes und des definierten Hintergrundbildes im RGB-Raum fuer jeden Bildbereich. \f$ colordistance_{ij}=\sqrt{{(red_{bg_{ij}}-red_{ct_{ij}})}^2 + {(green_{bg_{ij}}-green_{ct_{ij}})}^2 + {(blue_{bg_{ij}}-blue_{ct_{ij}})}^2} \f$
 */
void NetzeUndRauschen::setColorDistanceMatrix()
{
	float speed = 0;
	for (int i = 0; i < netSizeX; i++)
	{
		for (int j = 0; j < netSizeY; j++)
		{
			colorDistanceMatrix[i][j] = 3 * sqrt(pow(abs(cameraCaptureColorMatrix[i][j].r - backgroundColorMatrix[i][j].r), 2) + pow(abs(cameraCaptureColorMatrix[i][j].g - backgroundColorMatrix[i][j].g), 2) + pow(abs(cameraCaptureColorMatrix[i][j].b - backgroundColorMatrix[i][j].b), 2)) / sqrt(3);
			speed += colorDistanceMatrix[i][j];
		}
	}

	// passt speed an Inhalt des BIldes an.
	speed /= (netSizeX * netSizeY);
	perlinSpeed.setSpeed(speed);
}

/**
 *Manipuliert die Auslenkungsvektoren der Vertices zeitbasiert, also verschiebt das Perlin-Rauschen einmal in x- und einmal in y-Richtung, je nachdem, wie viel Zeit seit dem letzten Frame vergangen ist, um sicherzustellen, dass die Perlin-Geschwindigkeit von der Framerate unabhaengig ist.
 *Passt die Auslenkungsstaerke der Vertices an, je nachdem, wie gross der Farbabstand zwischen Hintergrundbild und aktuellem Bild im jeweiligen Bereich ist.
 *Passt den Faktor, um den das Rauschen pro Frame verschoben wird dem perlinShiftSpeed an.
 *Die Zeitbasierte Komponente wird ermittelt, indem in jeden Frame die Zeit, die seit dem letzten Frame vergangen ist, aufaddiert wird. Wenn die Perlin-Geschwindigkeit erhoeht werden soll, wird die Zeitspanne mit einem Faktor multipliziert und erst dann aufaddiert, um schnelleres Vergehen der Zeit zu simulieren, was direkten Einfluss auf die Effektgeschwindigkeit hat.
 *Die Zeit wird in ms abgespeichert und muss durch 1000 dividiert, um ein korrektes Ergebnis zu erzeugen.
 */
void NetzeUndRauschen::applyManipulationsToVertices()
{
	float timeElapsed = (float)perlinSpeed.elapse();
	for (int i = 0; i < netSizeX; i++)
	{
		for (int j = 0; j < netSizeY; j++)
		{
			pointPositionsMatrix[i][j].x = i + perlinEffectStrength * colorDistanceMatrix[netSizeX - 1 - i][netSizeY - 1 - j] * perlinNoise.noise(((float)timeElapsed / 1000 + i), ((float)timeElapsed  / 1000 + j)) - .5;
			pointPositionsMatrix[i][j].y = j + perlinEffectStrength * colorDistanceMatrix[netSizeX - 1 - i][netSizeY - 1 - j] * perlinNoise.noise(((float)timeElapsed / 1000 + i) + .5, ((float)timeElapsed / 1000 + j) + .5) - .5;

		}
	}
}

/**
 *Setzt die Farbe der Dreiecke. Falls flagSingleColor == true, werden die Mittelwerte der Kamerapixel in dem Bereich genutzt (Low-Poly-Look des Kamerabildes). Ansonsten werden die Dreiecke in den Farben gezeichnet, die in der colorsHSV-Matrix festgelegt wurden. Die Saettigung wird gemaess der Farbabstandsmatrix angepasst. Bei Bedarf kann ein Saettigungsaddend angewendet werden, der fuer alle Bereiche identisch ist.
 *Fuer jeden Bereich werden zwei Dreiecke gezeichnet.
 */
void NetzeUndRauschen::applyColorsAndDrawTriangles()
{

	for (int i = 0; i < netSizeX; i++)
	{
		for (int j = 0; j < netSizeY; j++)
		{
			cinder::Color cameraAverageValue(cameraCaptureColorMatrix[netSizeX - 1 - i][netSizeY - 1 - j].r, cameraCaptureColorMatrix[netSizeX - 1 - i][netSizeY - 1 - j].g, cameraCaptureColorMatrix[netSizeX - 1 - i][netSizeY - 1 - j].b);
			cinder::Color hsvColor(CM_HSV, cameraAverageValue);

			if (mapCameraToNoiseNet)
			{
				gl::color(cinder::Color(cinder::ColorModel::CM_RGB, vec3(cameraCaptureColorMatrix[netSizeX - 1 - i][netSizeY - 1 - j].r, cameraCaptureColorMatrix[netSizeX - 1 - i][netSizeY - 1 - j].g, cameraCaptureColorMatrix[netSizeX - 1 - i][netSizeY - 1 - j].b)));
			}
			else
			{
				float hue = (defaultHue / 360) + (float)colorsHSV[i][j].hueNumber / (float)numberOfHues - floor((defaultHue / 360) + (float)colorsHSV[i][j].hueNumber / (float)numberOfHues);

				if (flagSingleColor == false || numberOfHues > 1)
				{
					gl::color(cinder::Color(cinder::ColorModel::CM_HSV, hue, colorsHSV[i][j].s + hsvColor.get(CM_HSV).x * colorDistanceMatrix[netSizeX - 1 - i][netSizeY - 1 - j] + saturationAddend, colorsHSV[i][j].v));
				}
				else
				{
					gl::color(cinder::Color(cinder::ColorModel::CM_HSV, hue, colorsHSV[i][j].s, colorsHSV[i][j].v));
				}
			}
			if ((i + 1 < netSizeX) && (j + 1 < netSizeY))
				gl::drawSolidTriangle(pointPositionsMatrix[i][j], pointPositionsMatrix[i][j + 1], pointPositionsMatrix[i + 1][j]);


			if ((i - 1 >= 0) && (j - 1 >= 0))
				gl::drawSolidTriangle(pointPositionsMatrix[i][j], pointPositionsMatrix[i][j - 1], pointPositionsMatrix[i - 1][j]);
		}
	}
}

/**
 *Passt die Farbe(n) des Netzes ueber die Zeit an. 
 */
void NetzeUndRauschen::adaptColor()
{
	defaultHue = (colorChangeSpeed.elapse() / 1000) % 360;	
}

/**
 *Wird in jedem Frame aufgerufen. Richtet das Netz zentral im Koordinatensystem aus. Ruft alle notwendigen Berechnungen nacheinander auf und passt das Netz (Positionen, Farben) entsprechend an.
 */
void NetzeUndRauschen::draw()
{
	//gl::drawCoordinateFrame();
	gl::translate(vec3(-netSizeX * 0.5f, -netSizeY * 0.5f, 0.0f));
	gl::clear();

	gl::setMatrices(mCam);

	setCameraCaptureColorMatrix();
	setColorDistanceMatrix();
	adaptColor();
	applyManipulationsToVertices();
	applyColorsAndDrawTriangles();

#ifdef PROTOTYPE
	mParams.draw();
#endif

}

/**
 *Initialisiert OpenGL mit einem 4x Anti-Aliasing.
 */
CINDER_APP( NetzeUndRauschen, RendererGl( RendererGl::Options().msaa(4)) )
