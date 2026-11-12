
/*
  ==========================================
       CROENIX V2.2 - SD MUSIC PLAYER
  ==========================================

  ESP32-WROOM-32
  ST7735 128x160 Landscape
  PCM5102 I2S DAC
  MicroSD SPI
  Rotary Encoder

  Formats: MP3 / FLAC / WAV

  Controls:
  Rotate Library = Browse tracks
  Short Click    = Open selected track
  Short Click    = Play / Pause
  Long Press     = Back to library
  Rotate Player  = Volume

  SD Scanner: Recursive (all folders)
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <TJpg_Decoder.h>

#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorFLAC.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// ==========================================
// PIN CONFIGURATION
// ==========================================

#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4

#define SD_CS     15

#define ENC_CLK   32
#define ENC_DT    33
#define ENC_SW    25

#define I2S_BCLK  26
#define I2S_LRCK  22
#define I2S_DOUT  27

// ==========================================
// COLORS RGB565
// ==========================================

#define BG          0x100C
#define PANEL       0x210F
#define PURPLE      0x801F
#define PINK        0xF8B2
#define CYAN        0x07FF
#define WHITE       0xFFFF
#define GREY        0xBDF7
#define DARKGREY    0x632C
#define LIGHTGREY   0xC618

// ==========================================
// DISPLAY
// ==========================================

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// ==========================================
// AUDIO
// ==========================================

AudioFileSourceSD *source = nullptr;
AudioFileSourceID3 *id3 = nullptr;

AudioGeneratorMP3 *mp3 = nullptr;
AudioGeneratorFLAC *flac = nullptr;
AudioGeneratorWAV *wav = nullptr;

AudioOutputI2S *audioOut = nullptr;

// ==========================================
// MUSIC DATABASE
// ==========================================

#define MAX_SONGS 200
#define MAX_SCAN_DEPTH 8

String songList[MAX_SONGS];

int songCount = 0;
int selectedSong = 0;
int firstVisible = 0;

// ==========================================
// PLAYER STATE
// ==========================================

enum ScreenMode {
  LIBRARY_SCREEN,
  PLAYER_SCREEN
};

ScreenMode currentScreen = LIBRARY_SCREEN;

bool sdReady = false;
bool playing = false;
bool paused = false;

float volume = 0.65f;

String currentPath = "";
String trackTitle = "";
String trackArtist = "Unknown Artist";
String trackAlbum = "Unknown Album";
String audioFormat = "";

String statusText = "STARTING";

unsigned long playStarted = 0;
unsigned long pausedAt = 0;
unsigned long totalPaused = 0;

unsigned long lastUIUpdate = 0;

// ==========================================
// ENCODER STATE
// ==========================================

int lastCLK = HIGH;

bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

unsigned long debounceTime = 0;
unsigned long pressStart = 0;

bool longPressHandled = false;

#define DEBOUNCE_MS 40
#define LONG_PRESS_MS 700

// ==========================================
// AUDIO DECODER HELPERS
// ==========================================

bool decoderRunning() {

  return
    (mp3 && mp3->isRunning()) ||
    (flac && flac->isRunning()) ||
    (wav && wav->isRunning());
}

void stopDecoder() {

  if (mp3) {
    if (mp3->isRunning()) mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }

  if (flac) {
    if (flac->isRunning()) flac->stop();
    delete flac;
    flac = nullptr;
  }

  if (wav) {
    if (wav->isRunning()) wav->stop();
    delete wav;
    wav = nullptr;
  }

  if (id3) {
    delete id3;
    id3 = nullptr;
  }

  if (source) {
    source->close();
    delete source;
    source = nullptr;
  }

  playing = false;
  paused = false;
}

void releasePlayer() {

  stopDecoder();

  if (audioOut) {
    audioOut->SetGain(volume);
  }
}

// ==========================================
// FILE FORMAT DETECTION
// ==========================================

String getExtension(String path) {

  path.toLowerCase();

  int dot = path.lastIndexOf('.');

  if (dot < 0) return "";

  return path.substring(dot);
}

bool isMusicFile(String path) {

  String ext = getExtension(path);

  return ext == ".mp3" ||
         ext == ".flac" ||
         ext == ".wav";
}

// ==========================================
// FULL PATH GENERATOR
// ==========================================

String makeFullPath(String parent, String name) {

  if (name.startsWith("/")) {
    return name;
  }

  if (parent == "/") {
    return "/" + name;
  }

  return parent + "/" + name;
}

// ==========================================
// RECURSIVE SD SCANNER
// ==========================================

void scanDirectory(String folder, int depth) {

  if (depth > MAX_SCAN_DEPTH) return;

  if (songCount >= MAX_SONGS) return;

  File directory = SD.open(folder);

  if (!directory) {
    Serial.print("OPEN FAILED: ");
    Serial.println(folder);
    return;
  }

  if (!directory.isDirectory()) {
    directory.close();
    return;
  }

  while (songCount < MAX_SONGS) {

    File entry = directory.openNextFile();

    if (!entry) break;

    String name = String(entry.name());

    String fullPath = makeFullPath(folder, name);

    bool isDir = entry.isDirectory();

    entry.close();

    if (isDir) {

      Serial.print("FOLDER: ");
      Serial.println(fullPath);

      scanDirectory(fullPath, depth + 1);

    } else if (isMusicFile(fullPath)) {

      songList[songCount] = fullPath;

      Serial.print("FOUND SONG: ");
      Serial.println(fullPath);

      songCount++;
    }
  }

  directory.close();
}

// ==========================================
// SCAN ENTIRE SD CARD
// ==========================================

void scanMusic() {

  songCount = 0;

  Serial.println();
  Serial.println("================================");
  Serial.println("       CROENIX SD SCANNER");
  Serial.println("================================");

  scanDirectory("/", 0);

  Serial.println("--------------------------------");

  Serial.print("TOTAL TRACKS: ");
  Serial.println(songCount);

  if (songCount > 0) {
    statusText = "SD READY";
  } else {
    statusText = "NO MUSIC FOUND";
  }
}

// ==========================================
// ID3 METADATA CALLBACK
// ==========================================

void metadataCallback(
  void *cbData,
  const char *type,
  bool isUnicode,
  const char *string
) {

  String value = "";

  if (isUnicode) {
    string += 2;

    while (*string) {
      value += *string;
      string += 2;
    }
  } else {
    value = String(string);
  }

  String tag = String(type);

  if (tag == "Title") {
    trackTitle = value;
  }

  else if (tag == "Performer") {
    trackArtist = value;
  }

  else if (tag == "Album") {
    trackAlbum = value;
  }

  Serial.printf(
    "METADATA %s: %s\n",
    type,
    value.c_str()
  );
}

// ==========================================
// JPEG CALLBACK
// ==========================================

bool jpegOutput(
  int16_t x,
  int16_t y,
  uint16_t w,
  uint16_t h,
  uint16_t *bitmap
) {

  if (x >= tft.width() || y >= tft.height()) {
    return true;
  }

  tft.drawRGBBitmap(x, y, bitmap, w, h);

  return true;
}

// ==========================================
// ALBUM ART
// ==========================================

void drawAlbumArt() {

  tft.fillRoundRect(
    4, 30, 68, 68, 5, PANEL
  );

  // Default artwork
  tft.setTextColor(PINK);
  tft.setTextSize(3);
  tft.setCursor(24, 48);
  tft.print("C");

  tft.setTextSize(1);
  tft.setTextColor(GREY);
  tft.setCursor(17, 79);
  tft.print("COVER");

  // External JPG fallback
  if (SD.exists("/Artwork/cover.jpg")) {

    TJpgDec.setJpgScale(2);

    TJpgDec.drawSdJpg(
      4, 30, "/Artwork/cover.jpg"
    );
  }
}

// ==========================================
// LIBRARY UI
// ==========================================

void drawLibrary() {

  tft.fillScreen(BG);

  tft.fillRect(0, 0, 160, 23, PANEL);

  tft.setTextColor(PINK);
  tft.setTextSize(2);
  tft.setCursor(5, 4);
  tft.print("CROENIX");

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(115, 8);
  tft.print("SD");

  tft.setTextColor(GREY);
  tft.setCursor(5, 29);

  tft.printf("%d TRACKS", songCount);

  for (int i = 0; i < 5; i++) {

    int index = firstVisible + i;

    if (index >= songCount) break;

    int y = 45 + i * 15;

    if (index == selectedSong) {

      tft.fillRoundRect(
        2, y - 2, 156, 14, 3, PURPLE
      );

      tft.setTextColor(WHITE);

    } else {

      tft.setTextColor(LIGHTGREY);
    }

    String name = songList[index];

    name = name.substring(
      name.lastIndexOf('/') + 1
    );

    if (name.length() > 24) {
      name = name.substring(0, 21) + "...";
    }

    tft.setCursor(6, y);
    tft.print(name);
  }

  tft.drawFastHLine(0, 122, 160, DARKGREY);

  tft.setTextColor(CYAN);
  tft.setCursor(5, 125);
  tft.print("ROTATE  OK SELECT");
}

// ==========================================
// NOW PLAYING UI
// ==========================================

void drawPlayerScreen() {

  tft.fillScreen(BG);

  tft.fillRect(0, 0, 160, 22, PANEL);

  tft.setTextColor(PINK);
  tft.setTextSize(2);
  tft.setCursor(5, 3);
  tft.print("CROENIX");

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(115, 8);
  tft.print("PLAYER");

  drawAlbumArt();

  tft.setTextColor(WHITE);
  tft.setCursor(78, 34);

  String title = trackTitle;

  if (title.length() > 14) {
    title = title.substring(0, 13) + ".";
  }

  tft.print(title);

  tft.setTextColor(GREY);
  tft.setCursor(78, 49);

  String artist = trackArtist;

  if (artist.length() > 14) {
    artist = artist.substring(0, 13) + ".";
  }

  tft.print(artist);

  tft.setTextColor(DARKGREY);
  tft.setCursor(78, 63);

  String album = trackAlbum;

  if (album.length() > 14) {
    album = album.substring(0, 13) + ".";
  }

  tft.print(album);

  tft.setTextColor(PINK);
  tft.setCursor(78, 79);
  tft.print(audioFormat);

  tft.drawFastHLine(5, 105, 150, DARKGREY);

  tft.setTextColor(WHITE);
  tft.setCursor(13, 113);
  tft.print("|<");

  tft.setTextColor(PINK);
  tft.setCursor(65, 113);

  if (!playing) {
    tft.print("SELECT");
  } else {
    tft.print(paused ? "PLAY" : "PAUSE");
  }

  tft.setTextColor(WHITE);
  tft.setCursor(132, 113);
  tft.print(">|");
}

// ==========================================
// OPEN SELECTED TRACK
// ==========================================

void openSelectedTrack() {

  if (songCount == 0) return;

  currentPath = songList[selectedSong];

  trackTitle = currentPath.substring(
    currentPath.lastIndexOf('/') + 1
  );

  trackArtist = "Unknown Artist";
  trackAlbum = "Unknown Album";

  audioFormat = getExtension(currentPath);
  audioFormat.toUpperCase();

  currentScreen = PLAYER_SCREEN;

  drawPlayerScreen();
}

// ==========================================
// PLAY SELECTED TRACK
// ==========================================

void playSelectedSong() {

  if (songCount == 0) return;

  releasePlayer();

  currentPath = songList[selectedSong];

  trackTitle = currentPath.substring(
    currentPath.lastIndexOf('/') + 1
  );

  trackArtist = "Unknown Artist";
  trackAlbum = "Unknown Album";

  audioFormat = getExtension(currentPath);
  audioFormat.toUpperCase();

  source = new AudioFileSourceSD();

  if (!source->open(currentPath.c_str())) {

    Serial.println("FILE OPEN ERROR");

    releasePlayer();

    return;
  }

  bool started = false;

  if (audioFormat == ".MP3") {

    id3 = new AudioFileSourceID3(source);

    id3->RegisterMetadataCB(
      metadataCallback,
      nullptr
    );

    mp3 = new AudioGeneratorMP3();

    started = mp3->begin(id3, audioOut);
  }

  else if (audioFormat == ".FLAC") {

    flac = new AudioGeneratorFLAC();

    started = flac->begin(source, audioOut);
  }

  else if (audioFormat == ".WAV") {

    wav = new AudioGeneratorWAV();

    started = wav->begin(source, audioOut);
  }

  if (!started) {

    Serial.println("DECODER START ERROR");

    releasePlayer();

    return;
  }

  playing = true;
  paused = false;

  playStarted = millis();
  totalPaused = 0;

  audioOut->SetGain(volume);

  Serial.print("NOW PLAYING: ");
  Serial.println(currentPath);

  drawPlayerScreen();
}

// ==========================================
// PAUSE / RESUME
// ==========================================

void togglePause() {

  if (!playing) {

    playSelectedSong();

    return;
  }

  if (!paused) {

    paused = true;

    pausedAt = millis();

    audioOut->SetGain(0.0f);

  } else {

    totalPaused += millis() - pausedAt;

    paused = false;

    audioOut->SetGain(volume);
  }

  drawPlayerScreen();
}

// ==========================================
// NEXT / PREVIOUS
// ==========================================

void nextSong() {

  if (songCount == 0) return;

  selectedSong++;

  if (selectedSong >= songCount) {
    selectedSong = 0;
  }

  playSelectedSong();
}

void previousSong() {

  if (songCount == 0) return;

  selectedSong--;

  if (selectedSong < 0) {
    selectedSong = songCount - 1;
  }

  playSelectedSong();
}

// ==========================================
// ENCODER ROTATION
// ==========================================

void checkRotation() {

  int clk = digitalRead(ENC_CLK);

  if (clk != lastCLK && clk == LOW) {

    int dt = digitalRead(ENC_DT);

    int direction = (dt != clk) ? 1 : -1;

    if (currentScreen == LIBRARY_SCREEN) {

      if (songCount == 0) {
        lastCLK = clk;
        return;
      }

      selectedSong += direction;

      if (selectedSong < 0) {
        selectedSong = songCount - 1;
      }

      if (selectedSong >= songCount) {
        selectedSong = 0;
      }

      if (selectedSong < firstVisible) {
        firstVisible = selectedSong;
      }

      if (selectedSong >= firstVisible + 5) {
        firstVisible = selectedSong - 4;
      }

      drawLibrary();

    } else {

      volume += direction * 0.05f;

      volume = constrain(volume, 0.0f, 1.0f);

      if (!paused && audioOut) {
        audioOut->SetGain(volume);
      }

      tft.fillRect(105, 88, 55, 12, BG);

      tft.setTextColor(CYAN);
      tft.setCursor(105, 90);

      tft.printf("VOL %d", (int)(volume * 100));
    }
  }

  lastCLK = clk;
}

// ==========================================
// ENCODER BUTTON
// ==========================================

void checkButton() {

  bool reading = digitalRead(ENC_SW);

  if (reading != lastButtonReading) {
    debounceTime = millis();
  }

  if (millis() - debounceTime > DEBOUNCE_MS) {

    if (reading != stableButtonState) {

      stableButtonState = reading;

      if (stableButtonState == LOW) {

        pressStart = millis();

        longPressHandled = false;
      }

      else {

        if (!longPressHandled) {

          if (currentScreen == LIBRARY_SCREEN) {

            openSelectedTrack();

          } else {

            togglePause();
          }
        }
      }
    }
  }

  if (
    stableButtonState == LOW &&
    !longPressHandled &&
    millis() - pressStart >= LONG_PRESS_MS
  ) {

    longPressHandled = true;

    if (currentScreen == PLAYER_SCREEN) {

      releasePlayer();

      currentScreen = LIBRARY_SCREEN;

      drawLibrary();
    }
  }

  lastButtonReading = reading;
}

// ==========================================
// ANIMATED VISUALIZER
// ==========================================

void animateWaveform() {

  if (currentScreen != PLAYER_SCREEN) return;

  if (millis() - lastUIUpdate < 100) return;

  lastUIUpdate = millis();

  tft.fillRect(4, 86, 68, 13, BG);

  for (int i = 0; i < 18; i++) {

    int h = random(2, 12);

    int x = 6 + i * 3;

    uint16_t color =
      (i % 3 == 0) ? PINK : PURPLE;

    tft.drawFastVLine(
      x, 98 - h, h, color
    );
  }

  unsigned long elapsed = 0;

  if (playing) {

    elapsed = paused
      ? pausedAt - playStarted - totalPaused
      : millis() - playStarted - totalPaused;
  }

  unsigned long seconds = elapsed / 1000;

  char timeText[10];

  sprintf(
    timeText,
    "%02lu:%02lu",
    seconds / 60,
    seconds % 60
  );

  tft.fillRect(5, 101, 42, 10, BG);

  tft.setTextColor(GREY);
  tft.setTextSize(1);
  tft.setCursor(5, 101);

  tft.print(timeText);
}

// ==========================================
// AUDIO ENGINE
// ==========================================

void serviceAudio() {

  if (!playing || paused) return;

  bool running = false;

  if (mp3) {
    running = mp3->loop();
  }

  else if (flac) {
    running = flac->loop();
  }

  else if (wav) {
    running = wav->loop();
  }

  if (!running) {

    Serial.println("TRACK FINISHED");

    releasePlayer();

    if (songCount > 0) {
      nextSong();
    }
  }
}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  randomSeed(analogRead(34));

  // ENCODER
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  lastCLK = digitalRead(ENC_CLK);

  lastButtonReading = digitalRead(ENC_SW);
  stableButtonState = lastButtonReading;

  // SPI
  SPI.begin(18, 19, 23);

  // TFT
  tft.initR(INITR_BLACKTAB);

  tft.setRotation(1);

  tft.fillScreen(BG);

  TJpgDec.setCallback(jpegOutput);

  tft.setTextColor(PINK);
  tft.setTextSize(2);
  tft.setCursor(10, 40);

  tft.print("CROENIX");

  tft.setTextSize(1);
  tft.setCursor(10, 70);
  tft.setTextColor(WHITE);

  tft.print("INITIALIZING...");

  // SD
  if (!SD.begin(SD_CS, SPI, 10000000)) {

    Serial.println("SD INITIALIZATION FAILED");

    tft.fillScreen(BG);

    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_RED);

    tft.print("SD ERROR");

    return;
  }

  sdReady = true;

  Serial.println("SD READY");

  // AUDIO
  audioOut = new AudioOutputI2S();

  audioOut->SetPinout(
    I2S_BCLK,
    I2S_LRCK,
    I2S_DOUT
  );

  audioOut->SetGain(volume);

  // RECURSIVE SD SCAN
  scanMusic();

  selectedSong = 0;
  firstVisible = 0;

  if (songCount == 0) {

    tft.fillScreen(BG);

    tft.setTextColor(PINK);
    tft.setTextSize(2);
    tft.setCursor(5, 20);

    tft.print("CROENIX");

    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(5, 55);

    tft.print("NO MUSIC FOUND");

    tft.setCursor(5, 75);
    tft.print("Check SD card");

    return;
  }

  currentScreen = LIBRARY_SCREEN;

  drawLibrary();

  Serial.println("CROENIX V2.2 READY");
}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

  checkRotation();

  checkButton();

  serviceAudio();

  animateWaveform();
}
