# Test senaryoları ve sonuçları

İki tür test var: bilgisayarda çalışan otomatik testler (`tests/`) ve donanım üzerinde elle yapılan testler. Elle yapılan testlerde kanıt merkezin seri kaydı (115200), panel ve AWS konsolu/CLI çıktısıdır.

## Otomatik testler

```bash
python -m unittest discover -s tests -v
```

| Test | Ne doğrular | Son sonuç |
|---|---|---|
| `test_rfpacket` | 32 bitlik paketin bit yerleşimi başlık dosyasıyla aynı; 28 800 değer kombinasyonu bozulmadan gidip geliyor; tek bit hatası olan her paket (32/32) reddediliyor | Geçti (2026-11-10) |
| `test_decide` | Bulut kararı: sıcaklık ve gaz histerezisi, değişiklik yokken gölgeye yazmama, çevrimdışı düğümü yok sayma | Geçti (2026-11-10) |
| `test_pc_server` | PC paneli: telemetri kaydı, tekrarlanan olayların tek kayda inmesi, yanlış anahtara 401, bozuk veriye 400, girişsiz panele 401, komut gidiş-dönüşü (kuyruk → cihaza iletildi → uygulandı), bilinmeyen komuta 400, onaylanmayan komutun zaman aşımı | Geçti (2026-11-10, 13/13 toplam) |

## Donanım testleri

| # | Senaryo | Adımlar | Beklenen | Sonuç |
|---|---|---|---|---|
| D1 | Sensör verisi akışı (Nano, 433 MHz) | Nano'yu çalıştır, merkez kaydını izle | "Düğüm 0 bağlandı", panelde sıcaklık/nem/gaz | Geçti (2026-10-19) |
| D2 | Sensör verisi akışı (ESP32 D1 Mini, ESP-NOW) | Düğümü çalıştır, merkez kaydını izle | "Düğüm 1 bağlandı", panelde Düğüm 1 | Bekliyor (kart kurulumu) |
| D3 | Düğüm kopması | Çalışan düğümün gücünü kes | 15 sn sonra "Düğüm N sinyali kesildi" (acil olay), panelde çevrimdışı | Bekliyor |
| D4 | Kartla giriş | Yetkili kartı okut | "Giris onaylandi", servo açılıp 5 sn sonra kapanır, alarm kapanır | Geçti (2026-10-19, Kart 1) |
| D5 | Hatalı giriş ve kilit | Tanımsız kart / yanlış şifre ×3 | Her denemede "Erisim reddedildi", 3. denemede 30 sn kilit + acil olay | Kısmen: tanımsız kart reddi geçti (2026-10-19), kilit bekliyor |
| D6 | Kimlikli kurma ve izinsiz giriş alarmı | `A` + kart/şifre, 10 sn bekle, PIR önünde hareket | Kuruluyor → Kurulu → ALARM, siren, acil olay; kartla susar | Bekliyor |
| D7 | Gaz alarmı ve histerezis | MQ-2'ye çakmak gazı (ateşlemeden) | Eşik 100'ü aşınca alarm + kırmızı LED, 80 altına inince susar | Bekliyor |
| D8 | Panelden komut ve etkisi | Panelde "Röle" ve "Kapıyı Aç" | Röle tıklar/LED, servo döner, olay kaydına düşer | Geçti (2026-10-19, röle open-drain düzeltmesinden sonra) |
| D9 | Yetkisiz panel isteği | Şifresiz `GET /api/status` | 401 | Geçti (2026-10-16 ve 2026-10-19) |
| D10 | Bulut telemetrisi | AWS açıkken 45 sn bekle | DynamoDB'de 5 sn aralıklı kayıtlar | Geçti (2026-10-22: 12 kayıt / 45 sn) |
| D11 | Bulut kararı → eyleyici | Lambda'ya 40 °C ölçümü gönder | Gölge `fan:on, valve:open` → ESP32 uygular ve raporlar → gerçek ölçümle geri kapanır | Geçti (2026-10-22, gölge sürüm 2 → 6) |
| D12 | Olay kaybı/tekrarı | Merkezi yeniden başlat | Açılış olaylarının her biri buluta bir kez | Geçti (2026-10-22: 3/3, sıra 1-3) |
| D13 | PC kapalıyken davranış | PC sunucusu kapalıyken merkezi kullan | Tuş/kart tepkisi yavaşlamaz (0,8 sn zaman aşımı, 30 sn geri çekilme) | Geçti (2026-10-19, gözlem) |
| D14 | İnternetsiz yerel güvenlik | Bulut bağlı değilken gaz alarmı | Fan (röle) ve vana (servo) yerelde açılır, gaz geçince kapanır | Bekliyor |
| D15 | Kurulum portalı | Açılıştan sonra telefondan `Guvenlik-Kurulum-XXXX` ağına bağlan | Kurulum sayfası açılır, kayıttan sonra kart yeniden başlar | Kısmen: yayın açılıyor (2026-10-22 kayıt), telefon testi bekliyor |
| D16 | Modül testi | `B` + kart/şifre | Servo → 2 sn → röle 10 sn → LED kayan ışık, ekranda adımlar | Bekliyor |
| D17 | İnternet üzerinden dashboard ve komut | `start.ps1`, telefondan (mobil veri) tünel adresini aç, giriş yap, "Fanı aç" | Röle tıklar, komut listesinde "Uygulandı", olay akışında "Uzaktan komut uygulandı" | Kısmen: tünel adresi internetten şifresiz 401 / şifreyle 200 (2026-11-10); kartla komut testi bekliyor |
