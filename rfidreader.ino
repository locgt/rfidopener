/*
  RFID Garage door opener sample code by Ben Miller @VMfoo
  ReadKey function inspired and borrowed in part from the arduino playground
  example: http://playground.arduino.cc/Code/ID12
    
*/

// include the SoftwareSerial library so you can use its functions:
#include <SoftwareSerial.h>  //leaving the hardware serial ports for debugging

#define rxPin 2    //pin to receive data from RFID reader
#define txPin 3    //transmit pin for softserial initialization
#define doorPin 4  //pin to trigger relay

// set up a new serial port
SoftwareSerial RFIDPort(rxPin, txPin);
byte pinState = 0;

// for incoming serial data
int incomingByte = 0;
// character array for the value of the RFID tag
char tagValue[10]; 


//What tag values are authorized
char* goodTags[] = {
  "00000001EE",
  "3D00CF5F0F",
  "3D00CDC8F7",
};
// Calculate the number of tags in the array above
int tagCount = sizeof(goodTags)/sizeof(goodTags[0]);

void setup()  {
  // define pin modes for the opener relay
  pinMode(doorPin, OUTPUT);

  // set the data rate for the SoftwareSerial port
  RFIDPort.begin(9600);
  Serial.begin(38400);  //serial monitor rate
  Serial.println("RFID GDO V0.1");  //hello world
}

void loop() {
  //loop and read
  if ( RFIDPort.available() ) {
      if ( readKey() ) {
       //check tag value
        if(goodTag()){  //if this is allowed
          openDoor();
        } else {
          Serial.println("Bad tag.  Go away."); 
        }
      }
  }
}

int goodTag() {
  for(int i=0; i < tagCount; i++) {  //walk through the tag list
    if(strcmp(tagValue, goodTags[i]) == 0) {
      return 1;
    }   
  }
   return 0;  
}

void openDoor(){
  Serial.println("Opening Door!");
  digitalWrite(doorPin, HIGH);
  delay(500); // half a second is plenty of time to let trigger the contact
  digitalWrite(doorPin, LOW);  
  //to prevent "bounce" or secondary reads if the tag is still close to the reader
  //we delay 3 seconds
  delay(3000);
}


int readKey(){
  byte i         = 0;
  byte val       = 0;
  byte checksum  = 0;
  byte bytesRead = 0;
  byte tempByte  = 0;
  byte tagBytes[6];    // "Unique" tags are only 5 bytes but we need an extra byte for the checksum
//  char tagValue[10]; this is defined globaly to simplify code
  
  if((val = RFIDPort.read()) == 2) {        // Check for header
    bytesRead = 0;
    while (bytesRead < 12) {            // Read 10 digit code + 2 digit checksum
    if (RFIDPort.available()) {
      val = RFIDPort.read();

      // Append the first 10 bytes (0 to 9) to the raw tag value
      if (bytesRead < 10)
      {
        tagValue[bytesRead] = val;
      }

      // Check if this is a header or stop byte before the 10 digit reading is complete
      if((val == 0x0D)||(val == 0x0A)||(val == 0x03)||(val == 0x02)) {
        break;                          // Stop reading
      }

      // Ascii/Hex conversion:
      if ((val >= '0') && (val <= '9')) {
        val = val - '0';
      }
      else if ((val >= 'A') && (val <= 'F')) {
        val = 10 + val - 'A';
      }

      // Every two hex-digits, add a byte to the code:
      if (bytesRead & 1 == 1) {
        // Make space for this hex-digit by shifting the previous digit 4 bits to the left
        tagBytes[bytesRead >> 1] = (val | (tempByte << 4));

        if (bytesRead >> 1 != 5) {                // If we're at the checksum byte,
          checksum ^= tagBytes[bytesRead >> 1];   // Calculate the checksum... (XOR)
        };
      } else {
        tempByte = val;                           // Store the first hex digit first
      };

      bytesRead++;                                // Ready to read next digit
      }
    }
    // Send the result to the host connected via USB
    if (bytesRead == 12) {                        // 12 digit read is complete
      tagValue[10] = '\0';                        // Null-terminate the string

      Serial.print("Tag read: ");
      for (i=0; i<5; i++) {
        // Add a leading 0 to pad out values below 16
        if (tagBytes[i] < 16) {
          Serial.print("0");
        }
        Serial.print(tagBytes[i], HEX);
      }
      Serial.println();
      
      Serial.print("Checksum: ");
      Serial.print(tagBytes[5], HEX);
      Serial.println(tagBytes[5] == checksum ? " -- passed." : " -- error.");
      Serial.println(tagValue);
      Serial.println();
      return 1; //return value to indicate that we read something
    }
  }
  bytesRead=0;
  return 0;
}
