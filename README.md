# IoT Akıllı Güvenlik Sistemi

ESP32-S3 merkez + oda düğümü (Arduino Nano/433 MHz ya da ESP32-WROOM mini/ESP-NOW). Kart/şifre ile kapı, hareket ve gaz alarmı, kablosuz sensör ağı, web panel, AWS IoT Core.

## Mimari

```
[Nano: PIR + DHT11 + MQ-2] -----------433 MHz--+
                                                 +--> [ESP32-S3: RFID + tuş takımı + LCD + servo + röle] --Wi-Fi--> [PC sunucusu] --Cloudflare Tunnel--> [İnternet dashboard'u]
[ESP32-WROOM mini: PIR + DHT11 + MQ-2] --ESP-NOW-+
```

## Kurulum (bir kez)

1. Arduino IDE 2.x ve kart paketleri: **esp32 by Espressif 3.3.x** (ESP32-S3 ve ESP32 D1 Mini), **Arduino AVR Boards** (Nano).
2. Arduino kütüphaneleri: MFRC522 1.4.12, Keypad 3.1.1, ESP32Servo 3.2.1, rc-switch 2.6.4, DHT sensor library 1.4.7, Adafruit Unified Sensor 1.1.15, LiquidCrystal_I2C 2.0.0, ArduinoJson 7.4.x, PubSubClient 2.8.
3. PC paneli ve testler: Python 3.10+ (yalnız standart kütüphane). AWS kısmı için isteğe bağlı: AWS CLI v2.
4. `libraries/RfPacket` klasörünü `Belgeler/Arduino/libraries/` içine kopyala (iki kart da aynı paket formatını kullanır).
5. `firmware/security_hub/secrets.example.h` dosyasını `secrets.h` adıyla kopyala; Wi-Fi, panel şifresi, kapı şifresi, telemetri anahtarı ve kurulum yayını şifresini yaz. PC paneli için `pc-dashboard/config.example.json` → `config.json`. Bu iki dosya `.gitignore`'dadır, depoya girmez.

## Yükleme

| Kart | Sketch | Arduino IDE kart seçimi |
|---|---|---|
| ESP32-S3 | `firmware/security_hub` | ESP32S3 Dev Module |
| Nano | `firmware/room_node` | Arduino Nano (yüklenmezse İşlemci: ATmega328P (Old Bootloader)) |
| ESP32 D1 Mini | `firmware/room_node_esp32` | ESP32 Dev Module |

ESP32-S3 açılınca IP adresini LCD'de ve Seri Monitörde (115200) gösterir. Panel: `http://guvenlik.local` (açılmazsa `http://<IP>`).

## ESP32 D1 Mini oda düğümü (ESP-NOW)

Nano + 433 MHz yerine kullanılabilir; ikisi aynı anda da çalışır (Nano düğüm 0, ESP32 düğüm 1). Aynı paket biçimini (`RfPacket`) ESP-NOW ile gönderir. Modem gerekmez; merkezin Wi-Fi kanalını düğüm kendisi bulur (her kanalda merkezin onayını dener). Merkezin MAC adresi `room_node_esp32/config.h` > `kHubMac` içinde; merkez açılışta bunu "ESP-NOW alıcı hazır, merkez MAC: ..." kaydıyla yazar, kart değişirse oradan güncelle.

Düzen: ayrı büyük breadboard + MB102 güç modülü. Modülün üst tarafı **5V**, alt tarafı **3.3V** (sarı kapaklar); dört hatta 29-34 köprüsü. Giriş: 9V USB kablo (telefon şarj aletine). Kart **ESP32 D1 Mini**: iki yanında çift sıra pin olduğu için breadboard'a takılmaz (iç ve dış sıra kısa devre olur), yanına konup dişi-erkek kablolarla bağlanır. Pin adları kartın alt yüzündedir.

| Parça | Bağlantı | Not |
|---|---|---|
| ESP32 D1 Mini | VCC → üst +5V, GND (antene en yakın köşe) → üst GND | 3.3V pinine bir şey bağlanmaz |
| PIR HC-SR501 | VCC → üst +5V, GND → üst GND, OUT → IO26 | Çıkışı 3.3V, doğrudan bağlanır |
| DHT11 | U → alt +3.3V, G → alt GND, D → j10, IO27 → i10, 10k g10-g14, j14 → alt +3.3V | 10k veri hattını 3.3V'a çeker |
| MQ-2 | VCC → üst +5V, GND → üst GND, AO → a18, 4.7k b18-b22, 10k c22-c26, a26 → üst GND, IO34 → d22 | AO 5V'a çıkar; bölücü ADC'yi korur |
| Kırmızı LED (gaz) | IO16 → f36, 220Ω g36-g39, LED h39(+)-h40(−), j40 → alt GND | |
| Sarı LED (hareket) | IO17 → f41, 220Ω g41-g44, LED h44-h45, j45 → alt GND | |
| Mavi LED (sıcaklık) | IO18 → f46, 220Ω g46-g49, LED h49-h50, j50 → alt GND | |

Kod yüklerken USB'yi takmadan önce güç modülünü kapat. Adım adım 3D gösterim: breadboard rehberinin "Oda düğümü · ESP32 mini" sekmesi.

## AWS IoT Core (bulut, isteğe bağlı)

> Şu an kapalı: `config.h` > `Cloud::kEnabled = false`. Asıl dashboard PC + Cloudflare Tunnel üzerinden çalışıyor (aşağıda). AWS altyapısı hazır ve test edildi; `kEnabled = true` ile yeniden açılabilir.

Merkez, `hub01` adlı Thing olarak X.509 sertifikasıyla MQTT/TLS (8883) üzerinden AWS IoT Core'a bağlanır. `factory/hub01/telemetry` (5 sn) ve `factory/hub01/events` konularına yayın yapar. IoT Rules verileri DynamoDB'ye yazar (7 gün TTL). Lambda (`cloud/lambda/decide.py`) sıcaklık/gaz eşiklerine (histerezisli, `cloud/config.json`) göre Device Shadow'a `fan` (röle) ve `valve` (servo) komutu yazar, ESP32 komutu uygulayıp geri raporlar.

- Kurulum: AWS CLI ayarlıyken `powershell -File cloud/deploy.ps1` (yığın, Lambda kodu, Thing, sertifika, `firmware/security_hub/aws_secrets.h`). Ardından firmware'i derleyip yükle.
- Kaldırma: `powershell -File cloud/destroy.ps1`.
- Ek kütüphane: PubSubClient.
- `aws_secrets.h` ve `cloud/.certs/` gizlidir, depoya girmez.

## İnternet dashboard'u (PC kontrol merkezi)

Ana dashboard PC'de çalışır ve Cloudflare Tunnel ile internete açılır; şifreyle korunur.

```
ESP32-S3 --(yerel ağ, 2 sn'de bir POST /api/telemetry, X-Api-Key)--> PC sunucusu (SQLite) <--(HTTPS, Cloudflare Tunnel, giriş)-- Tarayıcı (internet)
         <--(cevapta bekleyen komutlar; sonuçları bir sonraki POST'ta "acks")--
```

- **İzleme:** durum halkası, sıcaklık/nem/gaz göstergeleri, hareket radarı, son 60 dakikanın grafikleri, olay akışı.
- **Kontrol:** "Uzaktan kontrol" bölümündeki düğmeler (sistemi kur / devre dışı bırak, kapı/vana, fan rölesi aç/kapa, modül testi). Komut PC'de kuyruğa girer, merkez en geç 2 sn içinde telemetri cevabında alır, uygular ve sonucu bir sonraki gönderimde bildirir. Paneldeki komut listesinde durum: Bekliyor → Cihaza iletildi → Uygulandı.
- **Neden bu yol:** merkeze internetten doğrudan bağlantı açılmaz (port yönlendirme yok); cihaz yalnız dışarıya istek yapar.

Kurulum:

1. `pc-dashboard/config.example.json` → `config.json`. `api_key` = `secrets.h` > `kTelemetryKey`; `dashboard_user` / `dashboard_password` internetteki giriş bilgisidir, güçlü bir şifre yaz.
2. Merkezde `Telemetry::kServerUrl` (ya da kurulum portalındaki "PC telemetri adresi") PC'nin yerel IP'sini göstermeli.
3. PC'de: `powershell -File pc-dashboard/start.ps1`. Sunucu açılır ve `cloudflared` herkese açık `https://...trycloudflare.com` adresini yazar (her başlatışta değişir). Yalnız yerel ağda denemek için `python pc-dashboard/server.py` ve `http://localhost:8080` yeterli.

Kararlılık ve güvenlik: bilinmeyen komut 400 ile reddedilir; 60 sn içinde onaylanmayan komut "Zaman aşımı" olur ve sonradan uygulanmaz; merkez çevrimdışıyken düğmeler pasiftir; PC kapalıyken merkez etkilenmez (0,8 sn zaman aşımı, 30 sn geri çekilme); yanlış telemetri anahtarı 401, bozuk veri 400. ESP32'nin yerel paneli (`http://guvenlik.local`) yedek olarak çalışmaya devam eder.

## Kurulum modu (flashlamadan Wi-Fi/AWS ayarı)

Kart her açılışta 3 dakika boyunca `Guvenlik-Kurulum-XXXX` adlı bir Wi-Fi yayını açar (şifre: `secrets.h` > `kSetupApPassword`). Kayıtlı ağa bağlanamazsa yayın kapanmaz. Elle açmak için: şifre + `D` tuşu ya da yerel paneldeki "Kurulum Modu" düğmesi (kart yeniden başlar, yayın kaydedilene kadar açık kalır).

Telefonla yayına bağlanınca kurulum sayfası kendiliğinden açılır (açılmazsa `http://192.168.4.1/setup`). Sayfada Wi-Fi ağı, AWS uç noktası, Thing adı, konu öneki, PC telemetri adresi ve üç sertifika dosyası (kök CA, cihaz sertifikası, özel anahtar; `cloud/.certs/` içinde) girilir. Boş bırakılan alan eski değerini korur. Kaydedince ayarlar kartın kalıcı hafızasına (NVS) yazılır ve kart yeniden başlar. Ekran, yayın açıkken yayın adını ve 192.168.4.1 adresini dönüşümlü gösterir.

## Modül testi

Tuş takımında `B`, ardından kart ya da şifre (ya da paneldeki "Modül Testi"): ekranda "TEST EDILIYOR" yazar, servo açılıp kapanır, 2 sn sonra röle 10 sn açık kalır, ardından LED'ler kayan ışıkla yanıp söner.

## Güç

- Breadboard güç modülü (MB102/HW-131) **12V, en az 2A** adaptörle beslenir. 9V pil ya da 700 mA USB kablo servo ve röleyi birlikte taşıyamaz.
- Modülün iki ucundaki "5V OFF 3.3" başlığında sarı kapaklar **5V** tarafında olmalı. Kapaklar 3.3V'tayken ekran bozuk yazar, RF alıcı ve servo çalışmaz.
- 830 delikli breadboard'da kenar hatları ortadan bölünebilir: dört hatta da 29-34 arası köprü kablosu tak.
- ESP32-S3 USB'den beslenir. YD-ESP32-S3 klon kartında 5Vin pini arkadaki IN-OUT pedi lehimlenmeden ne giriş ne çıkış olarak çalışır.
- Röle 5V'luk aktif-düşük modül; yazılım pini open-drain sürer, böylece 3.3V'luk ESP32 röleyi tam kapatabilir.

## Kullanım

- Kart okut veya şifre + `#` : kapı açılır, alarm kapanır. `*` : girişi sil.
- `A`, ardından kart ya da şifre + `#` : sistemi kurar (10 sn çıkış süresi). Kimlik doğrulaması 15 sn içinde gelmezse istek düşer.
- İnternet ya da AWS yokken gaz alarmı fanı (röle) ve vanayı (servo) yerelde açar; bulut geri gelince kontrolü yeniden bulut alır.
- Tanımsız kart okutunca UID'si olay kaydına düşer; `config.h` > `kAuthorizedCards` listesine ekle.
- 3 hatalı denemede giriş 30 sn kilitlenir. Gaz alarmı sistem kurulu olmasa da çalışır.
- MQ-2 ilk 60 sn ısınır, bu sürede gaz değeri 0 gönderilir. Gaz eşiğini (`kGasAlarmLevel`) paneldeki normal değere göre ayarla.
- Durum LED'leri (merkez GPIO38/39/40, Nano D6/D7/D8): kırmızı gaz alarmı, sarı hareket, mavi sıcaklık >= `kTempHighC` (30°C). Bağlantı: pin → LED uzun bacak, kısa bacak → 220Ω → GND.
- PIR sürekli tetikli kalıyorsa modülün iki turuncu trimpotunu saat yönünün tersine sonuna kadar çevir (süre ~3 sn).
- Telegram bildirimi hazır ama kapalı: `secrets.h` içine bot token ve chat id yazınca devreye girer.

Tüm pinler, süreler, eşikler ve metinler `config.h` dosyalarında.

## Test

Otomatik testler (paket biçimi, bulut karar mantığı, PC sunucusu):

```bash
python -m unittest discover -s tests -v
```

Donanım test senaryoları, adımları ve tarihli sonuçları: [docs/test-senaryolari.md](docs/test-senaryolari.md).

## Proje yapısı

| Klasör | İçerik |
|---|---|
| `firmware/security_hub/` | ESP32-S3 merkez: giriş, alarm, düğüm dinleme (433 MHz + ESP-NOW), yerel panel, kurulum portalı, PC telemetrisi, AWS bağlantısı |
| `firmware/room_node/` | Arduino Nano oda düğümü (433 MHz) |
| `firmware/room_node_esp32/` | ESP32 D1 Mini oda düğümü (ESP-NOW) |
| `libraries/RfPacket/` | Düğümlerin ortak 32 bit paket biçimi |
| `pc-dashboard/` | PC kontrol merkezi (Python sunucu + panel) |
| `cloud/` | AWS IoT Core kurulumu (CloudFormation, Lambda, betikler) |
| `tests/` | Otomatik testler |
| `docs/` | 3D breadboard rehberi, mimari rehberi, test senaryoları |

