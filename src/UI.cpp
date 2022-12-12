// #include "mbed.h"
// #include "drivers/LCD_DISCO_F429ZI.h"
// #define BACKGROUND 1
// #define FOREGROUND 0
// #define GRAPH_PADDING 5

// LCD_DISCO_F429ZI lcd;
// char display_buf[3][60];

// void setupScreen(){
//     //background
//     lcd.SelectLayer(BACKGROUND);
//     lcd.Clear(LCD_COLOR_BLACK);
//     lcd.SetBackColor(LCD_COLOR_BLACK);
//     lcd.SetTextColor(LCD_COLOR_YELLOW);
//     lcd.SetLayerVisible(BACKGROUND,ENABLE);
//     lcd.SetTransparency(BACKGROUND,0x7Fu);
//     //forground
//     lcd.SelectLayer(FOREGROUND);
//     lcd.Clear(LCD_COLOR_BLACK);
//     lcd.SetBackColor(LCD_COLOR_BLACK);
//     lcd.SetTextColor(LCD_COLOR_LIGHTGREEN);
// }

// volatile uint8_t temp = 36.6;
// volatile uint8_t moi = 76;
// volatile uint8_t pres = 88;

// void updateValue(uint8_t newTemp, uint8_t newMoi, uint8_t newPres){
//     temp = newTemp;
//     moi = newMoi;
//     pres = newPres;
// }

// void displayValue(uint8_t temp, uint8_t moi, uint8_t pres, bool warn, bool initializing){
//     //if initializing
//     if (initializing){
//         snprintf(display_buf[0],60,"Initializing...");
//         lcd.SelectLayer(BACKGROUND);
//         lcd.Clear(LCD_COLOR_BLACK);
//         lcd.DisplayStringAt(0, LINE(10), (uint8_t *)display_buf[0], CENTER_MODE);
//         return;
//     }

//     //if warnning, set screen to red
//     if(warn){
//         lcd.SelectLayer(BACKGROUND);
//         lcd.Clear(LCD_COLOR_RED);
//         lcd.SetBackColor(LCD_COLOR_RED);
//     }else{
//         lcd.SelectLayer(BACKGROUND);
//         lcd.Clear(LCD_COLOR_BLACK);
//         lcd.SetBackColor(LCD_COLOR_BLACK);
//     }
//     //update value in buffer
//     snprintf(display_buf[0],60,"Temperature: %d ",temp);
//     snprintf(display_buf[1],60,"Moisture: %d ",moi);
//     snprintf(display_buf[2],60,"Pressure: %d ",lcd.GetXSize());
//     lcd.SelectLayer(FOREGROUND);
//     //display
//     lcd.DisplayStringAt(0, LINE(1), (uint8_t *)display_buf[0], LEFT_MODE);
//     lcd.DisplayStringAt(0, LINE(2), (uint8_t *)display_buf[1], LEFT_MODE);
//     lcd.DisplayStringAt(0, LINE(3), (uint8_t *)display_buf[2], LEFT_MODE);
// }


// int main() {
//   setupScreen();

//     int initial = 0;
//   while(1){
//     //first 10s initialization
//     if (initial<=10){
//         displayValue(0,0,0,false,true);
//         initial++;
//         thread_sleep_for(1000);
//         continue;
//     }

//     //processing
//     updateValue(++temp,++moi,++pres);
//     if(temp%10==0){ //demo condition
//         displayValue(temp,moi,pres,true,false);
//     }else{
//         displayValue(temp,moi,pres,false,false);
//     }
//     thread_sleep_for(1000);

//   }
// }