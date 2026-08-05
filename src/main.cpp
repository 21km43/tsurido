/*****************************************************************************/
//  Function:    Get the accelemeter of X/Y/Z axis and print out on the
//                  serial monitor.
//  Usage:       This program is for fishing. Use the accelerometer at the end
//               of the rod to see if the fish is caught.
//  Hardware:    M5StickS3
//  PlatformIO:  platform = espressif32@6.12.0, framework = arduino
//  Author:  Koki Mizumoto (Original: Hideto Manjo)
//  Date:    Aug 5, 2026
//  Version: v0.4
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
/*******************************************************************************/
#include <M5Unified.h>
#include <M5PM1.h>
#include <WiFi.h>
#include "esp_bt.h"

// device select
#define USE_INTERNAL_IMU true // Use internal IMU unit as acc sensor

// basic
#define LCD_ROTATION 0    // 90 * num (degree) [Counterclockwise]
#define SCREENBREATH 12   // LCD brightness (max 12)
#define DELAY 50          // milliseconds
#define BAUDRATE 115200   // Serial communication baud rate
#define CPU_FREQ_HIGH 160 // Set FREQ MHz (normal mode)
#define CPU_FREQ_HIGH 40  // Set FREQ MHz (low energy mode)

// warning
#define ONLINE_BUFFER_SIZE 200 // Buffer size for statisics
                               // (must be define before online.h)

// plot
#define X0 5      // Plot left padding
#define TH_WARN 5 // Warning threshold(sigma)
#define TH_MAX 10 // Maxrange(sigma)

// menu offsets
#define OFFSET_MENU 2  // Menu
#define OFFSET_MAIN 12 // Main

#include "online.h"

// FLAGS
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool plotEnabled = true;
bool lowEnergyMode = false;

// batt (percentage)
int batt_charge = 100;

M5PM1 pm1;

inline double SCALAR(int x, int y, int z)
{
    return std::sqrt(
        x * x +
        y * y +
        z * z);
}

void updateStateBATT()
{
  if (!M5.Power.isCharging() && batt_charge < 10)
  {
    pm1.shutdown();
  }

  if (!lowEnergyMode)
  {
    M5.Display.setCursor(0, OFFSET_MENU);
    if (M5.Power.isCharging() || batt_charge > 20)
    {
      M5.Display.setTextColor(WHITE, BLACK);
    }
    else
    {
      M5.Display.setTextColor(RED, BLACK);
    }
    M5.Display.setTextSize(2);
    M5.Display.printf("%d%%", batt_charge);
  }
}

void read_acc(int *x, int *y, int *z)
{
  if (USE_INTERNAL_IMU)
  {
    auto imu_update = M5.Imu.update();
    if (imu_update)
    {
      auto ImuData = M5.Imu.getImuData();

      int16_t ax, ay, az;
      ax = ImuData.accel.x * 1000.0;
      ay = ImuData.accel.y * 1000.0;
      az = ImuData.accel.z * 1000.0;

      *x = (int)ax;
      *y = (int)ay;
      *z = (int)az;
    }
  }
}

void plot(int *val, double *standard)
{
  static int i = 0;
  static int diff[ONLINE_BUFFER_SIZE] = {};

  int sigma = 0;
  int y0 = 0;
  int y1 = 0;
  int top = (int)TH_MAX * (*standard);
  int height = M5.Display.height() - 5;
  int width = M5.Display.width() - X0;

  if (i == width)
    i = 0;

  diff[i] = *val;

  if (i != 0)
  {
    // plot
    y0 = map((int)(diff[i - 1]), 0, top, height, 0);
    y1 = map((int)(diff[i]), 0, top, height, 0);
    M5.Display.drawLine(i - 1 + X0, y0, i + X0, y1, GREEN);
  }
  else
  {
    // new page
    M5.Display.fillScreen(BLACK);
    updateStateBATT();

    sigma = map((int)TH_WARN * (*standard), 0, top, height, 0);
    M5.Display.drawLine(X0, sigma, width + X0, sigma, YELLOW);
  }

  M5.Display.setTextSize(2);
  M5.Display.setCursor(0, OFFSET_MAIN + 16);
  M5.Display.setTextColor(YELLOW, BLACK);
  M5.Display.printf("Warn %4.0lf ", TH_WARN * (*standard));
  M5.Display.setCursor(0, OFFSET_MAIN + 32);
  M5.Display.setTextColor(GREEN, BLACK);
  M5.Display.printf("Value%4d ", *val);
  i++;
}

bool warn(double *outlier)
{
  static long lastring = 0;
  static bool ring = false;

  if (millis() - lastring > 2000)
  {
    if (*outlier > TH_WARN)
    {
      lastring = millis();
      ring = true;
      // M5.Speaker.setVolume(128);
      // M5.Speaker.tone(880, 1000);
      Serial.println("Warning: Outlier detected!");
    }
    else
    {
      ring = false;
    }
  }

  return ring;
}

int battery_charge()
{
  return M5.Power.getBatteryLevel();
}

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(BAUDRATE);
  Serial.flush();

  setCpuFrequencyMhz(CPU_FREQ_HIGH);

  WiFi.mode(WIFI_OFF);
  btStop();

  M5.Display.setBrightness(SCREENBREATH);
  M5.Display.setRotation(LCD_ROTATION);
  M5.Display.fillScreen(BLACK);

  auto pin_num_sda = M5.getPin(m5::pin_name_t::in_i2c_sda);
  auto pin_num_scl = M5.getPin(m5::pin_name_t::in_i2c_scl);
  M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
  Wire.end();
  Wire.begin(pin_num_sda, pin_num_scl, 100000U);

  // Initialize PM1
  m5pm1_err_t err = pm1.begin(&Wire, M5PM1_DEFAULT_ADDR, pin_num_sda, pin_num_scl, M5PM1_I2C_FREQ_100K);

  if (err == M5PM1_OK)
  {
    Serial.println("PM1 initialization successful");
  }
  else
  {
    Serial.printf("PM1 initialization failed, error code: %d\n", err);
  }

  pm1.setLedEnLevel(false);
}

void loop()
{
  // sensor
  int x = 0;
  int y = 0;
  int z = 0;
  int scalar = 0;
  char msg[128];

  // plot
  int diff = 0;
  double outlier = 0.0;
  double mean = 0.0;
  double standard = 0.0;

  long t = millis();
  long wait = 0;

  // get Accelerometer data
  read_acc(&x, &y, &z);
  scalar = SCALAR(x, y, z);

  OL.get_stat(&scalar, &mean, &standard);
  diff = (int)abs(scalar - mean);
  outlier = diff / standard;

  // get battery charge(%)
  batt_charge = battery_charge();
  updateStateBATT();

  sprintf(msg, "Ax,Ay,Az,A,O:%d,%d,%d,%d,%.2lf", x, y, z, scalar, outlier);
  // Serial.println(msg);

  if (M5.BtnB.wasPressed())
  {
    if (lowEnergyMode)
    {
      setCpuFrequencyMhz(CPU_FREQ_HIGH);
      M5.Display.wakeup();
      M5.Display.setBrightness(SCREENBREATH);
      lowEnergyMode = false;
    }
    else
    {
      setCpuFrequencyMhz(CPU_FRE_LOW);
      M5.Display.sleep();
      lowEnergyMode = true;
    }
  }

  if (!lowEnergyMode)
  {
    if (M5.BtnA.wasPressed())
    {
      plotEnabled = !plotEnabled;
      M5.Display.fillScreen(BLACK);
      updateStateBATT();

      if (!plotEnabled)
      {
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(WHITE, BLACK);
        M5.Display.setCursor(M5.Display.width() / 2 - 44, OFFSET_MAIN + 16);
        M5.Display.printf("Tsurido");
        M5.Display.setTextSize(1);

        M5.Display.setTextColor(YELLOW, BLACK);
        M5.Display.setCursor(0, M5.Display.height() - 20);
        M5.Display.printf("Side button\n"
                      "-> power save");
      }
    }

    warn(&outlier);

    if (plotEnabled)
    {
      plot(&diff, &standard);
    }
    else
    {
      M5.Display.setTextSize(2);
      M5.Display.setCursor(M5.Display.width() / 2 - 8 * 6,
                       M5.Display.height() / 2);
      M5.Display.setTextColor(WHITE, BLACK);
      M5.Display.printf("%5d", scalar);
    }

  }

  M5.update();

  wait = DELAY - (millis() - t);
  if (wait > 0)
    delay(wait);
}
