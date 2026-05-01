#include <Arduino.h>
#include <Wire.h>
#include "HPDL1414.h"

#define I2C_ADDRESS 0x14
#define MAX_MESSAGE_SIZE 400
#define MIN_INTERVAL 50
#define MAX_INTERVAL 1000

// SDA=PC1, SDA=PC2
const byte dataPins[7] = {PC0, PA2, PA1, PD7, PC7, PD4, PD5}; // Segment data pins: D0 - D6
const byte addrPins[2] = {PC3, PC4};                          // Segment address pins: A0, A1
const byte wrenPins[] = {PD6, PC6};                           // Write Enable pins (left to right)

volatile bool display = true;
volatile bool scroll = true;
volatile uint16_t interval = 150;
HPDL1414 hpdl(dataPins, addrPins, wrenPins, sizeof(wrenPins));

static char messageBuffer[MAX_MESSAGE_SIZE];
static uint16_t messageLength = 0;
static uint16_t messageStart = 0;
static int16_t scrollPos = 0;
static uint32_t lastScroll = 0;

void setMessage(const char *newMessage)
{
  int displayWidth = hpdl.segments() * 4;
  int pad = displayWidth;
  int maxCopy = MAX_MESSAGE_SIZE - (2 * pad) - 1;
  int msgLen = strnlen(newMessage, maxCopy);

  int index = 0;

  // prepend spaces
  for (int i = 0; i < pad && index < MAX_MESSAGE_SIZE - 1; i++)
  {
    messageBuffer[index++] = ' ';
  }

  messageStart = index; // offset of first real character

  // copy message
  for (int i = 0; i < msgLen && index < MAX_MESSAGE_SIZE - 1; i++)
  {
    messageBuffer[index++] = newMessage[i];
  }

  // reset after clearing original message
  messageLength = index;

  // append spaces
  for (int i = 0; i < pad && index < MAX_MESSAGE_SIZE - 1; i++)
  {
    messageBuffer[index++] = ' ';
  }

  messageBuffer[index] = '\0';

  scrollPos = 0;
}

void handleOnReceive(int bytesReceived)
{
  if (bytesReceived < 2)
  {
    return;
  }

  static char buffer[MAX_MESSAGE_SIZE];
  static int bufferIndex = 0;

  uint8_t command = Wire.read();

  // setDisplay
  if (command == 0x00)
  {
    display = Wire.read();
  }
  // setMessage
  else if (command == 0x01)
  {
    // read chunk into buffer, discard extra bytes if past buffer size
    while (Wire.available())
    {
      uint8_t byte = Wire.read();
      if (bufferIndex < MAX_MESSAGE_SIZE - 1)
      {
        buffer[bufferIndex++] = byte;
      }
    }
    buffer[bufferIndex] = '\0';

    // last chunk (or buffer overflow)
    if (bufferIndex > 0 && (buffer[bufferIndex - 1] == '\n' || bufferIndex >= MAX_MESSAGE_SIZE - 1))
    {
      if (buffer[bufferIndex - 1] == '\n')
      {
        buffer[--bufferIndex] = '\0';
      }

      setMessage(buffer);
      bufferIndex = 0;
    }
  }
  // setScrollSpeed
  else if (command == 0x02)
  {
    uint8_t scrollSpeed = Wire.read();
    interval = map(constrain(scrollSpeed, 0, 100), 100, 0, MIN_INTERVAL, MAX_INTERVAL);
  }
  // setScrollMode
  else if (command == 0x03)
  {
    scroll = Wire.read();
    scrollPos = 0;
  }
  // showTempMessage
  // else if (command == 0x04) {
  // }
}

void setup()
{
  if (!hpdl.begin())
  {
    while (true)
      ;
  }

  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(handleOnReceive);

  setMessage("Once upon a midnight dreary...");
}

void loop()
{
  if (millis() - lastScroll >= interval)
  {
    lastScroll = millis();
    hpdl.clear();

    if (!display)
      return;

    if (scroll)
    {
      hpdl.print(messageBuffer + scrollPos);
      scrollPos = (scrollPos + 1) % messageLength;
    }
    else
    {
      hpdl.print(messageBuffer + messageStart);
    }
  }
}