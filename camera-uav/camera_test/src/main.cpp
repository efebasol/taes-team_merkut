#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <SoftwareSerial.h>
#include <LoRa_E32.h>

/* Pin Şeması

---- IMU -> MPU6050
SCL     --> A6  (Offset ayarlarken A5 ama her zaman A5 gerekebilir)
SDA     --> A7  (Offset ayarlarken A4 ama her zaman A4 gerekebilir)
INT     --> 4   (Offset alırken bağlı değil) //! Çalışmayabilir? Çalışmazsa Loranın TXD piniyle yer değiştir

---- Lora -> E32 433T20D
M0      --> 11  (Setup yaparken değer alması gerekiyor, her zaman GND alamaz bu yüzden pine bağlı)
M1      --> 12  (Setup yaparken değer alması gerekiyor, her zaman GND alamaz bu yüzden pine bağlı)
RXD     --> 3
TXD     --> 2
AUX     -->

---- Servo -> MG996R
Servo0  --> 5
Servo1  --> 6
Servo2  --> 9
Servo3  --> 10
Servo4  --> 11

*/

//* IMU isimlendirmesi ve tanımlamaları
MPU6050 IMUsensor;
int16_t *XAccel, *YAccel, *ZAccel, *XGyro, *YGyro, *ZGyro;

//* Lora bağlantı ayarları
SoftwareSerial LoraSerial(2, 3);
LoRa_E32 LoraTAES(&LoraSerial);
#define M0 11
#define M1 12

// Lora Parametre Ayarları
#define Adres 1
#define Kanal 22

/* Lora Genel Adres Ayarları
Kamera yer adres  --> 0
Kamera İHA adres  --> 1

UKK yer adres     --> 2
UKK İHA adres     --> 3
*/

struct Signal
{
  char sifre[14] = "TAESUAVTEAM22";
  bool CamMode; //* False --> Kamera içeride, True --> Kamera dışarıda
  byte mode[1];
  byte XJoystick[10];
  byte YJoystick[10];
  byte ZJoystick[10];
} LoraData;

void setup()
{
  //* IMU Ayarları
  Serial.begin(9600);
  Wire.begin();
  IMUsensor.initialize();

  // TODO aşağıdaki ofset ayarları içersindeki "0" değeri yapılan teste göre değiştirilecek!
  //  IMU Accel Offset ayarları
  IMUsensor.setXAccelOffset(0);
  IMUsensor.setYAccelOffset(0);
  IMUsensor.setZAccelOffset(0);

  // IMU Gyro Offser ayarları
  IMUsensor.setXGyroOffset(0);
  IMUsensor.setYGyroOffset(0);
  IMUsensor.setZGyroOffset(0);

  //* Lora Ayarları
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);

  LoraTAES.begin();

  // Lora ayarlar fonksiyonu çağırılıyor
  LoraE32Ayarlar();

  digitalWrite(M0, LOW);
  digitalWrite(M1, LOW);
  delay(500);

  Serial.print("Setup tamamlandı iletişim başlatılıyor...");
}

void loop()
{
  IMUsensor.getMotion6(XAccel, YAccel, ZAccel, XGyro, YGyro, ZGyro);

  //* Lora data alımı
  while (LoraTAES.available() > 1)
  {

    ResponseStructContainer rsc = LoraTAES.receiveMessage(sizeof(Signal));
    struct Signal data = *(Signal *)rsc.data;
    rsc.close();

    if (strcmp(data.sifre, "TAESUAVTEAM22") == 0)
    {
      Serial.print("Şifre --> ");
      Serial.print(LoraData.sifre);
      Serial.println("Cam Mode --> ");
      Serial.println(LoraData.CamMode);
      Serial.print("Mode -->");
      Serial.print(*(int*)(LoraData.mode));
      Serial.println("X --> ");
      Serial.println(*(float*)(LoraData.XJoystick));
      Serial.print("Y --> ");
      Serial.print(*(float*)(LoraData.YJoystick));
      Serial.println("Z --> ");
      Serial.println(*(float*)(LoraData.ZJoystick));

      // TODO Kamera ayarlamalarının hepsi burada yapılacak!
    }
  }
}

void LoraE32Ayarlar()
{
  digitalWrite(M0, HIGH);
  digitalWrite(M1, HIGH);

  ResponseStructContainer c;
  c = LoraTAES.getConfiguration();
  Configuration configuration = *(Configuration *)c.data;

  // Kanal ve Adres ayarları yapılıyor
  configuration.ADDL = lowByte(Adres);
  configuration.ADDH = highByte(Adres);
  configuration.CHAN = Kanal;

  // En yüksek gönderi hızı ayarlanıyor (Veri Gönderim Hızı 19,2)
  configuration.SPED.airDataRate = AIR_DATA_RATE_101_192;

  configuration.OPTION.transmissionPower = POWER_20;

  // GELİŞMİŞ AYARLAR
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.uartParity = MODE_00_8N1;
  configuration.OPTION.fec = FEC_0_OFF;
  // configuration.OPTION.fec = FEC_1_ON;
  configuration.OPTION.fixedTransmission = FT_FIXED_TRANSMISSION;
  // configuration.OPTION.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
  configuration.OPTION.wirelessWakeupTime = WAKE_UP_250;
  configuration.OPTION.ioDriveMode = IO_D_MODE_PUSH_PULLS_PULL_UPS;

  // Ayarları kaydediliyor
  ResponseStatus rs = LoraTAES.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  c.close();
}