
/** RF24Mesh_Example.ino by TMRh20
 *
 * This example sketch shows how to manually configure a node via RF24Mesh, and send data to the
 * master node.
 * The nodes will refresh their network address as soon as a single write fails. This allows the
 * nodes to change position in relation to each other and the master node.
 */


#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <EEPROM.h>
#include <Servo.h>
#include "SoftReset.h"
//#include <printf.h>


/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

Servo servoPorte;

/**
 * User Configuration: nodeID - A unique identifier for each radio. Allows addressing
 * to change dynamically with physical changes to the mesh.
 *
 * In this example, configuration takes place below, prior to uploading the sketch to the device
 * A unique value from 1-255 must be configured for each node.
 * This will be stored in EEPROM on AVR devices, so remains persistent between further uploads, loss of power, etc.
 *
 **/


uint32_t displayTimer = 0;
boolean saveSettings(uint8_t timeOpen, uint8_t valMax, uint8_t valMin);

void setup() {
  Serial.begin(115200);
  uint8_t writeID = registerNetwork();
  mesh.setNodeID(writeID);
  // Connect to the mesh
  Serial.print(F("NodeID = "));
  Serial.println(mesh.getNodeID());
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin();
  Serial.print(F("NodeID = "));
  Serial.println(mesh.getNodeID());
  servoPorte.attach(9);
}

void loop() {

  mesh.update();

  // Send to the master node every second
  if (millis() - displayTimer >= 1000) {
    displayTimer = millis();

    // Send an 'M' type message containing the current millis()
    if (!mesh.write(&displayTimer, 'M', sizeof(displayTimer))) {

      // If a write fails, check connectivity to the mesh network
      if ( ! mesh.checkConnection() ) {
        //refresh the network address
        Serial.println("Renewing Address");
        mesh.renewAddress();
      } else {
        Serial.println("Send fail, Test OK");
      }
    } else {
      Serial.print("Send OK: "); Serial.println(displayTimer);
    }
  }

  while (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);
    switch(header.type){
      // Display the incoming millis() values from the sensor nodes
    case 'S':
    {
      struct settings_t {
        uint8_t valMin;
        uint8_t valMax;
        uint8_t timeOpen;
      };
      settings_t settings;
      boolean answer = false;
      network.read(header,&settings,sizeof(settings));
      answer = saveSettings(settings.timeOpen, settings.valMax, settings.valMin);
      mesh.write(&answer, 'S', sizeof(answer), header.from_node);
      break;
    }
    case 'O':
    {
      boolean action = false;
      network.read(header,&action,sizeof(action));
      if(action)
        //openDoor();
        servoPorte.write(90);
      else
        //closeDoor();
        servoPorte.write(0);
      break;
    }
    default: 
      network.read(header,0,0); 
      Serial.println(header.type);
      break;
    }
  }
}

boolean saveSettings(uint8_t timeOpen, uint8_t valMax, uint8_t valMin){
  EEPROM.write(110,valMin);
  EEPROM.write(120,valMax);
  EEPROM.write(130,timeOpen);
  return true;
}

uint8_t registerNetwork(){
  if(EEPROM.read(100) > 0 &&  EEPROM.read(100) < 255){
    Serial.print(F("read eeprom 100 : "));
    Serial.println(uint8_t(EEPROM.read(100)));
    return uint8_t(EEPROM.read(100));
  }
  else {
    mesh.setNodeID(255);
    Serial.println(F("ID 255"));
    delay(50);
    Serial.println(F("Connecting to the mesh..."));
    mesh.begin();
    boolean active = true;
    mesh.write(&displayTimer, 'N', sizeof(displayTimer));
    uint8_t myID = 255;
    while(active){
      mesh.update();
      while (network.available()) {
        RF24NetworkHeader header;
        network.read(header, &myID, sizeof(myID));
        Serial.print("New ID : ");
        Serial.println(myID);
        active = false;
      }
    }
    delay(500);
    EEPROM.write(100,myID);
    Serial.print(F("Let's reset"));
    soft_restart();
  }
}






