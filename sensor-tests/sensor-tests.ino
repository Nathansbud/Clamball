// Sharp IR GP2Y0A41SK0F Distance Test
// http://tinkcore.com/sharp-ir-gp2y0a41-skf/

#define sensor A0 // Sharp IR GP2Y0A41SK0F (4-30cm, analog)
// #define sensor2 A1

void setup() {
  Serial.begin(9600); // start the serial port
}

void loop() {
  
  // 5v
  float volts = analogRead(sensor)*0.0048828125;  // value from sensor * (5/1024)
  delay(50);
  // float volts2 = analogRead(sensor2)*0.0048828125;
  float distance = 13*pow(volts, -1); // worked out from datasheet graph
  // float distance2 = 13*pow(volts2, -1);
  delay(50); // slow down serial port 
  // Serial.println();
  
  Serial.print("Sensor 1: ");
  Serial.println(distance);   // print the distance
  
  // Serial.print("Sensor 2: ");
  // Serial.println(distance2);   // print the distance
}
