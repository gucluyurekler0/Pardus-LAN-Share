# 🖥️ Pardus LAN Share (Gelişmiş Sunucu Paneli)

Pardus LAN Share, yerel ağ (Wi-Fi/Ethernet) üzerindeki akıllı telefonlar ile bilgisayarlar (özellikle Pardus işletim sistemi) arasında hızlı, güvenli ve kablosuz veri aktarımı sağlayan Qt tabanlı bir masaüstü sunucu uygulamasıdır.

Bu proje sayesinde telefondan kopyalanan metinleri bilgisayarda anlık olarak düzenleyebilir, gönderilen dosyaları tek bir tıklamayla doğrudan açabilirsiniz.

## 📱 Android İstemcisi

Pardus LAN Share'in Android istemcisi ile aynı yerel ağ üzerindeki dosyaları mobil cihazınızdan görüntüleyebilir ve paylaşabilirsiniz.

👉 **Pardus LAN Share Mobile:**  
Github : https://github.com/gucluyurekler0/PardusLanShareMobile <br>
Uygulama Dosyası : [https://github.com/gucluyurekler0/PardusLanShareMobile](https://github.com/gucluyurekler0/PardusLanShareMobile/releases/download/v1.0.0/app-debug.apk)
## 📷 Android Mobil Uygulama Ekran Görüntüleri

<p align="center">
  <img src="https://github.com/gucluyurekler0/PardusLanShareMobile/blob/main/screenshots/Screenshot_2026-07-19-07-33-24-15_4ebbdcdb072baa522dd1ba6c306860e3.jpg" alt="Ana Ekran 1" width="300"/>

  
</p>

---

# ✨ Özellikler

- **Gelişmiş Metin Editörü (`QTextEdit`)**
  - Telefondan gelen pano (clipboard) içeriklerini kırpılmadan tam haliyle görebilir, düzenleyebilir ve tek butonla bilgisayar panosuna aktarabilirsiniz.

- **Akıllı Dosya Yöneticisi**
  - Gelen dosyalar tamamlandığında sistem günlüklerinin en üstünde görünür.
  - Dosyaya çift tıklayarak varsayılan uygulama ile açabilirsiniz.

- **Sanal Ağ Filtreleme**
  - VirtualBox, VMware ve WSL gibi sanal adaptörleri otomatik filtreler.
  - Gerçek yerel IP adresini gösterir.

- **Sistem Tepsisi (Tray Icon)**
  - Arka planda çalışabilir.
  - Sistem tepsisinden kolayca yönetilebilir.

- **Anlık İlerleme Çubuğu**
  - Dosya aktarım yüzdesini canlı olarak gösterir.

---

# 🐧 Pardus İşletim Sisteminde Kurulum ve Derleme

## 1️⃣ Gerekli Bağımlılıkların Kurulması

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-private-dev qt6-tools-dev
```

---

## 2️⃣ Projenin Klonlanması

```bash
git clone https://github.com/gucluyurekler0/Pardus-LAN-Share.git
cd Pardus-LAN-Share
```

---

## 3️⃣ Derleme

```bash
mkdir -p build
cd build

cmake ..
make
```

---

## 4️⃣ Uygulamayı Çalıştırma

```bash
./PardusLanShare
```

---

# 📱 Kullanım

1. Uygulamayı Pardus üzerinde çalıştırın.
2. **Sunucu IP Adresi** bölümünde görünen adresi not edin. (Örn: `192.168.1.50:9999`)
3. Aynı Wi-Fi ağına bağlı telefonunuzdaki istemci uygulamasından bu IP adresine bağlanın.
4. Metin gönderdiğinizde düzenleme alanında görüntülenir.
5. **Değişiklikleri Panoya Kopyala** butonuyla bilgisayar panosuna aktarabilirsiniz.
6. Dosya gönderildiğinde sistem günlüğünün en üstünde görünür.
7. Dosyaya çift tıklayarak doğrudan açabilirsiniz.

---

# 🛠️ Kullanılan Teknolojiler

| Teknoloji | Açıklama |
|-----------|----------|
| **Dil** | C++17 |
| **Arayüz** | Qt 6 (QtWidgets, QtNetwork) |
| **Derleme Sistemi** | CMake |

---

# 📸 Ekran Görüntüsü

![Pardus LAN Share](https://raw.githubusercontent.com/gucluyurekler0/Pardus-LAN-Share/main/screenshots/1.png)

![Pardus LAN Share](https://raw.githubusercontent.com/gucluyurekler0/Pardus-LAN-Share/main/screenshots/2.png)

---

# 📄 Lisans

Bu proje açık kaynak olarak geliştirilmektedir.
