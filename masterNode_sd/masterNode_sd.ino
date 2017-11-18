/*
 * Simple data logger. adapted from sdFat lib example 
 */

/*NodeMCU has weird pin mapping.*/
/*Pin numbers written on the board itself do not correspond to ESP8266 GPIO pin numbers.*/
#define D0 16
#define D1 5 // I2C Bus SCL (clock)
#define D2 4 // I2C Bus SDA (data)
#define D3 0
#define D4 2 // Same as "LED_BUILTIN", but inverted logic
#define D5 14 // SPI Bus SCK (clock)
#define D6 12 // SPI Bus MISO 
#define D7 13 // SPI Bus MOSI
#define D8 15 // SPI Bus SS (CS)
#define D9 3 // RX0 (Serial console)
#define D10 1 // TX0 (Serial console)

//enable this for debug statements
//#define DEBUG


extern "C" { //needed for "ESP.getResetInfoPtr()"
#include <user_interface.h>
}

#include <SPI.h>
#include "SdFat.h"
#include <SparkFunHTU21D.h>

HTU21D humSens;

// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
constexpr uint8_t chipSelect = D3;//SS;

// Interval between data records in milliseconds.
// The interval must be greater than the maximum SD write latency plus the
// time to acquire and write data to the SD to avoid overrun errors.
// Run the bench example to check the quality of your SD card.
constexpr uint32_t SAMPLE_INTERVAL_S = 5; //sample every 5 seconds
constexpr uint32_t SAMPLE_INTERVAL_MS = SAMPLE_INTERVAL_S*1000;
constexpr uint32_t SAMPLE_INTERVAL_uS = SAMPLE_INTERVAL_MS*1000;

constexpr uint32_t OFFSET = 0.07; //too slow
constexpr uint32_t OFFSET_uS = SAMPLE_INTERVAL_uS*OFFSET;

// Log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "Data"

//==============================================================================
// Error messages stored in flash.
#define error(msg) sd.errorHalt(F(msg))
//------------------------------------------------------------------------------


// File system object.
SdFat sd;

// Log file.
SdFile file;

// Time in seconds for next data record.
uint32_t* logTime = new uint32_t;

//power up reason
constexpr uint8_t POWER_REBOOT = 0; /*0 Power reboot*/
constexpr uint8_t HARDWARE_WDT_RESET = 1; /*1 Hardware WDT reset*/
constexpr uint8_t FATAL_EXCEPTION = 2; /*2 Fatal exception*/
constexpr uint8_t SOFTWARE_WATCHDOG_RESET = 3; /*3 Software watchdog reset*/
constexpr uint8_t SOFTWARE_RESET = 4; /*4 Software reset*/
constexpr uint8_t DEEP_SLEEP = 5; /*5 Deep-sleep*/
constexpr uint8_t HARDWARE_RESET = 6; /*6 Hardware reset*/

//------------------------------------------------------------------------------
// Write data header.
void writeHeader() {
  file.println("#time in seconds since start of measurement, temperature (C), humidity (%)");
}
//------------------------------------------------------------------------------
// Log a data record.
void logData() {

	humSens.begin();
  // Read all channels to avoid SD write latency between readings.
	float humd = humSens.readHumidity();
	float temp = humSens.readTemperature();

	#ifdef DEBUG 
	Serial.println("logging :");
	Serial.print(*logTime);
	Serial.print(", ");
	Serial.print(temp);
	Serial.print(", ");
	Serial.println(humd);
	#endif

  // Write data to file.  Start with log time in micros.
  file.print(*logTime);
  file.write(',');
  file.print(temp, 2);
  file.write(',');
  file.println(humd, 2);

  // Force data to SD and update the directory entry to avoid data loss.
	#ifdef DEBUG 
  if (!file.sync() || file.getWriteError()) {
    error("write error");
  }
	#endif
	file.sync(); 
}

void wakeSetup(){
  char fileName[13] = FILE_BASE_NAME "00.csv"; //TODO

	#ifdef DEBUG  
	Serial.begin(9600); //for debug purpose
	while(!Serial){} //wait for serial
	Serial.println();
	#endif

	ESP.rtcUserMemoryRead(0, logTime, sizeof(uint32_t));
	ESP.rtcUserMemoryRead(sizeof(uint32_t), (uint32_t*)fileName, 13*sizeof(char));

	//update log time
	*logTime += SAMPLE_INTERVAL_S;
	#ifdef DEBUG
	Serial.print("logTime: ");  
	Serial.println(*logTime);
	Serial.print("fileName: ");  
	Serial.println(fileName);
	#endif

  if (!sd.begin(chipSelect, SD_SCK_MHZ(50))) {
    sd.initErrorHalt();
  }

  if (!file.open(fileName, O_WRITE | O_APPEND)) {
    error("file.open");
  }

}

void bootSetup() {
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME "00.csv";

	#ifdef DEBUG 
  Serial.begin(9600); //for debug purpose
	while(!Serial){} //wait for serial
	#endif

	//configure humidity sensor
	humSens.begin();
  
  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  if (!sd.begin(chipSelect, SD_SCK_MHZ(50))) {
    sd.initErrorHalt();
  }

  // Find an unused file name.
  if (BASE_NAME_SIZE > 6) {
    error("FILE_BASE_NAME too long");
  }
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
      error("Can't create file name");
    }
  }
	
  if (!file.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
    error("file.open");
  }

  // Write data header.
  writeHeader();

	//save fileName to sleep-persistant memory
	*logTime = 0;
	ESP.rtcUserMemoryWrite(0, logTime, sizeof(uint32_t));
	ESP.rtcUserMemoryWrite(sizeof(uint32_t), (uint32_t*)fileName, 13*sizeof(char));
	#ifdef DEBUG 
	Serial.println("Boot setup completed");
	#endif
}

void setup(){
	uint8_t reason = ESP.getResetInfoPtr()->reason; 
	if(reason == DEEP_SLEEP)
		wakeSetup();
	else if(reason == POWER_REBOOT || reason == HARDWARE_RESET)
		bootSetup();
	else{
		#ifdef DEBUG 
		Serial.println("Could not identify wakeup event");
		#endif
	}

  logData();

	//Serial.begin(9600);
	//Serial.println(micros());

	//update logTime
	ESP.rtcUserMemoryWrite(0, logTime, sizeof(uint32_t));
	//sleep for the measure freq then wake without enabling radio

	//ESP.deepSleep(SAMPLE_INTERVAL_uS-micros(), WAKE_RF_DISABLED); 
	ESP.deepSleep(SAMPLE_INTERVAL_uS-OFFSET_uS, WAKE_RF_DISABLED); 
}
//------------------------------------------------------------------------------
void loop(){//not used as we sleep before we hit this loop
}
