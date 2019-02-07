/**
   BusStopMonitor.ino

   Created on: 02.02.2019
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>
#include <sys/time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_RST 16 //D0
#define TFT_CS  5  //D1
#define TFT_DC  4  //D2

int offset = 8 * 60 * 60;

ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "sg.pool.ntp.org", offset, 60000);

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const int NUMBER_OF_BUSSES   = 40;
const int NUMBER_OF_BUSSTOPS = 4;
const int NUMBER_OF_TIMES    = 3;

class BusInfo
{
   public:
     String busNo;
     String arrival[NUMBER_OF_TIMES];
};

class Busstop
{
  public:
    String id;
    String description;
    String roadName;
    BusInfo listOfBusses[NUMBER_OF_BUSSES];
};

class BusstopList
{
  public:
    Busstop busstops[NUMBER_OF_BUSSTOPS];
};

BusstopList busstopList;

void setup() 
{
    Serial.begin(115200);
  
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP("XXX", "YYY");
  
    timeClient.begin();
  
    tft.initR(INITR_BLACKTAB);
    tft.setTextWrap(false);
    tft.fillScreen(0x0000);
}

void loop()
{    
    // wait for WiFi connection
    if ((WiFiMulti.run() == WL_CONNECTED)) 
    {
        WiFiClient client;
        HTTPClient http;

        busstopList.busstops[0].id = "10181";
        busstopList.busstops[1].id = "10189";
        busstopList.busstops[2].id = "10359";
        busstopList.busstops[3].id = "10351";


        busstopList.busstops[0].description = "BEF AR-RABITAH MQUE";
        busstopList.busstops[1].description = "Blk 104A CP";
        busstopList.busstops[2].description = "Aft Tiong Bahru Rd";
        busstopList.busstops[3].description = "Bef Tiong Bahru Rd";

        busstopList.busstops[0].roadName = "Tiong Bahru Rd";
        busstopList.busstops[1].roadName = "Tiong Bahru Rd";
        busstopList.busstops[2].roadName = "Henderson Rd";
        busstopList.busstops[3].roadName = "Henderson Rd";

        
        for (int n = 0; n < NUMBER_OF_BUSSTOPS; n++)
        {           
            String url = "http://datamall2.mytransport.sg/ltaodataservice/BusArrivalv2?BusStopCode=" + busstopList.busstops[n].id;
           
            if (http.begin(client, url))
            {        
                http.setTimeout(5000);
                http.addHeader("AccountKey", "X");

                int httpCode = http.GET();

                // httpCode will be negative on error
                if (httpCode > 0)
                {
                    // file found at server
                    if (httpCode == HTTP_CODE_OK ||
                        httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                    {
                        timeClient.update();
                        String currentDate = timeClient.getFormattedTime();
                            
                        String payload = http.getString();
                        Serial.println(payload);
              
                        int pos = 0;
                         
                        for (int m = 0; m < NUMBER_OF_BUSSES; m++)
                        {
                            String busNo;
                            
                            GetBusNo(payload, busNo, pos, m);

                            Serial.print(" Bus: ");
                            Serial.print(busNo);
                          
                            busstopList.busstops[n].listOfBusses[m].busNo = busNo;
                
                            if (pos == -1)
                                break;

                            Serial.print(" Current: ");
                            Serial.print(currentDate);
                          
                            for (int p = 0; p < NUMBER_OF_TIMES; p++)
                            {
                                String arrivalTime;
                                GetArrivalTime(payload, arrivalTime, pos);
                          
                                if(pos == -1)
                                  break;
                          
                                String minutes;

                                if(arrivalTime != "NA")
                                    minutes = SubtractDates(currentDate, arrivalTime);
                                else
                                    minutes = "NA";  

                                busstopList.busstops[n].listOfBusses[m].arrival[p] = minutes;

                                Serial.print(" Arrival: ");
                                Serial.print(arrivalTime);
                                Serial.print(" Minutes: ");
                                Serial.print(minutes);
                               
                                pos++;
                            }
                            
                            Serial.println(" ");              
                        }
              
                        Serial.println("@@@@@@@");
                    }
                }
                else 
                {
                    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
                }
          
                http.end(); 
            }
            else 
            {
                Serial.printf("[HTTP} Unable to connect\n");
            }
            
        }

        PrintFrame(busstopList);
    }
    
}

void PrintFrame(BusstopList &busstopList)
{
    bool validData = false;

    for (int x = 0; x < NUMBER_OF_BUSSTOPS; x++)
    {
        tft.fillScreen(0x0000);

        timeClient.update();
        String currentDate = timeClient.getFormattedTime();
        PrintTime(currentDate);

        tft.setTextSize(2);
        tft.setCursor(0, 0);
        tft.setTextColor(0xDDDD);
        tft.println(busstopList.busstops[x].id);

        tft.setTextSize(1);
        tft.setCursor(0, 18);
        tft.setTextColor(0xDDDD);
        tft.println(busstopList.busstops[x].description);

        tft.setTextSize(1);
        tft.setCursor(0, 28);
        tft.setTextColor(0xDDDD);
        tft.println(busstopList.busstops[x].roadName);

        
        for (int n = 0; busstopList.busstops[x].listOfBusses[n].busNo != ""; n++)
        {
            tft.setTextSize(1);
            tft.setCursor(5, 40 + 12 * n);
            tft.setTextColor(0xEEEE);
            tft.println(busstopList.busstops[x].listOfBusses[n].busNo);
            
            for (int m = 0; m < NUMBER_OF_TIMES; m++)
            {
                tft.setTextSize(1);
                tft.setCursor(40 + m * 30, 40 + 12 * n);
                tft.setTextColor(0xDDDD);
                tft.println(busstopList.busstops[x].listOfBusses[n].arrival[m]);

                validData = true;
            }
        }

        delay(6000);
    }
}


void PrintTime(String currentDate)
{
    tft.setTextSize(1);
    tft.setCursor(97, 0);
    tft.setTextColor(0xDDDD);
    tft.println(currentDate.substring(0,5));   
}

void GetArrivalTime(String &payload, String &arrivalTime, int &pos)
{
      pos = payload.indexOf("EstimatedArrival", pos);

      if (pos == -1)
        return;

      arrivalTime = payload.substring(pos + 30, pos + 38);  

      if (arrivalTime[0] == '\"')
      {
        arrivalTime = "NA";
      }
}

void GetBusNo(String &payload, String &busNo, int &pos, int &m)
{
    pos = payload.indexOf("ServiceNo", pos + 1);

    if (pos == -1)
      return;

    busNo = payload.substring(pos + 12, pos + 15);

    if (busNo[busNo.length() - 1] == '\"')
    {
      busNo = busNo.substring(0,2);
    }

    if (busNo[busNo.length() - 1] == ',')
    {
      busNo = busNo.substring(0,1);
    }    
}

void ParseDate(String &strDate, int *arrDate)
{
    arrDate[0] = strDate.substring(0,2).toInt();
    arrDate[1] = strDate.substring(3,5).toInt();
    arrDate[2] = strDate.substring(6,8).toInt();
}


int SubtractDates(String currentDate, String arrivalDate)
{
    int currentDateAr[3];
    int arrivalDateAr[3];
  
    ParseDate(currentDate, currentDateAr);
    ParseDate(arrivalDate, arrivalDateAr);
  
    int minutes = arrivalDateAr[1] - currentDateAr[1];
  
    if (minutes < 0)
    {
        if (arrivalDateAr[0] == currentDateAr[0])
            minutes = 0;
        else
            minutes += 60;
    }
  
    return minutes;
}
