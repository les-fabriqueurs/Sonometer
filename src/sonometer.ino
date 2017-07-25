// -------------------------------------------------------------------------------
// Sonometer for Classroom 1.0
// pierre@fabriqueurs
//--------------------------------------------------------------------------------
 
// required librairies
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <SPI.h>

// constants definition
#define N_PIXELS  60      // Number of pixels in the strip
#define MIC_PIN   A6      // Analog pin attached to Microphone output
#define LED_PIN   52      // Pin connected to NeoPixel LED strip input 
#define MAX_PIN   A8      // Pin connected to "high level control" potentiometer
#define MIN_PIN   A9      // Pin connected to "low level control" potentiometer

#define SAMPLE_WINDOW 100 // Sample window for average level (in ms)
#define NOISE     12      // Noise in mic output signal

#define INPUT_FLOOR_MIN 1     //Min Lower range of mic analog output
#define INPUT_FLOOR_MAX 50    //Max Lower range of mic analog output
#define INPUT_CEILING_MIN 60  //Min upper range of mic analog output
#define INPUT_CEILING_MAX 200 //Max upper range of mic analog output



unsigned int sample;
int previousC = INPUT_CEILING_MIN;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ----------- fscale function --------------
// Floating Point Autoscale Function V0.1
// Written by Paul Badger 2007
// Modified from code by Greg Shakar
float fscale( float originalMin,
              float originalMax,
              float newBegin, 
              float newEnd,
              float inputValue,
              float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function


  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}
//-----------------------------------------------------------------------------------



// light-on leds between two points
void drawLine(uint8_t from, uint8_t to) {

  //  -- start by lighting all on ---
  int max_green = N_PIXELS/3;
  int max_bleu = max_green + N_PIXELS/3;
  // green
  for (int i=0; i<max_green; i++){
    strip.setPixelColor(i, 0,255,0);
  }
  // yellow
  for (int i=max_green; i<max_bleu; i++){
    strip.setPixelColor(i, 0,0,255);
  }
  // red
  for (int i=max_bleu; i<=N_PIXELS-1; i++){
    strip.setPixelColor(i, 255,0,0);
  }
  
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  
  for(int i=from; i<=to; i++){
    strip.setPixelColor(i,0,0,0);
  } 
  
  strip.show();
}

// Adjust  floor level for from analog input ('low level' potentiometer)
int getFloorLevel(){
    int valRead= analogRead(MIN_PIN);
    long valMin= INPUT_FLOOR_MIN + (INPUT_FLOOR_MAX - INPUT_FLOOR_MIN)*long(valRead)/1023;    
    return(int(valMin));  
}

// Adjust  ceiling level for from analog input ('high level' potentiometer)
int getCeilingLevel(){
    int valRead= analogRead(MAX_PIN);
    long valMax= INPUT_CEILING_MIN + (INPUT_CEILING_MAX - INPUT_CEILING_MIN)*long(valRead)/1023;
}

//-----------------------------------------------------------------------------------

void setup() 
{
  analogReference(DEFAULT);
  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'
//  Serial.begin(115200);  
}

void loop() 
{
  unsigned long startMillis= millis();  // Start of sample window
  int level; 

  unsigned int signalMax = 0;
  unsigned int signalMin = 65535;
  long signalAv = 0;
  int numSample = 0;
  long sumSample = 0;

  unsigned int sample;
  int c;

 
  // collect data within the sample window (in mS)
  while (millis() - startMillis < SAMPLE_WINDOW)
  {
    sample = analogRead(MIC_PIN);
    
    // get min and max and what's needed for average (sum and number of samples)
    if (sample > signalMax)
    {
      signalMax = sample;  
    }
    else if  (sample < signalMin){
      signalMin = sample; 
    }
    sumSample = sumSample + sample;
    numSample++;
  }

  // compute average
  signalAv = sumSample / numSample; 
  
  // compute amplitude
  level = signalMax - signalMin;  


  //Scale the input logarithmically instead of linearly
  c = fscale(getFloorLevel(), getCeilingLevel(), strip.numPixels(), 0, (float)level, 2);

  if (c > (int)strip.numPixels()){
    c=(int)strip.numPixels();
  }
  
  //  progressive rise 
  while (c < previousC) {
    delay(5);
    previousC--;
    drawLine(strip.numPixels(), strip.numPixels()-previousC);
  }
  
  //  drop 1 by 1
  if (c > previousC) {
    previousC++;
    drawLine(strip.numPixels(), strip.numPixels()-previousC);
  }
}

