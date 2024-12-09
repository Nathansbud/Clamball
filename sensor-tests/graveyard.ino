
void calibrateIR() {
  int thresh = 100000;
  int value = 0;
  float counter = 0;
  while(value < thresh) {
    float volts0 = analogRead(sensor0) * 0.0048828125;
      // float distance = 13*pow(volts, -1); // worked out from datasheet graph
    float distance0 = 13 * pow(volts0, -1);
      // Serial.println(distance0);
      
    counter += distance0;
    value++;
  }

  // counter /= thresh;
  //Serial.println(numreadings);
  Serial.print("OUR READING WAS: ");
  Serial.println(counter / thresh);
  delay(1); 
}

void calibrateButtons() {
  for(int i = 0; i < NUM_BUTTONS; i++) {
    if(digitalRead(i) == 0) {
      Serial.print("GOLF BALL ON ");
      Serial.println(i);
    }
  }
}


int sensor0_hole() {
  float volts0 = analogRead(sensor0) * 0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13 * pow(volts0, -1);
  // Serial.println(distance0);
  // test ret;

  //4.7 to 5.81 is our average calibrated range
  if(distance0 >= 2 && distance0 < 6.82) {
    // ret.hole = 0;
    // ret.dist = distance0;
    return 0;
  } // 7.83 to 8.72
  else if(distance0 >= 6.82 && distance0  < 9.5){
    // ret.hole = 1;
    // ret.dist = distance0;
    return 1;
  }

  // 10.27 to 11
  else if(distance0 >= 9.5 && distance0 < 11.7){
    // ret.hole = 2;
    // ret.dist = distance0;
    return 2;
  }
  
  // 12.40 to 13.08
  else if(distance0 >= 11.7 && distance0 < 13.7){
    // Serial.println(distance0);
    // ret.hole = 3;
    // ret.dist = distance0;
    return 3;
  }

  // actual 14.33 to 14.88
  else if(distance0 >= 13.7 && distance0 < 15){
    // ret.hole = 4;
    // ret.dist = distance0;
    return 4;
  } else { 
    return -1; 
  }
}

int sensor1_hole() {
  float volts0 = analogRead(sensor1)*0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13*pow(volts0, -1);

  if(distance0 >= 3 && distance0 < 5.5){
    return 0;
  }
  else if(distance0 >= 5.5 && distance0  < 9){
    return 1;
  }
  else if(distance0 >= 9 && distance0 < 11.5){
    return 2;
  }
  else if(distance0 >= 11.5 && distance0 < 14.5){ // struggling with these two
    // Serial.println(distance0);
    return 3;
  }
  else if(distance0 >= 14.5 && distance0 < 18){ // struggling with these two
    // Serial.println(distance0);
    return 4;
  }
  else{
    return 33;
  }
}

int sensor2_hole() {
  float volts0 = analogRead(sensor2)*0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13*pow(volts0, -1);

  if(distance0 >= 3 && distance0 < 5.5){
    return 0;
  }
  else if(distance0 >= 5.5 && distance0  < 9){
    return 1;
  }
  else if(distance0 >= 9 && distance0 < 12){
    return 2;
  }
  else if(distance0 >= 12 && distance0 < 15){
    // Serial.println(distance0);
    return 3;
  }
  else if(distance0 >= 15 && distance0 < 18.5){
    // Serial.println(distance0);
    return 4;
  }
  else{
    // Serial.println(distance0);
    return 33;
  }
}

int sensor3_hole() {
  float volts0 = analogRead(sensor3)*0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13 * pow(volts0, -1);

  if(distance0 >= 3 && distance0 < 5.5){
    return 0;
  } else if(distance0 >= 5.5 && distance0  < 9){
    return 1;
  } else if(distance0 >= 9 && distance0 < 12){
    return 2;
  } else if(distance0 >= 12 && distance0 < 15){
    return 3;
  } else if(distance0 >= 15 && distance0 < 19){
    return 4;
  } else {
    return 33; 
  }
}

int sensor4_hole() {
  float volts0 = analogRead(sensor4)*0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13*pow(volts0, -1);
  // Serial.println(distance0);

  if(distance0 >= 3 && distance0 < 5.5){
    return 0;
  }
  else if(distance0 >= 5.5 && distance0  < 9){
    return 1;
  }
  else if(distance0 >= 9 && distance0 < 12){
    // Serial.println(distance0);
    return 2;
  }
  else if(distance0 >= 12 && distance0 < 15){
    return 3;
  }
  else if(distance0 >= 15 && distance0 < 20){ // not sure about this one 
    return 4;
  }
  else{
    return 33; 
  }
}