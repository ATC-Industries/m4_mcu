// src/touch/touch.cpp
//
// Resistive touch (XPT2046 via Bodmer TFT_Touch for raw reads) with a 3x3 grid
// bilinear calibration.
//
// Why bilinear and not affine:
//   This panel has nonlinear distortion (axis cross-coupling: raw X shifts with
//   Y position). Measured on-device, a single affine transform leaves 21-27px
//   error at the left/right columns no matter how it is fit (3-point or
//   least-squares 5-point) because the distortion is not planar. A 3x3 grid with
//   inverse-bilinear interpolation gives each cell its own local mapping and
//   brings worst-case to ~9px (verified against this panel's real data).
//
// Why Bodmer is fine for the read:
//   Press-to-press repeatability at a fixed point was measured at 32-48 raw
//   counts spread (~12px) -- a clean, repeatable read. The accuracy problem was
//   the mapping model, not the driver. The discard-first-conversion read keeps
//   per-sample jitter low.
//
// Calibrate in native orientation (SCREEN_ROTATION == 0). LVGL does any 180-deg
// rotation in software, so this layer maps straight to LVGL space.

#include "touch/touch.h"

#include "display/display.h"
#define LOG_TAG "Touch"
#include "Logging.h"

#include <TFT_Touch.h>
#include <math.h>

#define HRES SCREEN_WIDTH
#define VRES SCREEN_HEIGHT

// Grid node screen coordinates (3x3, row-major: index = row*3 + col).
// Placed close to the edges so the calibrated region covers nearly the whole
// panel, touches out to the borders fall inside the grid and map correctly.
// Grid node screen coordinates (3x3, row-major: index = row*3 + col).
// Inset from the true corners so the targets are visible and tappable. Touches
// in the strip outside these nodes are reached by extrapolation (the bounds in
// mapRawBilinear are loose enough to extend to the physical screen edges).
static const int kGridX[3] = {30, HRES / 2, HRES - 30};   // 30, 400, 770
static const int kGridY[3] = {30, VRES / 2, VRES - 30};   // 30, 240, 450
// Aliases used by the calibration draw loop (same coordinates).
#define kGridDrawX kGridX
#define kGridDrawY kGridY

TFT_Touch touch = TFT_Touch(TOUCH_CS_PIN, TOUCH_SCK_PIN, TOUCH_MOSI_PIN, TOUCH_MISO_PIN);
extern LGFX lcd;

bool recalibrateTouch = false;

static TouchCalibration g_cal;
static const char *kPrefsNamespace = "touch_cal";

// Settle time before a deliberate calibration/test capture (ms).
static const uint32_t kCaptureSettleMs = 80;

// ----------------------------------------------------------------------------
// Raw reading
// ----------------------------------------------------------------------------

static int medianOfInts(int *a, int n) {
  for (int i = 1; i < n; i++) {
    int key = a[i], j = i - 1;
    while (j >= 0 && a[j] > key) { a[j + 1] = a[j]; j--; }
    a[j + 1] = key;
  }
  return a[n / 2];
}

static int readAxisSettled(bool yAxis) {
  if (yAxis) { (void)touch.ReadRawY(); return touch.ReadRawY(); }
  else       { (void)touch.ReadRawX(); return touch.ReadRawX(); }
}

bool readTouchRaw(int *rx, int *ry, int samples) {
  if (rx == nullptr || ry == nullptr) return false;
  if (samples < 3) samples = 3;
  if (samples > 15) samples = 15;

  int xbuf[15], ybuf[15];
  int n = 0;
  for (int i = 0; i < samples; i++) {
    int x = readAxisSettled(false);
    int y = readAxisSettled(true);
    if (x > 40 && x < 4055 && y > 40 && y < 4055) { xbuf[n] = x; ybuf[n] = y; n++; }
  }
  if (n < 2) return false;
  *rx = medianOfInts(xbuf, n);
  *ry = medianOfInts(ybuf, n);
  return true;
}

// ----------------------------------------------------------------------------
// Inverse-bilinear mapping (raw -> screen) within the 3x3 grid
// ----------------------------------------------------------------------------

static inline int nodeRawX(int col, int row) { return g_cal.rawX[row * 3 + col]; }
static inline int nodeRawY(int col, int row) { return g_cal.rawY[row * 3 + col]; }

// Solve, within one grid cell, for normalized (u,v) such that the bilinear
// blend of the cell's four raw corners equals (rx,ry). For border touches we
// intentionally allow u/v outside [0,1] so the edge strips extrapolate.
static bool solveCell(int cellCol, int cellRow, double rx, double ry,
                      double *u_out, double *v_out) {
  double X00 = nodeRawX(cellCol,   cellRow),   X10 = nodeRawX(cellCol+1, cellRow);
  double X01 = nodeRawX(cellCol,   cellRow+1), X11 = nodeRawX(cellCol+1, cellRow+1);
  double Y00 = nodeRawY(cellCol,   cellRow),   Y10 = nodeRawY(cellCol+1, cellRow);
  double Y01 = nodeRawY(cellCol,   cellRow+1), Y11 = nodeRawY(cellCol+1, cellRow+1);

  double u = 0.5, v = 0.5;
  for (int it = 0; it < 25; it++) {
    double bx = (1-u)*(1-v)*X00 + u*(1-v)*X10 + (1-u)*v*X01 + u*v*X11;
    double by = (1-u)*(1-v)*Y00 + u*(1-v)*Y10 + (1-u)*v*Y01 + u*v*Y11;
    double dxu = -(1-v)*X00 + (1-v)*X10 - v*X01 + v*X11;
    double dxv = -(1-u)*X00 - u*X10 + (1-u)*X01 + u*X11;
    double dyu = -(1-v)*Y00 + (1-v)*Y10 - v*Y01 + v*Y11;
    double dyv = -(1-u)*Y00 - u*Y10 + (1-u)*Y01 + u*Y11;
    double fx = bx - rx, fy = by - ry;
    double det = dxu*dyv - dxv*dyu;
    if (fabs(det) < 1e-9) break;
    u -= ( dyv*fx - dxv*fy) / det;
    v -= (-dyu*fx + dxu*fy) / det;
  }
  if (!isfinite(u) || !isfinite(v)) return false;
  *u_out = u;
  *v_out = v;
  return true;
}

static double cellPenalty(double u, double v) {
  double penalty = 0.0;
  if (u < 0.0) penalty += -u;
  else if (u > 1.0) penalty += u - 1.0;
  if (v < 0.0) penalty += -v;
  else if (v > 1.0) penalty += v - 1.0;
  return penalty;
}

static bool mapRawBilinear(int rx, int ry, uint16_t *x, uint16_t *y) {
  // Solve all four cells and pick the one requiring the least extrapolation.
  // This keeps border touches stable instead of relying on a single center-node
  // split that can pick the wrong cell near an edge.
  bool found = false;
  int cc = 0, cr = 0;
  double u = 0.0, v = 0.0;
  double bestPenalty = 1e30;

  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 2; col++) {
      double cu, cv;
      if (!solveCell(col, row, rx, ry, &cu, &cv)) continue;
      double penalty = cellPenalty(cu, cv);
      if (!found || penalty < bestPenalty) {
        found = true;
        bestPenalty = penalty;
        cc = col;
        cr = row;
        u = cu;
        v = cv;
      }
    }
  }

  if (!found) return false;

  if (u < -2.0) u = -2.0; if (u > 3.0) u = 3.0;
  if (v < -2.0) v = -2.0; if (v > 3.0) v = 3.0;

  double x0 = kGridX[cc], x1 = kGridX[cc + 1];
  double y0 = kGridY[cr], y1 = kGridY[cr + 1];
  double sx = x0 + u * (x1 - x0);
  double sy = y0 + v * (y1 - y0);

  int ix = (int)lroundf((float)sx), iy = (int)lroundf((float)sy);
  if (ix < 0) ix = 0; if (ix > HRES - 1) ix = HRES - 1;
  if (iy < 0) iy = 0; if (iy > VRES - 1) iy = VRES - 1;
  *x = (uint16_t)ix; *y = (uint16_t)iy;
  return true;
}

bool readTouchMapped(uint16_t *x, uint16_t *y) {
  if (x == nullptr || y == nullptr) return false;
  if (!g_cal.valid) return false;

  int rx, ry;
  if (!readTouchRaw(&rx, &ry, 3)) return false;   // report immediately, no settle
  return mapRawBilinear(rx, ry, x, y);
}

// ----------------------------------------------------------------------------
// Calibration UI (9-point grid)
// ----------------------------------------------------------------------------

static void drawTarget(int x, int y, uint16_t color) {
  lcd.drawLine(x - 12, y, x + 12, y, color);
  lcd.drawLine(x, y - 12, x, y + 12, color);
  lcd.drawCircle(x, y, 8, color);
}

static bool captureNode(int sx, int sy, int *rawx, int *rawy) {
  int rx, ry;
  // ensure released
  while (readTouchRaw(&rx, &ry, 3)) delay(10);
  delay(60);

  uint32_t start = millis();
  while (!touch.Pressed()) {
    delay(8);
    if (millis() - start > 30000) return false;
  }
  delay(kCaptureSettleMs);
  if (!readTouchRaw(rawx, rawy, 15)) return false;

  while (readTouchRaw(&rx, &ry, 3)) delay(10);  // wait for lift
  delay(120);
  return true;
}

bool calibrate_touch() {
  LOGI("Starting touch calibration (3x3 grid / bilinear)...");
  lcd.setRotation(SCREEN_ROTATION);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setFont(&fonts::Font2);

  TouchCalibration cal;
  cal.valid = false;

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      int idx = row * 3 + col;
      // Draw/tap a bit inside the panel so corner targets are fully visible and
      // tappable. The node is stored at the position actually tapped (drawX/Y),
      // and the grid cell coordinates use these same inset positions.
      int sx = kGridDrawX[col], sy = kGridDrawY[row];

      lcd.fillScreen(TFT_BLACK);
      lcd.drawString("TOUCH CALIBRATION", HRES / 2, 30);
      char msg[40];
      snprintf(msg, sizeof(msg), "Point %d of 9  (tap the green target)", idx + 1);
      lcd.drawString(msg, HRES / 2, 54);
      drawTarget(sx, sy, TFT_GREEN);

      if (!captureNode(sx, sy, &cal.rawX[idx], &cal.rawY[idx])) {
        LOGE("Calibration timed out on point %d", idx + 1);
        return false;
      }
      drawTarget(sx, sy, TFT_DARKGREY);
      LOGI("Cal node %d: screen(%d,%d) raw(%d,%d)", idx + 1, sx, sy,
           cal.rawX[idx], cal.rawY[idx]);
    }
  }

  // Sanity: raw must increase left->right across each row, top->bottom down each
  // column. If not, the operator mistapped or axes are swapped -> reject.
  bool ok = true;
  for (int row = 0; row < 3; row++)
    if (!(cal.rawX[row*3+0] < cal.rawX[row*3+1] && cal.rawX[row*3+1] < cal.rawX[row*3+2])) ok = false;
  for (int col = 0; col < 3; col++)
    if (!(cal.rawY[0*3+col] < cal.rawY[1*3+col] && cal.rawY[1*3+col] < cal.rawY[2*3+col])) ok = false;

  if (!ok) {
    LOGE("Calibration node ordering invalid (mistap?). Retrying.");
    lcd.fillScreen(TFT_BLACK);
    lcd.drawString("Calibration looked wrong - try again", HRES / 2, VRES / 2);
    delay(1500);
    return false;
  }

  cal.valid = true;
  g_cal = cal;
  if (!save_touch_calibration(g_cal)) { LOGE("save failed"); return false; }
  setRecalibrationFlag(false);

  // Verify screen: live tracking dot.
  lcd.fillScreen(TFT_BLACK);
  lcd.drawString("Calibration complete - tap to verify", HRES / 2, 30);
  uint32_t end = millis() + 4000;
  while (millis() < end) {
    uint16_t x, y;
    if (readTouchMapped(&x, &y)) { lcd.fillCircle(x, y, 3, TFT_GREEN); end = millis() + 2000; }
    delay(10);
  }

  LOGI("Touch calibration complete and saved.");
  return true;
}

// ----------------------------------------------------------------------------
// Persistence (store 9 nodes)
// ----------------------------------------------------------------------------

bool save_touch_calibration(const TouchCalibration &cal) {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) { LOGE("prefs write open failed"); return false; }
  prefs.putBytes("gridX", cal.rawX, sizeof(cal.rawX));
  prefs.putBytes("gridY", cal.rawY, sizeof(cal.rawY));
  prefs.putBool("valid", cal.valid);
  prefs.end();
  return true;
}

bool load_touch_calibration() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) { LOGW("no touch_cal namespace"); return false; }
  if (!prefs.isKey("valid") || !prefs.getBool("valid", false)) {
    prefs.end(); LOGW("no valid calibration stored"); return false;
  }
  size_t gotX = prefs.getBytes("gridX", g_cal.rawX, sizeof(g_cal.rawX));
  size_t gotY = prefs.getBytes("gridY", g_cal.rawY, sizeof(g_cal.rawY));
  prefs.end();
  if (gotX != sizeof(g_cal.rawX) || gotY != sizeof(g_cal.rawY)) {
    LOGE("stored calibration wrong size"); g_cal.valid = false; return false;
  }
  g_cal.valid = true;
  LOGI("Touch calibration loaded from flash.");
  return true;
}

bool setRecalibrationFlag(bool force) {
  recalibrateTouch = force;
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) { LOGE("prefs flag open failed"); return false; }
  prefs.putBool("recalibrate", force);
  prefs.end();
  LOGI("Recalibration flag set to %s", force ? "true" : "false");
  return true;
}

static bool loadRecalibrationFlag() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) return false;
  bool f = prefs.getBool("recalibrate", false);
  prefs.end();
  return f;
}

// ----------------------------------------------------------------------------
// Boot entry
// ----------------------------------------------------------------------------

void init_touch() {
  touch.setCal(0, 4095, 0, 4095, HRES, VRES, false);
  touch.setRotation(1);

  bool haveCal = load_touch_calibration();
  bool wantRecal = recalibrateTouch || loadRecalibrationFlag();

  if (!haveCal || wantRecal) {
    if (!haveCal) LOGW("Fresh unit - no calibration. Calibrating now.");
    else          LOGI("Recalibration requested. Calibrating now.");
    int attempts = 0;
    while (!calibrate_touch()) {
      attempts++;
      LOGE("Calibration attempt %d failed; retrying.", attempts);
      delay(1000);
    }
  } else {
    LOGI("Using stored touch calibration.");
  }
}

// ----------------------------------------------------------------------------
// Accuracy test (9 targets, off-grid points to test interpolation)
// ----------------------------------------------------------------------------

void touch_accuracy_test() {
  LOGI("=== TOUCH ACCURACY TEST ===");
  lcd.setRotation(SCREEN_ROTATION);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setFont(&fonts::Font2);

  // Targets pushed near the edges/corners to test the newly-covered border
  // region (and deliberately off the calibration grid nodes).
  const int tx[9] = {20, HRES-20, HRES/2, 20, HRES-20, HRES/2, 20, HRES-20, HRES/2};
  const int ty[9] = {20, 20, 12, VRES-20, VRES-20, VRES-12, VRES/2, VRES/2, VRES/2};

  int maxErr = 0, errs[9];

  for (int i = 0; i < 9; i++) {
    lcd.fillScreen(TFT_BLACK);
    char msg[40];
    snprintf(msg, sizeof(msg), "Tap target %d of 9", i + 1);
    lcd.drawString(msg, HRES / 2, 24);
    drawTarget(tx[i], ty[i], TFT_YELLOW);

    int rx, ry;
    while (readTouchRaw(&rx, &ry, 3)) delay(10);
    delay(60);
    uint32_t start = millis();
    bool got = false;
    while (millis() - start < 6000) {            // 6s per target
      if (touch.Pressed()) { got = true; break; }
      delay(5);
    }
    if (!got) {
      // No press registered -> honest miss, not a phantom hit.
      errs[i] = -1;
      LOGW("Target %d: NO TOUCH REGISTERED (edge dead-zone?) intended(%d,%d)",
           i + 1, tx[i], ty[i]);
      continue;
    }
    delay(kCaptureSettleMs);
    if (!readTouchRaw(&rx, &ry, 15)) { errs[i] = -1; LOGW("Target %d: unstable read", i+1); continue; }

    uint16_t mx, my;
    mapRawBilinear(rx, ry, &mx, &my);
    int dx = (int)mx - tx[i], dy = (int)my - ty[i];
    int err = (int)lroundf(sqrtf((float)(dx*dx + dy*dy)));
    errs[i] = err; if (err > maxErr) maxErr = err;

    lcd.fillCircle(mx, my, 4, TFT_RED);     // where you actually hit
    LOGI("Target %d: intended(%d,%d) actual(%d,%d) raw(%d,%d) err=%dpx",
         i + 1, tx[i], ty[i], mx, my, rx, ry, err);

    while (readTouchRaw(&rx, &ry, 3)) delay(10);
    delay(200);
  }

  LOGI("=== ACCURACY SUMMARY ===");
  int hits = 0, hitSum = 0, misses = 0;
  for (int i = 0; i < 9; i++) {
    if (errs[i] < 0) { LOGW("Target %d: MISS (not registered)", i + 1); misses++; }
    else { LOGI("Target %d error: %d px", i + 1, errs[i]); hits++; hitSum += errs[i]; }
  }
  if (hits > 0)
    LOGI("Average error: %d px   Worst: %d px   Misses: %d/9", hitSum / hits, maxErr, misses);
  else
    LOGE("All targets missed.");
  LOGI("=== TEST DONE ===");

  lcd.fillScreen(TFT_BLACK);
  char done[56];
  if (hits > 0)
    snprintf(done, sizeof(done), "Avg %dpx  Worst %dpx  Misses %d/9", hitSum / hits, maxErr, misses);
  else
    snprintf(done, sizeof(done), "All targets missed");
  lcd.drawString(done, HRES / 2, VRES / 2);
  delay(4000);
}
