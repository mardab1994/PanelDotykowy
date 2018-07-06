#include <stdint.h>
#include <HX8347D.h>
#include <SPI.h>
#include <XPT2046.h>
#include <Touch.h>

int x=0;    // Współrzędna x punktu dotyku
int y=0;    // Współrzędna y punktu dotyku
int gUP=6,  // Godzina podniesienia rolet
    mUP=45, // Minuta podniesienia rolet
    gDW=16, // Godzina opuszczenia rolet
    mDW=40; // Minuta opuszczenia rolet


// W A R U N K I   P O G O D O W E
float T_in=30;    // Temperatura wewnętrzna
float T_out=20;   // Temperatura zewnętrzna 
float T_inOld=T_in, T_outOld=T_out, TT_outOld, TT_out;  // Zmienne pomocnicze przechowujące poprzednie wartości temperatur

int Wilgotnosc=70;              // Wilgotność powietrza wewnątrz pomieszczenia
int WilgotnoscOld=Wilgotnosc;   // Zmienna pomocnicza przechowująca poprzednią wartość wilgotności powietrza 

int rain=NULL;      // Flaga przecowująca informację o opadach deszczu 1-pada deszcz / 0-nie pada deszcz
int rainOld=rain;   // Flaga pomocnicza przechowująca poprzednia stan opadów

// C Z A S   I   D A T A
int g=12;             // Aktualna godzina
int m=15;             // Aktualna minuta
int gOld=12, mOld=15; // Zmienne pomocnicze przechowujące poprzednia wartości zmiennych przechowujących aktualny czas

int day=NULL;     // Numer dnia miesiąca
int month=NULL;   // Numer miesiąca
int year1=NULL;   // Rok 
int dayOld=day, monthOld=month, year1Old=year1; // Zmienne pomocnicze przechowujące poprzednie wartości zmiennych przechowujących datę

int dzienTygodnia=NULL;       // Numer dnia tygodnia
int dzienTygodniaOld=NULL;    // Zmienna pomocnicza przechowująca poprzednią wartość zmiennej 

//char miesiac[4],miesiacOld[4];

//int G_up=6, M_up=50;
//int G_dw=18, M_dw=25;

// FLAGI AKTUALNYCH POŁOŻEŃ OKIEN 
int oknoK=NULL;   // Flaga przechowująca położenie okna kuchennego
int oknoM=NULL;   // Flaga przechowująca położenie okna małego
int oknoB=NULL;   // Flaga przechowująca położenie drzwi balkonowych
int gaz=NULL;     // Zmienna przechowująca aktualną wartość zwróconą przez czujnik gazu

// FLAGI ODPOWIADAJĄCE ZA WYSWIETLENIE IKON POGODOWYCH
bool setSun=false;    // Ikona Słońca       false-nie ustawiona / true-ustawiona
bool setMoon=false;   // Ikona Księżyca
bool setRain=false;   // Ikona Deszczu


int alarm=1; //flaga odpowiadająca za aktywację alarmu 1-alarm aktywny/ 0 alarm nieaktywny

//MAKSIMA MINIMA
int T_in_Min=100,       T_in_Max=-100;
int T_out_Min=100,      T_out_Max=-100;
int wilgotnosc_Min=100, wilgotnosc_Max=0;


// F U N K C J A   I N I C J A L I Z U J Ą C A
void setup()
{ 

  
// inicjalizacja magistrali SPI
   __SD_CS_DISABLE();
   
    SPI.setDataMode(SPI_MODE3);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV4);
    SPI.begin();
    
    Serial.begin(9600);               // inicjalizacja portu szeregowego 
    Tft.lcd_init();                   // inicjalizacja ekranu
    Tft.lcd_clear_screen(BLACK);      // ustawienie koloru tła na kolor czarny 
    
    top();
    domek();
    Tp.tp_init();             // inicjalizacja panelu dotykowego 
    int i=Tp.tp_scan(1);

    //DEMO DEMO DEMO
    drawSun();
    //delay(2000);
    deleteSun();

    drawCloud();
    //delay(2000);
    deleteCloud();

    drawRainCloud();
    //delay(2000);
    deleteRainCloud();

    drawMoon();
    //delay(2000);
    deleteMoon();
}


void loop()
{

    int j=Tp.tp_scan(1);  //pobranie współrzędnych dotyku
    x=Tp.getX();    
    y=Tp.getY();

// wyświetlenie pobranych wspołrzędnych dotyku    
    // Serial.print("X=");
    // Serial.print(x);
    // Serial.print("     Y=");
    // Serial.println(y);

// Sprawdzenie czy użytkownik kliknął w ikonkę domku, jeżeli tak to zostaje przeniesiony do menu głównego     
    if(x>188 && x<1233 && y<1522 && y>1050)//   KASUJE PULPIT GŁÓWNY 
    {
       kasujDomek();
       menu();
       x=0;y=0;
    }
// W godzinach 6-19 usuwana jest z ekranu ikona księżyca ,a wyświetlana jest ikonka pogodowa z wizerunkiem słońca symbolizująca dzień
// Po wyświetleniu ikony ustawiana jest flaga setSun=true oznaczajaca, że ikonka jest wyświetlona i nie ma 
// potrzeby ponownego jej wyświetlania.
// Ustawiona też zostaje flaga setMoon=false informująca, że jesli warunek czasu zostanie spełniony zaistnieje możliwość 
// wyświetlenie ikony ksiezyca.

  if(g>=6 && g<=19 && setSun==false)
  {
    deleteMoon();
    drawSun();
    setSun=true;
    setMoon=false;
  }

// W godzinach nocncych tj. 20-6 usuwana jest z ekranu ikona słońca i wyświetlana jest ikona z wizerunkiem półksiężyca.
// Po wyśietleniu ikony ustawiana jest flaga setMoon=true oznaczająca, że aktualnie wysielona jest ikona księzyca i 
// nie ma potrzeby ponownego jej wyświetlenia.
// Ustawiona też zostaje flaga setSun=false, informująca, że jeśli warunek czasu zostanie spełniony (godziny dzienne) to 
// możliwe będzie wyswietlenie ikony słońca. 
  if((g>19 && g<=23 && setMoon==false) || (g>=0 && g<6 && setMoon==false))
  {
    deleteSun();
    drawMoon();
    setMoon=true;
    setSun=false;
  }
  
  odbierz();  // Odbieranie danych z modułu głównego

// Sprawdzenie, czy ustawiona jest flaga opadów deszczu, 
  if(rain==1)
  {
    // gdy warunek spełniony to oznacza, że aktualnie pada deszcz 
    // i wyświetlona zostaje ikona chmury przysłaniająca ikonę słońca/księzyca
    // oraz zostaje wywołana funkcja odpowiadająca za animację opadu deszczu  
    drawCloud();
    drawRain();
   // setRain=true;
  }

// Sprawdzenie czy poprzedni stan opadów deszczu jest różny od aktualnego 
  if(rainOld!=rain)
  {
    // jeżeli tak, to z ekranu usuwana jest ikona pogodowa z chmurą oraz wyłączana 
    // jest animacja deszczu, nastepnie w zalezności od czasu - dzień/noc 
    // wyświetlana jest odpowiednia ikona 
     deleteRainCloud();
     if(g>=6 && g<=19)
     {
      drawSun();
     }
     if((g>19 && g<=23) || (g>=0 && g<6))
     {
      drawMoon();
     }
  //  deleteCloud();
  }
  rainOld=rain; // aktualizacja flagi opadów deszczu, stan z przed chwili jest stanem poprzednim  



  top();          // Wyświetlenie górnej stopki z czasem i datą
  drawTemp();     // Wyświetlenie na ekranie aktualnych temperatur i wilgotności 

minMax();
 // wyslijDoCentralki();
  //wyslijDoCentralki();
 // drawRainCloud();

}
//----------------< < FUNKCJE FUNKCJE > >-------------------------------
int Max(int a, int b)
{
  if(a>b)return a;
  return b;
}
int Min(int a, int b)
{
  if(a<b)return a;
  return b;
}

void minMax()
{
  T_in_Min=Min(T_in_Min, T_in);
  T_out_Min=Min(T_out_Min,T_out);
  wilgotnosc_Min=Min(wilgotnosc_Min, Wilgotnosc);

  T_in_Max=Max(T_in_Max, T_in);
  T_out_Max=Max(T_out_Max,T_out);
  wilgotnosc_Max=Max(wilgotnosc_Max, Wilgotnosc);
}

/*------------------< < drawTemp > >------------------------------------
 * Zadaniem funkcji jest wyświetlenie na ekranie aktualnej temperatury
 * wewnętrznej, zewnetrznej oraz wilgotości powietrza.
 * Pisanie na ekranie odbywa się poprzez zapalanie poszczególnych pixeli
 * i nadpisanie aktualnego stanu na ekranie, dlatego sprawdzane jest czy 
 * wyświetlona temperatura jest identyczna z aktualnie pobraną temperaturą.
 * 
 * Jeżeli aktualna temperatura jest różna od wyświetlonej to następuje 
 * skasowanie starej wartości z ekranu poprzez wypisanie jej w kolorze tła,
 * po czym nastepuje wypisanie nowej wartości.
 * 
 * T_in - aktualna temperatura wewnetrzna
 * T_inOld - poprzednia temperatura wewnętrzna 
 * 
 * T_out - aktualna temperatura zewnętrzna
 * T_outOld - poprzednia temperatura wewnętrzna
 * 
 * Wilgotność - aktualna wilgotność  powietrza
 * WilgotnośćOld - poprzednia zarejestrowana wilgotność powietrza
 *-----------------------------------------------------------------------*/
void drawTemp()
{ 

  //Sprawdzenie czy wyświetlana temperatura jest różna od aktualnie popranej
     if(T_inOld!=T_in){Tft.lcd_display_num(117,125,T_inOld,4,FONT_1608, WHITE);}
     Tft.lcd_display_num(117,125,T_in,4,FONT_1608, BLACK);//in
     Tft.lcd_display_string(152, 125, (const uint8_t *)"'C", FONT_1608, BLACK);
     T_inOld=T_in;// atualizacja zmiennej, 

// Wyświetlenie temperatury zewnętrznej 
     if(T_out>=0) // wyświetlenie temperatury dodatniej
     {
      if(T_outOld!=T_out){ Tft.lcd_display_num(184,50,T_outOld,4,FONT_1608, BLACK);}//out
      Tft.lcd_display_num(184,50,T_out,4,FONT_1608, WHITE);//out
      Tft.lcd_display_string(219, 50, (const uint8_t *)"'C", FONT_1608, WHITE);
      Tft.lcd_display_string(184, 50, (const uint8_t *)"-", FONT_1608, BLACK);
      T_outOld=T_out;
     }
     else // W przypadku, gdy temperatura jest ujemna, należy ręcznie wypisać znak minus "-"
     {    // i wypisać temperaturę pomnożoną *-1
      TT_out=-1*T_out;
      if(TT_outOld!=TT_out){Tft.lcd_display_num(184,50,TT_outOld,4,FONT_1608, BLACK);}//out
      Tft.lcd_display_string(184, 50, (const uint8_t *)"-", FONT_1608, WHITE);
      Tft.lcd_display_num(184,50,TT_out,4,FONT_1608, WHITE);//out
      Tft.lcd_display_string(219, 50, (const uint8_t *)"'C", FONT_1608, WHITE);
      TT_outOld=TT_out;
     }
//wyswietlenie wilgotnisci powietrza
     if(WilgotnoscOld!=Wilgotnosc){ Tft.lcd_display_num(184,70,WilgotnoscOld,4,FONT_1608, BLACK);}
     { Tft.lcd_display_num(184,70,Wilgotnosc,4,FONT_1608, WHITE);}
     Tft.lcd_display_string(219, 70, (const uint8_t *)"%", FONT_1608, WHITE);
      WilgotnoscOld=Wilgotnosc;
}

/*-------------< < kasujTemp > > -----------------------------------
 * Zadaniem funkcji jest usunięcie z ekranu wyświetlanych temperatur
 * i wilgotności powietrza poprzez zapalenie w ich miejscu tych 
 * samych pixeli ale w kolorze tła - czarnym.
 *-----------------------------------------------------------------*/
void kasujTemp()//kasuje z ekranu tempaeraturu po wejsciu do menu
{
      Tft.lcd_display_string(184, 50, (const uint8_t *)"-", FONT_1608, BLACK);
      Tft.lcd_display_num(184,50,T_out,4,FONT_1608, BLACK);
      Tft.lcd_display_num(184,50,T_outOld,4,FONT_1608, BLACK);
      Tft.lcd_display_num(184,50,TT_outOld,4,FONT_1608, BLACK);
      Tft.lcd_display_string(219, 50, (const uint8_t *)"'C", FONT_1608, BLACK);
      Tft.lcd_display_string(219, 70, (const uint8_t *)"%", FONT_1608, BLACK);
      Tft.lcd_display_num(184,70,WilgotnoscOld,4,FONT_1608, BLACK);
}

/*--------------------------- < < odbierz > > ------------------------------
 * Funkcja sprawdza, czy na port szeregowy zostały nadesłane dane, jeżeli tak
 * to sprawdzane jest czy początkiem ramki danych jest kod = 666. Jeżeli 
 * rozpozano początek ramki to nastepuje odczyt kolejnych danych i przypisanie 
 * ich do odpowiednich zmiennych.
 * 
 * Parsowanie danych pozwala uzyskać dane o odpowiednim typie.
 *---------------------------------------------------------------------------*/
void odbierz()
{
  while (Serial.available())  // dopóki przychodzą dane z kuchni do zczytuj znaki i zapisuj do stringa readdata2
    {
     int kod=Serial.parseInt();
     if(kod==666)
     {
       T_in=Serial.parseFloat();
       T_out=Serial.parseFloat();
       Wilgotnosc=Serial.parseInt();    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

       rain=Serial.parseInt();
       
       g=Serial.parseInt();
       m=Serial.parseInt();
       day=Serial.parseInt();
       month=Serial.parseInt();
       year1=Serial.parseInt();

       gUP=Serial.parseInt();
       mUP=Serial.parseInt();
       gDW=Serial.parseInt();
       mDW=Serial.parseInt();

      // oknoK=Serial.parseInt();
      // oknoM=Serial.parseInt();
      // oknoB=Serial.parseInt();
     
       gaz=Serial.parseInt();
       
       dzienTygodnia=Serial.parseInt();//!!!!!!!!!!!!!!!!!!!!!!!!!!           !!!
     }
    } 
}

/*------------- < < wyslijDoCentralki > > ----------------------------------------
 * Funkcja tworzy ramkę danych o początku 666, w ramce do modułu głównego zostają 
 * wysłane zmodyfikowane dane: godzina i minuta podniesienia i opuszczenia 
 * rolet oraz flaga aktywacjii alarmu.
 *-------------------------------------------------------------------------------*/
void wyslijDoCentralki()
{
  int kod=666;

  Serial.println(kod);
  Serial.println(gUP);
  Serial.println(mUP);
  Serial.println(gDW);
  Serial.println(mDW);

  Serial.println(alarm);
}

/*------------------- < < top > > -----------------------------------------------------
 * Funkcja odpowiada za wyświetlenie w górnej częsci ekranu aktualnego czasu i daty
 * oraz za wyświetlenie linii oddzielającej od pozostałej częsci ekranu.
 * 
 * Wyświeltanie wszystkich zmiennych odbywa się poprzez zapalanie pixeli w kolorze białym,
 * w momencie zmiany wrtości zmiennej, stara wartość jest kasowana z ekranu poprzez 
 * wypisanie jej w kolorze tła, a nastepnie wypisana zostaje nowa wartość.
 *-------------------------------------------------------------------------------------*/
void top()//górna stopka z godziną i datą
{
// wyświetlenie czasu   
    if(gOld!=g){Tft.lcd_display_num(8,8,gOld,2,FONT_1608, BLACK);}
    Tft.lcd_display_num(8,8,g,2,FONT_1608, WHITE);
    gOld=g; //aktualizacja zmiennej 

    Tft.lcd_display_string(26, 8, (const uint8_t *)":", FONT_1608, WHITE);
    
    if(mOld!=m){    Tft.lcd_display_num(36,8,mOld,2,FONT_1608, BLACK);}
    Tft.lcd_display_num(36,8,m,2,FONT_1608, WHITE);
    mOld=m;//aktualizacja zmiennej
//---------------------dzien miesiaca-----------------------
    if(dayOld!=day){Tft.lcd_display_num(148-10,8,dayOld,2,FONT_1608, BLACK);}
    Tft.lcd_display_num(148-10,8,day,2,FONT_1608, WHITE);
    dayOld=day;//aktualizacja zmiennej

    Tft.lcd_display_string(168-10, 8, (const uint8_t *)"-", FONT_1608, WHITE);
//-----------------miesiac-------------------------------------------------
    
    if(monthOld!=month){getMiesiac(monthOld, 0);}
    getMiesiac(month,1);
    monthOld=month;//aktualizacja zmiennej

    Tft.lcd_display_string(196, 8, (const uint8_t *)"-", FONT_1608, WHITE);
//---------------------------rok rok rok------------------------------------------
    if(year1Old!=year1){ Tft.lcd_display_num(206,8,year1Old,4,FONT_1608, BLACK);}
    Tft.lcd_display_num(206,8,year1,4,FONT_1608, WHITE);
    year1Old=year1;//aktualizacja zmiennej

    
//---------------------------dzien tygodnia-----------------------------------------      !!!!!!!!!!!!!!!!!!!!!!!!!
   // Tft.lcd_display_string(184, 22, (const uint8_t *)"Piatek", FONT_1608, WHITE);
  if(dzienTygodniaOld!=dzienTygodnia){getDzienTygodnia(dzienTygodniaOld,0);}
  getDzienTygodnia(dzienTygodnia,1);
  dzienTygodniaOld=dzienTygodnia;

//-------------------------------------------------------------------------------------
    Tft.lcd_draw_line(0, 40, 239, 40, WHITE);//podkreslenie gory
    Tft.lcd_draw_line(0, 41, 239, 41, WHITE);//podkreslenie gory
}
 /*----------------------- < < getMiesiac > > -----------------------------------------------
  * Funkcja wypisuje na ekranie skrót aktualnego miesiąca na podstawie numeru miesiąca.
  * Parametetry wejściowe:
  * mies - numer miesiąca (1-12)
  * rysuj_kasuj - zmienna może przyjąć wartość 0 co oznacza, że miesiąc ma zostać skasowany 
  * z ekranu lub wartość 1 co oznacza wypisanie skrótu na ekranie
  *-----------------------------------------------------------------------------------------*/
void getMiesiac(int mies, int rysuj_kasuj)//wypisuje slownie nazwe aktualnego miesiaca
{
  if(mies==1 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Sty", FONT_1608, BLACK);return;}
  if(mies==2 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Lut", FONT_1608, BLACK);return;}
  if(mies==3 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Mar", FONT_1608, BLACK);return;}
  if(mies==4 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Kwi", FONT_1608, BLACK);return;}
  if(mies==5 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Maj", FONT_1608, BLACK);return;}
  if(mies==6 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Cze", FONT_1608, BLACK);return;}
  if(mies==7 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Lip", FONT_1608, BLACK);return;}
  if(mies==8 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Sie", FONT_1608, BLACK);return;}
  if(mies==9 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Wrz", FONT_1608, BLACK);return;}
  if(mies==10 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Paz", FONT_1608, BLACK);return;}
  if(mies==11 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Lis", FONT_1608, BLACK);return;}
  if(mies==12 && rysuj_kasuj==0){Tft.lcd_display_string(168, 8, (const uint8_t *)"Gru", FONT_1608, BLACK);return;}
//--------------------------
  if(mies==1 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Sty", FONT_1608, WHITE);return;}
  if(mies==2 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Lut", FONT_1608, WHITE);return;}
  if(mies==3 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Mar", FONT_1608, WHITE);return;}
  if(mies==4 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Kwi", FONT_1608, WHITE);return;}
  if(mies==5 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Maj", FONT_1608, WHITE);return;}
  if(mies==6 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Cze", FONT_1608, WHITE);return;}
  if(mies==7 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Lip", FONT_1608, WHITE);return;}
  if(mies==8 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Sie", FONT_1608, WHITE);return;}
  if(mies==9 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Wrz", FONT_1608, WHITE);return;}
  if(mies==10 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Paz", FONT_1608, WHITE);return;}
  if(mies==11 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Lis", FONT_1608, WHITE);return;}
  if(mies==12 && rysuj_kasuj==1){Tft.lcd_display_string(168, 8, (const uint8_t *)"Gru", FONT_1608, WHITE);return;}

}
 /*----------------------- < < getDzienTygodnia > > -----------------------------------------------
  * Funkcja wypisuje na ekranie nazwę aktualnego dnia tygodnia na podstawie numeru dnia tygodnia.
  * Parametetry wejściowe:
  * dzien - numer dnia tygodnia (1-7)
  * rysuj_kasuj - zmienna może przyjąć wartość 0 co oznacza, że nazwa dnia tygodnia ma zostać 
  * skasowana z ekranu lub wartość 1 co oznacza wypisanie na ekranie
  *-----------------------------------------------------------------------------------------*/
void getDzienTygodnia(int dzien, int rysuj_kasuj)
{
  if(dzien==1 && rysuj_kasuj==0){ Tft.lcd_display_string(140, 22, (const uint8_t *)"Poniedzialek", FONT_1608, BLACK);return;}
  if(dzien==2 && rysuj_kasuj==0){ Tft.lcd_display_string(190, 22, (const uint8_t *)"Wtorek", FONT_1608, BLACK);return;}
  if(dzien==3 && rysuj_kasuj==0){ Tft.lcd_display_string(195, 22, (const uint8_t *)"Sroda", FONT_1608, BLACK);return;}
  if(dzien==4 && rysuj_kasuj==0){ Tft.lcd_display_string(175, 22, (const uint8_t *)"Czwartek", FONT_1608, BLACK);return;}
  if(dzien==5 && rysuj_kasuj==0){ Tft.lcd_display_string(190, 22, (const uint8_t *)"Piatek", FONT_1608, BLACK);return;}
  if(dzien==6 && rysuj_kasuj==0){ Tft.lcd_display_string(190, 22, (const uint8_t *)"Sobota", FONT_1608, BLACK);return;}
  if(dzien==7 && rysuj_kasuj==0){ Tft.lcd_display_string(165, 22, (const uint8_t *)"Niedziela", FONT_1608, BLACK);return;}
//-------------------------------------------------------------------------------------------------------------------
  if(dzien==1 && rysuj_kasuj==1){ Tft.lcd_display_string(140, 22, (const uint8_t *)"Poniedzialek", FONT_1608, WHITE);return;}
  if(dzien==2 && rysuj_kasuj==1){ Tft.lcd_display_string(190, 22, (const uint8_t *)"Wtorek", FONT_1608, WHITE);return;}
  if(dzien==3 && rysuj_kasuj==1){ Tft.lcd_display_string(195, 22, (const uint8_t *)"Sroda", FONT_1608, WHITE);return;}
  if(dzien==4 && rysuj_kasuj==1){ Tft.lcd_display_string(175, 22, (const uint8_t *)"Czwartek", FONT_1608, WHITE);return;}
  if(dzien==5 && rysuj_kasuj==1){ Tft.lcd_display_string(190, 22, (const uint8_t *)"Piatek", FONT_1608, WHITE);return;}
  if(dzien==6 && rysuj_kasuj==1){ Tft.lcd_display_string(190, 22, (const uint8_t *)"Sobota", FONT_1608, WHITE);return;}
  if(dzien==7 && rysuj_kasuj==1){ Tft.lcd_display_string(165, 22, (const uint8_t *)"Niedziela", FONT_1608, WHITE);return;}
}

/*------------------------ < < domek > > ------------------------------
 * Poprzez użycie funkcji rysujących linie o zadanym początku i końcu 
 * wyrysowana zostaje ikonka w postaci białego domku. 
 *----------------------------------------------------------------------*/
void domek()
{
    int x1=100, x11=x1, x2=200,x22=x2,y=140; // Współrzędne na ekranie określające położenie ikony domu
    for(int i=0;i<((x2-x1)/2);i++)
    {
          Tft.lcd_draw_line(x11+i, y-i, x22-i,y-i, WHITE);//rysuje linie jedna nad drugą rysując 
    }                                                     // w efekcie trójkąt stanowiący dach domu 
    for(int i=0;i<40;i++)
    {
        Tft.lcd_draw_line(x1+6,y+i,x2-6,y+i,WHITE);//rysuje poziome linie tworzace podstawe domu
    }
  
    Tft.lcd_fill_rect(145,  162,  10,   18, BLACK);   // rysowanie drzwi 
    Tft.lcd_draw_line(x1-5, 180,  x2+5, 180,WHITE);   // podwórko
    Tft.lcd_draw_line(x1-10,181,  x2+10,181,WHITE);   // podwórko-fundament


    
//--------------
    Tft.lcd_draw_line(0, 190, 239, 190, WHITE);   // rysowanie linii odzielającej  
    Tft.lcd_draw_line(0, 191, 239, 191, WHITE);   // --------- | | ---------
}

/*----------------------- < < kasujDomek > >----------------------------------------
 * Funkcja usuwa z ekranu ikony pogodowe, ikone domu oraz takie parametry jak 
 * aktualna temperarura czy wilgotność. 
 * Funkcja jest stosowana w celu wyczyszczenia częsci ekranu, co jest konieczne 
 * do wyrysowania menu.
 *--------------------------------------------------------------------------------------*/
void kasujDomek()
{
    deleteSun();          // wymazanie ikony słońca
    deleteCloud();        // wymazanie ikony chmury
    deleteRainCloud();    // wymazanie ikony kropel deszczu
    deleteMoon();         // wymazanie ikony księżyca

    setSun=false;     // wyzerowanie flag ikon pogodowych
    setMoon=false;      
    setRain=false;     

    // Wymazanie ikony domku odbywa się poprzez jej ponowne wyrysowanie w kolorze 
    // tła 
    int x1=100, x11=x1, x2=200,x22=x2,y=140;
    
    for(int i=0;i<((x2-x1)/2);i++)
    {
          Tft.lcd_draw_line(x11+i, y-i, x22-i,y-i, BLACK);
    }
    for(int i=0;i<40;i++)
    {
        Tft.lcd_draw_line(x1+6,y+i,x2-6,y+i,BLACK);
    }
    Tft.lcd_fill_rect(145,  162,  10,   18, BLACK);
    Tft.lcd_draw_line(x1-5, 180,  x2+5, 180,BLACK);
    Tft.lcd_draw_line(x1-10,181,  x2+10,181,BLACK);

  kasujTemp();  // wywołanie funkcji wymazującej z ekranu wartości temperatur i wilgotności
//--------------
    Tft.lcd_draw_line(0, 190, 239, 190, BLACK);//podkreslenie 
    Tft.lcd_draw_line(0, 191, 239, 191, BLACK);//podkreslenie 
}

/*------------------ < < menu > > ------------------------------------------
 * Funkcja Wyświetla menu główne interfejsu użytkownika. W menu dostepne są 
 * następujące opcje:
 * ROLETY 
 * ALARM
 * CZUJNIKI
 * CZAS I DATA
 * POWRÓT
 * Wybranie jednej z pozycji powoduje przejście do odpowiedniego podmenu. 
 * Wybór odbywa sie poprzez kliknięcie w jedną z opcji. 
 * 
 * Rysowanie menu odbywa się poprzez wyrysowanie na ekranie prostokątnych ramek,
 * z odpowiednim napisem. 
 * 
 * Punkty dotknięcia są odczytywane, a na postawie uzyskanych współrzędnych x 
 * oraz y sprawdzane jest, którą opcję wybrał użytkownik.
 *---------------------------------------------------------------------------------*/
void menu()
{
   int j=0,m=0;
// Pętla wyświetlająca prostokątne ramki
   for(int i=50;i<240;i=i+48)
   {
      Tft.lcd_draw_rect(4,i, 231, 40, WHITE);
   }
   
    Tft.lcd_draw_rect(4,270, 231, 40, RED); // Opcja powrotu jest wyrysowana w kolorze czerownym
// Wypisanie nazw podmenu w ramkach
    Tft.lcd_display_string(45, 63,  (const uint8_t *) "- - R O L E T Y - - ",     FONT_1608, WHITE);   // 1
    Tft.lcd_display_string(45, 111, (const uint8_t *)"- -  A L A R M - - ",      FONT_1608, WHITE);   // 2
    Tft.lcd_display_string(30, 160, (const uint8_t *)"- - C Z U J N I K I - - ",  FONT_1608, WHITE);   // 3
    Tft.lcd_display_string(45, 206, (const uint8_t *)"- - CZAS I DATA - - ",      FONT_1608, WHITE);   // 4
    Tft.lcd_display_string(45, 283, (const uint8_t *)"- - P O W R O T - - ",      FONT_1608, RED);     // 5
//wykonanie odczytu z panelu dotykowego
    j=Tp.tp_scan(1);
    x=Tp.getX();
    y=Tp.getY();
    x=-1;y=-1;

//Pętla wykonująca odczyt z panelu dotykowego do moemntu wybrania jednej z opcji
    do{
      
      odbierz();
//wykonanie odczytu z panelu dotykowego
      j=Tp.tp_scan(1);
      x=Tp.getX();
      y=Tp.getY();
// sprawdzenie uzyskanych z pomiaru współrzędnych dotyku 
           if( y<1631 && y>1530) { m=1;}
      else if(y<1330 && y >1230) { m=2; }
      else if(y<1080 && y>990)   { m=3; }
      else if(y<800 && y>650)    { m=4; }
      else if(y<480 && y>230)    { m=5; }
    }while(m==0);

// Switch odpowiadający za przejście do wybranego podmenu
      switch (m){
      case 1: // wybranie menu sterowania roletami
          menuRolet();  
          //rolety 
          m=5;
          break;
      case 2: // wybranie menu odpowiadającego za sterowanie alarmem 
        Serial.println(" A L A R M");   
        //czas i data
        menuAlarm();
        m=5;
        break;
      case 3: // wybranie menu czujników
        Serial.println("CZUJNIKI");
        //czujniki
        menuCzujniki();
        m=5;
        break;
      case 4: 
        Serial.println("CZAS I DATA 2");
        //czas i data
        m=5;
        break;
      //wyjscie
      case 5:   //wyjscie z menu głównego
        break;
       }
    
//kasowanie menu z ekranu odbywa się poprzez ponowne wyrysowanie całego menu w kolorze tła

    for(int i=50;i<240;i=i+48)
    {
      Tft.lcd_draw_rect(4,i, 231, 40, BLACK);
    }
    Tft.lcd_draw_rect(4,270, 231, 40, BLACK);
    Tft.lcd_display_string(45, 63,  (const uint8_t *) "- - R O L E T Y - - ",    FONT_1608, BLACK);
    Tft.lcd_display_string(45, 111, (const uint8_t *)"- -  A L A R M - - ",     FONT_1608, BLACK);
    Tft.lcd_display_string(30, 160, (const uint8_t *)"- - C Z U J N I K I - - ", FONT_1608, BLACK);   // 3
    Tft.lcd_display_string(45, 206, (const uint8_t *)"- - CZAS I DATA - - ",     FONT_1608, BLACK);
    Tft.lcd_display_string(45, 283, (const uint8_t *)"- - P O W R O T - - ",     FONT_1608, BLACK);
// po wykasoaniu menu następuje wyrysowanie ikony domku
    domek();
}

/*----------------------< < menuRolet > > -------------------------------------------------------
 * Funkcja czyści ekran kasując z niego menu główne i wyswietla menu obsługi rolet. 
 * Ponad to menu obsługi rolet pozwala na niezależne ustawienie czasu podniesienia i opuszczenia
 * rolet z dokładnością do minuty.
 * W celu ustawienia:
 * -godziny należy kliknąć w ikonę G+ (powoduje zwiększenie wartości 
 *  zmiennej o 1) lub G- (powoduje zmniejszenie wartości zmiennej o 1).  
 * -minuty należy kliknąć w ikonę M+ (by zwiększyć wartość zmiennej) lub M-(by zmniejszyć 
 *  wartość zmiennej)
 *  
 *  Zatwierdzenie zmian i wysłanie nowych wartości zmiennych do modułu głównego następuje 
 *  po wybraniu opcji "POWRÓT".
 *  
 *  Wysyłane dane: 
 *  -gUP godzina podniesienia rolet
 *  -mUP minuta podniesienia rolet
 *  -gDW godzina opuszczenia rolet
 *  -mDW minuta opuszczenia rolet
 *  
 *--------------------------------------------------------------------------------------------*/
void menuRolet()
{
// Czyszczenie ekranu ze starego menu 
    for(int i=98;i<240;i=i+48)
    {
       Tft.lcd_draw_rect(4,i, 231, 40, BLACK);
    }
    Tft.lcd_display_string(45, 111, (const uint8_t *)"- -  A L A R M - - ",     FONT_1608, BLACK);
    Tft.lcd_display_string(30, 160, (const uint8_t *)"- - C Z U J N I K I - - ", FONT_1608, BLACK);   // 3
    Tft.lcd_display_string(45, 206, (const uint8_t *)"- - CZAS I DATA - - ",     FONT_1608, BLACK);

    //rysuje nowe menu 
    //PODNOSZENIE
    drawRolety(1);    // wywołanie funkcji rysującej menu rolet
    
    int j=0,m=0;
    j=Tp.tp_scan(1);
    x=Tp.getX();
    y=Tp.getY();
    x=-1;y=-1;
    do{
      
      j=Tp.tp_scan(4);    //odczyt współrzędnych dotyku z panelu 
      x=Tp.getX();
      y=Tp.getY();
    
      // Serial.print("X=");    Serial.print(x);    Serial.print("     Y=");    Serial.println(y);

      // ustawienie czasu podniesienia i opuszczenia rolet odbywa się poprzez porównanie współrzędnych
      // dotyku z wartościami odpowiadającymi położeniom odpowiednich przecisków na wyświetlaczu 
 
         if(x<1850 && x>1590 && y<1240 && y>1060) {kasujGodziny( gUP,  mUP,  gDW, mDW); gUP++; }  // inkrementacja gUP
    else if(x<1440 && x>1160 && y<1240 && y>1060) {kasujGodziny( gUP,  mUP,  gDW, mDW); gUP--; }  // dekrementacja gUP
    else if(x<880  && x>600  && y<1240 && y>1060) {kasujGodziny( gUP,  mUP,  gDW, mDW); mUP++; }
    else if(x<460  && x>230  && y<1240 && y>1060) {kasujGodziny( gUP,  mUP,  gDW, mDW); mUP--; }
    
    else if(x<1850 && x>1590 && y<760 && y>560) {kasujGodziny( gUP,  mUP,  gDW, mDW); gDW++; }
    else if(x<1440 && x>1160 && y<760 && y>560) {kasujGodziny( gUP,  mUP,  gDW, mDW); gDW--; }
    else if(x<880  && x>600  && y<760 && y>560) {kasujGodziny( gUP,  mUP,  gDW, mDW); mDW++; }
    else if(x<460  && x>230  && y<760 && y>560) {kasujGodziny( gUP,  mUP,  gDW, mDW); mDW--; }
    
    else if(y<480 && y>50)    { m=5; }// przycisk POWRÓT - wyjście z menu
   
    if(gDW==24)gDW=0;     // format godziny jest 0-23 wiec gdy zmienna gUP lub gDW osiągnie 24 to do zmiennych zapisywane jest 0
    if(gUP==24)gUP=0;
    if(mDW==60)mDW=0;
    if(mUP==60)mUP=0;
    //
    if(gDW<0)gDW=23;    // gdy dekrementujemy zmienne i soiągniemy wartości <0 to ustawiamy na 23
    if(gUP<0)gUP=23;
    if(mDW==-1)mDW=59;
    if(mUP==-1)mUP=59;

// Wyświetlenie zmiennych gUP, mUP, gDW, mDW po aktualizacji
    Tft.lcd_display_num(100,103,gUP,2,FONT_1206, WHITE);  // 1
    Tft.lcd_display_num(120,103,mUP,2,FONT_1206, WHITE);  // 1
    Tft.lcd_display_num(100,190,gDW,2,FONT_1206, WHITE);  // 1
    Tft.lcd_display_num(120,190,mDW,2,FONT_1206, WHITE);  // 1
    delay(50);
 
}while(m!=5);   // wykonuj do póki nie zostanie wybrana opcja POWRÓT
    
       wyslijDoCentralki(); // wysłanie zmodyfikowanych danych do modułu głównego 


    //K A S O W A N I E 
//Wykasowanie z ekranu menu rolet 
    drawRolety(0);
    Tft.lcd_display_num(100,103,gUP,2,FONT_1206, BLACK);  // 1
    Tft.lcd_display_num(120,103,mUP,2,FONT_1206, BLACK);  // 1
    Tft.lcd_display_num(100,190,gDW,2,FONT_1206, BLACK);  // 1
    Tft.lcd_display_num(120,190,mDW,2,FONT_1206, BLACK);  // 1
 
}

/*----------------------- < < drawRolety > > -------------------------------------
 * Funkcja odpowiada za wyrysowanie na ekranie pełnego menu rolet tj. 
 * Podzielenie ekranu na sekcję dotyczącą ustawienia czasu podniesienia rolet i na
 * sekcję ustawienia czasu opuszczenia rolet. 
 * 
 * Argument funkcji rysuj_kasuj przyjmuje dwie możliwe wartości 
 * 1-menu jest rysowane na ekranie
 * 0-menu jest kasowane z ekranu 
 * 
 *-----------------------------------------------------------------------------------*/
void drawRolety(int rysuj_kasuj)//1-rysuje  0 kasuje
{
  if(rysuj_kasuj==1)
  {
    Tft.lcd_draw_rect(4,98, 231, 80, WHITE);
    Tft.lcd_display_string(20, 103,  (const uint8_t *) "PODNOSZENIE ", FONT_1206, WHITE);  // 1
    Tft.lcd_display_string(113, 103, (const uint8_t *) ":",            FONT_1206, WHITE);  // 1

    Tft.lcd_draw_rect(12,125,  40, 40, WHITE); // G+    kwadratowe ramki 40x40pixeli będące przyciskami 
    Tft.lcd_draw_rect(60,125,  40, 40, WHITE); // G-
    Tft.lcd_draw_rect(139,125, 40, 40, WHITE); //M+
    Tft.lcd_draw_rect(187,125, 40, 40, WHITE); //M-
    
    Tft.lcd_display_string(25,  137, (const uint8_t *)"G+", FONT_1608, WHITE);  // wpisanie odpowiedniej literki w przycisk
    Tft.lcd_display_string(73,  137, (const uint8_t *)"G-", FONT_1608, WHITE);
    Tft.lcd_display_string(152, 137, (const uint8_t *)"M+", FONT_1608, WHITE);
    Tft.lcd_display_string(200, 137, (const uint8_t *)"M-", FONT_1608, WHITE);

//OPUSZCZANIE
    Tft.lcd_draw_rect(4,186, 231, 80, WHITE);
    Tft.lcd_display_string(20,  190,  (const uint8_t *) "OPUSZCZANIE ", FONT_1206, WHITE);  // 1
    Tft.lcd_display_string(113, 190,  (const uint8_t *) ":", FONT_1206, WHITE);  // 1

    Tft.lcd_draw_rect(12,213,  40, 40, WHITE);      // G+
    Tft.lcd_draw_rect(60,213,  40, 40, WHITE);      // G-
    Tft.lcd_draw_rect(139,213, 40, 40, WHITE);     // M+
    Tft.lcd_draw_rect(187,213, 40, 40, WHITE);     // M-

    Tft.lcd_display_string(25,  225, (const uint8_t *)"G+", FONT_1608, WHITE);
    Tft.lcd_display_string(73,  225, (const uint8_t *)"G-", FONT_1608, WHITE);
    Tft.lcd_display_string(152, 225, (const uint8_t *)"M+", FONT_1608, WHITE);
    Tft.lcd_display_string(200, 225, (const uint8_t *)"M-", FONT_1608, WHITE);
  }
  else
  {
    Tft.lcd_draw_rect(4,98,  231, 80, BLACK);
    Tft.lcd_draw_rect(4,186, 231, 80, BLACK);
    Tft.lcd_display_string(20,  103,  (const uint8_t *) "PODNOSZENIE ", FONT_1206, BLACK);  // 1
    Tft.lcd_display_string(20,  190,  (const uint8_t *) "OPUSZCZANIE ", FONT_1206, BLACK);  // 1
    Tft.lcd_display_string(113, 103,  (const uint8_t *) ":", FONT_1206, BLACK);  // 1
    Tft.lcd_display_string(113, 190,  (const uint8_t *) ":", FONT_1206, BLACK);  // 1
    Tft.lcd_draw_rect(12,125,  40, 40, BLACK); //G+
    Tft.lcd_draw_rect(60,125,  40, 40, BLACK); //G-
    Tft.lcd_draw_rect(12,213,  40, 40, BLACK); //G+
    Tft.lcd_draw_rect(60,213,  40, 40, BLACK); //G-
    Tft.lcd_draw_rect(139,125, 40, 40, BLACK); //M+
    Tft.lcd_draw_rect(187,125, 40, 40, BLACK); //M-
    Tft.lcd_draw_rect(139,213, 40, 40, BLACK);     // M+
    Tft.lcd_draw_rect(187,213, 40, 40, BLACK);     // M-

    Tft.lcd_display_string(25,  137, (const uint8_t *)"G+", FONT_1608, BLACK);
    Tft.lcd_display_string(73,  137, (const uint8_t *)"G-", FONT_1608, BLACK);
    Tft.lcd_display_string(152, 137, (const uint8_t *)"M+", FONT_1608, BLACK);
    Tft.lcd_display_string(200, 137, (const uint8_t *)"M-", FONT_1608, BLACK);
    Tft.lcd_display_string(25,  225, (const uint8_t *)"G+", FONT_1608, BLACK);
    Tft.lcd_display_string(73,  225, (const uint8_t *)"G-", FONT_1608, BLACK);
    Tft.lcd_display_string(152, 225, (const uint8_t *)"M+", FONT_1608, BLACK);
    Tft.lcd_display_string(200, 225, (const uint8_t *)"M-", FONT_1608, BLACK);
  }
}

/*------------------------- < < kasujGodziny > > -----------------------------
 * Funkcja kasuje z ekranu stare wartości zmiennych przechowujących 
 * wartości odpowiadające czasom podniesienia rolet.
 *----------------------------------------------------------------------------*/
void kasujGodziny(int gUP, int mUP, int gDW, int mDW)
{
    Tft.lcd_display_num(100,103,gUP,2,FONT_1206, BLACK);  // 1
    Tft.lcd_display_num(120,103,mUP,2,FONT_1206, BLACK);  // 1
    Tft.lcd_display_num(100,190,gDW,2,FONT_1206, BLACK);  // 1
    Tft.lcd_display_num(120,190,mDW,2,FONT_1206, BLACK);  // 1
}

void menuCzujniki()
{
    odbierz();//////////////////
  
    int j=0,m=0;
    drawCzujniki(1);

    j=Tp.tp_scan(1);
    x=Tp.getX();
    y=Tp.getY();
    x=-1;y=-1;
    do{
      j=Tp.tp_scan(1);
      x=Tp.getX();
      y=Tp.getY();
//wyswietlanie jakis danych 
      if(y<480 && y>230)    { m=5; }
      
    }while(m==0);
//skasowanie menu czujniki 
    drawCzujniki(0);
}

void drawCzujniki(int kasuj_rysuj)
{
  int delta=20;
  if(kasuj_rysuj==1)
  {
//kasowanie starego menu 
    for(int i=98;i<240;i=i+48)
    {
      Tft.lcd_draw_rect(4,i, 231, 40, BLACK);
    }
    Tft.lcd_display_string(45, 63,  (const uint8_t *)"- - R O L E T Y - - ",     FONT_1608, BLACK);
    Tft.lcd_display_string(45, 111, (const uint8_t *)"- -  A L A R M - - ",     FONT_1608, BLACK);
    Tft.lcd_display_string(30, 160, (const uint8_t *)"- - C Z U J N I K I - - ", FONT_1608, BLACK);   
    Tft.lcd_display_string(45, 206, (const uint8_t *)"- - CZAS I DATA - - ",     FONT_1608, BLACK);
//rysowanie nowego 
    Tft.lcd_display_string(30, 63, (const uint8_t *)"- - C Z U J N I K I - - ",  FONT_1608, WHITE);   // 3
    
// wewnetrzna 
Tft.lcd_display_string(8, 95, (const uint8_t *)"T_wew_Max = ",  FONT_1608, WHITE);   // 3
Tft.lcd_display_num(99,95,T_in_Max,2,FONT_1608, WHITE);
Tft.lcd_display_string(119, 95, (const uint8_t *)"'C",  FONT_1608, WHITE);   // 3

Tft.lcd_display_string(8, 95+delta, (const uint8_t *)"T_wew_Min = ",  FONT_1608, WHITE);   // 3
Tft.lcd_display_num(99,95+delta,T_in_Min,2,FONT_1608, WHITE);
Tft.lcd_display_string(119, 95+delta, (const uint8_t *)"'C",  FONT_1608, WHITE);   // 3

//zewnetrzna

Tft.lcd_display_string(8, 95+2*delta, (const uint8_t *)"T_zew_Max = ",  FONT_1608, WHITE);   // 3
if(T_out_Max<0)
{
  Tft.lcd_display_string(99, 95+2*delta, (const uint8_t *)"-",  FONT_1608, WHITE);   // 3
  Tft.lcd_display_num(110,95+2*delta,((-1)*T_out_Max),2,FONT_1608, WHITE);
  Tft.lcd_display_string(130, 95+2*delta, (const uint8_t *)"'C",  FONT_1608, WHITE);   // 3
}
else
{
  Tft.lcd_display_num(99,95+2*delta,T_out_Max,2,FONT_1608, WHITE);
  Tft.lcd_display_string(119, 95+2*delta, (const uint8_t *)"'C",  FONT_1608, WHITE);   // 3
}
Tft.lcd_display_string(8, 95+3*delta, (const uint8_t *)"T_zew_Min = ",  FONT_1608, WHITE);   // 3
if(T_out_Min<0)
{
  Tft.lcd_display_string(99, 95+3*delta, (const uint8_t *)"-",  FONT_1608, WHITE);   // 3
  Tft.lcd_display_num(110,95+3*delta,((-1)*T_out_Min),2,FONT_1608, WHITE);
  Tft.lcd_display_string(130, 95+3*delta, (const uint8_t *)"'C",  FONT_1608, WHITE);   // 3
}
else
{
  Tft.lcd_display_num(99,95+3*delta,T_out_Min,2,FONT_1608, WHITE);
  Tft.lcd_display_string(119, 95+3*delta, (const uint8_t *)"'C",  FONT_1608, WHITE);   // 3
}

//wilgotnosc
Tft.lcd_display_string(8, 95+4*delta, (const uint8_t *)"Wilgotnosc_Max = ",  FONT_1608, WHITE);   // 3
Tft.lcd_display_num(140,95+4*delta,wilgotnosc_Max,2,FONT_1608, WHITE);
Tft.lcd_display_string(160, 95+4*delta, (const uint8_t *)"%",  FONT_1608, WHITE);   // 3

Tft.lcd_display_string(8, 95+5*delta, (const uint8_t *)"Wilgotnosc_Min = ",  FONT_1608, WHITE);   // 3
Tft.lcd_display_num(140,95+5*delta,wilgotnosc_Min,2,FONT_1608, WHITE);
Tft.lcd_display_string(160, 95+5*delta, (const uint8_t *)"%",  FONT_1608, WHITE);   // 3
          /*
      //MAKSIMA MINIMA
int T_in_Min=100,       T_in_Max=-100;
int T_out_Min=100,      T_out_Max=-100;
int wilgotnosc_Min=100, wilgotnosc_Max=0;
      */
      
  }
  else
  {
        Tft.lcd_draw_rect(4,270, 231, 40, BLACK);
        Tft.lcd_display_string(45, 283, (const uint8_t *)"- - P O W R O T - - ",    FONT_1608, BLACK);
        Tft.lcd_display_string(30, 63, (const uint8_t *)"- - C Z U J N I K I - - ", FONT_1608, BLACK);   // 3

        // wewnetrzna 
        Tft.lcd_display_string(8, 95, (const uint8_t *)"T_wew_Max = ",  FONT_1608, BLACK);   // 3
        Tft.lcd_display_num(99,95,T_in_Max,2,FONT_1608, BLACK);
        Tft.lcd_display_string(119, 95, (const uint8_t *)"'C",  FONT_1608, BLACK);   // 3

        Tft.lcd_display_string(8, 95+delta, (const uint8_t *)"T_wew_Min = ",  FONT_1608, BLACK);   // 3
        Tft.lcd_display_num(99,95+delta,T_in_Min,2,FONT_1608, BLACK);
        Tft.lcd_display_string(119, 95+delta, (const uint8_t *)"'C",  FONT_1608, BLACK);   // 3

        //zewnetrzna

      Tft.lcd_display_string(8, 95+2*delta, (const uint8_t *)"T_zew_Max = ",  FONT_1608, BLACK);   // 3
      if(T_out_Max<0)
      {
        Tft.lcd_display_string(99, 95+2*delta, (const uint8_t *)"-",  FONT_1608, BLACK);   // 3
        Tft.lcd_display_num(110,95+2*delta,((-1)*T_out_Max),2,FONT_1608, BLACK);
        Tft.lcd_display_string(130, 95+2*delta, (const uint8_t *)"'C",  FONT_1608, BLACK);   // 3
      }
      else
      {
        Tft.lcd_display_num(99,95+2*delta,T_out_Max,2,FONT_1608, BLACK);
        Tft.lcd_display_string(119, 95+2*delta, (const uint8_t *)"'C",  FONT_1608, BLACK);   // 3
      }
      Tft.lcd_display_string(8, 95+3*delta, (const uint8_t *)"T_zew_Min = ",  FONT_1608, BLACK);   // 3
      if(T_out_Min<0)
      {
        Tft.lcd_display_string(99, 95+3*delta, (const uint8_t *)"-",  FONT_1608, BLACK);   // 3
        Tft.lcd_display_num(110,95+3*delta,((-1)*T_out_Min),2,FONT_1608, BLACK);
        Tft.lcd_display_string(130, 95+3*delta, (const uint8_t *)"'C",  FONT_1608, BLACK);   // 3
      }
      else
      {
        Tft.lcd_display_num(99,95+3*delta,T_out_Min,2,FONT_1608, BLACK);
        Tft.lcd_display_string(119, 95+3*delta, (const uint8_t *)"'C",  FONT_1608, BLACK);   // 3
      }

      //wilgotnosc
      Tft.lcd_display_string(8, 95+4*delta, (const uint8_t *)"Wilgotnosc_Max = ",  FONT_1608, BLACK);   // 3
      Tft.lcd_display_num(140,95+4*delta,wilgotnosc_Max,2,FONT_1608, BLACK);
      Tft.lcd_display_string(160, 95+4*delta, (const uint8_t *)"%",  FONT_1608, BLACK);   // 3

      Tft.lcd_display_string(8, 95+5*delta, (const uint8_t *)"Wilgotnosc_Min = ",  FONT_1608, BLACK);   // 3
      Tft.lcd_display_num(140,95+5*delta,wilgotnosc_Min,2,FONT_1608, BLACK);
      Tft.lcd_display_string(160, 95+5*delta, (const uint8_t *)"%",  FONT_1608, BLACK);   // 3
  }
}

//---------------------------

/*---------------------------- < < menuAlarm > > -----------------------
 * Funkcja odpowiada za wyswietlenie menu alarmu. 
 * Włączanie alarmu odbywa sie poprzez kliknięcie przycisku ON (flaga alarm 
 * zostaje ustawiona na 1), a wyłączenie poprzez kliknięcie przycisku OFF  
 * (a flaga alarm ustawiona zostaje na 0). W zależności od tego czy alarm 
 * jest włączony czy wyłączony wyśietlany jest odpowiedni komunikat.
 * 
 * Kliknięcie przycisku POWRÓT powoduje wyjście z menu i zapisanie zmian oraz 
 * wysłanie ich do modułu głównego. 
 * 
 *-----------------------------------------------------------------------------------*/
void menuAlarm()
{
    odbierz();//////////////////
  
    int j=0,m=0;
    drawAlarm(1); // wywołanie funkcji rysującej menu 

    j=Tp.tp_scan(1);
    x=Tp.getX();
    y=Tp.getY();
    x=-1;y=-1;
    do{
      j=Tp.tp_scan(1);      // pobranie współrzędnych dotyku 
      x=Tp.getX();
      y=Tp.getY();

// porównanie współrzędnych dotyku ze współrzędnymi przycisków 
      if(x<1850 && x>1160 && y<760 && y>560) {alarm=1;}
      if(x<741  && x>170  && y<760 && y>560)  {alarm=0;}    

// sprawdzenie czy alarm jest załączony 
      if(alarm==1)
      {
        // Jeśli jest załączony to w zielonej ramce wyświetlany jest komunikat 
        // "WLACZONY" informujący o tym, że alarm jest aktywny 
             Tft.lcd_display_string(45, 111, (const uint8_t *)" W Y L A C Z O N Y",     FONT_1608, BLACK);  // wykasowanie z ekranu poprzedniego komunikatu
             
             Tft.lcd_draw_rect(4,98, 231, 40, GREEN);
             Tft.lcd_display_string(45, 111, (const uint8_t *)"  W L A C Z O N Y",     FONT_1608, GREEN);
      }
      else
      {
        // Jezeli alarm nie jest włączony to stosowny komunikat zostaje wysietlony 
        // w czerwonej ramce
             Tft.lcd_display_string(45, 111, (const uint8_t *)"  W L A C Z O N Y",     FONT_1608, BLACK);
             Tft.lcd_draw_rect(4,98, 231, 40, RED);
             Tft.lcd_display_string(45, 111, (const uint8_t *)" W Y L A C Z O N Y",     FONT_1608, RED);
             
      }
  
      if(y<480 && y>230)    { m=5; }  // sprawdzenie czy uzytkownik kliknął POWRÓT
    }while(m==0);
    drawAlarm(0);           // skasowanie menu alarmu z ekranu 
    wyslijDoCentralki();    // wysłanie zmodyfikowanych danyc do modułu głównego 
}

/*------------------------ < < drawAlarm > > -----------------------------------------
 * Funkcja Kasuje z ekranu menu główne i wyswietla menu alarmu. 
 * Wyswietlany jest nagłówek podmenu "ALARM" i odpowiednie przyciski ON/OFF.
 * 
 * W zależności od wartości argumentu rysuj_kasuj 
 * 1-funkcja rysuje menu na ekranie
 * 0-funkcja kasuje menu z ekranu 
 *----------------------------------------------------------------------------------------*/
void drawAlarm(int kasuj_rysuj)
{
  if(kasuj_rysuj==1)
  {
//kasowanie starego menu 
    for(int i=98;i<240;i=i+48)//146
    {
      Tft.lcd_draw_rect(4,i, 231, 40, BLACK);
    }
    Tft.lcd_display_string(45, 63,  (const uint8_t *)"- - R O L E T Y - - ",     FONT_1608, BLACK);
    Tft.lcd_display_string(45, 111, (const uint8_t *)"- -  A L A R M - - ",     FONT_1608, BLACK);
    Tft.lcd_display_string(30, 160, (const uint8_t *)"- - C Z U J N I K I - - ", FONT_1608, BLACK);   
    Tft.lcd_display_string(45, 206, (const uint8_t *)"- - CZAS I DATA - - ",     FONT_1608, BLACK);
//rysowanie nowego 
    Tft.lcd_display_string(30, 63, (const uint8_t *)"  - -  A L A R M - - ",  FONT_1608, WHITE);   // 3


    Tft.lcd_draw_rect(4,213,  40+68, 40, GREEN);      // on
    Tft.lcd_draw_rect(128,213, 40+68, 40, RED);     // off

    Tft.lcd_display_string(25,  225, (const uint8_t *)"   O N", FONT_1608, WHITE);
    Tft.lcd_display_string(152, 225, (const uint8_t *)" O F F", FONT_1608, WHITE);


    
  }
  else
  {
        Tft.lcd_draw_rect(4,270, 231, 40, BLACK);
        Tft.lcd_display_string(45, 283, (const uint8_t *)"- - P O W R O T - - ",    FONT_1608, BLACK);
        Tft.lcd_display_string(30, 63, (const uint8_t *)"  - -  A L A R M - - ", FONT_1608, BLACK);   // 3
        Tft.lcd_draw_rect(4,98, 231, 40, BLACK);
        Tft.lcd_display_string(45, 111, (const uint8_t *)"  W L A C Z O N Y",     FONT_1608, BLACK);
        Tft.lcd_display_string(45, 111, (const uint8_t *)" W Y L A C Z O N Y",     FONT_1608, BLACK);

  Tft.lcd_draw_rect(4,213,  40+68, 40, BLACK);      // on
    Tft.lcd_draw_rect(128,213, 40+68, 40, BLACK);     // off

    Tft.lcd_display_string(25,  225, (const uint8_t *)"   O N", FONT_1608, BLACK);
    Tft.lcd_display_string(152, 225, (const uint8_t *)" O F F", FONT_1608, BLACK);
  }
}
//---------------------------


// F U N K C J E   R Y S U J Ą C E   I K O N Y   P O G O D O W E

/*------------ < < drawRain > > --------------------------------
 * Funkcja odpowiada za wyrysowanie prostej animacji deszczu.
 * Krople deszczu rysowane są jako krótkie, skośne linie,
 * rysowane i wymazywane w odstępach czasowych
 *--------------------------------------------------------------------------*/
void drawRain()
{
  int a=45, b=90;
  int radius=20;
  
  Tft.lcd_draw_line(a,    b+radius+5,  a-3,   b+radius+16,BLUE);
  Tft.lcd_draw_line(a+15, b+radius+8,  a+15-3,b+radius+21,BLUE);
  delay(10);
  Tft.lcd_draw_line(a+30, b+radius+5,  a+30-3,b+radius+18,BLUE);
  delay(10);
  Tft.lcd_draw_line(a,    b+radius+5,  a-3,   b+radius+16,BLACK);
  delay(10);
  Tft.lcd_draw_line(a+42, b+radius+3,  a+42-3,b+radius+16,BLUE);
        
  Tft.lcd_draw_line(a+15, b+radius+8,  a+15-3,b+radius+21,BLACK);
  delay(10);
  Tft.lcd_draw_line(a+30, b+radius+5,  a+30-3,b+radius+18,BLACK);
  Tft.lcd_draw_line(a+42, b+radius+3,  a+42-3,b+radius+16,BLACK);
  delay(10);
}

/*------------------ < < drawSun > > ---------------------
 * Funkcja rysująca na ekranie ikonę słońca. Rysowanie odbywa 
 * się metodą siłową, poprzez zapalenie pojedynczych pixeli w kolorze
 * żółtym wewnątrz koła o promieniu 20pixeli.
 * Pixele muszą spełniać warunek x^2+y^2<=r^2
 * 
 * Rysowanie promieni odbywa się poprzez rysowanie linii.
 *-----------------------------------------------------------*/
void drawSun()
{
  int a=38, b=90; // zmienne zapewniające przesunięce środka koła 
  int radius=20;
// Wyrysowanie koła 
  for(int y=-radius; y<=radius; y++)
    for(int x=-radius; x<=radius; x++)
        if(x*x+y*y <= radius*radius)
            Tft.lcd_draw_point(x+a, y+b, YELLOW);
// wyrysowanie promieni
  Tft.lcd_draw_line(a+radius+5,b, a+radius+16,b,YELLOW);
  Tft.lcd_draw_line(a-radius-5,b, a-radius-16,b,YELLOW);
  Tft.lcd_draw_line(a,b-radius-5, a,b-radius-16,YELLOW);
  Tft.lcd_draw_line(a,b+radius+5, a,b+radius+16,YELLOW);

  Tft.lcd_draw_line(a+radius-3,b+radius-3, a+radius+7,b+radius+7,YELLOW);
  Tft.lcd_draw_line(a+radius-3,b-radius+3, a+radius+7,b-radius-7,YELLOW);

  Tft.lcd_draw_line(a-radius+3,b-radius+3, a-radius-7,b-radius-7,YELLOW);
  Tft.lcd_draw_line(a-radius+3,b+radius-3, a-radius-7,b+radius+7,YELLOW);
}

/*-------------------< < drawCloud > > ----------------------
 * Funkcja wyrysowuje na ekranie ikonę chmury. Rysowanie chmury 
 * odbywa się poprzez wyrysowanie kilku kół, w kolorze niebieskim,
 * których środki są przesunięte względem siebie.
 * 
 * Koła podobnie jak w funkcji drawSun rysowane są metodą siłową
 *-------------------------------------------------------------*/
void drawCloud()
{
  int a=45, b=90;
  int radius=20;    // promień rysowanych kół 
  for(int y=-radius; y<=radius; y++)
  {
    for(int x=-radius; x<=radius; x++)
    {
        if(x*x+y*y <= radius*radius)
        {
            Tft.lcd_draw_point(x+a,    y+b,   BLUE);  // pierwsze koło
            Tft.lcd_draw_point(x+a+42, y+b,   BLUE);  // drugie koło przesunięte o 42pixele w prawo
            Tft.lcd_draw_point(x+a+20, y+b-7, BLUE);  // trzecie koło przesunięte o 20 pixeli w prawo -7pixeli w górę
            Tft.lcd_draw_point(x+a+15, y+b+5, BLUE);  // czwarte koło przesuniete o 15pixeli w prawo i 5 pixeli w dół
         }
      }
   }
}

/*--------------------- < < drawRainCloud > > --------------------
 * Funkcja stanowi wersję funkcji drawCloud rozszerzoną o wyrysowanie
 * statycznych kropel deszczu. 
 * Krople deszczu rysowane są jako proste wyrysowane pod pewnym kątem.
 *--------------------------------------------------------------------*/
void drawRainCloud()
{
  int a=45, b=90;
  int radius=20;
  for(int y=-radius; y<=radius; y++){
    for(int x=-radius; x<=radius; x++){
        if(x*x+y*y <= radius*radius){
            Tft.lcd_draw_point(x+a,    y+b,   BLUE);
            Tft.lcd_draw_point(x+a+42, y+b,   BLUE);
            Tft.lcd_draw_point(x+a+20, y+b-7, BLUE);
            Tft.lcd_draw_point(x+a+15, y+b+5, BLUE);
            }}}
        Tft.lcd_draw_line(a,b+radius+5,    a-3,b+radius+16,   BLUE);
        Tft.lcd_draw_line(a+15,b+radius+8, a+15-3,b+radius+21,BLUE);
        Tft.lcd_draw_line(a+30,b+radius+5, a+30-3,b+radius+18,BLUE);
        Tft.lcd_draw_line(a+42,b+radius+3, a+42-3,b+radius+16,BLUE);
}
/*----------------< < drawMoon > >----------------------------
 * Funkcja rysująca ikonę pogodową - półksiężyc. Rysowanie obdywa sie 
 * poprzez wyrysowanie koła (o promieniu radius=25) w kolorze gold, 
 * a nastepnie wyrysowanie drugiego koła o mniejszym promieniu (radius=23)
 * ,w kolorze tła i przesuniętym środku koła.
 * Wyrysowanie czarnego koła pozwala uzyskać w efekcie półksiężyc
 *----------------------------------------------------------------------*/
void drawMoon()
{
int a=45, b=90;
  int radius=25;
  for(int y=-radius; y<=radius; y++){
    for(int x=-radius; x<=radius; x++){
        if(x*x+y*y <= radius*radius){
            Tft.lcd_draw_point(x+a-22, y+b, GOLD);    
            }}}
radius=23;
  for(int y=-radius; y<=radius; y++){
    for(int x=-radius; x<=radius; x++){
        if(x*x+y*y <= radius*radius){
            Tft.lcd_draw_point(x+a-7-22, y+b-2, BLACK);    
            }}}

}

// F U N K C J E  K A S U J Ą C E  I K O N Y  P O G O D O W E
/* Wszystkie funkcje z przedroskiem delete działają analogicznie
 * do bliźniaczych funkcji draw z tą różnicą, że ich zadaniem 
 * jest wykasowanie ikon pogodych z ekranu poprzez poprzez ponowne 
 * ich wyrysowanie w tym samym miejscu ale w kolorze tła-czarnym.
 *---------------------------------------------------------------------*/
void deleteSun()
{
  int a=38, b=90;
  int radius=20;
  for(int y=-radius; y<=radius; y++)
    for(int x=-radius; x<=radius; x++)
        if(x*x+y*y <= radius*radius)
            Tft.lcd_draw_point(x+a, y+b, BLACK);
               
  Tft.lcd_draw_line(a+radius+5,b, a+radius+16,b,BLACK);
  Tft.lcd_draw_line(a-radius-5,b, a-radius-16,b,BLACK);
  Tft.lcd_draw_line(a,b-radius-5, a,b-radius-16,BLACK);
  Tft.lcd_draw_line(a,b+radius+5, a,b+radius+16,BLACK);

  Tft.lcd_draw_line(a+radius-3,b+radius-3, a+radius+7,b+radius+7,BLACK);
  Tft.lcd_draw_line(a+radius-3,b-radius+3, a+radius+7,b-radius-7,BLACK);

  Tft.lcd_draw_line(a-radius+3,b-radius+3, a-radius-7,b-radius-7,BLACK);
  Tft.lcd_draw_line(a-radius+3,b+radius-3, a-radius-7,b+radius+7,BLACK);
}

void deleteCloud()
{
  int a=45, b=90;
  int radius=20;
  for(int y=-radius; y<=radius; y++){
    for(int x=-radius; x<=radius; x++){
        if(x*x+y*y <= radius*radius){
            Tft.lcd_draw_point(x+a,    y+b,   BLACK);
            Tft.lcd_draw_point(x+a+42, y+b,   BLACK);
            Tft.lcd_draw_point(x+a+20, y+b-7, BLACK);
            Tft.lcd_draw_point(x+a+15, y+b+5, BLACK);
            }}}
}

void deleteRainCloud()
{
  int a=45, b=90;
  int radius=20;
  for(int y=-radius; y<=radius; y++){
    for(int x=-radius; x<=radius; x++){
        if(x*x+y*y <= radius*radius){
            Tft.lcd_draw_point(x+a, y+b, BLACK);
            Tft.lcd_draw_point(x+a+42, y+b, BLACK);
            Tft.lcd_draw_point(x+a+20, y+b-7, BLACK);
            Tft.lcd_draw_point(x+a+15, y+b+5, BLACK);
            }}}
        Tft.lcd_draw_line(a,b+radius+5, a-3,b+radius+16,BLACK);
        Tft.lcd_draw_line(a+15,b+radius+8, a+15-3,b+radius+21,BLACK);
        Tft.lcd_draw_line(a+30,b+radius+5, a+30-3,b+radius+18,BLACK);
        Tft.lcd_draw_line(a+42,b+radius+3, a+42-3,b+radius+16,BLACK);

}

void deleteMoon()
{
  
  int a=45, b=90;
  int radius=25;
  for(int y=-radius; y<=radius; y++){
    for(int x=-radius; x<=radius; x++){
        if(x*x+y*y <= radius*radius){
            Tft.lcd_draw_point(x+a-22, y+b, BLACK);    
            }}}
radius=23;
  for(int y=-radius; y<=radius; y++){
    for(int x=-radius; x<=radius; x++){
        if(x*x+y*y <= radius*radius){
            Tft.lcd_draw_point(x+a-7-22, y+b-2, BLACK);    
            }}}
}
