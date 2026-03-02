#include <math.h>

// Pin Definitions
#define PWM_L      9   // Q1 (Left High)
#define PWM_L_COMP 10  // Q2 (Left Low)
#define PWM_R      11  // Q3 (Right High)
#define PWM_R_COMP 12  // Q4 (Right Low)

#define SINE_STEPS 100
uint16_t sineTable[SINE_STEPS];
const int carrier_top = 400; // ~20kHz at 16MHz clock

void setup() {
  pinMode(PWM_L, OUTPUT);
  pinMode(PWM_L_COMP, OUTPUT);
  pinMode(PWM_R, OUTPUT);
  pinMode(PWM_R_COMP, OUTPUT);

  // --- Timer 1 Setup (Pins 9 & 10) ---
  TCCR1A = 0b10100010; // Phase Correct PWM, TOP=ICR1
  TCCR1B = 0b00011001; // WGM 10, No prescaler
  ICR1 = carrier_top;

  // --- Timer 2 Setup (Pins 11 & 12) ---
  // Using Pin 11 & 12 requires careful direct register manipulation 
  // because they belong to different timers, but for efficiency, 
  // we will use a synchronized software-update approach.
  
  // Pre-calculate Sine Table (Scaled to ICR1)
  for (int i = 0; i < SINE_STEPS; i++) {
    // Standard Sinusoidal values
    float val = (sin(2 * PI * i / SINE_STEPS) + 1.0) / 2.0; 
    sineTable[i] = val * carrier_top;
  }
}

void loop() {
  int mod = analogRead(A0);
  float scale = mod / 1023.0;

  for (int i = 0; i < SINE_STEPS; i++) {
    uint16_t pwm_val = sineTable[i] * scale;
    
    // Half-cycle Logic for H-Bridge (Unipolar)
    if (i < SINE_STEPS / 2) {
      // POSITIVE HALF CYCLE: 
      // Q1 (PWM_L) Switches, Q4 (PWM_R_COMP) stays ON
      // Q2 and Q3 stay OFF
      OCR1A = pwm_val;  // Pin 9
      OCR1B = 0;        // Pin 10
      digitalWrite(PWM_R, LOW);      // Pin 11
      digitalWrite(PWM_R_COMP, HIGH); // Pin 12
    } 
    else {
      // NEGATIVE HALF CYCLE:
      // Q3 (PWM_R) Switches, Q2 (PWM_L_COMP) stays ON
      // Q1 and Q4 stay OFF
      OCR1A = 0;        // Pin 9
      OCR1B = 1;        // High-ish for Deadtime logic (Pin 10)
      digitalWrite(PWM_L_COMP, HIGH); // Pin 10
      digitalWrite(PWM_L, LOW);       // Pin 9
      
      // Note: For Pin 11/12, we use simplified logic to match Timer1
      analogWrite(PWM_R, pwm_val); 
      digitalWrite(PWM_R_COMP, LOW);
    }

    // This delay determines the output frequency (e.g., 50Hz or 60Hz)
    // For 50Hz: 1 / (50 * 100 steps) = 200us
    delayMicroseconds(200); 
  }
}