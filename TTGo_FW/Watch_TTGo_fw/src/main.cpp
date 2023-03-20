#include "config.h"
#include "../.pio/libdeps/ttgo-t-watch/TTGO TWatch Library/examples/TFT_espi/All_Free_Fonts_Demo/Free_fonts.h" // Include fonts
#include "esp_bt_main.h"
#include "esp_bt_device.h"

// Check if Bluetooth configs are enabled
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Bluetooth Serial object
BluetoothSerial SerialBT;

// Watch objects
TTGOClass *watch;
TFT_eSPI *tft;
BMA *sensor;

uint32_t sessionId = 30;

volatile uint8_t state;
volatile bool irqBMA = false;
volatile bool irqButton = false;

bool sessionStored = false;
bool sessionSent = false;
float updateTimeout;
float last;
float step_length = 0.8;  

// helps to avoid signal bounce on the button press, might be unnecesssary
void IRAM_ATTR btnClick(){
  static long lastClickedMillis;
  if (millis() - lastClickedMillis > 50) {
    lastClickedMillis = millis();
    irqButton = true;
  }
}

void initHikeWatch()
{
    // LittleFS
    if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LITTLEFS Mount Failed");
        return;
    }

    // Stepcounter
    //FROM BMA423_STEP_COUNT EXAMPLE
    // Accel parameter structure
    Acfg cfg;
    
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    // Configure the BMA423 accelerometer
    sensor->accelConfig(cfg);

    // Enable BMA423 accelerometer
    sensor->enableAccel();

    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] {
        // Set interrupt to set irq value to 1
        irqBMA = 1;
    }, RISING); //It must be a rising edge

    // Enable BMA423 step count feature
    sensor->enableFeature(BMA423_STEP_CNTR, true);

    // Reset steps
    sensor->resetStepCounter();

    // Turn on step interrupt
    sensor->enableStepCountInterrupt();

    // Side button
    pinMode(AXP202_INT, INPUT_PULLUP); //AXP202_INT
    attachInterrupt(AXP202_INT, btnClick, FALLING);

    //!Clear IRQ unprocessed first
    watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ | AXP202_PEK_LONGPRESS_IRQ, true); // longpress is used to enable turning off the watch
    watch->power->clearIRQ();

    return;
}

void sendDataBT(fs::FS &fs, const char * path)
{
    /* Sends data via SerialBT */
    fs::File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }
    Serial.println("- read from file:");
    while(file.available()){
        SerialBT.write(file.read());
    }
    file.close();
}

void sendSessionBT()
{
    // Read session and send it via SerialBT
    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    watch->tft->drawString("Sending session", 20, 80);
    watch->tft->drawString("to Hub", 80, 110);

    // Sending session id
    sendDataBT(LITTLEFS, "/id.txt");
    SerialBT.write(';');
    // Sending steps
    sendDataBT(LITTLEFS, "/steps.txt");
    SerialBT.write(';');
    // Sending distance
    sendDataBT(LITTLEFS, "/distance.txt");
    SerialBT.write(';');
    // Send connection termination char
    SerialBT.write('\n');
}


void saveIdToFile(uint16_t id)
{
    char buffer[10];
    itoa(id, buffer, 10);
    writeFile(LITTLEFS, "/id.txt", buffer);
}

void saveStepsToFile(uint32_t step_count)
{
    char buffer[10];
    itoa(step_count, buffer, 10);
    writeFile(LITTLEFS, "/steps.txt", buffer);
}

void saveDistanceToFile(float distance)
{
    char buffer[10];
    itoa(distance, buffer, 10);
    writeFile(LITTLEFS, "/distance.txt", buffer);
}

void deleteSession()
{
    deleteFile(LITTLEFS, "/id.txt");
    deleteFile(LITTLEFS, "/distance.txt");
    deleteFile(LITTLEFS, "/steps.txt");
    deleteFile(LITTLEFS, "/coord.txt");
}

void setup()
{
    Serial.begin(115200);
    watch = TTGOClass::getWatch();
    watch->begin();
    watch->openBL();

    //Receive objects for easy writing
    tft = watch->tft;
    sensor = watch->bma;
    
    initHikeWatch();

    state = 1;

    SerialBT.begin("Hiking Watch");
}

// a helper function for displaying the bluetooth MAC-address
void printDeviceAddress() {

  const uint8_t* point = esp_bt_dev_get_address();
  for (int i = 0; i < 6; i++) {
    char str[3];
    sprintf(str, "%02X", (int)point[i]);
    tft->print(str);
    if (i < 5){
      tft->print(":");
    }
  }
}
void printStatus() {
    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    tft->setCursor(45, 70);
    tft->print("State: ");
    tft->print(state);

    tft->setCursor(45, 140);
    tft->print("irqButton: ");
    tft->print(irqButton);
}
void loop()
{
    switch (state)
    {
    case 1:
    {
        /* Initial stage */
        //Basic interface
        int16_t x, y;

        watch->tft->fillScreen(TFT_BLACK);
        watch->tft->setTextFont(4);
        watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
        watch->tft->drawString("Hiking Watch",  45, 25, 4);
        watch->tft->drawString("Press button", 50, 80);
        watch->tft->drawString("to start session.", 40, 110);

        watch->tft->drawString("Touch the screen to",  15, 160);
        watch->tft->drawString("input step length.", 20, 190);

        bool exitSync = false;

        //Bluetooth discovery
        while (1)
        {
            /* Bluetooth sync */
            if (SerialBT.available())
            {
                char incomingChar = SerialBT.read();
                if (incomingChar == 'c' and sessionStored and not sessionSent)
                {
                    sendSessionBT();
                    sessionSent = true;
                }

                if (sessionSent && sessionStored) {
                    // Update timeout before blocking while
                    updateTimeout = 0;
                    last = millis();
                    while(1)
                    {
                        updateTimeout = millis();

                        if (SerialBT.available())
                            incomingChar = SerialBT.read();
                        if (incomingChar == 'r')
                        {
                            Serial.println("Got an R");
                            // Delete session
                            deleteSession();
                            sessionStored = false;
                            sessionSent = false;
                            incomingChar = 'q';
                            exitSync = true;
                            break;
                        }
                        else if ((millis() - updateTimeout > 2000))
                        {
                            Serial.println("Waiting for timeout to expire");
                            updateTimeout = millis();
                            sessionSent = false;
                            exitSync = true;
                            break;
                        }
                    }
                }
            }
            if (exitSync)
            {
                delay(1000);
                watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                watch->tft->drawString("Hiking Watch",  45, 25, 4);
                watch->tft->drawString("Press button", 50, 80);
                watch->tft->drawString("to start session", 40, 110);
                
                watch->tft->drawString("Touch the screen to",  15, 160);
                watch->tft->drawString("input step length", 20, 190);
                exitSync = false;
            }

            /*      IRQ     */
            if (irqButton) {
                irqButton = false;
                watch->power->readIRQ();
                if (watch->power->isPEKShortPressIRQ()) {
                    if (state == 1)
                    {
                        state = 2;
                    }
                }
                watch->power->clearIRQ();
            }
            if (state == 2) {
                if (sessionStored)
                {
                    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                    watch->tft->drawString("Overwriting",  55, 100, 4);
                    watch->tft->drawString("session", 70, 130);
                    delay(1000);
                }
                break;
            }
             /*screen touched*/
            if (watch->getTouch(x, y)) {
                // jumps to asking the user their step length
                state = 5;
                break;
            }
        }
        break;
    }
    case 2:
    {
        /* Hiking session initalisation */
        
        state = 3;
        break;
    }
    case 3:
    {
        /* Hiking session ongoing */
        uint32_t steps = 0;
        float distance = 0;

        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        watch->tft->drawString("Starting hike", 45, 100);
        delay(1000);
        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);

        watch->tft->setCursor(45, 70);
        watch->tft->print("Steps: 0");

        watch->tft->setCursor(45, 100);
        watch->tft->print("Dist: 0.00 m");

        last = millis();
        updateTimeout = 0;

        //FROM BMA423_STEPCOUNTER
        while (1) {
            if (irqBMA) {
                irqBMA = 0;
                bool  rlst;
                do {
                    // Read the BMA423 interrupt status,
                    // need to wait for it to return to true before continuing
                    rlst =  sensor->readInterrupt();
                } while (!rlst);

                // Check if it is a step interrupt
                if (sensor->isStepCounter()) {
                    // Get step data from register
                    steps = sensor->getCounter();
                    //clears the screen
                    tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                    
                    tft->setCursor(45, 70);
                    tft->print("Steps: ");
                    tft->print(steps);

                    watch->tft->setCursor(45, 100);
                    //Distance is just the steps multiplied by the step length
                    distance = steps * step_length;
                    watch->tft->print("Dist: ");
                    tft->print(distance);
                    tft->print(" m");
                    //Serial.println(step);
                }
            }
            if (irqButton) {
                irqButton = false;
                watch->power->readIRQ();
                if (watch->power->isPEKShortPressIRQ()) {
                    if (state == 3)
                    {
                        state = 4;
                    }
                }
                watch->power->clearIRQ(); 
            }
            if (state == 4) {
                break;
            }
            //delay(20);
        }
        break;
    }
    case 4:
    {
        //Save hiking session data
        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        watch->tft->drawString("Saving session", 45, 100);

        saveIdToFile(sessionId);
        uint32_t steps = sensor->getCounter();
        saveStepsToFile(steps);
        saveDistanceToFile(steps * step_length);
        // set session stored to true
        sessionStored = true;
        //reset step-counter
        sensor->resetStepCounter();
        delay(2000);
        state = 1;  
        break;
    }
    case 5:
    {   
        int16_t x, y;
        tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        tft->setCursor(35, 50);
        tft->print("Step length: ");
        tft->print(step_length);

        tft->setFreeFont(FMB24);
        watch->tft->drawString("-",  50, 140, GFXFF);
        watch->tft->drawString("+",  180, 140, GFXFF);
        tft->setTextFont(4);
        while (1) {
            /*asking the user for their step length*/
            if (watch->getTouch(x, y)) {
                // touched on the lower left side
                if (y > 70 && x < 120 && step_length > 0.1) {           // the step length can be 10 cm at minimum
                    step_length -= 0.05;
                }
                // touched on the lower rigth side
                else if ( y > 70 && x > 120 && step_length < 1.95) {     // the step length can be 2.0 m at maximum
                    step_length += 0.05;
                }

                tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                tft->setCursor(35, 50);
                tft->print("Step length: ");
                tft->print(step_length);

                tft->setFreeFont(FMB24);
                watch->tft->drawString("-", 50, 140, GFXFF);
                watch->tft->drawString("+", 180, 140, GFXFF);
                tft->setTextFont(4);
                delay(100);
            }
            if (irqButton) {
                irqButton = false;
                watch->power->readIRQ();
                // check that the interrput was a short press
                if (watch->power->isPEKShortPressIRQ()) {
                    if (state == 5)
                    {
                        state = 1;
                    }
                }
                watch->power->clearIRQ();
            }
            if (state == 1) {
                break;
            }
        }
        break;
    }
    default:
        // Restart watch
        ESP.restart();
        break;
    }
}