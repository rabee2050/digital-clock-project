/*
  Title  : Digital Clock Project
  Contact: info@tatco.cc
  Done By: TATCO Inc.
  Skecth on Github : https://github.com/rabee2050/esp32-room-project
  Youtube: http://tatco.cc
 */

#include <LiquidCrystal.h>
//LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int h = 12, h_t;
int m, m_t;
int s, s_t;
int f_t;
int flag;
int TIME;
const int hs = 5;
const int ms = 4;
const int switch_button = 6;

bool timer_status = false;
bool timer_start = false;
bool digital_clock_show = true;
//int state1;
//int state2;
unsigned long last = millis();
unsigned long last_t = millis();
unsigned long lastbutton = millis();

void setup()
{
  lcd.begin(16, 2);
  lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("CLOCK PROJECT");
      lcd.setCursor(1, 1);

      lcd.print("Welcome");
      delay(6000);
      lcd.clear();
}
void loop()
{

  digital_clock();

  if (digitalRead(switch_button) == HIGH ) { // pin 12
    if (millis() - lastbutton > 250) {
      timer_status = !timer_status;
      lcd.clear();
      timer_start = false;
      f_t = 0;
      s_t = 0;
      m_t = 0;
      h_t = 0;

    }
    lastbutton = millis();
  }

  if (timer_status) {
    digital_clock_show = false;
    timer();
    timer_buttons();

  } else {
    timer_start = false;
    digital_clock_show = true;
    clock_buttons();

  }



}

void digital_clock() {
  if (millis() - last >= 1000) {
    if (digital_clock_show) {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Digital Clock");
      lcd.setCursor(0, 1);

      lcd.print("TIME:");
      lcd.print(h);
      lcd.print(":");
      lcd.print(m);
      lcd.print(":");
      lcd.print(s);
      lcd.setCursor(14, 1);
      if (flag < 12)lcd.print("AM");
      if (flag == 12)lcd.print("PM");
      if (flag > 12)lcd.print("PM");


    }
    if (flag == 24)flag = 0;
    //    delay(1000);
    s = s + 1;
    if (s == 60) {
      s = 0;
      m = m + 1;
    }
    if (m == 60)
    {
      m = 0;
      h = h + 1;
      flag = flag + 1;
    }
    if (h == 13)
    {
      h = 1;
    }
    last = millis();
  }
}

void timer() {
  if (timer_start) {
    if (millis() - last_t >= 100) {
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("TIMER");
      lcd.setCursor(5, 1);
      f_t = f_t + 1;

      lcd.print(h_t);
      lcd.print(":");
      lcd.print(m_t);
      lcd.print(":");
      lcd.print(s_t);
      lcd.print(":");
      lcd.print(f_t);
      if (f_t == 10) {
        f_t = 0;
        s_t = s_t + 1;
      }
      if (s_t == 60) {
        s_t = 0;
        m_t = m_t + 1;
      }
      if (m_t == 60)
      {
        m_t = 0;
        h_t = h_t + 1;

      }
      if (h_t == 12)
      {
        f_t = 0;
        s_t = 0;
        m_t = 0;
        h_t = 0;
      }

      last_t = millis();
    }
  } else {

    lcd.setCursor(6, 0);
    lcd.print("TIMER");
    lcd.setCursor(5, 1);
    lcd.print(h_t);
    lcd.print(":");
    lcd.print(m_t);
    lcd.print(":");
    lcd.print(s_t);
    lcd.print(":");
    lcd.print(f_t);

  }
}

void clock_buttons() {
  if (digitalRead(hs) == HIGH ) { //  pin 10
    if (millis() - lastbutton > 50) {
      s = 0;
      h = h + 1;
      flag = flag + 1;
      if (flag < 12)lcd.print("AM");
      if (flag == 12)lcd.print("PM");
      if (flag > 12)lcd.print("PM");
      if (flag == 24)flag = 0;
      if (h == 13)h = 1;
      last = millis() - 1000;
    }
    lastbutton = millis();
  }

  if (digitalRead(ms) == HIGH ) { //  pin 11
    if (millis() - lastbutton > 50) {
      s = 0;
      m = m + 1;
      last = millis() - 1000;
    }
    lastbutton = millis();
  }
}
void timer_buttons() {
  if (digitalRead(hs) == HIGH ) { //  pin 10
    if (millis() - lastbutton > 50) {
      lcd.clear();
      timer_start = false;
      f_t = 0;
      s_t = 0;
      m_t = 0;
      h_t = 0;

      lcd.setCursor(6, 0);
      lcd.print("TIMER");
      lcd.setCursor(5, 1);
      lcd.print(h_t);
      lcd.print(":");
      lcd.print(m_t);
      lcd.print(":");
      lcd.print(s_t);
      lcd.print(":");
      lcd.print(f_t);
    }
    lastbutton = millis();
  }

  if (digitalRead(ms) == HIGH ) { //  pin 11
    if (millis() - lastbutton > 50) {
      timer_start = !timer_start;
    }
    lastbutton = millis();
  }
}

