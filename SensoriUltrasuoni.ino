#define TRIG1 4
#define ECHO1 5
#define TRIG2 18
#define ECHO2 19
#define TRIG3 21
#define ECHO3 22

float distanze[3];

void setup() {
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);
  Serial.begin(115200);
}

float misuraDistanza(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long durata = pulseIn(echoPin, HIGH, 30000);
  return durata * 0.0343 / 2;
}

void loop() {
  distanze[0] = misuraDistanza(TRIG1, ECHO1);
  delay(50);
  distanze[1] = misuraDistanza(TRIG2, ECHO2);
  delay(50);
  distanze[2] = misuraDistanza(TRIG3, ECHO3);
  delay(50);

  for (int i = 0; i < 3; i++) {
    Serial.print("Distanza ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(distanze[i]);
    Serial.println(" cm");
  }

  delay(500);
}
