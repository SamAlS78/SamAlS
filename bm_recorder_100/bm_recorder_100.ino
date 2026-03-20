/*
Part of arduino-bread project

Reads voltages of thermistor, motor and heater control signals for breadmaker
connected to its original control board.

This is to monitor original control signals to help in developing an arduino
board replacement.

circuit and documentation on www.elfnor.com/Arduino Bread Details.html
code https://github.com/elfnor/arduino-bread

This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.

elfnor


Доработано для хлебопечки LG-151 вход под термистор 100ком и два реле от ТЭНа и мотора

 */

 #include <GyverNTC.h>
 #include <Bounce2.h>
 
// GyverNTC therm(3, 100000, 3435); // пин, сопротивление при 25 градусах (R термистора = R резистора!), бета-коэффициент
 GyverNTC therm(0, 100000, 3950, 25, 100000); // пин, R термистора, B термистора, базовая температура, R резистора
#define heater 3
#define motor 2

  int s1 = 0;
  int s2 = 0;

Bounce  heater_on = Bounce(); 
Bounce  motor_on = Bounce();


// const int thermistor = A0;      // Analog input pin that the thermistor is connected to 

void setup() {
  // 10 second delay to make it easier to upload new programs
  delay(10000); 
  
  pinMode(motor, INPUT_PULLUP);
  pinMode(heater, INPUT_PULLUP);

  heater_on.attach(heater);
  heater_on.interval(5); // interval in ms

  motor_on.attach(motor);
  motor_on.interval(5); // interval in ms
  

  
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
}

void loop() { 
  // read the analog in value:
  heater_on.update();
  motor_on.update();

if (heater_on.read()==0)
  { //если кнопка нажата
   s1=1; //вывод сообщения о нажатии
  }
  else {s1=0; //вывод сообщения об отпускании
  }
  
if (motor_on.read()==0)
  { //если кнопка нажата
   s2=1; //вывод сообщения о нажатии
  }
  else {s2=0; //вывод сообщения об отпускании
  }
 
  int s3 = therm.getTempAverage();// analogRead(thermistor);
      
  // print the results to the serial monitor:   
  Serial.print(millis());
  Serial.print(",  ");
                   
  Serial.print(s1);
  Serial.print(",  ");
  Serial.print(s2);  
  Serial.print(",  ");
  Serial.println(s3); 

  // wait before the next loop
  //motor and heater on cycles vary from 3 to 5 seconds when pulsing
  delay(100);         
}            
