#include <Arduino.h>
#include <Wire.h>

#define LED PC7

#include "HPDL1414.h"
#include "HPDL1414Scroll.h"

#define I2C_ADDRESS 0x15
#define MAX_MESSAGE_SIZE 256
#define MIN_INTERVAL 5
#define MAX_INTERVAL 500
// SDA=PC1, SDA=PC2

const byte dataPins[7] = { PC0, PA2, PA1, PD7, PC7, PD4, PD5 }; // Segment data pins: D0 - D6
const byte addrPins[2] = { PC3, PC4 };                          // Segment address pins: A0, A1
const byte wrenPins[] = { PD6, PC6 };                           // Write Enable pins (left to right)

volatile bool display = true;
volatile uint8_t interval = 0xFF;
//HPDL1414 hpdl(dataPins, addrPins, wrenPins, sizeof(wrenPins));
HPDL1414Scroll hpdl(dataPins, addrPins, wrenPins, sizeof(wrenPins));
String ping = "PING";
String pong = "PONG";

int len;
int stop;

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

      //setMessage(buffer);
      bufferIndex = 0;
    }
  }
  // setScrollSpeed
  else if (command == 0x02)
  {
    uint8_t scrollSpeed = Wire.read();
    interval = map(constrain(scrollSpeed, 0, 100), 100, 0, MIN_INTERVAL, MAX_INTERVAL);
  }
}

// void scrollTextSetMessage(const char *newMessage)
// {
//   strncpy(scrollMessage, newMessage, MAX_MESSAGE_SIZE - 1);
//   scrollMessage[MAX_MESSAGE_SIZE - 1] = '\0';
//   scrollMessageWidth = getTextWidth(scrollMessage);
//   scrollMessageX = MATRIX_WIDTH;
// }

void setup()
{
  if (!hpdl.begin())
  {
    // Serial.println("Can't allocate buffer memory!");
    while (true)
      ;
  }

  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(handleOnReceive);

  len = ping.length();
  stop = (hpdl.segments() * 4) - len;
}

void loop()
{
  hpdl.clear();
  hpdl.print("12345678");
  hpdl.display();
  delay(5000);

  hpdl.clear();
  hpdl.print("abcdefgh");
  hpdl.display();
  delay(5000);

  hpdl.clear();
  hpdl.print("ABCDEFGH");
  hpdl.display();
  delay(5000);

  // // First character on the display
  // hpdl.setCursor(0);
  // hpdl.print(ping);
  // hpdl.display();

  // while (hpdl.getCursor() < stop)
  // {
  //   hpdl.scrollToRight();
  //   hpdl.display();
  //   delay(500);
  // }

  // // Clear again
  // hpdl.clear();
  // // Begin printing on stop-th character on the display
  // hpdl.setCursor(stop);
  // hpdl.print(pong);
  // hpdl.display();

  // while (hpdl.getCursor() > 0)
  // {
  //   hpdl.scrollToLeft();
  //   hpdl.display();
  //   delay(500);
  // }
}