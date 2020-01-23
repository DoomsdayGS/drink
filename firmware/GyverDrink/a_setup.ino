void setup() {
#if (DEBUG_UART == 1)
  Serial.begin(9600);
  DEBUG("start");
#endif
  // епром
  if (EEPROM.read(1000) != 10) {
    EEPROM.write(1000, 10);
    EEPROM.put(0, thisVolume);
  }
  EEPROM.get(0, thisVolume);
  
  // тыкаем ленту
  FastLED.setBrightness(255);
  strip.setBrightness(255);
  strip.clear();
  strip.show();
  DEBUG("strip init");

  // настройка пинов
  pinMode(PUMP_POWER, 1);
  pinMode(SERVO_POWER, 1);
  for (byte i = 0; i < NUM_SHOTS; i++) {
    pinMode(SW_pins[i], INPUT_PULLUP);
  }

//->OLED
  // старт дисплея
  //disp.clear();
  //disp.brightness(7);
  //DEBUG("disp init");
    // старт дисплея
  u8g2.begin();
  u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
  u8g2.setFont(u8g2_font_unifont_t_cyrillic);  // крупный
  u8g2.clearBuffer();
  u8g2.setCursor(30, 20);
  u8g2.print("Наливатор");   
  u8g2.sendBuffer();
  //<-OLED

  // настройка серво
  servoON();
  servo.attach(SERVO_PIN);
  servo.write(0);
  delay(800);
  servo.setTargetDeg(0);
  servo.setSpeed(60);
  servo.setAccel(0.8);  
  servoOFF();

  serviceMode();    // калибровка
  dispMode();       // выводим на дисплей стандартные значения
  timeoutReset();   // сброс таймаута
  TIMEOUTtimer.start();  
}