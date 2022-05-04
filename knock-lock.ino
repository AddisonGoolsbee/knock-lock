#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <Servo.h>

Servo myServo;
// Pin definitions
const int knockSensor = 0;         // Piezo sensor.
const int recordButton = 2;       // If this is high we program a new code.
const int lockMotor = 9;           // Gear motor used to turn the lock.
const int lockButton = 6;
const int lights = 3;              // LED ring


// Preset constants to tune the device
const int threshold = 105;           // Minimum signal from the piezo to register as a knock
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock..
const int avgRejectValue = 15; // If the average timing of the knocks is off by this percent we don't unlock.
const int knockFadeTime = 150;     // milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)

const int maxKnocks = 20;       // Maximum number of knocks to listen for.
const int knockComplete = 1200;     // Longest time to wait for a knock before we assume that it's finished.


// Variables.
int secretCode[maxKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial set knock pattern: "Shave and a Hair Cut, two bits."
int knockReadings[maxKnocks];   // When someone knocks this array fills with delays between knocks.
int knockSensorValue = 0;           // Last reading of the knock sensor.
boolean recordButtonPressed = false;   // Flag so we remember the programming button setting at the end of the cycle.
boolean locked = true;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, lights, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'
  

  pinMode(lockMotor, OUTPUT);
  pinMode(recordButton, INPUT);
  pinMode(lockButton, INPUT);
  myServo.attach(lockMotor, 600, 23000);
  Serial.begin(9600);
  myServo.write(0);
  Serial.println("WHY");
  // just a little boot up sequence: white to yellow to orange to red
  colorWipe(strip.Color(255, 150, 80), 20);
  colorWipe(strip.Color(255, 255, 0), 20); 
  colorWipe(strip.Color(255, 170, 0), 20); 
  colorWipe(strip.Color(255, 0, 0), 20); 
  myServo.detach();
  Serial.println("HELLO");
  
}

void loop() {
  Serial.println("J");
  

  knockSensorValue = analogRead(knockSensor);

  if (!locked) {
      if (digitalRead(recordButton) == HIGH) {
        recordButtonPressed = true;          // saves the state if program button is pressed
        colorWipe(strip.Color(0, 0, 255), 0); // blue
      } else {
        colorWipe(strip.Color(0, 255, 0), 0); // green
      }
  } else {
    recordButtonPressed = false;
    colorWipe(strip.Color(255, 0, 0), 0); // red
  }

  if (knockSensorValue >= threshold && (locked || digitalRead(recordButton) == HIGH)) {
    recordSecretKnock();
  }

  // go from unlocked to locked
  if (!locked && digitalRead(lockButton) == HIGH) {
    myServo.attach(lockMotor, 600, 23000);
    myServo.write(0);
    colorWipe(strip.Color(255, 0, 0), 10); // red
    locked = true;
    myServo.detach();
  }
}

// Records the knock pattern using the timing of the knocks.
void recordSecretKnock() {

  int i = 0;
  // Reset array of knocks.
  for (i = 0; i < maxKnocks; i++) {
    knockReadings[i] = 0;
  }

  int currentKnockNumber = 0;             // Incrementer for the array.
  int startTime = millis();               // serves as a reference for when the knock pattern started
  int now = millis();

  colorWipe(strip.Color(0, 0, 0), 0); // color off
  delay(knockFadeTime);                                 // wait until given knockFadeTime interval passes before we listen to the next one.
  
  colorWipe(strip.Color(recordButtonPressed ? 0:255, 0, recordButtonPressed ? 255:0), 0); // red or blue, depending on scenario
  do {
    //wait for next knock or wait for it to timeout.
    knockSensorValue = analogRead(knockSensor);
    if (knockSensorValue >= threshold) {                 //got another knock...
      //record the delay time.
      now = millis();
      knockReadings[currentKnockNumber] = now - startTime;
      currentKnockNumber ++;                             //move to next index in array by incrementing the counter
      startTime = now;
      // and reset our timer for the next knock
      colorWipe(strip.Color(0, 0, 0), 0); // color off
      delay(knockFadeTime);                              // again, a little delay to let the knock decay.
      colorWipe(strip.Color(recordButtonPressed ? 0:255, 0, recordButtonPressed ? 255:0), 0); // red or blue, depending on scenario
    }

    now = millis();

    //did we timeout or run out of knocks?
  } while ((now - startTime < knockComplete) && (currentKnockNumber < maxKnocks));

  //check if new knocks are valid knock patterns
  if (!recordButtonPressed) {
    if (locked) {
      if (checkKnock()) {
        unlock();
      } else {
        theaterChase(strip.Color(255, 0, 0), 35); // blue flashing
      }
    }
  } else { // if we're in programming mode and it is unlocked, set the new lock to be this
    if (!locked) {
      checkKnock(); // in programming mode this sets the new lock to be the inputted lock
      theaterChase(strip.Color(0, 0, 255), 35); // blue flashing
    }
  }
}

// runs the servo motor to unlock the door.
void unlock() {
  locked = false;
  int i = 0;
  myServo.attach(lockMotor, 600, 23000);
  myServo.write(90);

  theaterChase(strip.Color(0, 255, 0), 35); // Green flashing
  colorWipe(strip.Color(0, 255, 0), 10); // red
  myServo.detach();
}

// sees if our knock matches secret knock; returns true is knock is valid, false otherwise
boolean checkKnock() {
  int i = 0;

  // check if there are correct number of knocks
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;               // We use this later to normalize the times.

  for (i = 0; i < maxKnocks; i++) {
    if (knockReadings[i] > 0) {
      currentKnockCount++;
    }
    if (secretCode[i] > 0) {
      secretKnockCount++;
    }

    if (knockReadings[i] > maxKnockInterval) {  // get the data to normalize the time intervals between knocks for later comparison
      maxKnockInterval = knockReadings[i];
    }
  }

  // If we're recording a new knock, save the knock pattern
  if (recordButtonPressed) {
    for (i = 0; i < maxKnocks; i++) { // normalize the times
      secretCode[i] = map(knockReadings[i], 0, maxKnockInterval, 0, 100);
    }
    // And flash the lights in the recorded pattern to let us know it's been programmed.

    // turn ring off and wait a bit before starting
    colorWipe(strip.Color(0, 0, 0), 0); // color off
    delay(400);

    // first flash
    colorWipe(strip.Color(0, 0, 255), 0); // first blue
    delay(30);
    
    for (i = 0; i < maxKnocks; i++) {
      colorWipe(strip.Color(0, 0, 0), 0); // color off
      if (secretCode[i] > 0) {
        delay( map(secretCode[i], 0, 100, 0, maxKnockInterval)); // Expand the time back out to what it was.  Roughly.
        colorWipe(strip.Color(0, 0, 255), 0); // blink blue
        delay(30);
      }
    }
    delay(200);
    return false;   // Lock remains locked if new knock pattern is being recorred
  }

  if (currentKnockCount != secretKnockCount) {
    return false;
  }

  //compare relative intervals between the knocks to see if it is the same pattern, just at a different speed
  int totaltimeDifferences = 0;
  int timeDiff = 0;
  //time between knocks relative to one another are then compared (i.e. slow/fast)
  for (i = 0; i < maxKnocks; i++) {
    knockReadings[i] = map(knockReadings[i], 0, maxKnockInterval, 0, 100);
    timeDiff = abs(knockReadings[i] - secretCode[i]);
    if (timeDiff > rejectValue) { // compares time interval between knocks between our set value at which too much time has passed to register knock
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // rejects knock attempt if the pattern is too off.
  if (totaltimeDifferences / secretKnockCount > avgRejectValue) {
    return false;
  }
  return true;

}

// LED ring functions

// fill each light one by one in a circle
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<4; j++) {  //do 5 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}
