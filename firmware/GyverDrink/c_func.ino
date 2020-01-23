// различные функции

void serviceMode() {
  if (!digitalRead(BTN_PIN)) {
    //byte serviceText[] = {_S, _E, _r, _U, _i, _C, _E}; //OLED
    //disp.runningString(serviceText, sizeof(serviceText), 150); //OLED
    while (!digitalRead(BTN_PIN));  // ждём отпускания
    delay(200);
    servoON();
    int servoPos = 0;
    long pumpTime = 0;
    timerMinim timer100(100);
    //disp.displayInt(0); //OLED
	srvMode=true;
    DisplayRedraw ("Крути/Жми РУЧКУ",0,20);
    bool flag;
    for (;;) {
      servo.tick();
      enc.tick();

      if (timer100.isReady()) {   // период 100 мс
        // работа помпы со счётчиком
        if (!digitalRead(ENC_SW)) {
          if (flag) pumpTime += 100;
          else pumpTime = 0;
          DisplayRedraw (String("Помпа,время: " +String(pumpTime)),0,23); //disp.displayInt(pumpTime); //OLED
          pumpON();
          flag = true;
        } else {
          pumpOFF();
          flag = false;
        }

        // зажигаем светодиоды от кнопок
        for (byte i = 0; i < NUM_SHOTS; i++) {
          if (!digitalRead(SW_pins[i])) {
            strip.setLED(i, mCOLOR(GREEN));
          } else {
            strip.setLED(i, mCOLOR(BLACK));
          }
		  mayak();
          strip.show();
        }
      }

      if (enc.isTurn()) {
        // крутим серво от энкодера
        pumpTime = 0;
        if (enc.isLeft()) {
          servoPos += 5;
        }
        if (enc.isRight()) {
          servoPos -= 5;
        }
        servoPos = constrain(servoPos, 0, 180);
        DisplayRedraw (String("Серво,угол: " +String(servoPos)),0,23); //disp.displayInt(servoPos); //OLED
        servo.setTargetDeg(servoPos);
      }

      if (btn.holded()) {
        servo.setTargetDeg(0);
		srvMode=false;
        break;
      }
    }
  }
  //disp.clear(); //OLED
  while (!servo.tick());
  servoOFF();
}

// выводим объём и режим
void dispMode() {
/* //OLED	
  disp.displayInt(thisVolume);
  if (workMode) disp.displayByte(0, _A);
  else {
	disp.displayByte(0, _P);
	pumpOFF();
  } //OLED*/
    DisplayRedraw (String(thisVolume)+" мл.",40,20); //OLED
}

// наливайка, опрос кнопок
void flowTick() {
  if (FLOWdebounce.isReady()) {
    for (byte i = 0; i < NUM_SHOTS; i++) {
      if (!digitalRead(SW_pins[i]) && shotStates[i] == NO_GLASS) {  // поставили пустую рюмку
        timeoutReset();                                             // сброс таймаута
        shotStates[i] = EMPTY;                                      // флаг на заправку
        strip.setLED(i, mCOLOR(RED));                               // подсветили
        LEDchanged = true;
        DEBUG("set glass");
        DEBUG(i);
		dispMode(); //OLED
        displayNoGlass = false; //OLED
      }
      if (digitalRead(SW_pins[i]) && shotStates[i] != NO_GLASS) {   // убрали пустую/полную рюмку
        shotStates[i] = NO_GLASS;                                   // статус - нет рюмки
        strip.setLED(i, mCOLOR(BLACK));                             // нигра
        LEDchanged = true;
        timeoutReset();                                             // сброс таймаута
        if (i == curPumping) {
          curPumping = -1; // снимаем выбор рюмки
          systemState = WAIT;                                         // режим работы - ждать
          WAITtimer.reset();
          pumpOFF();                                                  // помпу выкл
		  displayNoGlass = false; //OLED
        }
        DEBUG("take glass");
        DEBUG(i);
      }
    }

    if (workMode) {         // авто
      flowRoutnie();        // крутим отработку кнопок и поиск рюмок
    } else {                // ручной
      if (btn.clicked()) {  // клик!
        systemON = true;    // система активирована
		displayNoGlass=true; //OLED
        timeoutReset();     // таймаут сброшен
      }
      if (systemON) flowRoutnie();  // если активны - ищем рюмки и всё такое
    }
  }
}

// поиск и заливка
void flowRoutnie() {
  if (systemState == SEARCH) {                            // если поиск рюмки
    bool noGlass = true;
    for (byte i = 0; i < NUM_SHOTS; i++) {
      if (shotStates[i] == EMPTY && i != curPumping) {    // поиск
	    TIMEOUTtimer.stop();
        noGlass = false;                                  // флаг что нашли хоть одну рюмку
        curPumping = i;                                   // запоминаем выбор
        systemState = MOVING;                             // режим - движение
        shotStates[curPumping] = IN_PROCESS;              // стакан в режиме заполнения
        servoON();                                        // вкл питание серво
        servo.attach();
        servo.setTargetDeg(shotPos[curPumping]);          // задаём цель
        DEBUG("find glass");
		displayNoGlass = false; //OLED
        DEBUG(curPumping);
        break;
      }
    }
    if (noGlass) {                                        // если не нашли ни одной рюмки
      servoON();
      servo.setTargetDeg(0);                              // цель серво - 0
      if (servo.tick()) {                                 // едем до упора
	  if (displayNoGlass) if (systemON) DisplayRedraw("ПОСТАВЬТЕ РЮМКУ",0,20); //OLED
	  displayNoGlass = false; //OLED
        servoOFF();                                       // выключили серво
        systemON = false;                                 // выключили систему
        DEBUG("no glass");
		//timeoutReset();
      }
    }
  } else if (systemState == MOVING) {                     // движение к рюмке
    if (servo.tick()) {                                   // если приехали
      systemState = PUMPING;                              // режим - наливание
      FLOWtimer.setInterval((long)thisVolume * time50ml / 50);  // перенастроили таймер
      FLOWtimer.reset();                                  // сброс таймера
      pumpON();                                           // НАЛИВАЙ!
      strip.setLED(curPumping, mCOLOR(YELLOW));           // зажгли цвет
      strip.show();
      DEBUG("fill glass");
      DEBUG(curPumping);
    }

  } else if (systemState == PUMPING) {                    // если качаем
    if (FLOWtimer.isReady()) {                            // если налили (таймер)
      pumpOFF();                                          // помпа выкл
      shotStates[curPumping] = READY;                     // налитая рюмка, статус: готов
      strip.setLED(curPumping, mCOLOR(LIME));             // подсветили
      strip.show();
      curPumping = -1;                                    // снимаем выбор рюмки
      systemState = WAIT;                                 // режим работы - ждать
      WAITtimer.reset();
      DEBUG("wait");
    }
  } else if (systemState == WAIT) {
    if (WAITtimer.isReady()) {
      systemState = SEARCH;
      DEBUG("search");
    }
  }
}

// отрисовка светодиодов по флагу (100мс)
void LEDtick() {
  if (LEDchanged && LEDtimer.isReady()) {
    LEDchanged = false;
    strip.show();
  }
}

// сброс таймаута
void timeoutReset() {
  if (!timeoutState) //disp.brightness(7); //OLED
  timeoutState = true;
  TIMEOUTtimer.reset();
  TIMEOUTtimer.start();
  DEBUG("timeout reset");
}

// сам таймаут
void timeoutTick() {
  if (timeoutState && TIMEOUTtimer.isReady()) {
    DEBUG("timeout");
    timeoutState = false;
    //disp.brightness(1); //OLED
    POWEROFFtimer.reset();
    jerkServo();
    if (volumeChanged) {
      volumeChanged = false;
      EEPROM.put(0, thisVolume);
    }
  }

  // дёргаем питание серво, это приводит к скачку тока и powerbank не отключает систему
  if (!timeoutState && TIMEOUTtimer.isReady()) {
    if (!POWEROFFtimer.isReady()) {   // пока не сработал таймер полного отключения
      jerkServo();
    } else {
      disp.clear();
    }
  }
}

void jerkServo() {
  if (KEEP_POWER) {
    //disp.brightness(7); //OLED
    servoON();
    servo.attach();
    servo.write(random(0, 4));
    delay(200);
    servo.detach();
    servoOFF();
    //disp.brightness(1); //OLED
  }
}

void mayak(){
          int led0, led1, led2, led3, led4, led5, led6, led7;
          int satuR = 0;

          int coloR = 0;
          leds[initLED] = CHSV(0, 255, 127);
          leds[initLED + 1] = CHSV(0, 255, 255);
          leds[initLED + 2] = CHSV(0, 255, 128);
          LEDS.show();
          delay(100);
          
          for(int i=0;i<200;i++){
          leds[initLED] = CHSV(0, 255-i, 27+i/2);
          leds[initLED + 1] = CHSV(0, 255-i, 55+i);
          leds[initLED + 2] = CHSV(0, 255-i, 27+i/2);
          LEDS.show();
          delay(10);
          }

      for (int k = 0; k <3;k++){
            for (int i = 1; i<1019; i++){
              if (i == 1){
                led0 = 127;
                led1 = 255;
                led2 = 128;
                led3 = 0;
                led4 = 0;
                led5 = 0;
                led6 = 0;
                led7 = 0;
              }
      
                if (i >1 and i<129){
                  --led0;
                  --led1;
                  ++led2;
                  ++led3;
                  led4 = 0;
                  led5 = 0;
                  led6 = 0;
                  led7 = 0;
                }
                
                if (i >= 129 and i < 257){
                  --led1;
                  --led2;
                  ++led3;
                  ++led4;
                  led0 = 0;
                  led5 = 0;
                  led6 = 0;
                  led7 = 0;
                }
                
                if (i >= 257 and i < 384){
                  --led2;
                  --led3;
                  ++led4;
                  ++led5;
                  led0 = 0;
                  led1 = 0;
                  led6 = 0;
                  led7 = 0;
                }
                
                if (i >= 384 and i < 511){
                  --led3;
                  --led4;
                  ++led5;
                  ++led6;
                  led0 = 0;
                  led1 = 0;
                  led2 = 0;
                  led7 = 0;
                }
                
                if (i >= 511 and i < 639){
                  --led4;
                  --led5;
                  ++led6;
                  ++led7;
                  led0 = 0;
                  led1 = 0;
                  led2 = 0;
                  led3 = 0;
                } 
                
                if (i >= 639 and i < 765){
                  --led5;
                  --led6;
                  ++led7;
                  ++led0;
                  led1 = 0;
                  led2 = 0;
                  led3 = 0;
                  led4 = 0;
                } 
                if (i >= 765 and i < 893){
                  --led6;
                  --led7;
                  ++led0;
                  ++led1;
                  led2 = 0;
                  led3 = 0;
                  led4 = 0;
                  led5 = 0;
                } 
                
                if (i >= 893 and i < 1019){
                  --led7;
                  --led0;
                  ++led1;
                  ++led2;
                  led3 = 0;
                  led4 = 0;
                  led5 = 0;
                  led6 = 0;
                }
                
                leds[initLED] = CHSV(coloR, satuR, led0);
                leds[initLED + 1] = CHSV(coloR, satuR, led1);
                leds[initLED + 2] = CHSV(coloR, satuR, led2);
                leds[initLED + 3] = CHSV(coloR, satuR, led3);
                leds[initLED + 4] = CHSV(coloR, satuR, led4);
                leds[initLED + 5] = CHSV(coloR, satuR, led5);
                leds[initLED + 6] = CHSV(coloR, satuR, led6);
                leds[initLED + 7] = CHSV(coloR, satuR, led7);
                LEDS.show();
            }
        }
         for(int i=0;i<255;i++){
          leds[initLED] = CHSV(0, 255, 127-i/2);
          leds[initLED + 1] = CHSV(0, 255, 255-i);
          leds[initLED + 2] = CHSV(0, 255, 127-i/2);
          delay(10);
          LEDS.show();
          }
          one_color_all(0, 0, 0);          // погасить все светодиоды
          LEDS.show();                     // отослать команду
          delay(2000);

}

void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
  for (int i = initLED ; i < LED_COUNT; i++ ) {
    leds[i].setRGB( cred, cgrn, cblu);
  }
}