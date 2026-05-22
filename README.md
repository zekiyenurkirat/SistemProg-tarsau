# tarsau – Sistem Programlama Arşiv Projesi

**tarsau**, metin dosyalarını özel `.sau` formatında arşivleyen ve geri çıkaran Linux/Unix uyumlu bir C programıdır. Proje, Sistem Programlama dersi akademik gereksinimlerine göre modüler olarak geliştirilmiştir.

## Özellikler

- `-b` ile arşiv oluşturma (varsayılan çıktı: `a.sau`)
- `-a` ile arşiv açma ve dosya izinlerini `chmod` ile geri yükleme
- Yalnızca metin (ASCII) dosyaları kabul eder
- En fazla **32** dosya, toplam **200 MB** sınırı
- Güvenli bellek yönetimi (`malloc` / `free`)
- `stat`, `chmod`, `fopen`, `fread`, `fwrite` kullanımı

## Proje Yapısı

```
SistemProg_Proje/
├── include/          # Header dosyaları
├── src/              # Kaynak kodlar
├── tests/            # Test dosyaları ve betik
├── rapor/            # Akademik rapor (Markdown)
├── Makefile
└── README.md
```

## Derleme (Linux / WSL)

```bash
make          # bin/tarsau oluşturur
make clean    # obj/ ve bin/ temizler
make test     # otomatik testleri çalıştırır
```

## Kullanım

### Arşiv oluşturma

```bash
./bin/tarsau -b dosya1.txt dosya2.txt -o cikti.sau
./bin/tarsau -b tek_dosya.txt          # -> a.sau
```

### Arşiv açma

```bash
./bin/tarsau -a cikti.sau hedef_dizin
```

Göreli ve mutlak dizin yolları desteklenir; hedef dizin yoksa oluşturulur.

## .sau Formatı (Özet)

| Bölüm | Açıklama |
|--------|----------|
| İlk 10 bayt | `SAUARCH01\n` magic / içerik alanı |
| Metadata | `dosyaAdi\|izin\|boyut\|ilk10bayt_hex` satırları |
| Ayırıcılar | `META_END`, `CONTENT_START` |
| İçerik | Dosyalar sırayla, ham bayt olarak |

## Gereksinimler

- GCC (C11)
- POSIX uyumlu ortam (Linux, WSL, macOS)

## Lisans

Akademik proje – eğitim amaçlı kullanım.

## Yazar

Zekiye Nur Kırat – Sistem Programlama Ders Projesi
