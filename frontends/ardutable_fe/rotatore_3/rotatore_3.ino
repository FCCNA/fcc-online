// Code maintained and documented by Lucrezia Borriello for FCC test beam (2025)

/*
STEPPER MOTOR – AVAILABLE COMMANDS
---------------------------------
Recommended operating voltage: 6.4 V
Max supply current: 3.20 A

Commands (send via Serial Monitor):

1. aXX
   → Rotate clockwise by XX degrees.
   Example: "a10" → rotates +10°.

2. iXX
   → Rotate counterclockwise by XX degrees.
   Example: "i15" → rotates -15°.

3. s
   → Reset position to 0° (set current angle as reference).
   Example: "s"

4. return
   → Move back to the 0° reference position.
   Example: "return"

5. pos
   → Print the current position in degrees.
   Example: "pos"

6. setXX
   → Force current position to XX degrees (manual alignment).
   Example: "set90" → sets current angle = 90°.

---------------------------------
📄 Full setup and usage guide:
https://docs.google.com/document/d/1yLf09a52iqJWmRaEvBR44DiIFsmvp0T2C7AQl0-1fHc/edit?usp=sharing
*/



int Pin0 = 10; 
int Pin1 = 12; 
int Pin2 = 11; 
int Pin3 = 13; 

int _step = 0; 
boolean dir = false; // true = orario, false = antiorario

// Specifiche del motore con riduzione
const float motorStepDeg = 1.8;      
const float gearRatio = 50.9;        
float degPerStep = motorStepDeg / gearRatio; // ≈ 0.035363° per step
float positionDeg = 0;               // posizione attuale in gradi
float residualSteps = 0;             // accumula frazioni di step

void setup() {
  Serial.begin(9600);

  pinMode(Pin0, OUTPUT);  
  pinMode(Pin1, OUTPUT);  
  pinMode(Pin2, OUTPUT);  
  pinMode(Pin3, OUTPUT);  

  Serial.println("Ready: aXX, iXX, s, return, pos, setXX");
}

void moveSteps(long steps, boolean direction) {
  dir = direction;
  for (long i = 0; i < steps; i++) {
    switch(_step){ 
      case 0: digitalWrite(Pin0,HIGH); digitalWrite(Pin1,HIGH); digitalWrite(Pin2,LOW); digitalWrite(Pin3,LOW); break;
      case 1: digitalWrite(Pin0,LOW); digitalWrite(Pin1,HIGH); digitalWrite(Pin2,HIGH); digitalWrite(Pin3,LOW); break;
      case 2: digitalWrite(Pin0,LOW); digitalWrite(Pin1,LOW); digitalWrite(Pin2,HIGH); digitalWrite(Pin3,HIGH); break;
      case 3: digitalWrite(Pin0,HIGH); digitalWrite(Pin1,LOW); digitalWrite(Pin2,LOW); digitalWrite(Pin3,HIGH); break;
    }

    if(dir){ _step++; } else { _step--; }
    if(_step>3){ _step=0; }
    if(_step<0){ _step=3; }

    delay(2); // velocità del motore
  }

  // spegne il motore
  digitalWrite(Pin0, LOW);
  digitalWrite(Pin1, LOW);
  digitalWrite(Pin2, LOW);
  digitalWrite(Pin3, LOW);
}

void moveAngle(float angle) {
  float stepsFloat = angle / degPerStep + residualSteps; 
  long steps = round(stepsFloat);                      
  residualSteps = stepsFloat - steps;                 

  moveSteps(abs(steps), steps > 0 ? true : false);
  positionDeg += angle; 
}

void printPosition() {
  Serial.print("Current position: "); 
  Serial.print(positionDeg, 3); // stampa con 3 decimali
  Serial.println("°");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if(cmd.length() == 0) return;

    if(cmd.startsWith("a")){ 
      int angle = cmd.substring(1).toInt();
      moveAngle(angle);
      Serial.print("Moved clockwise "); Serial.print(angle); Serial.println("°");
      printPosition();
    }
    else if(cmd.startsWith("i")){ 
      int angle = cmd.substring(1).toInt();
      moveAngle(-angle);
      Serial.print("Moved counterclockwise "); Serial.print(angle); Serial.println("°");
      printPosition();
    }
    else if(cmd=="s"){ 
      _step = 0;
      positionDeg = 0;
      residualSteps = 0;
      Serial.println("Stepper reset to 0°");
      printPosition();
    }
    else if(cmd=="return"){ 
      float delta = -positionDeg;
      moveAngle(delta);
      Serial.println("Returned to 0°");
      printPosition();
    }
    else if(cmd=="pos"){ 
      printPosition();
    }
    else if(cmd.startsWith("set")){ 
      int newPos = cmd.substring(3).toInt();
      positionDeg = newPos;  // forzo posizione attuale
      residualSteps = 0;     // azzero il residuo
      Serial.print("Position manually set to "); 
      Serial.print(newPos); 
      Serial.println("°");
      printPosition();
    }
    else{
      Serial.println("Command not valid");
    }
  }
}
