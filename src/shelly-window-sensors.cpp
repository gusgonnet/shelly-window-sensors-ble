#include "Particle.h"
#include "BeaconScanner.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO);

#define PUSHBULLET_NOTIF_PERSONAL "pushbulletPERSONAL"

enum State
{
  STATE_INIT = 0,
  STATE_DOOR_CLOSED,
  STATE_DOOR_OPEN,
  STATE_DOOR_OPEN_ALARM,
};
State stateRearDoor = STATE_INIT;
unsigned long stateTimeRearDoor;
bool notifSentRearDoor = false;
int rearDoor = -1;

State stateFrontDoor = STATE_INIT;
unsigned long stateTimeFrontDoor;
bool notifSentFrontDoor = false;
int frontDoor = -1;

void rearDoorAlarm();
void frontDoorAlarm();

String getWindowTextFront();
String getWindowTextRear();

void setup()
{
  BLE.on();
  BLE.selectAntenna(BleAntennaType::EXTERNAL);

  Particle.variable("frontDoorState", getWindowTextFront);
  Particle.variable("rearDoorState", getWindowTextRear);
}

void loop()
{
  static unsigned long scannedTime = 0;
  if (Particle.connected() && (millis() - scannedTime) > 100)
  {
    Log.info("Scanning...");
    scannedTime = millis();

    Scanner.scan(1, SCAN_BTHOME);

    Vector<BTHome> beacons = Scanner.getBTHome();
    while (!beacons.isEmpty())
    {
      BTHome beacon = beacons.takeFirst();

      // rear door, replace the mac address with the one of your beacon
      if (strcmp(beacon.getAddress().toString().c_str(), "B0:C7:DE:2C:44:45") == 0)
      {
        rearDoor = beacon.getWindowState();
        if (beacon.getWindowState() == 1)
        {
          Log.info("Door open (garage)");
        }
        else
        {
          Log.info("Door closed (garage)");
        }
      }

      // front door, replace the mac address with the one of your beacon
      else if (strcmp(beacon.getAddress().toString().c_str(), "B0:C7:DE:61:51:52") == 0)
      {
        frontDoor = beacon.getWindowState();
        if (beacon.getWindowState() == 1)
        {
          Log.info("Door open (front)");
        }
        else
        {
          Log.info("Door closed (front)");
        }
      }
      else
      {
        Log.info("Unknown beacon: %s", beacon.getAddress().toString().c_str());
      }
    }
  }

  rearDoorAlarm();
  frontDoorAlarm();
}

void rearDoorAlarm()
{
  // fsm to send alarm to the cloud if door is open for more than two minutes
  switch (stateRearDoor)
  {
  case STATE_INIT:
    if (rearDoor == 1)
    {
      stateRearDoor = STATE_DOOR_OPEN;
      stateTimeRearDoor = millis();
    }
    else if (rearDoor == 0)
    {
      stateRearDoor = STATE_DOOR_CLOSED;
      stateTimeRearDoor = millis();
    }
    break;

  case STATE_DOOR_CLOSED:
    if (rearDoor == 1)
    {
      stateRearDoor = STATE_DOOR_OPEN;
      stateTimeRearDoor = millis();
    }
    break;

  case STATE_DOOR_OPEN:
    if (rearDoor == 0)
    {
      stateRearDoor = STATE_DOOR_CLOSED;
      stateTimeRearDoor = millis();
    }
    else if (millis() - stateTimeRearDoor > 120000)
    {
      stateRearDoor = STATE_DOOR_OPEN_ALARM;
      stateTimeRearDoor = millis();
    }
    break;

  case STATE_DOOR_OPEN_ALARM:
    if (!notifSentRearDoor)
    {
      notifSentRearDoor = true;
      Particle.publish(PUSHBULLET_NOTIF_PERSONAL, "Rear door open", WITH_ACK);
      stateTimeRearDoor = millis();
    }

    if (rearDoor == 0)
    {
      stateRearDoor = STATE_DOOR_CLOSED;
      stateTimeRearDoor = millis();
      notifSentRearDoor = false;
      Particle.publish(PUSHBULLET_NOTIF_PERSONAL, "Rear door closed", WITH_ACK);
    }

    break;
  }
}
void frontDoorAlarm()
{
  // fsm to send alarm to the cloud if door is open for more than two minutes
  switch (stateFrontDoor)
  {
  case STATE_INIT:
    if (frontDoor == 1)
    {
      stateFrontDoor = STATE_DOOR_OPEN;
      stateTimeFrontDoor = millis();
    }
    else if (frontDoor == 0)
    {
      stateFrontDoor = STATE_DOOR_CLOSED;
      stateTimeFrontDoor = millis();
    }
    break;

  case STATE_DOOR_CLOSED:
    if (frontDoor == 1)
    {
      stateFrontDoor = STATE_DOOR_OPEN;
      stateTimeFrontDoor = millis();
    }
    break;

  case STATE_DOOR_OPEN:
    if (frontDoor == 0)
    {
      stateFrontDoor = STATE_DOOR_CLOSED;
      stateTimeFrontDoor = millis();
    }
    else if (millis() - stateTimeFrontDoor > 120000)
    {
      stateFrontDoor = STATE_DOOR_OPEN_ALARM;
      stateTimeFrontDoor = millis();
    }
    break;

  case STATE_DOOR_OPEN_ALARM:
    if (!notifSentFrontDoor)
    {
      notifSentFrontDoor = true;
      Particle.publish(PUSHBULLET_NOTIF_PERSONAL, "Front door open", WITH_ACK);
      stateTimeFrontDoor = millis();
    }

    if (frontDoor == 0)
    {
      stateFrontDoor = STATE_DOOR_CLOSED;
      stateTimeFrontDoor = millis();
      notifSentFrontDoor = false;
      Particle.publish(PUSHBULLET_NOTIF_PERSONAL, "Front door closed", WITH_ACK);
    }

    break;
  }
}

String getWindowTextRear()
{
  switch (rearDoor)
  {
  case 0:
    return "Closed";
    break;
  case 1:
    return "Open";
    break;
  default:
    return "Unknown";
    break;
  }
}
String getWindowTextFront()
{
  switch (frontDoor)
  {
  case 0:
    return "Closed";
    break;
  case 1:
    return "Open";
    break;
  default:
    return "Unknown";
    break;
  }
}
