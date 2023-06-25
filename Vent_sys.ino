//дисплей 320х480
//Библиотеки
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include "DHT.h"//датчик влажности
#include <OneWire.h>//датчик температуры
#include <EEPROM.h> //библиотека для ПЗУ
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif
//константы для экрана
// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELHIGH  0xFFE0
#define WHITE   0xFFFF
#define DHTPIN 39
#define DHTTYPE DHT22
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
//константы
const word Nism=81;//количество точек измерений
const word HustT=1;//гистерезис уставки
const byte TustAdr=3;
//переменные
word Count=0;//счётчик для сброса автомасштабирования
word Vm=0;//Vmode режим вентилятора по умолчанию 0, вентилятор выключен
float Tust=23;//температура уставки
float rT=Tust;//температура с датчика
float rH=Tust;//влажность с датчика
float sT[Nism]={};//сохранённые значения температуры
float sH[Nism]={};//сохранённые значения влажности
unsigned long last_time=0;//последнее время
byte TFl=0;//температурный флажок на изменение масшатаба     
TSPoint p;
DHT dht(DHTPIN,DHTTYPE);//Создаем объект dht
OneWire ds(49); // Создаем объект OneWire для шины 1-Wire, с помощью которого будет осуществляться работа с датчиком
//начало настройки/////////////////////////////////////////////////////
void setup(void) {
  dht.begin();//запуск датчика влажности
  Tust=(float)EEPROM.read(TustAdr);//Чтение температуры уставки из ПЗУ
  //инициализация дисплея
  tft.reset();
  uint16_t identifier = 0x9341;
  tft.begin(identifier);
  //настройка пинов
  pinMode(13, OUTPUT);//для экрана
  pinMode(31,OUTPUT);//Мигающий светодиод (3с)
  //pinMode(33,OUTPUT);//Всегда горящий светодиод
  pinMode(35,OUTPUT);//Светодиод когда работает охлаждение
  pinMode(37,OUTPUT);//Светодиод когда работает обогрев
  pinMode(41,OUTPUT);//Светодиод режима вентиляции 0
  pinMode(43,OUTPUT);//Светодиод режима вентиляции 1
  pinMode(45,OUTPUT);//Светодиод режима вентиляции 2
  pinMode(47,OUTPUT);//Светодиод режима вентиляции 3
  pinMode(51,OUTPUT);//Светодиод режима вентиляции 4
  pinMode(53,OUTPUT);//Светодиод режима вентиляции 5
  pinMode(33,OUTPUT);//Привод жалюзи
  //настройка экрана
  tft.setRotation(1);//положение экрана
  tft.fillScreen(BLACK);//заполняем всё чёрным
  tft.setCursor(0, 0);//установка курсора в начало
  tft.setTextColor(WHITE);  tft.setTextSize(5);//выбираем цвет и размер текста
  tft.println("Get first mesuaring");//выводим на экрна слова
  tft.println("Please wait");
  //Проводим первые измерения, чтоб изначально построить графики
  for(int i=0;i<Nism;i++){
    //rT=random(-2,-1);
    rT=dht.readTemperature();
    rH=dht.readHumidity();
    if(rT>50){//Если значене больше 50 приравниваем к 50
      rT=50;
    }
    if(rT<0){
      TFl=1;
      if(rT<-50){//Если значене меньше -50 приравниваем к -50
      rT=-50;
      }
    }  
    //rH=random(20,60); 
    sT[i]=rT;  
    sH[i]=rH; 
    }
  //Выводим на экран неизменные слова
  tft.fillScreen(BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(RED);  tft.setTextSize(3);
  tft.println("Temp ");
  tft.setTextColor(BLUE);  tft.setTextSize(3); 
  tft.println("Hum  ");
  tft.setTextColor(WHITE);  tft.setTextSize(3); 
  tft.println("Tust ");
   tft.setTextColor(WHITE);  tft.setTextSize(3); 
  tft.println("Mode ");
  tft.setTextColor(RED);  tft.setTextSize(1); 
  tft.setCursor(0, 100);
  tft.println("  50*C");
  tft.setTextColor(BLUE);  tft.setTextSize(1);
  tft.println("  100%");
//Отрисовка конопок
  tft.setCursor(219, 28);
  tft.setTextColor(WHITE);  tft.setTextSize(3);
  tft.println("UPt"); 
  tft.drawRect(214, 10, 60, 60, WHITE);
  tft.setCursor(280, 28);
  tft.println("DNt"); 
  tft.drawRect(276, 10, 60, 60, WHITE);
  tft.setCursor(358, 28);
  tft.println("UPv"); 
  tft.drawRect(354, 10, 60, 60, WHITE);
  tft.setCursor(420, 28);
  tft.println("DNv"); 
  tft.drawRect(416, 10, 60, 60, WHITE);
  tft.setCursor(0, 300);  
  tft.setTextColor(RED);  tft.setTextSize(1);
  if(TFl==0){
    tft.println("   0*C");
    } 
  else{
    tft.println(" -50*C");
    }  
  tft.setTextColor(BLUE);  tft.setTextSize(1);
  tft.println("  0%");  
  tft.drawRect(39, 99, 403, 203, CYAN);//отрисовка границ графика
}
//Начало основного цикла/////////////////////////////////////////////////////
void loop() {
  digitalWrite(13, LOW);//необходимо для работы экрана
  TSPoint p = ts.getPoint();//получаем значение нажатых пикселей
  digitalWrite(13, HIGH);//необходимо для работы экрана
  pinMode(XM, OUTPUT);//необходимо для работы экрана
  pinMode(YP, OUTPUT);//необходимо для работы экрана
  
  p.y = map(p.y, 0, 920, 480, 0);
  
  //digitalWrite(33,LOW);//Всегда горящий светодиод

  
  if (millis()-last_time>1500){
    digitalWrite(31,LOW);//Мигающий светодиод (3с)
    if( p.y>110 && p.y<130){//мёртвая зона, так как по умолчанию эти значения
      p.y=0;
    }
      if (p.y>170 && p.y<250){//UPt
    Tust=Tust+0.1;
    delay(100);
    if(Tust>50)//ели уставка больше 50 градусов, то приравниваем к 50
    {Tust=50;}
    EEPROM.write(TustAdr, (float)Tust);//Запись Туст в пзу
  }
    if (p.y>130 && p.y<165 ){//DNt
    Tust=Tust-0.1;
    delay(100);
    if(Tust<-50)//ели уставка меньше -50 градусов, то приравниваем к -50
    {Tust=-50;}
    EEPROM.write(TustAdr, (float)Tust);//Запись Туст в пзу
  }
  if (p.y>42 && p.y<79){//UPv
    Vm=Vm+1;
    delay(100);

  }
  if (p.y>0 && p.y<38){//DNv
    Vm=Vm-1;
    delay(100);
    
  }
  }
  if(Vm<=0)//ели режим меньше 0 то 0
    {Vm=0;}
  if(Vm>=5)//ели режим больше 5 то 5
    {Vm=5;}
  switch(Vm){//переключатель режима дивгателя
    case 0:
    digitalWrite(41,LOW);
    digitalWrite(43,HIGH);
    digitalWrite(45,HIGH);
    digitalWrite(47,HIGH);
    digitalWrite(51,HIGH);
    digitalWrite(53,HIGH);
    digitalWrite(33,HIGH);
    break;
    
    case 1:
    digitalWrite(41,HIGH);
    digitalWrite(43,LOW);
    digitalWrite(45,HIGH);
    digitalWrite(47,HIGH);
    digitalWrite(51,HIGH);
    digitalWrite(53,HIGH);
    digitalWrite(33,LOW);
    break;
    
    case 2:
    digitalWrite(41,HIGH);
    digitalWrite(43,HIGH);
    digitalWrite(45,LOW);
    digitalWrite(47,HIGH);
    digitalWrite(51,HIGH);
    digitalWrite(53,HIGH);
    digitalWrite(33,LOW);
    break;
    
    case 3:
    digitalWrite(41,HIGH);
    digitalWrite(43,HIGH);
    digitalWrite(45,HIGH);
    digitalWrite(47,LOW);
    digitalWrite(51,HIGH);
    digitalWrite(53,HIGH);
    digitalWrite(33,LOW);
    break;
    
    case 4:
    digitalWrite(41,HIGH);
    digitalWrite(43,HIGH);
    digitalWrite(45,HIGH);
    digitalWrite(47,HIGH);
    digitalWrite(51,LOW);
    digitalWrite(53,HIGH);
    digitalWrite(33,LOW);
    break;

    case 5:
    digitalWrite(41,HIGH);
    digitalWrite(43,HIGH);
    digitalWrite(45,HIGH);
    digitalWrite(47,HIGH);
    digitalWrite(51,HIGH);
    digitalWrite(53,LOW);
    digitalWrite(33,LOW);
    break;
  }
  
  
  if (millis()-last_time>3000){//Снимаем данные и отрисовываем раз в полторы секунды
    last_time=millis();//обнуляеем счётчик
    digitalWrite(31,HIGH);//Мигающий светодиод (3с)
    ReadData(); //процедура для чтения информации с датчика DHT22 и DS18B20
    DrawGrapth();//Процедура для отрисовки графиков и измерений            
    } 
  if(rT>=Tust+HustT ){//если считанная температура превышает установленную + гистерезис
      digitalWrite(35,LOW);
      digitalWrite(37,HIGH);
    }
  else if((rT<=Tust-HustT) && (Vm!=0) ){//если считанная температура ниже установленной - гистерезис
      //включаем обогрев
      digitalWrite(37,LOW);
      digitalWrite(35,HIGH);
    }
  else{//если всё хорошо выключаем вентилятор,закрываем жалюзи  и обогрев
      digitalWrite(37,HIGH);
      digitalWrite(35,HIGH);
    }
}
//Процедуры://///////////////////////////////////////////////////
void ReadData(){//Процедура считывающая значения с датчика
  //читаем с датчика
  rH = dht.readHumidity();//Читаем влажность
  
  byte data[2]; // Место для значения температуры
  
  ds.reset(); // Начинаем взаимодействие со сброса всех предыдущих команд и параметров
  ds.write(0xCC); // Даем датчику DS18b20 команду пропустить поиск по адресу. В нашем случае только одно устрйоство 
  ds.write(0x44); // Даем датчику DS18b20 команду измерить температуру. Само значение температуры мы еще не получаем - датчик его положит во внутреннюю память
  delay(10); // Микросхема измеряет температуру, а мы ждем.  
  
  ds.reset(); // Теперь готовимся получить значение измеренной температуры
  ds.write(0xCC); 
  ds.write(0xBE); // Просим передать нам значение регистров со значением температуры

  // Получаем и считываем ответ
  data[0] = ds.read(); // Читаем младший байт значения температуры
  data[1] = ds.read(); // А теперь старший

  // Формируем итоговое значение: 
  //    - сперва "склеиваем" значение, 
  //    - затем умножаем его на коэффициент, соответсвующий разрешающей способности (для 12 бит по умолчанию - это 0,0625)
  rT =  ((data[1] << 8) | data[0]) * 0.0625;
  if(rT>50){//если больше 50 то 50
      rT=50;
    }
  if(rT<0){//если меньше 0 поднимаем флаг маштабирования и меняем значение внизу
      TFl=1;
      Count=0;
      tft.fillRect(0, 280, 30, 300, BLACK);//заполняем нижнюю границу, где было 0*С
      tft.setTextColor(RED);  tft.setTextSize(1); 
      tft.setCursor(0, 300); 
      tft.println(" -50*C");
      if(rT<-50){//Если значене меньше -50 приравниваем к -50
        rT=-50;
      }
  }
  //Сдвигаем все значения из легенды влажности и температуры на 1 влево
  for(int i=0;i<(Nism-1);i++){
      sT[i]=sT[i+1];
      sH[i]=sH[i+1];
    }
  //Меняем последние элементы
  sT[Nism-1]=rT;//В поледний элемент массива записываем новую температуру
  sH[Nism-1]=rH;//В поледний элемент массива записываем новую влажность
  if(TFl==1 && Count<Nism){//если масштабируем и массив не перезаписался, то увеличиваем счётчие
    Count++;
    }
  else{//иначе убираем автомасштабирование
    TFl=0;
    Count=0;
    tft.setTextColor(RED);  tft.setTextSize(1); 
    tft.fillRect(0, 280, 30, 300, BLACK);//заполняем нижнюю границу, где было -50*С
    tft.setCursor(0, 300); 
    tft.println("   0*C");
    tft.setTextColor(BLUE);  tft.setTextSize(1);
    tft.println("  0%"); 
    }
}
void DrawGrapth(){//Процедура отрисовки графиков и значений
  tft.fillRect(70, 0, 144, 93, BLACK);//Запалняем область значений чёрным квадратом
  tft.fillRect(40, 100, 401, 201, BLACK);//Запалняем область графиков чёрным квадратом
  DrawIzm();//Процедура отрисовки значений
  if(TFl==0){
  for(int i=0; i<(Nism-1); i++){//строим графики по Nism точкам. имеем по x 400px по y 200px.Относительно курсора (0;0)
    tft.drawLine(40+5*i, -4*sT[i]+300, 5*i+45, -4*sT[i+1]+300, RED);//график температуры
    tft.drawLine(40+5*i, -sH[i]/2+200, 5*i+45, -sH[i+1]/2+200, BLUE);//график влажности
  }
  tft.drawLine(40, -4*Tust+300, 440, -4*Tust+300, WHITE);//рисуем линию уставки
  }
  else{
    for(int i=0; i<(Nism-1); i++){//строим графики по Nism точкам. имеем по x 400px по y 200px.Относительно курсора (0;0)
    tft.drawLine(40+5*i, int(-2*sT[i]+200), 5*i+45, int(-2*sT[i+1]+200), RED);//график температуры
    tft.drawLine(40+5*i, -sH[i]/2+200, 5*i+45, -sH[i+1]/2+200, BLUE);//график влажности
  }
  tft.drawLine(40, int(-2*Tust+200), 440, int(-2*Tust+200), WHITE);//рисуем линию уставки
    }
  }
void DrawIzm(){//Процедура отрисовки значений
  tft.setCursor(0, 0);//Установка курора в левый верхний угол
  tft.setTextColor(RED);  tft.setTextSize(3);//Выбор цвета и размера текста
  tft.print("     ");//пробелы так, как слева имеем постоянные слова Temp,Hum,Tust
  tft.print(rT,1);//Выводим последнюю полученную температуру
  tft.println(" *C");//Выводим цельсий
  //tft.println(p.y);
  tft.setTextColor(BLUE);  tft.setTextSize(3); 
  tft.print("     ");
  tft.print(rH,1);//Выводим последнюю полученную влажность
  tft.println(" %");
  tft.setTextColor(WHITE);  tft.setTextSize(3); 
  tft.print("     ");
  tft.print(Tust,1);//Выводим температуру уставки
  tft.println(" *C");
  tft.print("     ");
  tft.print(Vm,1);//Выводим режим работы двигателя
  }
