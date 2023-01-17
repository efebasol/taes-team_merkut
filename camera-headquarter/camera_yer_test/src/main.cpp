#include <Arduino.h>
#include <LoRa_E32.h>
#include <SoftwareSerial.h>

//TODO USB Shield kodlarını ekleyip joystickten alınan verileri tanımla

/* Pin Şeması

---- Lora -> E32 433T20D
M0      --> 11  (Setup yaparken değer alması gerekiyor, her zaman GND alamaz bu yüzden pine bağlı)
M1      --> 12  (Setup yaparken değer alması gerekiyor, her zaman GND alamaz bu yüzden pine bağlı)
RXD     --> 3
TXD     --> 2
AUX     -->

*/

//* Lora bağlantı ayarları
SoftwareSerial LoraSerial(2, 3);
LoRa_E32 LoraTAES(&LoraSerial);
#define M0 11
#define M1 12

// Lora Parametre Ayarları
#define Adres 0
#define Kanal 22
#define AliciAdres 1

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
  byte XJoystick[9];
  byte YJoystick[9];
  byte ZJoystick[9];
} LoraData;

void setup()
{
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
  //* Lora struct ayarları
  LoraData.CamMode = false;
  *(int*)(LoraData.mode) = 0;
  *(float*)(LoraData.XJoystick) = 12.121212;
  *(float*)(LoraData.YJoystick) = 12.121212;
  *(float*)(LoraData.ZJoystick) = 12.121212;

  // Lora mesaj gönderme
  ResponseStatus rs = LoraTAES.sendFixedMessage(highByte(AliciAdres), lowByte(AliciAdres), Kanal, &LoraData, sizeof(Signal));
  //! Gerekli testlerden sonra süresi kısalıp uzayabilir!
  delay(500);
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