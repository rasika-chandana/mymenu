// avr-libc library includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include "U8glib.h"

U8GLIB_ST7920_128X64_1X u8g(23, 17, 16);

/* Rotary encoder (dial) pins */
#define ROT_EN_A 31
#define ROT_EN_B 33

/* Rotary encoder button pin */
#define BUTTON_DIO 35

/* Reset button pin */
#define RESET_DIO 41

/* Buzzer pin */
#define BUZZER_DIO 37


#define MAIN_MENU_ITEMS 2
#define PLATE_MENU_ITEMS 6
#define TOWER_MENU_ITEMS 2
const char *mainMenuItems[MAIN_MENU_ITEMS] = { "Number of Plates : ", "Destination Tower: "};
char *mainMenuValues[MAIN_MENU_ITEMS] = { "6", "B"};

const char *plateMenuItems[PLATE_MENU_ITEMS] = { "Number of plates"};
const char *plateMenuValues[PLATE_MENU_ITEMS] = { "1", "2", "3", "4", "5", "6"};

const char *towerMenuItems[TOWER_MENU_ITEMS] = { "Destination Tower"};
const char *towerMenuValues[TOWER_MENU_ITEMS] = { "B", "C"};

uint8_t currentNumberOfMenuItems = MAIN_MENU_ITEMS;
uint8_t currentMenuItem = 0;
uint8_t currentPageNumber = 1;

uint8_t menu_redraw_required = 1;

volatile int myInterruptVar = 0;
volatile int rotary_button_checked = 0;
volatile int reset_button_checked = 0;

volatile uint8_t rotary_button_pressd = 0;
volatile uint8_t reset_button_pressd = 0;

volatile uint8_t rotary_button_check = 1;
volatile uint8_t reset_button_check = 1;

volatile byte DialCount = 120;
volatile byte PreDialCount = 120;
volatile byte DialPos = 0;
volatile byte Last_DialPos = 0;

long speedOfLight = 999991L;

void setup()
{

  cli();          // disable global interrupts
  TCCR3A = 0;     // set entire TCCR3A register to 0
  TCCR3B = 0;     // same for TCCR3B

  // set compare match register to desired timer count:  @~744 Hz
  OCR3A = 150;
  // turn on CTC mode:
  TCCR3B |= (1 << WGM32);
  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR3B |= (1 << CS30) | (1 << CS32);
  // enable timer compare interrupt:
  TIMSK3 |= (1 << OCIE3B);
  // enable global interrupts:
  sei();


  // rotate screen, if required
  // u8g.setRot180();

  pinMode(BUZZER_DIO, OUTPUT);
  pinMode(BUTTON_DIO, INPUT);
  digitalWrite(BUTTON_DIO, HIGH);
  pinMode(RESET_DIO, INPUT);
  digitalWrite(RESET_DIO, HIGH);
  pinMode(ROT_EN_A, INPUT);
  pinMode(ROT_EN_B, INPUT);
  digitalWrite(ROT_EN_A, HIGH);
  digitalWrite(ROT_EN_B, HIGH);

  // force initial redraw
  menu_redraw_required = 1;

  Serial.begin(9600);
}

void drawCurrentPage(void) {
  if (1 == currentPageNumber) {
    drawFirstPage();
  } else if (2 == currentPageNumber) {
    drawNumberOfPlatesPage();
  } else if (3 == currentPageNumber) {
    drawDestinationTowerPage();
  }

}

void drawNumberOfPlatesPage(void) {
  uint8_t i, h;
  u8g_uint_t w, d, hieght;

  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setDefaultForegroundColor();
  
  h = u8g.getFontAscent() - u8g.getFontDescent();
  w = u8g.getWidth();
  //hieght = u8g.getHeight();
  d = 5;

  d = (w - u8g.getStrWidth(plateMenuItems[0])) / 2;
  u8g.drawStr(d, 2, plateMenuItems[0]);

  u8g.setFont(u8g_font_osb35);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();

  w = u8g.getWidth();
  //hieght = u8g.getHeight();

  d = (w - u8g.getStrWidth(plateMenuValues[currentMenuItem])) / 2;
  u8g.drawStr(d, (h + 10), plateMenuValues[currentMenuItem]);
  //Serial.print("H >> "); Serial.println(h); Serial.print("W >> "); Serial.println(w); Serial.print("hieght >> "); Serial.println(hieght);
}

void drawDestinationTowerPage(void) {
  uint8_t i, h;
  u8g_uint_t w, d, hieght;

  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setDefaultForegroundColor();

  h = u8g.getFontAscent() - u8g.getFontDescent();
  w = u8g.getWidth();
  //hieght = u8g.getHeight();

  d = (w - u8g.getStrWidth(towerMenuItems[0])) / 2;
  u8g.drawStr(d, 2, towerMenuItems[0]);

  u8g.setFont(u8g_font_osb35);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();

  w = u8g.getWidth();
  //hieght = u8g.getHeight();

  d = (w - u8g.getStrWidth(towerMenuValues[currentMenuItem])) / 2;
  u8g.drawStr(d, (h + 10), towerMenuValues[currentMenuItem]);
  //Serial.print("H >> "); Serial.println(h); Serial.print("W >> "); Serial.println(w); Serial.print("hieght >> "); Serial.println(hieght);
}

void drawFirstPage(void) {
  uint8_t i, h;
  u8g_uint_t w, d, hieght;

  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();

  h = u8g.getFontAscent() - u8g.getFontDescent();
  w = u8g.getWidth();
  hieght = u8g.getHeight();

  Serial.print("H >> "); Serial.println(h); Serial.print("W >> "); Serial.println(w); Serial.print("hieght >> "); Serial.println(hieght);

  for ( i = 0; i < MAIN_MENU_ITEMS; i++ ) {
    //d = (w - u8g.getStrWidth(mainMenuItems[i])) / 2;
    d = 1;
    u8g.setDefaultForegroundColor();
    if ( i == currentMenuItem ) {
      u8g.drawBox(0, i * h + 2, w, h);
      u8g.setDefaultBackgroundColor();
    }
    u8g.drawStr(d, i * h + 2, mainMenuItems[i]);
    u8g.drawStr(w - 15, i * h + 2, mainMenuValues[i]);
  }

  //  char temp[10];
  //  ltoa(speedOfLight, temp, 10);
  //  u8g.drawStr(d, i * h, temp);
  //
  //  u8g.drawStr(d, (i+1) * h, temp);
}

void handleRotaryButton(void) {
  Serial.println("Rotary Button pressed >>>> ");
  if (1 == currentPageNumber) {
    if (0 == currentMenuItem) {
      currentPageNumber = 2;
      currentNumberOfMenuItems = PLATE_MENU_ITEMS;
      currentMenuItem = 0;
    } else if (1 == currentMenuItem) {
      currentPageNumber = 3;
      currentNumberOfMenuItems = TOWER_MENU_ITEMS;
      currentMenuItem = 0;
    }
  } else if (2 == currentPageNumber) {
    mainMenuValues[0] = plateMenuValues[currentMenuItem];
    currentPageNumber = 1;
    currentNumberOfMenuItems = MAIN_MENU_ITEMS;
    currentMenuItem = 0;
  } else if (3 == currentPageNumber) {
    mainMenuValues[1] = towerMenuValues[currentMenuItem];
    currentPageNumber = 1;
    currentNumberOfMenuItems = MAIN_MENU_ITEMS;
    currentMenuItem = 1;
  } else {
    mainMenuValues[0] = "3";
    mainMenuValues[1] = "B";
    currentPageNumber = 1;
    currentNumberOfMenuItems = MAIN_MENU_ITEMS;
    currentMenuItem = 0;
  }
  menu_redraw_required = 1;
}

void handleResetButton(void) {
  Serial.println("Reset Button pressed >>>> ");
  menu_redraw_required = 1;
}


void loop() {
  drawDisplay();
}

void drawDisplay(void) {
  if (  menu_redraw_required != 0 ) {
    u8g.firstPage();
    do  {
      drawCurrentPage();
    } while ( u8g.nextPage() );
    menu_redraw_required = 0;
  }

  if (myInterruptVar > 1000) {
    myInterruptVar = 0;
  }

  if (rotary_button_pressd == 1) {
    handleRotaryButton();
    rotary_button_pressd = 0;
  }

  if (reset_button_pressd == 1) {
    handleResetButton();
    reset_button_pressd = 0;
  }

  if (DialCount != PreDialCount) {
    //Serial.println("DialCount >>>> "); Serial.print(DialCount);
    PreDialCount = DialCount;
    currentMenuItem = DialCount % currentNumberOfMenuItems;
    menu_redraw_required = 1;
  }
  //Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); Serial.println(myInterruptVar);
}

ISR(TIMER1_OVF_vect)
{
  Serial.println("==========================================");
}


ISR(TIMER3_COMPB_vect)
{
  if (!digitalRead(BUTTON_DIO) && rotary_button_check == 1) {
    rotary_button_pressd = 1;
    rotary_button_checked = myInterruptVar;
    rotary_button_check = 0;
  }

  if (!digitalRead(RESET_DIO) && reset_button_check == 1) {
    reset_button_pressd = 1;
    reset_button_checked = myInterruptVar;
    reset_button_check = 0;
  }


  if (abs(myInterruptVar - rotary_button_checked) > 70) {
    rotary_button_check = 1;
  }

  if (abs(myInterruptVar - reset_button_checked) > 70) {
    reset_button_check = 1;
  }

  DialPos = (digitalRead(ROT_EN_B) << 1) | digitalRead(ROT_EN_A);

  if (DialPos == 3 && Last_DialPos == 1)
  {
    /* If so increase the dial counter and display it */
    DialCount++;
  }

  /* Is the dial being turned counter-clockwise ? */
  if (DialPos == 3 && Last_DialPos == 2)
  {
    /* If so decrease the dial counter and display it */
    DialCount--;
  }

  Last_DialPos = DialPos;

  myInterruptVar++;
  //Serial.println("++++++++++++++++++++++++++++++++++++++++++");
}







