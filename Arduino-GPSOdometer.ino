#include <MyFRAM_SPI.h>
#include <NeoSWSerial.h>
#include <NMEAGPS.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>

#define GPSBaud               38400

#define BLACK                 0x0000
#define BLUE                  0x001F
#define RED                   0xF800
#define GREEN                 0x07E0
#define CYAN                  0x07FF
#define MAGENTA               0xF81F
#define YELLOW                0xFFE0 
#define WHITE                 0xFFFF
#define GREY                  0xC618

#define GPS_RX                2
#define GPS_TX                3
#define GPS_MIN_SPEED_kph     5
#define TFT_RST               7
#define TFT_CS                9
#define TFT_DC                8
#define FRAM_CS               6
#define BTN                   5
#define BTN_PRESS_TIME_ms     5000

#define FRAM_ID_ODO           1
#define FRAM_ID_TRIP          2

#define MINUTES(minutes)      60*minutes
#define TRIP_SAVE_TIME_m      1

NeoSWSerial serial(GPS_RX, GPS_TX); // RX, TX
MyFRAM_SPI fram;
Adafruit_ST7735               tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

NMEAGPS gps;
gps_fix fix;
uint16_t                      distance_buf_m = 0;
uint32_t                      odometer_km = 127000;
uint32_t                      trip_m = 0;
uint32_t                      trip_old_m = 0;
NeoGPS::Location_t            last_loc;
bool                          last_loc_okay = false;

static const int32_t          zone_hours = +2L;
static const int32_t          zone_minutes = 0L;
static const NeoGPS::clock_t  zone_offset =
  zone_hours   * NeoGPS::SECONDS_PER_HOUR +
  zone_minutes * NeoGPS::SECONDS_PER_MINUTE;

unsigned long                 btn_press_duration_ms = 0;

unsigned long                 count_s;
unsigned long                 millis_old;
bool                          time_to_save_trip = false;

void setup()
{
  serial.begin(GPSBaud);
  Serial.begin(GPSBaud);

  fram.begin(FRAM_CS);
  fram.setWriteEnableBit(1);

  pinMode(BTN, INPUT);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setCursor(0,0);
  tft.setTextWrap(true);
  tft.setTextColor(BLUE, BLACK);
  tft.setTextSize(1);

  initFram();
}

void loop()
{
  if (gps.available(serial))
  {
    fix = gps.read();
    //if (fix.valid.location && (fix.speed_kph() > GPS_MIN_SPEED_kph))
    if (fix.valid.location)
    {
      if (last_loc_okay)
      {
        float distance_m = fix.location.DistanceKm(last_loc)*1000000;
        distance_buf_m += distance_m;
        trip_m += distance_m;
        if ((distance_buf_m >= 500) ||
          (time_to_save_trip)
        )
        {
          //fram.writeUINT32T(FRAM_ID_TRIP, trip_m);
          time_to_save_trip = false;
          trip_old_m = trip_m;
          count_s = 0;
        }

        if (distance_buf_m >= 1000)
        {
          odometer_km += distance_buf_m/1000;
          distance_buf_m = distance_buf_m % 1;
          //fram.writeUINT32T(FRAM_ID_ODO, odometer_km);
        }
      }
      last_loc   = fix.location;
      last_loc_okay = true;
    }
    printDisplay();
  }

  handleTripReset();
  handleStaticTripSaveTime();
}

void printDisplay()
{
  tft.setCursor(0,0);
  tft.setTextColor(BLUE, BLACK);
  tft.println("Longitude  Latitude");
  tft.setTextColor(convertRGB(255,128,0), BLACK);
  printFloat(fix.longitude(), fix.valid.location, 11, 6);
  printFloat(fix.latitude(), fix.valid.location, 10, 6);
  tft.println("");

  tft.setTextColor(BLUE, BLACK);
  tft.println("Date/Time");
  tft.setTextColor(convertRGB(255,128,0), BLACK);
  bool date_time_valid = fix.valid.date || fix.valid.time;
  printDateTime(fix.dateTime, date_time_valid);
  tft.println("");
  tft.println("");

  tft.setTextColor(BLUE, BLACK);
  tft.println("ODO");
  tft.setTextColor(convertRGB(255,128,0), BLACK);
  printInt(odometer_km, true, 9);
  tft.println("km");

  tft.setTextColor(BLUE, BLACK);
  tft.println("TRIP");
  tft.setTextColor(convertRGB(255,128,0), BLACK);
  printFloat((trip_m/1000.0), true, 9, 1);
  tft.println("km");
  tft.println("");
  
  tft.setTextSize(2);
  tft.setTextColor(convertRGB(255,128,0), BLACK);
  printInt(fix.speed_kph(), fix.valid.speed, 4);
  tft.println("km/h");
  tft.setTextSize(1);
  
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
    {
      tft.print('*');
    }
    tft.print(' ');
  }
  else
  {
    tft.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
    {
      tft.print(' ');
    }
  }
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
  {
    sprintf(sz, "%ld", val);
  }
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
  {
    sz[i] = ' ';
  }
  if (len > 0)
  {
    sz[len-1] = ' ';
  }
  tft.print(sz);
}

static void printDateTime(NeoGPS::time_t dt, bool valid)
{
  if(valid)
  {
    adjustTime(dt);
  }
  if (!valid)
  {
    tft.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", dt.date, dt.month, dt.year);
    tft.print(sz);
  }
  
  if (!valid)
  {
    tft.print(F("******* "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", dt.hours, dt.minutes, dt.seconds);
    tft.print(sz);
  }
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
  {
    tft.print(i<slen ? str[i] : ' ');
  }
}

static void adjustTime(NeoGPS::time_t & dt)
{
  NeoGPS::clock_t seconds = dt;
  seconds += zone_offset;
  dt = seconds;
}

word convertRGB(byte R, byte G, byte B)
{
  return ( ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3) );
}

void initFram()
{
  if (digitalRead(BTN) == HIGH)
  {
    fram.clearAll();
  }

  if (fram.readUINT32T(FRAM_ID_ODO) > odometer_km)
  {
    odometer_km = fram.readUINT32T(FRAM_ID_ODO);
  }
  if (fram.readUINT32T(FRAM_ID_TRIP) > trip_m)
  {
    trip_m = fram.readUINT32T(FRAM_ID_TRIP);
    trip_old_m = trip_m;
  }
}

void handleTripReset()
{
  if (digitalRead(BTN) == HIGH)
  {
    btn_press_duration_ms += millis();
    if (btn_press_duration_ms > BTN_PRESS_TIME_ms)
    {
      trip_m = 0;
      trip_old_m = trip_m;
      fram.writeUINT32T(FRAM_ID_TRIP, trip_m);
    }
  }
  else
  {
    btn_press_duration_ms = 0;
  }
}

void handleStaticTripSaveTime()
{
  if (millis() - millis_old > 1000)
  {
      count_s++;
      millis_old = millis();
      if (count_s > MINUTES(5))
      {
        count_s = 0;
      }
  }

  if (
    count_s >= MINUTES(TRIP_SAVE_TIME_m) &&
    !time_to_save_trip &&
    (trip_old_m != trip_m) &&
    ((trip_m - trip_old_m) > 100)
  )
  {
    time_to_save_trip = true;
  }
    
}
