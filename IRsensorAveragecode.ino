// Define the hole ranges (in distance units)
const float holeRanges[5][2] = {
  {4.00, 4.76},   // Hole 1 range
  {7.14, 7.64},   // Hole 2 range
  {9.66, 10.16},  // Hole 3 range
  {12.03, 12.53}, // Hole 4 range
  {14.21, 14.61}, // Hole 5 range
};

// Wall baseline range (tolerance 1.0)
const float wallBaselineMin = 18.64;
const float wallBaselineMax = 20.64;

// Variables for moving average calculation
const int numReadings = 5;
float readings[numReadings];  // Array to store last 5 sensor readings
int readIndex = 0;            // Current index for storing readings
float total = 0;              // Total of the last 5 readings
float average = 0;            // Moving average of the last 5 readings

// Sensor pin
int sensorPin = A0; // Update with your actual sensor pin

// State tracking
bool ballInHole = false;     // Flag to check if ball is in a hole
int detectedHole = -1;       // Index of the detected hole (-1 means no hole detected)
int holeDetectCount = 0;     // Count to ensure consistent detection of a hole

void setup() {
  // Initialize readings array with 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  // Initialize Serial monitor for debugging
  Serial.begin(9600);
}

void loop() {
  // Read the sensor voltage
  float volts = analogRead(sensorPin) * 0.0048828125;  // Convert ADC value to volts

  // Convert volts to distance (from datasheet formula)
  float distance = 13 * pow(volts, -1);  // Distance in appropriate units
  
  // Update moving average
  total -= readings[readIndex];  // Subtract the last reading
  readings[readIndex] = distance; // Add the new reading
  total += readings[readIndex];  // Update the total
  readIndex = (readIndex + 1) % numReadings; // Move to the next index

  // Calculate the moving average
  average = total / numReadings;

  // Print the average distance for debugging
  Serial.print("Average Distance: ");
  Serial.println(average);

  // Check if the average is within the wall baseline range
  if (average >= wallBaselineMin && average <= wallBaselineMax) {
    Serial.println("Sensor is at the wall.");
    
    // Reset hole detection after wall detection
    ballInHole = false;
    detectedHole = -1; // Reset detected hole
    holeDetectCount = 0; // Reset hole detection count
  } else {
    // Check if the ball is already in a hole
    if (!ballInHole) {
      // Check if the average is within any hole's range
      for (int i = 0; i < 5; i++) {
        if (average >= holeRanges[i][0] && average <= holeRanges[i][1]) {
          // If the average is within the hole range, increment the hole detection count
          holeDetectCount++;
          
          // If the moving average stays within the hole range for 3 readings, trigger detection
          if (holeDetectCount >= 3) {
            ballInHole = true;
            detectedHole = i;
            Serial.print("Ball likely in hole ");
            Serial.println(i + 1);  // Hole numbers are 1-indexed
            break;  // Exit the loop once a hole is detected
          }
          break;  // Exit the loop if the average is within the current hole's range
        }
      }
    } else {
      // If the ball has already entered a hole, don't check for higher holes
      // Only allow lower hole detection
      for (int i = 0; i < detectedHole; i++) {
        if (average >= holeRanges[i][0] && average <= holeRanges[i][1]) {
          holeDetectCount++;
          // If the moving average stays within the hole range for 3 readings, trigger detection
          if (holeDetectCount >= 3) {
            ballInHole = true;
            detectedHole = i;
            Serial.print("Ball likely in hole ");
            Serial.println(i + 1);  // Hole numbers are 1-indexed
            break;
          }
          break;
        }
      }
    }
  }

  // Delay to allow for smoother readings
  delay(20); // Adjust delay as necessary
}
