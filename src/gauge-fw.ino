#include "elm327.h"
#include "elm327.cpp"
#include "PID.h"

class ArrayMockSerial : public MockSerial {
  char data[20];
  int total = 20;
  int i = 0;

  public:
    void println(String payload) {
      Serial.println("sending:");
      Serial.println(payload);
      i = 0;
      if (payload == "ATI") {
        strcpy(data, " ELM327            ");
      }
      else if (payload == PID_RPM) {
        strcpy(data, " ZZ ZZ 1F  FF      ");
      }
    }
    
    int available() {
      return total - i;
    }
    
    int read() {
      return data[i++];
    }
};

ArrayMockSerial serialMock;
ELM327<ArrayMockSerial> obd(&serialMock);


void setup() {
  Serial.begin(9600);
  while(!Serial){}
  delay(500);
  obd.init();
}

void loop() {
  String bleh = obd.get(PID_RPM);
  Serial.println("Receiving");
  Serial.println(bleh);
  Serial.println(" ");
  delay(1500);
}
