# 🖥️ Pardus LAN Share (Gelişmiş Sunucu Paneli)

Pardus LAN Share, yerel ağ (Wi-Fi/Ethernet) üzerindeki akıllı telefonlar ile bilgisayarlar (özellikle Pardus işletim sistemi) arasında hızlı, güvenli ve kablosuz veri aktarımı sağlayan Qt tabanlı bir masaüstü sunucu uygulamasıdır. 

Bu proje sayesinde telefondan kopyalanan metinleri bilgisayarda anlık olarak düzenleyebilir, gönderilen dosyaları tek bir tıklamayla doğrudan açabilirsiniz.

## ✨ Özellikler

*   **Gelişmiş Metin Editörü (`QTextEdit`):** Telefondan gelen pano (clipboard) içeriklerini kırpılmadan tam haliyle görebilir, üzerinde düzenleme yapabilir ve tek butonla bilgisayar panosuna güncelleyebilirsiniz.
*   **Akıllı Dosya Yöneticisi:** Gelen dosyalar tamamlandığı an sistem günlüklerinin **en üstünde** belirir. Listeden dosyaya mouse ile **çift tıklayarak** işletim sisteminin varsayılan uygulamasıyla (Görüntüleyici, VLC, Belge Okuyucu vb.) anında açabilirsiniz.
*   **Sanal Ağ Filtreleme:** IP çakışmalarını önlemek adına VirtualBox, VMware veya WSL gibi sanal ağ adaptörlerinin IP adreslerini (`192.168.56.x` vb.) otomatik olarak filtreler ve sadece gerçek yerel IP adresinizi gösterir.
*   **Sistem Tepsisi (Tray Icon) Entegrasyonu:** Arka planda çalışırken ekranı kaplamaz, sistem çekmecesinden kolayca yönetilebilir.
*   **Anlık İlerleme Çubuğu:** Dosya transfer durumunu ve yüzdesini canlı olarak takip etmenizi sağlar.

---

## 🐧 Pardus İşletim Sisteminde Kurulum ve Derleme Kılavuzu

Uygulamayı Pardus (veya Debian/Ubuntu tabanlı sistemler) üzerinde sıfırdan derleyip çalıştırmak için aşağıdaki adımları takip edin:

### 1. Gerekli Bağımlılıkların Kurulması
Öncelikle sisteminizde C++ derleyicisinin, CMake mimarisinin ve Qt6 kütüphanelerinin kurulu olduğundan emin olun. Terminali açın ve şu komutu çalıştırın:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-private-dev qt6-tools-dev
