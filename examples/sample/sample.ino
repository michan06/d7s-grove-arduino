#include <M5Stack.h>
#include <D7S.h>

//old earthquake data
float oldSI = 0.0;
uint8_t oldPGA = 0;

//flag variables to handle collapse/shutoff only one time during an earthquake
bool shutoffHandled = false;
bool collapseHandled = false;

//function to handle collapse event
void handleCollapse()
{
  //put here the code to handle the collapse event
  Serial.println("-------------------- COLLAPSE! --------------------");
}

void setup()
{
  // Open serial communications and wait for port to open:
  // Serial.begin(9600);
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  M5.begin();
  M5.Lcd.clear(BLACK);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);
  
  Serial.print("Starting D7S communications (it may take some time)...");
  M5.Lcd.print("Starting D7S communications (it may take some time)...");
  //start D7S connection
  D7S.begin();
  //wait until the D7S is ready
  do {
    Serial.print(".");
    M5.Lcd.print(".");
    delay(1000);
  } while (!D7S.isReady());
  Serial.println("STARTED");

  //setting the D7S to switch the axis at inizialization time
  Serial.println("Setting D7S sensor to switch axis at inizialization time.");
  D7S.setAxis(SWITCH_AT_INSTALLATION);

  Serial.println("Initializing the D7S sensor in 2 seconds. Please keep it steady during the initializing process.");
  M5.Lcd.println("Initializing the D7S sensor in 2 seconds. Please keep it steady during the initializing process.");
  delay(2000);
  Serial.print("Initializing...");
  M5.Lcd.print("Initializing...");
  //start the initial installation procedure
  D7S.initialize();
  //wait until the D7S is ready (the initializing process is ended)
  while (!D7S.isReady())
  {
    Serial.print(".");
    M5.lcd.print(".");
    delay(500);
  }
  Serial.println("INITIALIZED!");
  M5.lcd.println("INITIALIZED!");

  //check if there there was a collapse (if this is the first time the D7S is put in place the installation data may be wrong)
  if (D7S.isInCollapse())
  {
    handleCollapse();
  }

  //reset the events shutoff/collapse memorized into the D7S
  D7S.resetEvents();

  M5.Lcd.clear(BLACK);
  Serial.println("\nListening for earthquakes!");
}

int count = 0;
const char *signs="-\\|/";

void loop()
{
  M5.Lcd.fillRect( 0, 0, 320, 20, TFT_BLACK);
  M5.Lcd.setTextFont(4);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print(signs[count%4]);
  count++;
  // if(M5.BtnC.wasPressed()){
  //   Serial.print("Starting D7S communications (it may take some time)...");
  //   //start D7S connection
  //   D7S.begin();
  //   //wait until the D7S is ready
  //   do {
  //     Serial.print(".");
  //     delay(1000);
  //   } while (!D7S.isReady());
  //   Serial.println("STARTED");

  //   //setting the D7S to switch the axis at inizialization time
  //   Serial.println("Setting D7S sensor to switch axis at inizialization time.");
  //   D7S.setAxis(SWITCH_AT_INSTALLATION);

  //   Serial.println("Initializing the D7S sensor in 2 seconds. Please keep it steady during the initializing process.");
  //   delay(2000);
  //   Serial.print("Initializing...");
  //   //start the initial installation procedure
  //   D7S.initialize();
  //   //wait until the D7S is ready (the initializing process is ended)
  //   while (!D7S.isReady())
  //   {
  //     Serial.print(".");
  //     delay(500);
  //   }
  //   Serial.println("INITIALIZED!");
  // }

  //checking if there is an earthquake occuring right now
  if (D7S.isEarthquakeOccuring())
  {
    M5.Lcd.fillRect( 0, 20, 319, 79, TFT_BLACK);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.setTextFont(4);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setCursor(0, 20);
    M5.Lcd.print("Earthquake!!");

    //check if the shutoff event has been handled and if the shutoff condition is met
    //the call of D7S.isInShutoff() is executed after to prevent useless I2C call
    if (!collapseHandled && D7S.isInCollapse())
    {
      M5.Lcd.setCursor(0, 40);
      M5.Lcd.print("Collapse!!");

      handleCollapse();
      collapseHandled = true;
    }

    //print information about the current earthquake
    float currentSI = D7S.getInstantaneusSI();
    uint16_t currentPGA = D7S.getInstantaneusPGA();

    if (currentSI > oldSI || currentPGA > oldPGA)
    {
      //getting instantaneus SI
      Serial.print("\tInstantaneus SI: ");
      Serial.print(currentSI/10);
      Serial.println(" [cm/s]");

      M5.Lcd.fillRect( 0, 100, 319, 199, TFT_BLACK);
      M5.Lcd.setTextFont(4);
      M5.Lcd.setTextColor(TFT_YELLOW);
      M5.Lcd.setTextSize(2);
      M5.Lcd.drawString("SI:",0,100);
      M5.Lcd.setTextDatum(TR_DATUM);
      M5.Lcd.drawFloat(currentSI,1,319,100);

      //getting instantaneus PGA
      Serial.print("\tInstantaneus PGA (Peak Ground Acceleration): ");
      Serial.print(currentPGA);
      Serial.println(" [cm/s^2]\n");

      M5.Lcd.drawString("PGA:",0,140);
      M5.Lcd.drawFloat((float)currentPGA,1,319,140);

      //save the current data
      oldSI = currentSI;
      oldPGA = currentPGA;
    }
  }
  else
  {
    //reset the old earthquake data
    oldPGA = 0;
    oldSI = 0;
    //reset the flag of the handled events
    shutoffHandled = false;
    collapseHandled = false;
    //reset D7S events
    D7S.resetEvents();
    M5.Lcd.fillRect( 0, 20, 320, 79, TFT_BLACK);
  }
  M5.update();
}