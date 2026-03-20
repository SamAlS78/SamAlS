/*
 * История создания и описание можно посмотреть по ссылке:
 * https://www.drive2.ru/b/729096331934574012/
 *  
 * 
 * 
 */

//#include <Arduino.h>

#define EB_DEB_TIME  5      // таймаут гашения дребезга кнопки (кнопка)
#define EB_CLICK_TIME 150   // таймаут ожидания кликов (кнопка)
#define EB_HOLD_TIME 400    // таймаут удержания (кнопка)
#define EB_STEP_TIME 200    // таймаут импульсного удержания (кнопка)
#define EB_FAST_TIME 10     // таймаут быстрого поворота (энкодер)
#define EB_TOUT_TIME 500   // таймаут действия (кнопка и энкодер)

#define o_ten 6   // пин подключения ТЭНа
#define o_motor 7 // пин подключения мотора

#include <stdint.h>

#include <GyverNTC.h>
#include <EncButton.h>  // для энкодера
#include <uPID.h>       // ПИД регулятор
uPID pid(D_INPUT | I_SATURATE);    // параметры пид регулятора

#include <EEPROM.h>     // для работы с памятью EEPROM
#include <Blinker.h>  // для пищалки

Blinker o_dinam(8);   // для пищалки

 EncButton eb(2, 3, 4);

#define _LCD_TYPE 1  // для работы с I2C дисплеями
#include <LCD_1602_RUS_ALL.h>
LCD_1602_RUS lcd(0x27, 16, 2);
GyverNTC therm(0, 100000, 3950, 25, 100000); // пин, R термистора, B термистора, базовая температура, R резистора

const uint8_t kn_menu = 11; //количество пунктов меню 

void p_menu_hleb(uint8_t n_men) {   // Заполняем пункты меню --- menu_hleb
             
             lcd.setCursor(0, 0);  // курсор на (столбец, строка)
             switch (n_men) {  
                case 0:    // 1
                     lcd.print (F("Основной    "));
                     break;
                case 1:    // 2
                     lcd.print (F("Пшеничный   "));
                     break;
                case 2:    // 3
                     lcd.print (F("Славянский  "));
                     break;
                case 3:    // 4
                     lcd.print (F("Скорый      "));
                     break;
                case 4:    // 5
                     lcd.print (F("На кефире   "));
                     break;
                case 5:    // 6
                     lcd.print (F("Тесто       "));
                     break;
                case 6:    // 7        
                     lcd.print (F("Джем        "));
                     break;
                case 7:    //  8       важно МЕСТО в меню, последний-4
                     lcd.print (F(" Свой       "));
                     break;
                case 8:    //  9       важно МЕСТО в меню, последний-3
                     lcd.print (F("Мешалка     "));
                     break;
                case 9:    //  10       важно МЕСТО в меню, предпоследний
                     lcd.print (F("Поддерж.тем."));
                     break;
                case 10:    //   11     таймер, важно МЕСТО в меню, должен быть всегда последний в списке, в установках времени и мотора не учитывать
                     lcd.print (F("Уст.таймера "));
                     break;
                }
 
} // конец меню


uint16_t vrem_st[kn_menu-3][13] = {  // ВРЕМЯ на каждуюю стадию [строка][столбец] в СЕКУНДАХ, всего 13 (0-12) стадий, где 0 это таймер отсрочки,
                                   // рецепты перечислены построчно согласно меню
// 1-Таймер* 2-Прогрев 3-1замес 4-Отдых 5-2замес 6-1подъём 7-формовка 8-2подъём 9-Формовка 10-3подъём 11-Выпечка 12-Остывание 13-Подогрев
        {0,      0,      420,     300,    720,    2400,          4,      1560,       4,       3600,       3000,       1200, 10800  }, // 1 Основной       В минутах: 0- 0- 7- 5-12-40-4с-26-4с-60-50-20-3:40 
        {0,   1800,      780,    3000,    720,    1500,          4,      1200,       4,       3600,       3000,       1200, 10800  }, // 2 Пшеничный      В минутах: 0-20-13-50-12-25-4с-20-4с-60-50-20-4:30 
        {0,   1200,      780,    3000,    720,    1200,          4,       900,       4,       2400,       3000,       1200, 10800  }, // 3 Славянский     В минутах: 0-20-13-50-12-20-4с-15-4с-40-50-20-4ч 
        {0,      0,      420,     300,    420,     780,          4,         0,       0,       3120,       2100,          0, 10800  }, // 4 Скорый         В минутах: 0- 0- 7- 5- 7-13-4с- 0- 0-52-35- 0-2ч 
        {0,      0,        0,       0,    600,       0,          0,         0,       0,       1800,       2700,          0, 10800  }, // 5 На кефире         
        {0,      0,      360,     300,    720,    2400,          4,         0,       0,          0,          0,          0,     0  }, // 6 Тесто          В минутах: 0- 0- 6- 5-12-40-4с- 0- 0- 0- 0- 0-0 
        {0,      0,      600,       0,      0,       0,          0,         0,       0,          0,       3600,        600,     0  }, // 7 Джем           В минутах: 0- 0-10- 0- 0- 0- 0- 0- 0- 0-60-10-0 
        {0,   1200,      780,    3000,    720,    1200,          4,       900,       4,       2400,       3000,       1200, 10800  }  // 8 Свой   //по умолчанию как Славянский 
};

uint8_t temp_st[kn_menu-3][13] = {  // ТЕМПЕРАТУРА на каждуюю стадию [строка][столбец] в ГРАДУСАХ, всего 0-12 стадий, где 0 это таймер отсрочки,
                                  // рецепты перечислены построчно согласно меню
        {20,    33,      35,      33,       33,     32,         32,        32,      32,        32,        110,        60,      60  }, // В градусах Основной     1
        {20,    35,      33,      32,       32,     32,         32,        32,      32,        32,        110,        60,      60  }, // В градусах Пшеничный    2
        {20,    35,      33,      32,       32,     32,         32,        32,      32,        32,        110,        60,      60  }, // В градусах Славянский   3
        {20,    33,      35,      33,       33,     32,         32,        32,      32,        32,        110,        60,      60  }, // В градусах Скорый       4
        {20,    33,      33,      33,       40,     40,         40,        40,      40,        40,        105,        60,      60  }, // В градусах На кефире    5
        {20,    33,      33,      33,       33,     33,         34,        34,      34,        34,         34,        34,      35  }, // В градусах ТЕСТО        6
        {20,    20,      98,      20,       20,     20,         20,        20,      20,        20,         98,        98,      20  }, // В градусах ДЖЕМ         7
        {20,    35,      33,      32,       32,     32,         32,        32,      32,        32,        110,        60,      60  }  // 8 Свой   //по умолчанию как  В градусах Славянский   8
};  

const uint8_t motor_st[kn_menu-3][13] = {  // Включение мотора на каждуюю стадию [строка][столбец] в ГРАДУСАХ, всего 0-12 стадий, где 0 это таймер отсрочки,
                                  // рецепты перечислены построчно согласно меню
      { 0,       0,        1,      0,       3,       0,         3,         0,        3,         0,          0,          0,     0  }, //  1 Основной      0-ВЫК 
      { 0,       0,        1,      0,       3,       0,         3,         0,        3,         0,          0,          0,     0  }, //  2 Пшеничный
      { 0,       0,        1,      0,       3,       0,         3,         0,        3,         0,          0,          0,     0  }, //  3 Славянский    1-ИМ/ВКЛ (первые 60 секунд импульс, кроме формовки 7,9)
      { 0,       0,        1,      0,       3,       0,         3,         0,        0,         0,          0,          0,     0  }, //  4 Скорый        2-ИМПУЛЬС 20/80  
      { 0,       0,        0,      0,       1,       0,         0,         0,        0,         0,          0,          0,     0  }, //  5 На кефире 
      { 0,       0,        1,      0,       3,       0,         3,         0,        0,         0,          0,          0,     0  }, //  6 ТЕСТО         3-ВКЛ
      { 0,       0,        2,      0,       0,       0,         0,         0,        0,         0,          2,          2,     0  }, //  7 ДЖЕМ
      { 0,       0,        1,      0,       3,       0,         3,         0,        3,         0,          0,          0,     0  }  //  8 Свой //по умолчанию как Славянский   
};                          

uint16_t  p= 10;    // для ПИД регулятора 2 байта  ячейка 0
uint16_t  i= 20;    // для ПИД регулятора 2 байта  ячейка 2 
uint16_t  d= 5;     // для ПИД регулятора 2 байта  ячейка 4
boolean   p_isp=false;            // флаг использовать пид да/нет  ячейка 6
uint8_t   t_pid=1;   // период вычислений ПИД регулятора секунд / ячейка 47

uint8_t temp_0;             // значение температуры
uint8_t korka=8;            // цвет корки +8 градусов к температуре выпечки ячейка 46

boolean fl_temp_0=true;     // флаг исправности датчика температуры
uint32_t tmr_temp_0;        // переменная таймера неисправности датчика температуры

uint8_t t_pod = 20;         // поддержиние температуры

boolean fl_run=false;       // флаг запущен ли рецепт
boolean fl_run1=false;      // флаг запущен ли процесс где поддерживается температура
uint16_t vrem_temp=1081;    // время сколько поддерживать в минутах 1080 минут - 18 часов // 1081 значит бесконечно поддерживаем и выводим знак бесконечность

uint8_t n_menu, n1_menu  = 0;        // номер пункта меню
uint8_t st_rec = 0;                  //стадия приготовления рецепта

uint32_t time_full  = 0;            // полное время приготовления рецепт+таймер
uint32_t time_rec   = 0;            // время приготовления рецепта
uint32_t time_timer = 0;            // время отсрочки приготовления
uint32_t timer_stad  = 0;           // таймер стадии
uint32_t timer_start = 0;           // таймер старта приготовления ()для обратного отсчёта
uint32_t tmr_tc=0;                  // таймер для фунции моргания точки

uint32_t tmr_ct=0;              // таймер  контроля ШИМ ТЭНа
uint32_t tmr_cm=0;              // таймер  контроля ШИМ мотора
bool flag_ct=true;              //для контроля ШИМ ТЭНа
bool flag_cm=true;              //для контроля ШИМ мотора


void isr() {      //
  eb.tickISR();   // для аппаратной работы энкодера
}                 //


void setup() {
  pinMode(o_ten, OUTPUT);    // выход на ТЭН       D6
  pinMode(o_motor, OUTPUT);  // выход на мотор     D7

    digitalWrite(o_ten, LOW);
    digitalWrite(o_motor, LOW);  

  attachInterrupt(0, isr, CHANGE);  //
  attachInterrupt(1, isr, CHANGE);  // для аппаратной работы энкодера
  eb.setEncISR(true);               // 
  
  eb.setBtnLevel(LOW);
  eb.setEncReverse(0);
  eb.setEncType(EB_STEP4_LOW); // тип энкодера EB_STEP1, EB_STEP2, EB_STEP4_LOW (по умолчанию), EB_STEP4_HIGH. Если ваш энкодер работает странно, смените тип
  eb.counter = 0;              // сбросить счётчик энкодера
  delay(300);                  // Ждем готовности модуля отвечать на запросы.


if (EEPROM.read(0) == 255 && EEPROM.read(4) == 255) // проверка по настройкам p d (если 255- новая)
  {
    EEPROM.write(0, p);                           // записываем в ЕЕПРОМ по умолчанию
    EEPROM.write(2, i);
    EEPROM.write(4, d);
    EEPROM.write(6, p_isp);
    EEPROM.put(7,  temp_st[kn_menu-4]); // пишем в ячейки EEPROM 7-19  температуру  рецепта свой
    EEPROM.put(20, vrem_st[kn_menu-4]); // пишем в ячейки EEPROM 20-45 время стадии рецепта свой
    EEPROM.write(46, korka);
    EEPROM.write(47, t_pid);
  }
else{
  p=EEPROM.read(0);                               // считываем из ЕЕПРОМ сохранённыее значения
  i=EEPROM.read(2);
  d=EEPROM.read(4);
  p_isp=EEPROM.read(6);
  EEPROM.get(7,  temp_st[kn_menu-4]);
  EEPROM.get(20, vrem_st[kn_menu-4]);
  korka=EEPROM.read(46);
  t_pid=EEPROM.read(47);
    }
    
  //          Тест дисплея:
  lcd.init(); // Инициализация дисплея
  lcd.backlight(); //подсветка дисплея ВКЛ
  delay(500);
  //   краcивая   "загузка"   программы   при   старте  :)
  lcd.setCursor(0, 0);  // курсор на (столбец, строка)
  lcd.print (F("-= Здравствуй =-")) ;
  delay(500);
  lcd.setCursor(0, 1 ) ; // курсор на (столбец, строка)
  lcd.print (F("     ХОЗЯИН     ")) ;
  delay(3000);
  lcd.noBacklight(); //подсветка дисплея ВЫКЛ
  lcd.clear() ;
  delay(1000);
  lcd.backlight(); //подсветка дисплея ВК
  //   конец  Тест дисплея


//  Serial.begin(9600); Serial.println("Ok");      // Инициируем передачу данных в монитор последовательного порта на скорости 9600 бод.

//  for (int i = 0; i < 50; i++)  { Serial.println(String(i)+" "+String(EEPROM.read(i)));  }   // проверка что в ячеёках для отладки
  


////////// настройки ПИД /////////
  pid.setKp(p);
  pid.setKi(i);
  pid.setKd(d);
  
  pid.setDt(t_pid);  // переод вызова расчёта 
  pid.outMax = 255;  // максимальное значение ШИМ на пине (8 бит характеристика самой ардуины)
  pid.outMin = 0;    // минимальное значение ШИМ на пине  ()
////////// настройки ПИД конец/////////

 tmr_temp_0 = millis(); // сброс таймера неиспровности датчика температуры датчик температуры
 lcd.clear() ;
}

void loop() {

tek_temp(); // Обновление значения температуры и энкодера с динамиком 
  if  ((temp_0 < 185) && (fl_temp_0 ))   {                         // проверяем работает ли датчик температуры
          tmr_temp_0 = millis();         // сброс таймера неиспровности датчика температуры датчик температуры 

  if (!fl_run) { // если рецепт НЕ запущен выводим меню //начало
             p_menu_hleb(n_menu);       // Вывод пункта менюрецепта из функции
             lcd.setCursor(11, 1);
             switch (n_menu) {               // если в пункте таймер или поддержка, то время меню не выводим 
                case (kn_menu-3):    // мешалка 
                     if (eb.hasClicks(1)) { meshalka(); } // переходим в режим мешалки
                     lcd.print (F("     ")) ;
                     break; 
                case (kn_menu-2):    // поддержания температуры
                     if (eb.hasClicks(1)) {pod_temper();} // переходим в режим поддержания температуры
                     lcd.print (F("     ")) ;
                     break; 
                case (kn_menu-1):    // время таймера
                     if (eb.hasClicks(1)) {ust_timer();}    // переходим в установку таймера
                     if (eb.hasClicks(2)) { time_timer=0; } // 2-клика сброс таймера в 0 
                     lcd.print (TimePrint(time_timer, false)) ;
                     break;    
                default:   // время рецепта из массива + таймер если есть
                     time_rec = TimeRecept();                           // подсчет времени приготовления рецепта
                     time_full = time_rec +time_timer;                 // подсчет времени приготовления рецепта + таймер
                     lcd.print (TimePrint(time_full, false)) ;
                     break;                  
             }

              
             if (eb.right()) { if (n_menu == kn_menu-1)  {n_menu=0;} else n_menu++;} // смена рецептов вправо
             if (eb.left())  { if (n_menu == 0)  {n_menu=kn_menu-1;} else n_menu--;} // смена рецептов влево
             if ((eb.hasClicks(1)) && (n_menu < kn_menu-2)) { 
                                                             if (temp_st[n_menu][10]>100) {ust_korka();} // если выпечка t>100 то устанавливаем цвет корки 
                                                               fl_run=true;                     // Признак запуска готовки RUN
                                                               time_rec = TimeRecept();
                                                               timer_start = millis();
                                                               tmr_ct= millis();
                                                               tmr_cm= millis();
                                                               timer_stad = millis();  // нажали на рецепте, выставляем флаг старта приготовления //сброс таймера стадии 
                                                               flag_ct=true;                //для контроля ШИМ ТЭНа
                                                               flag_cm=true;                //для контроля ШИМ мотора
                                                               o_dinam.blink(1, 150, 200);  // активная пищалка пискнуть 1 раз, 150мс вкл, 200мс выкл                                                               
                                                                                                                                                              
               }        

            if (eb.hasClicks(5)) { ust_pid();}   // если не запущен и жмём 5 раз запускаем настройки ПИД регулятора

            if ((eb.hasClicks(2)) && (n_menu==(kn_menu-4))) { ust_svoy();}   // если не запущен и стоим меню на "Свой" и жмём 2 жмём запускаем настройки рецепта Свой
             
              
              
                                
  }            // если рецепт НЕ запущен выводим меню //конец

  if (time_timer>0) {            // если таймер не равен нулю выводим "Т" на экран
         lcd.setCursor(10, 1);  // курсор на (столбец, строка)
         lcd.print (F("Т") ) ;  
                     } else { lcd.setCursor(10, 1); lcd.print (F(" ")) ;}
                            
 
  if (eb.hold()) { if ( fl_run) {o_dinam.blink(2, 150, 150);} // долгое нажатие остановить приготовление //  активная пищалка пискнуть 1 раз, 150мс вкл, 200мс выкл
                   fl_run=false;   
                
  }   
  
 if (fl_run) { // если рецепт запущен выводим меню //начало

         lcd.setCursor(12, 0);  lcd.print (F(" RUN") ) ;    // курсор на (столбец, строка) отображение ссотояния RUN 
         lcd.setCursor(11, 1);  
          if (st_rec < 12){ lcd.print (   TimePrint( (time_full-((millis()-timer_start)/1000) ), true)   ); }  // обратный отсчёт времени приготовления (кроме подогрева вконце)         
          else            { lcd.print (   TimePrint( (vrem_st[n_menu][st_rec]-((millis()-timer_stad)/1000)), true)  ); }// обратный отсчёт времени подогрева вконце   
        
        
       switch (st_rec) { // cтадии рецепта
           case 0: {   // 1- Таймер отсрочки
            digitalWrite(o_motor, LOW);           // мотор не нужен на всякий случай стопорим
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
           
            if (((millis() - timer_stad)/1000) >= time_timer) {
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              time_timer=0;
              timer_stad = millis();
              tmr_cm = millis();

            }
           } break;
           case 1:{    // 2- Прогрев      
            digitalWrite(o_motor, LOW);           // мотор не нужен на всякий случай стопорим
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
           
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
           }break;
           case 2:{    // 3- 1й замес    ДЖЕМ  ((temp_st[n_menu][st_rec]<100) ? 1 : 2)

            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
            contr_motor(motor_st[n_menu][st_rec]); // управление мотором      

             if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
           }break;
          case 3:{     // 4- Отдых
            digitalWrite(o_motor, LOW);           // мотор не нужен на всякий случай стопорим
              contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
           
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
          }break;
           case 4:{    // 5- 2й замес
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
            contr_motor(motor_st[n_menu][st_rec]); // управление мотором   

            if (((millis() - timer_stad)/1000) == vrem_st[n_menu][st_rec]-410 ) {o_dinam.blink(3, 350, 200); } // Сигнал для добавок за 410 секуд до окончания 6,8 минут  активная пищалка пискнуть 3 раз, 350мс вкл, 200мс выкл
            
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
           }break;
           case 5:{   // 6- 1й подъём
            digitalWrite(o_motor, LOW);           // мотор не нужен на всякий случай стопорим
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
           
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
           }break;
           case 6:{    // 7- Формовка
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
            contr_motor(motor_st[n_menu][st_rec]); // управление мотором  
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
           }break;
           case 7:{    // 8- 2й подъём
            digitalWrite(o_motor, LOW);           // мотор не нужен на всякий случай стопорим
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
           
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
           } break;
           case 8:{    // 9- Формовка
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
            contr_motor(motor_st[n_menu][st_rec]); // управление мотором  
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
           }break;
          case 9:{     // 10- 3й подъём
            digitalWrite(o_motor, LOW);           // мотор не нужен на всякий случай стопорим
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
           
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
          }break;
           case 10:{    // 11- Выпечка   ДЖЕМ     
            if (temp_st[n_menu][st_rec]>100) {contr_temper(temp_st[n_menu][st_rec]+korka);} else {contr_temper(temp_st[n_menu][st_rec]);}//контроль температуры tp-поддерживаемая темпрература если выпечка t>100 то прибавляем корку 
            contr_motor(motor_st[n_menu][st_rec]); // управление мотором  

            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
            }
           }break;
           case 11:{   //  12- Остывание   ДЖЕМ    
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
            contr_motor(motor_st[n_menu][st_rec]); // управление мотором   
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              timer_stad = millis();
              st_rec++;        // таймер иссяк увеличиваем номер стадии для перехода на следующую  
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
              o_dinam.blink(4, 400, 200);  // активная пищалка пискнуть 4 раз, 400мс вкл, 200мс выкл
            }
           }break;
           case 12:{   //  13- Подогрев
            lcd.setCursor(0, 0); lcd.print(F("ГОТОВО подог"));
            digitalWrite(o_motor, LOW);           // мотор не нужен на всякий случай стопорим
            contr_temper(temp_st[n_menu][st_rec]); //контроль температуры tp-поддерживаемая темпрература
           
            if (((millis() - timer_stad)/1000) >= vrem_st[n_menu][st_rec]) {
              digitalWrite(o_ten, LOW);
              digitalWrite(o_motor, LOW);
              tmr_cm = millis();
              
              st_rec=0;        // таймер иссяк обнуляем шаг
              fl_run=false;    // остановлтваем приготовление   
                           
              }
             }
               break;
      } //switch
      
  }   else {     // если рецепт запущен выводим меню //конец
               st_rec=0;
               digitalWrite(o_ten, LOW);     // приготовление STOP или остановлено выключаем ТЭН и мотор
               digitalWrite(o_motor, LOW);   //
               lcd.setCursor(12, 0); lcd.print (F("STOP") ) ;   // курсор на (столбец, строка)
               lcd.setCursor(5, 1); lcd.print (F("     ")); 
           }
  } else {          // проверка датчика температуры // конец
            if (millis() - tmr_temp_0 >= 2000) { // если датчик не восстановился через 2 секунды остановка всего и вывод о неисправности
    lcd.setCursor(0, 0);  // курсор на (столбец, строка)
    lcd.print (F("Датчик t потерян ")) ;              
    lcd.setCursor(0, 1);  // курсор на (столбец, строка)             
    lcd.print (F(" !!!  STOP !!!  ")) ;     
    fl_temp_0=false;
    o_dinam.blink(7, 500, 200); //  // активная пищалка пискнуть 7 раз, 500мс вкл, 200мс выкл
    digitalWrite(o_ten, LOW);
    digitalWrite(o_motor, LOW);
    
            }
    }
}


String TimePrint(uint16_t tv, boolean tch)   // переводит секунды в часы минуты секунды tch==true точки моргают.
{
String st="";
static char ch;
static uint32_t tmr_tc;
        if (millis() - tmr_tc >= 500) {
        tmr_tc = millis();
        static bool flag_tc;          // флаг состояния :
         if (flag_tc = !flag_tc) { ch=' ';  } else { ch=':';}
        } 
if ((tv/60/60==0) && (tv/60%60==0)  )  { st=st+F("  :"); if (tv%60<10) { st=st+F("0");} st=st+String(tv%60); // усли меньше минуты осталось то точки не моргают и выводятся секунды
} else {
       if (tv/60/60<10)  { st=' '; }  
           st=st+String(tv/60/60);
if (tch) { st=st+ch;} else { st=st+':';}
if (tv/60%60<10) { st=st+F("0"); }
st=st+String((tv/60)%60);//8
}
return(st);
}


uint32_t TimeRecept()   // подсчёт общего времени рецепта
{
   uint16_t t=0;
        for (int i = 0; i < 12; i++) {
           t = t+vrem_st[n_menu][i];  // подсчёт общего времени приготовления рецепта 
        }
 return(t);
}

void tek_temp() // получение текущей температуры, обновляем энкодер
{
static uint32_t tmr_tt;

    if (millis() - tmr_tt >= 1000) {
        tmr_tt = millis();     // сброс таймера
        temp_0 = therm.getTempAverage();   // Обновление значения температуры раз в секунду 
        lcd.setCursor(0, 1); lcd.print( ((temp_0/100) <= 0) ? ' '+String(temp_0) : String(temp_0) ); lcd.setCursor(3, 1); lcd.write(223);// вывод текущей температуры 
    }
 if (digitalRead(o_ten)) { lcd.setCursor(4, 1); lcd.write(244); } else { lcd.setCursor(4, 1);  lcd.write(165);}    // вывод состояния ТЭНа,244 +ВКЛ 165-ВЫКЛ // курсор на (столбец, строка)        
 if (fl_run) { lcd.setCursor(5, 1); lcd.print ( ((temp_st[n_menu][st_rec]/100) <= 0) ? ' '+String(temp_st[n_menu][st_rec]) : String(temp_st[n_menu][st_rec]+korka));lcd.write(223);}  // выводим поддерживаемую температуру 
             
  eb.tick(); //обновление состояния энкодера
  o_dinam.tick(); // для пищалки 
}


void ust_timer() // функция установки таймера
{
    lcd.setCursor(9, 1);
    lcd.print (F("->")) ;
    
    boolean fl_t=true;
    while (fl_t) {
       
            tek_temp(); // Обновление значения температуры и энкодера с динамиком 
         
            if (eb.right())  { if (time_timer >= 88000)  {time_timer=88000;} else {time_timer=time_timer+300;}} // увеличение таймера
            if (eb.left())   { if (time_timer < 300)     {time_timer=0;}     else {time_timer=time_timer-300;}} // уменьшение таймера

            if (eb.hasClicks(1)) {  fl_t=false;} // выход из установки таймера     
            if (eb.hasClicks(2)) {  fl_t=false; time_timer=0; } // 2клика  выход из установки таймера и сброс в 0    
            if (eb.hold()) { fl_t=false;  }   // долгое нажатие остановить установки 
            lcd.setCursor(11, 1);
            lcd.print (TimePrint(time_timer, false)) ;

             if (!fl_t){      
              lcd.setCursor(9, 1);
              lcd.print (F("  ")) ;
             }
         }
}  //  установки таймера КОНЕЦ

void pod_temper() // функция поддержиния температуры
{
     lcd.setCursor(9, 1); lcd.print (F("->     ")) ;
     uint8_t loc_men=0;    // переменная локального меню  
    boolean fl_pt=true;
    while (fl_pt) {
           tek_temp(); // Обновление значения температуры и энкодера с динамиком 
               lcd.setCursor(11, 1);
               switch (loc_men) {               //  
                case 0:    // установка температуры поддержания 
                   if (eb.right())  { if (t_pod >= 130)   {t_pod=130;} else {t_pod++;}} // увеличение температуры мешалки
                   if (eb.left())   { if (t_pod <= 20 )   {t_pod= 20;} else {t_pod--;}} // уменьшение температуры мешалки
                     lcd.print ( ((t_pod/100) <= 0) ? ' '+String(t_pod) : String(t_pod));lcd.write(223);
                     break;                  
                case 1:    // установка времени поддерживания t 
                   if (eb.right())  { if (vrem_temp == 1081)   {vrem_temp=1;}     else {vrem_temp++;}} // увеличение время поддерживания t
                   if (eb.left())   { if (vrem_temp == 1)      {vrem_temp=1081; } else {vrem_temp--;}} // уменьшение время поддерживания t
                   if (eb.hasClicks(2)) {  vrem_temp=1081; }                                         // 2-клика    время в бесконечность
                   if (vrem_temp == 1081)   {lcd.write(243);lcd.print (F(" мин"));}     else {lcd.print (TimePrint(vrem_temp*60, false));}
                     break;                      
               } //switch
               if (eb.hasClicks(1)) { loc_men++;}
               if (eb.hold()) { fl_pt=false;  }   // долгое нажатие остановить установки 

               if (loc_men==2) {lcd.setCursor(9, 1); lcd.print (F("       ")) ;
                             boolean  beep_1=true;
                             fl_run1=true;
                             uint32_t timer_mes=millis();
                             lcd.setCursor(12, 0);  lcd.print (F(" RUN") ) ;    // курсор на (столбец, строка) отображение ссотояния RUN 
                             while (fl_run1) {
  
                                  tek_temp(); // Обновление значения температуры и энкодера с динамиком 

                                  if (beep_1){ o_dinam.blink(1, 150, 150); beep_1=false;}  // активная пищалка пискнуть 1 раз, 150мс вкл, 150мс выкл 
                                                               
                                  contr_temper(t_pod);
                                  lcd.setCursor(5, 1); lcd.print (( (t_pod/100) <= 0) ? ' '+String(t_pod) : String(t_pod));lcd.write(223);  // выводим поддерживаемую температуру  

                                  
                                  if (vrem_temp != 1081){   // если не бесконечность, то запускаем таймер
                                   lcd.setCursor(11, 1);  
                                   lcd.print (   TimePrint( ((vrem_temp*60)-((millis()-timer_mes)/1000) ), true)   );   // обратный отсчёт времени 
                                   if (((millis() - timer_mes)/1000) >= vrem_temp*60) {
                                     digitalWrite(o_ten, LOW);
                                     digitalWrite(o_motor, LOW);
                                     fl_pt =false;
                                     fl_run1=false;
                                     o_dinam.blink(4, 400, 200);  // активная пищалка пискнуть 4 раз, 400мс вкл, 200мс выкл
                                   }
                                  }                        // таймер КОНЕЦ
                                  
                                  if ((eb.hold()) || ((temp_0 > 185) && (temp_0 < 1)))  {
                                    fl_pt=false;
                                    fl_run1=false; 
                                    digitalWrite(o_ten, LOW);
                                    digitalWrite(o_motor, LOW);
                                    lcd.setCursor(9, 1); lcd.print (F("       ")) ;
                                    o_dinam.blink(2, 150, 150);
                                  }   // долгое нажатие остановить установки поддерживаемой темпеоратуры + проверяем работает ли датчик температуры
                             }
             }
             if (!fl_pt){      
              lcd.setCursor(9, 1);
              lcd.print (F("  ")) ;
             }
         }
} // функция поддержиния температуры КОНЕЦ

void meshalka() // "Мешалка"  перемешиваем
{
    if (t_pod >= 90)   {t_pod=90;}
    lcd.setCursor(9, 1);
    lcd.print (F("->     ")) ;
    boolean fl_pt=true;
    static uint16_t vrem_mes=5;  // время сколько мешать
    uint8_t loc_men=0;    // переменная локального меню  
    while (fl_pt) {
               tek_temp(); // Обновление значения температуры и энкодера с динамиком 
               lcd.setCursor(11, 1);
               switch (loc_men) {               //  
                case 0:    // установка температуры замешивания 
                   if (eb.right())  { if (t_pod >= 90)   {t_pod=90;} else {t_pod++;}} // увеличение температуры мешалки
                   if (eb.left())   { if (t_pod <= 20)   {t_pod=20;} else {t_pod--;}} // уменьшение температуры мешалки
                   lcd.print (String(t_pod));lcd.write(223);
                     break;                  
                case 1:    // установка времени замешивания 
                   if (eb.right())  { if (vrem_mes >= 90)   {vrem_mes=90;} else {vrem_mes++;}} // увеличение время мешалки
                   if (eb.left())   { if (vrem_mes == 0)    {vrem_mes=0; } else {vrem_mes--;}} // уменьшение время мешалки
                   { if (vrem_mes < 10)    {lcd.print (String(vrem_mes)+F("мин ")); } else lcd.print (String(vrem_mes)+F("мин"));}
                     break;                      
               } //switch
               if (eb.hasClicks(1)) { loc_men++;}
               if (eb.hold()) { fl_pt=false;  }   // долгое нажатие остановить установки 
                   
                   
                   if (loc_men==2) {lcd.setCursor(9, 1);
                                        lcd.print (F("       ")) ;
                                        boolean beep_1=true;
                                        fl_run1=true;
                                        uint32_t timer_mes=millis();
                                        lcd.setCursor(12, 0);  lcd.print (F(" RUN") ) ;    // курсор на (столбец, строка) отображение ссотояния RUN 
                                        while (fl_run1) {
                                                        tek_temp(); // Обновление значения температуры и энкодера с динамиком 
                                                        if (beep_1){ o_dinam.blink(1, 150, 150); beep_1=false;}  // активная пищалка пискнуть 1 раз, 150мс вкл, 150мс выкл 
                                                        lcd.setCursor(11, 1);  
                                                        lcd.print (   TimePrint( ((vrem_mes*60)-((millis()-timer_mes)/1000) ), true)   );   // обратный отсчёт времени  
                                                      
                                                        contr_motor(3);
                                                        contr_temper(t_pod);  
                                                        
                                                        lcd.setCursor(5, 1); lcd.print (( (t_pod/100) <= 0) ? " "+String(t_pod) : String(t_pod));lcd.write(223);  // выводим поддерживаемую температуру
                                                        if (((millis() - timer_mes)/1000) >= vrem_mes*60) {
                                                             digitalWrite(o_ten, LOW);
                                                             digitalWrite(o_motor, LOW);
                                                             fl_pt =false;
                                                             fl_run1=false;
                                                             o_dinam.blink(4, 400, 200);  // активная пищалка пискнуть 4 раз, 400мс вкл, 200мс выкл
                                                             }

                                                         if ((eb.hold()) || ((temp_0 > 185) && (temp_0 < 1)))  {
                                                          digitalWrite(o_ten, LOW);
                                                          digitalWrite(o_motor, LOW);
                                                          fl_pt=false;
                                                          fl_run1=false; 
                                                          lcd.setCursor(9, 1);
                                                          lcd.print (F("       ")) ;
                                                          o_dinam.blink(2, 150, 150);
                                                          }   // долгое нажатие остановить установки  + проверяем работает ли датчик температуры
                                  
                                                        }
             }

             if (!fl_pt){      
              lcd.setCursor(9, 1);
              lcd.print (F("  ")) ;
             }
         }

}// "Мешалка"  перемешиваем КОНЕЦ

void contr_temper(uint8_t tp ) //контроль температуры tp-поддерживаемая темпрература
 {

  if (p_isp){        // если ПИД включён то используем процендуру ПИД
          
             pid.setpoint = tp; // уставка температуры ПИД
             
      static uint32_t tmr_pt;
      if (((millis() - tmr_pt)/1000) >= t_pid) {
          tmr_pt = millis();     // сброс таймера
          float result = pid.compute(therm.getTempAverage());  // закидываем значения датчика температуры для расчёта ПИД
          analogWrite(o_ten, result); // результат вычисления ПИД отправляем в значение ШИМ на пин  //применить(result);
          if (vrem_temp == 1081 && fl_run1 && n_menu==9 ){ lcd.setCursor(10, 1); lcd.write(37);lcd.write(126); lcd.print(String(result/2.55));} // выводим % загрузки ШИМ при подогреве         
       }            
    
            } else { // нет ПИД
  
    if ((temp_0 >= tp) && digitalRead(o_ten)) {digitalWrite(o_ten, LOW );} else {
    if (temp_0 < tp){// if te
                   if (tp >= 80 ) { //  выпечка
                     if (millis() - tmr_ct >= (flag_ct ? 34000 : 1000)) {
                        tmr_ct = millis();
                        if (flag_ct = !flag_ct) {digitalWrite(o_ten, HIGH );} else {digitalWrite(o_ten, LOW );}                     
                     }
                   } 
                   if (tp < 40 ) { //  поддержание
                     if (millis() - tmr_ct >= (flag_ct ? 5000 : 50000)) {   //flag_ct ? 5000 : 70000
                      tmr_ct = millis();
                      if (flag_ct = !flag_ct) {digitalWrite(o_ten, HIGH );} else {digitalWrite(o_ten, LOW );}
                      }     
     
                   }

                   if ((tp < 80 ) && (tp >= 40)) { //  поддержание
                     if (millis() - tmr_ct >= (flag_ct ? 8000 : 40000)) {   //flag_ct ? 5000 : 70000
                      tmr_ct = millis();
                      if (flag_ct = !flag_ct) {digitalWrite(o_ten, HIGH );} else {digitalWrite(o_ten, LOW );}
                      }     
     
                   }
     
    }               // if te
    }
                  } // else нет ПИД
 }


     //  0-ВЫК 
     //  1-ИМ/ВКЛ (первые 60 секунд импульс 0.2/0.8  сек, кроме формовки 7,9)
     //  2-ИМПУЛЬС 0.2/0.8  сек  
     //  3-ВКЛ
     //  ДЖЕМ

void contr_motor(uint8_t dmot ) // управление мотором 
 {
             switch (dmot) {  
                case 0:    // 0-ВЫК 
                     digitalWrite(o_motor, LOW );
                     break;
                case 1:    // 1-ИМ/ВКЛ (первые 60 секунд ИМПУЛЬС 0.2/0.8  сек, далее ВКЛ кроме формовки 7,9)
                   if (millis()-timer_stad < 60000) {
                       if (millis() - tmr_cm >= (flag_cm ? 200 : 800)) {
                         tmr_cm = millis();
                         if (flag_cm = !flag_cm) {digitalWrite(o_motor, HIGH );} else {digitalWrite(o_motor, LOW );}                     
                       }    
                   } else {digitalWrite(o_motor, HIGH );} 
                     break;
                case 2:    // 2-ИМПУЛЬС 0.2/0.8  сек
                   if (millis() - tmr_cm >= (flag_cm ? 200 : 800)) {
                      tmr_cm = millis();
                      if (flag_cm = !flag_cm) {digitalWrite(o_motor, HIGH );} else {digitalWrite(o_motor, LOW );}                     
                   }    
                     break;                     
                case 3:    // 3-ВКЛ
                  digitalWrite(o_motor, HIGH );
                     break;                                          
             } //switch
 }
void ust_pid() // функция установки ПИД
{
    lcd.clear() ;
    lcd.setCursor(0, 0);
    lcd.print (F("Наст.ПИД:")) ;
    lcd.setCursor(12, 0);  lcd.print (F("t=")); lcd.print(String(t_pid));    
    lcd.setCursor(0, 1);  lcd.print (F("p=")); lcd.print(String(p));
    lcd.setCursor(5, 1);  lcd.print (F("i=")); lcd.print(String(i));
    lcd.setCursor(10, 1); lcd.print (F("d=")); lcd.print(String(d));
    boolean fl_pt=true;
    uint8_t loc_men=0;    // переменная локального меню  

    while (fl_pt) {
               eb.tick(); //обновление состояния энкодера
               switch (loc_men) {               //  
                case 0:    // установка использовать ПИД да/нет 
                     if (eb.turn()) { p_isp=!p_isp;}
                     lcd.setCursor(9, 0); lcd.write(126);   // вывод стрелки
                     if (p_isp) {
                      lcd.setCursor(10, 0); lcd.write(43);
                      lcd.setCursor(12, 0);  lcd.print (F("t=")); lcd.print(String(t_pid));   
                      lcd.setCursor(0, 1);  lcd.print (F("p=")); lcd.print(String(p)); 
                      lcd.setCursor(5, 1);  lcd.print (F("i=")); lcd.print(String(i));
                      lcd.setCursor(10, 1); lcd.print (F("d=")); lcd.print(String(d));
                     } else {lcd.setCursor(10, 0); lcd.write(45);
                             lcd.setCursor(12, 0);  lcd.print (F("    ")); 
                             lcd.setCursor(0, 1);  lcd.print (F("                ")); 
                            }       
                    if (eb.hasClicks(1)) { lcd.setCursor(9, 0);lcd.write(32); EEPROM.update(6, p_isp);
                                           if (p_isp) { loc_men++;} else {fl_pt=false;}
                                           }
                   break; 
                case 1:    // установка t_pid 
                   if (eb.right())  { if (t_pid >= 99)   {t_pid=99;}  else {t_pid++;} lcd.setCursor(14, 0); lcd.print (F("   ")); lcd.setCursor(14, 0); lcd.print (String(t_pid)); } // увеличение t_pid
                   if (eb.left())   { if (t_pid <= 1)    {t_pid=1;}   else {t_pid--;} lcd.setCursor(14, 0); lcd.print (F("   ")); lcd.setCursor(14, 0); lcd.print (String(t_pid)); } // уменьшение t_pid
                   lcd.setCursor(13, 0); lcd.write(126);           
                    if (eb.hasClicks(1)) { lcd.setCursor(13, 0);lcd.write(61); EEPROM.update(47, t_pid); pid.setDt(t_pid); loc_men++; }
                   break;                  
                case 2:    // установка p 
                   if (eb.right())  { if (p >= 900)   {p=900;} else {p++;} lcd.setCursor(2, 1); lcd.print (F("   ")); lcd.setCursor(2, 1); lcd.print (String(p)); } // увеличение p
                   if (eb.left())   { if (p <= 0)     {p=0;}   else {p--;} lcd.setCursor(2, 1); lcd.print (F("   ")); lcd.setCursor(2, 1); lcd.print (String(p)); } // уменьшение p
                   lcd.setCursor(1, 1); lcd.write(126);           
                    if (eb.hasClicks(1)) { lcd.setCursor(1, 1);lcd.write(61); EEPROM.update(0, p);   pid.setKp(p); loc_men++; }
                   break;                  
                case 3:    // установка i
                   if (eb.right())  { if (i >= 900)   {i=900;} else {i++;} lcd.setCursor(7, 1); lcd.print (F("   ")); lcd.setCursor(7, 1); lcd.print (String(i));} // увеличение i
                   if (eb.left())   { if (i <= 0)     {i=0;}   else {i--;} lcd.setCursor(7, 1); lcd.print (F("   ")); lcd.setCursor(7, 1); lcd.print (String(i));} // уменьшение i
                   lcd.setCursor(6, 1); lcd.write(126); 
                   if (eb.hasClicks(1)) { lcd.setCursor(6, 1);lcd.write(61); EEPROM.update(2, i); pid.setKi(i); loc_men++; }
                     break;                      
                case 4:    // установка d
                   if (eb.right())  { if (d >= 900)   {d=900;} else {d++;} lcd.setCursor(12, 1); lcd.print (F("   ")); lcd.setCursor(12, 1); lcd.print (String(d));} // увеличение d
                   if (eb.left())   { if (d <= 0)     {d=0;}   else {d--;} lcd.setCursor(12, 1); lcd.print (F("   ")); lcd.setCursor(12, 1); lcd.print (String(d));} // уменьшение d
                   lcd.setCursor(11, 1); lcd.write(126);
                   if (eb.hasClicks(1)) { EEPROM.update(4, d); pid.setKd(d);  fl_pt=false; }
                     break;                        
               } //switch
               if (eb.hold()) { fl_pt=false;  }   // долгое нажатие остановить установки 
      }//while
}  // функция установки ПИД КОНЕЦ

void ust_svoy() // функция установки рецепта Свой // 1-Таймер* 2-Прогрев 3-1замес 4-Отдых 5-2замес 6-1подъём 7-формовка 8-2подъём 9-Формовка 10-3подъём 11-Выпечка 12-Остывание 13-Подогрев
{
    lcd.clear() ;
    boolean fl_pt=true;
    uint8_t loc_men=2;    // переменная локального меню  
    lcd.setCursor(0, 0); lcd.print (F(" 2 - Прогрев    ")) ;
    
    while ( loc_men<14) {
               eb.tick(); //обновление состояния энкодера

             lcd.setCursor(0, 1); lcd.print (F("t"));lcd.write(223); if (fl_pt)  {lcd.write(126);} else {lcd.write(61);}  lcd.print (String(temp_st[kn_menu-4][loc_men-1]));lcd.write(223); if (((temp_st[kn_menu-4][loc_men-1])/100)<1) {lcd.write(' ');}   // выводим параметры стадии рецепта 
             lcd.setCursor(8, 1); lcd.print (F("V"));                if (!fl_pt) {lcd.write(126);} else {lcd.write(61);}  lcd.print (TimePrint((vrem_st[kn_menu-4][loc_men-1]),false));

             if (eb.right())  { if (fl_pt)  { if (temp_st[kn_menu-4][loc_men-1] >= 135)     {temp_st[kn_menu-4][loc_men-1]=135;} else {temp_st[kn_menu-4][loc_men-1]++;} }                                        //  градусы
                                if (!fl_pt) {
                                               if (loc_men==7 || loc_men==9 ) { if (vrem_st[kn_menu-4][loc_men-1] >= 30)      {vrem_st[kn_menu-4][loc_men-1]=30;}    else {vrem_st[kn_menu-4][loc_men-1]++;}}
                                                                               else  {if (vrem_st[kn_menu-4][loc_men-1] >= 36000)   {vrem_st[kn_menu-4][loc_men-1]=36000;} else {vrem_st[kn_menu-4][loc_men-1]+=60;}}
                                             }
                                // для таймера по минутам
                                       // для формовке по секундам
                                }                                 
             if (eb.left())   { if (fl_pt)  { if (temp_st[kn_menu-4][loc_men-1] <= 15)      {temp_st[kn_menu-4][loc_men-1]=15;}  else {temp_st[kn_menu-4][loc_men-1]--;} }
                                if (!fl_pt) {
                                              if (loc_men==7 || loc_men==9 ) { if (vrem_st[kn_menu-4][loc_men-1] == 0)      {vrem_st[kn_menu-4][loc_men-1]=00;}    else {vrem_st[kn_menu-4][loc_men-1]--;}}
                                                                               else  {if (vrem_st[kn_menu-4][loc_men-1] <= 59)   {vrem_st[kn_menu-4][loc_men-1]=0;} else {vrem_st[kn_menu-4][loc_men-1]-=60;}}
                                             }                            
                               }  // 
                if (eb.hasClicks(1)) {      // переход к следующеё стадии рецепта
                  if (!fl_pt) {loc_men++;}
                  fl_pt=!fl_pt;
                }
              if (eb.hold()) { loc_men=14;  }   // долгое нажатие остановить установки 
              if (eb.hasClicks(2)) {  // двойной клик - шаг назад
                 if (loc_men>2) {loc_men--; fl_pt=true;}
                
              }
                                switch (loc_men) {               // Выводим заголовок стадии рецепьа
                   case 2:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 2 - Прогрев    ")) ;
                   break;
                   case 3:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 3 - 1-й замес  ")) ;                  
                   break;                      
                   case 4:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 4 - Отдых      ")) ;
                   break;
                   case 5:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 5 - 2-й замес  ")) ;                  
                   break;                      
                   case 6:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 6 - 1-й подъём ")) ;
                   break;
                   case 7:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 7 - формовка   ")) ;                  
                   break;                      
                   case 8:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 8 - 2-й подъём ")) ;
                   break;
                   case 9:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 9 - Формовка   ")) ;                  
                   break;                      
                   case 10:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 10 - 3-й подъём")) ;
                   break;
                   case 11:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 11 - Выпечка   ")) ;                  
                   break;                      
                   case 12:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 12 - Остывание ")) ;
                   break;
                   case 13:    // 
                      lcd.setCursor(0, 0); lcd.print (F(" 13 - Подогрев  ")) ;                  
                   break;    
                 } //switch 
      }//while

    EEPROM.put(7,  temp_st[kn_menu-4]); // пишем в ячейки EEPROM 7-19  температуру  рецепта свой
    EEPROM.put(20, vrem_st[kn_menu-4]); // пишем в ячейки EEPROM 20-19 время стадии рецепта свой
    
}  // функция установки рецепта Свой КОНЕЦ

void ust_korka() // функция установки цвета корки, прибавляем температуру к температуре выпечки
{
    lcd.setCursor(0, 0); lcd.print (F(" Цвет корочки:  ")) ;
    lcd.setCursor(0, 1); lcd.print (F("                "));
    boolean fl_pt=true;
    lcd.setCursor(0, 1);
    for (int i = 0; i < korka+1; i++) { lcd.setCursor(i, 1);  lcd.write (255) ;  }
    while ( fl_pt) {
               eb.tick(); //обновление состояния энкодера
                   if (eb.right())  { if (korka >= 15)   {korka=15;} else { korka++;}  lcd.setCursor(korka,  1); lcd.write (255) ; } // увеличение температуры мешалки
                   if (eb.left())   { if (korka <= 0 )   {korka=0; } else { korka--;}  lcd.setCursor(korka+1,1); lcd.write (32)  ; } // уменьшение температуры мешалки 
                //   if (korka==0) {}
            if (eb.hasClicks(1)) { fl_pt=false;}   
            if (eb.hold()) { fl_run=false; fl_pt=false;} // долгое нажатие остановить приготовление 
                   
    }
    EEPROM.update(46, korka);
    p_menu_hleb(n_menu);       // Вывод пункта менюрецепта из функции
    lcd.setCursor(9, 1); lcd.write (32) ;
}// функция установки цвета корки, прибавляем температуру к температуре выпечки КОНЕЦ


