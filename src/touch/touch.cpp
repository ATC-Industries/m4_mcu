#include "touch/touch.h"
#define HRES SCREEN_WIDTH
#define VRES SCREEN_HEIGHT

// Call up the TFT driver library
#include "display/display.h"
// Call up touch screen library
#include <TFT_Touch.h>

bool recalibrateTouch = false;

// TFT_Touch touch = TFT_Touch(DCS, DCLK, DIN, DOUT);
TFT_Touch touch = TFT_Touch(TOUCH_CS_PIN, TOUCH_SCK_PIN, TOUCH_MOSI_PIN, TOUCH_MISO_PIN);
extern LGFX lcd;

int X_Raw = 0, Y_Raw = 0;

void init_touch() {
  if (!load_touch_calibration()) {
    Serial.println("Touch calibration not found - running calibration...");
    Serial.println("TFT_Touch Calibration, follow TFT screen prompts..");

    while (!calibrate_touch()) {
      Serial.println("Touch calibration failed - trying again...");
      Serial.println("Please follow the TFT screen prompts carefully.");
      delay(2000);  // Brief delay before retrying
    }
    Serial.println("Touch calibration completed successfully");
  } else if (recalibrateTouch) {
    Serial.println("Recalibration requested - running calibration...");
    Serial.println("TFT_Touch Calibration, follow TFT screen prompts..");

    while (!calibrate_touch()) {
      Serial.println("Touch calibration failed - trying again...");
      Serial.println("Please follow the TFT screen prompts carefully.");
      delay(2000);  // Brief delay before retrying
    }
    Serial.println("Touch calibration completed successfully");
  } else {
    Serial.println("Touch calibration loaded successfully.");
  }

  // Set Touch screen to the same landscape orientation
  touch.setRotation(1);
}

bool calibrate_touch() {
  Serial.println("Running touch calibration...");
  lcd.setTextDatum(TC_DATUM);              // Set text plotting reference datum to Top Centre (TC)
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Set text to white on black

  int x1, y1;
  int x2, y2;
  int x3, y3;
  bool xyswap = 0, xflip = 0, yflip = 0;

  Serial.println("TFT_Touch Calibration, follow TFT screen prompts..");

  // Reset the calibration values
  touch.setCal(0, 4095, 0, 4095, 320, 240, 0);  //, 0, 0);

  // Set TFT the screen to landscape orientation
  lcd.setRotation(SCREEN_ROTATION);

  // Set Touch the screen to the same landscape orientation
  touch.setRotation(SCREEN_ROTATION);

  // Clear the screen
  lcd.fillScreen(TFT_BLACK);

  // Show the screen prompt
  drawPrompt();

  drawCross(30, 30, TFT_RED);
  while (!touch.Pressed());
  delay(100);

  getCoord();  // This function assigns values to X_Raw and Y_Raw

  drawCross(30, 30, TFT_BLACK);
  Serial.print("First point : Raw x,y = ");
  Serial.print(X_Raw);
  Serial.print(",");
  Serial.println(Y_Raw);

  x1 = X_Raw;
  y1 = Y_Raw;

  drawCross(HRES / 2, VRES / 2, TFT_RED);
  delay(10);

  while (getCoord());  // This waits for the centre area to be touched

  drawCross(HRES / 2, VRES / 2, TFT_BLACK);
  Serial.print("Second point : Raw x,y = ");
  Serial.print(X_Raw);
  Serial.print(",");
  Serial.println(Y_Raw);

  drawCross(HRES - 30, VRES - 30, TFT_RED);

  while (!getCoord());  // This waits until the centre area is no longer pressed
  delay(10);            // Wait a little for touch bounces to stop after release

  getCoord();
  drawCross(HRES - 30, VRES - 30, TFT_BLACK);
  Serial.print("Third point : Raw x,y = ");
  Serial.print(X_Raw);
  Serial.print(",");
  Serial.println(Y_Raw);

  x2 = X_Raw;
  y2 = Y_Raw;

  drawCross(HRES / 2, VRES / 2, TFT_RED);
  delay(10);

  while (getCoord());  // This waits for the centre area to be touched

  drawCross(HRES / 2, VRES / 2, TFT_BLACK);
  Serial.print("Fourth point : Raw x,y = ");
  Serial.print(X_Raw);
  Serial.print(",");
  Serial.println(Y_Raw);

  drawCross(30, VRES - 30, TFT_RED);

  while (!getCoord());  // This waits until the centre area is no longer pressed
  delay(10);            // Wait a little for touch bounces to stop after release

  getCoord();
  drawCross(30, VRES - 30, TFT_BLACK);
  Serial.print("Fifth point : Raw x,y = ");
  Serial.print(X_Raw);
  Serial.print(",");
  Serial.println(Y_Raw);

  x3 = X_Raw;
  y3 = Y_Raw;

  int temp;
  if (abs(x1 - x3) > 1000) {
    xyswap = 1;
    temp = x1;
    x1 = y1;
    y1 = temp;
    temp = x2;
    x2 = y2;
    y2 = temp;
    temp = x3;
    x3 = y3;
    y3 = temp;
  } else
    xyswap = 0;

  // if (x2 < x1) {
  //   temp = x2; x2 = x1; x1 = temp;
  //   xflip = 1;
  // }

  // if (y2 < y1) {
  //   temp = y2; y2 = y1; y1 = temp;
  //   yflip = 1;
  // }

  int hmin = x1;  // - (x2 - x1) * 3 / (HRES/10 - 6);
  int hmax = x2;  // + (x2 - x1) * 3 / (HRES/10 - 6);

  int vmin = y1;  // - (y2 - y1) * 3 / (VRES/10 - 6);
  int vmax = y2;  // + (y2 - y1) * 3 / (VRES/10 - 6);

  Serial.println();
  Serial.println("  //This is the calibration line to use");
  Serial.print("  touch.setCal(");
  Serial.print(hmin);
  Serial.print(", ");
  Serial.print(hmax);
  Serial.print(", ");
  Serial.print(vmin);
  Serial.print(", ");
  Serial.print(vmax);
  Serial.print(", ");
  Serial.print(HRES);
  Serial.print(", ");
  Serial.print(VRES);
  Serial.print(", ");
  Serial.print(xyswap);  // Serial.print(", ");
  // Serial.print(xflip); Serial.print(", ");
  // Serial.print(yflip);
  Serial.println(");");
  // Save the calibration data
  if (!save_touch_calibration(hmin, hmax, vmin, vmax)) {
    return false;  // Save failed
  }

  // Simple 15-second test
  Serial.println("Touch test - 15 seconds remaining...");
  touch.setRotation(1);
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font2);
  lcd.drawString("Touch test - touch anywhere", lcd.width() / 2, 50);
  lcd.drawString("15 seconds remaining", lcd.width() / 2, 70);

  unsigned long testStart = millis();
  while (millis() - testStart < 15000) {  // 15 seconds
    if (touch.Pressed()) {
      int X_Coord = touch.X();
      int Y_Coord = touch.Y();
      drawCross(X_Coord, Y_Coord, TFT_GREEN);

      // Show coordinates
      lcd.setCursor(lcd.width() / 2, 90);
      lcd.print("X=");
      lcd.print(X_Coord);
      lcd.print(" Y=");
      lcd.print(Y_Coord);
      lcd.print("   ");
    }

    // Update countdown
    int remaining = (15000 - (millis() - testStart)) / 1000;
    lcd.setCursor(lcd.width() / 2, 110);
    lcd.print(remaining);
    lcd.print(" seconds left   ");

    delay(100);
  }
  recalibrateTouch = false;  // Reset recalibration flag after successful calibration
  return true;               // Success
}

bool load_touch_calibration() {
  Preferences prefs;
  if (!prefs.begin("touch_cal", true)) {  // true = read-only mode
    Serial.println("Failed to open preferences for touch calibration");
    return false;  // Failed to open preferences
  }

  // Check if calibration data exists by trying to get one key
  if (!prefs.isKey("x_min")) {
    prefs.end();
    Serial.println("No touch calibration data found");
    return false;  // Calibration data doesn't exist
  }

  // Load calibration values
  int hmin = prefs.getInt("x_min", 0);
  int hmax = prefs.getInt("x_max", 0);
  int vmin = prefs.getInt("y_min", 0);
  int vmax = prefs.getInt("y_max", 0);
  int h_res = prefs.getInt("h_res", HRES);
  int v_res = prefs.getInt("v_res", VRES);
  recalibrateTouch = prefs.getBool("recalibrate");

  prefs.end();

  // Validate that we got reasonable values (not all zeros)
  if (hmin == 0 && hmax == 0 && vmin == 0 && vmax == 0) {
    Serial.println("Invalid touch calibration data found");
    return false;  // Invalid calibration data
  }

  // Apply calibration to touch object
  touch.setCal(hmin, hmax, vmin, vmax, h_res, v_res, false);
  Serial.println("Touch calibration loaded successfully");
  return true;  // Success
}

bool save_touch_calibration(int hmin, int hmax, int vmin, int vmax) {
  Preferences prefs;
  if (!prefs.begin("touch_cal", false)) {
    return false;  // Failed to open preferences
  }

  // Store calibration values
  prefs.putInt("x_min", hmin);
  prefs.putInt("x_max", hmax);
  prefs.putInt("y_min", vmin);
  prefs.putInt("y_max", vmax);
  prefs.putInt("h_res", HRES);
  prefs.putInt("v_res", VRES);
  prefs.putBool("recalibrate", recalibrateTouch);  // Store recalibration flag
  prefs.end();

  // Apply calibration to touch object
  touch.setCal(hmin, hmax, vmin, vmax, HRES, VRES, false);

  return true;  // Success
}

// void test(void) {
//   lcd.fillScreen(TFT_BLACK);
//   lcd.setFont(&fonts::Font2);

//   drawCross(30, 30, TFT_WHITE);

//   drawCross(lcd.width() - 30, lcd.height() - 30, TFT_WHITE);

//   int centre = lcd.width() / 2;  // Get and work out x coord of screen centre

//   String text;
//   text += "Screen rotation = ";
//   text += lcd.getRotation();
//   char buffer[30];
//   text.toCharArray(buffer, 30);

//   lcd.drawString(buffer, centre, 50);

//   lcd.drawString("Touch anywhere on screen", centre, 70);
//   lcd.drawString("to test settings", centre, 90);

//   lcd.drawString("Send a character from the", centre, 120);
//   lcd.drawString("IDE Serial Monitor to", centre, 140);
//   lcd.drawString("continue!", centre, 160);

//   while (Serial.available()) Serial.read();  // Empty the serial buffer before we start

//   while (!Serial.available()) {
//     if (touch.Pressed())  // Note this function updates coordinates stored within library variables
//     {
//       /* Read the current X and Y axis as co-ordinates at the last touch time*/
//       // The values returned were captured when Pressed() was called!
//       int X_Coord = touch.X();
//       int Y_Coord = touch.Y();

//       drawCross(X_Coord, Y_Coord, TFT_GREEN);

//       // delay(20);
//       lcd.setCursor(centre, 0);
//       lcd.print("X = ");
//       lcd.print(X_Coord);
//       lcd.print("   ");
//       lcd.setCursor(centre, 20);
//       lcd.print("Y = ");
//       lcd.print(Y_Coord);
//       lcd.print("   ");
//     }
//   }
// }

void drawPrompt(void) {
  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  int centre = lcd.width() / 2;  // Get and work out x coord of screen centre

  lcd.drawString("CALIBRATION", centre, 20);

  lcd.drawString("Touch the green cross accurately", centre, 61);
  lcd.drawString("( using a cocktail stick works well! )", centre, 81);
}

void drawCross(int x, int y, unsigned int color) {
  lcd.drawLine(x - 5, y, x + 5, y, color);
  lcd.drawLine(x, y - 5, x, y + 5, color);
}

bool getCoord() {
  bool Xwait = 1, Ywait = 1;
  int X_Temp1 = 9999, Y_Temp1 = 9999;
  int X_Temp2 = -1, Y_Temp2 = -1;
  X_Raw = -1;
  Y_Raw = -1;

  while (Xwait || Ywait) {
    if (touch.Pressed())  // Note this function updates coordinates stored within library variables
    {
      /* Read the current X and Y axis as co-ordinates at the last touch time*/
      // The values returned were captured when Pressed() was called!
      X_Temp1 = touch.RawX();
      Y_Temp1 = touch.RawY();
    }
    delay(5);
    if (touch.Pressed())  // Note this function updates coordinates stored within library variables
    {
      /* Read the current X and Y axis as co-ordinates at the last touch time*/
      // The values returned were captured when Pressed() was called!
      X_Temp2 = touch.RawX();
      Y_Temp2 = touch.RawY();
    }

#define RAW_ERROR 10

    if ((abs(X_Temp1 - X_Temp2) < RAW_ERROR) && Xwait) {
      X_Raw = (X_Temp1 + X_Temp2) / 2;
      Xwait = 0;
    }
    if ((abs(Y_Temp1 - Y_Temp2) < RAW_ERROR) && Ywait) {
      Y_Raw = (Y_Temp1 + Y_Temp2) / 2;
      Ywait = 0;
    }
  }

  // Check if press is near middle third of screen
  if ((X_Raw > 1365) && (X_Raw < 2731) && (Y_Raw > 1365) && (Y_Raw < 2371)) return 0;

  // otherwise it is near edge for calibration points
  else
    return 1;
}

bool setRecalibrationFlag() {
  recalibrateTouch = true;  // Set to true to force recalibration
  Preferences prefs;
  if (!prefs.begin("touch_cal", false)) {
    Serial.println("Failed to open preferences for recalibration flag");
    return false;  // Failed to open preferences
  }

  // Store calibration values
  prefs.putBool("recalibrate", true);  // Store recalibration flag
  prefs.end();
  Serial.println("Recalibration flag set to true");
  return true;  // Success
}