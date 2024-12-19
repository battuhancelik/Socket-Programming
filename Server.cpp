#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080 // Kullanýlacak port numarasý
#define MAX_OYUNCU 2 // Maksimum oyuncu sayýsý

SOCKET sunucuSoketi; // Sunucuya ait soket
SOCKET bagliOyuncular[MAX_OYUNCU]; // Baðlý oyuncularýn soketlerini tutar
int bagliOyuncuSayisi = 0; // Baðlý oyuncu sayýsýný takip eder
CRITICAL_SECTION oyuncuKilidi; // Ayný anda eriþim için kilit mekanizmasý

char oyuncuHamleleri[MAX_OYUNCU][1024] = {0}; // Oyuncularýn hamlelerini saklar
int alinanHamleSayisi = 0; // Gelen hamle sayýsýný takip eder

// Kazananý belirleme fonksiyonu
void kazananBelirle();

DWORD WINAPI oyuncuyuIsle(LPVOID arg) { // Ýþ parçacýðý bitince, iþ parçacýðýnýn durumu hakkýnda bilgi vermek için kullanýlýr
    SOCKET oyuncuSoketi = *(SOCKET*)arg;
    char gelenVeri[1024] = {0};
    int oyuncuIndex;

    // Oyuncuyu listeye ekle
    EnterCriticalSection(&oyuncuKilidi);
    oyuncuIndex = bagliOyuncuSayisi;
    bagliOyuncular[bagliOyuncuSayisi++] = oyuncuSoketi;
    LeaveCriticalSection(&oyuncuKilidi);

    printf("Oyuncu %d basariyla baglandi!\n", oyuncuIndex + 1);

    // Oyuncudan hamlesini al
    int gelenVeriBoyutu = recv(oyuncuSoketi, gelenVeri, sizeof(gelenVeri) - 1, 0);
    if (gelenVeriBoyutu > 0) {
        gelenVeri[gelenVeriBoyutu] = '\0'; // Gelen veriyi string sonlandýr
        EnterCriticalSection(&oyuncuKilidi); 
        strcpy(oyuncuHamleleri[oyuncuIndex], gelenVeri);
        printf("Oyuncu %d'nin hamlesi: %s\n", oyuncuIndex + 1, gelenVeri);

        // Gelen hamle sayýsýný artýr
        alinanHamleSayisi++;

        // Tüm hamleler alýndýysa kazananý belirle
        if (alinanHamleSayisi == MAX_OYUNCU) {
            kazananBelirle();
        }
        LeaveCriticalSection(&oyuncuKilidi);
    } else {
        printf("Oyuncu %d baglantiyi kesti veya gecersiz veri gonderdi.\n", oyuncuIndex + 1);
    }

    return 0;
}

// Kazananý belirler ve sonucu oyunculara gönderir
void kazananBelirle() {
    char *sonuc;

    // Hamlelere göre sonucu hesapla
    if (strcmp(oyuncuHamleleri[0], oyuncuHamleleri[1]) == 0) {
        sonuc = "Beraberlik!";
    }
    else if ((strcmp(oyuncuHamleleri[0], "1") == 0 && strcmp(oyuncuHamleleri[1], "3") == 0) ||
             (strcmp(oyuncuHamleleri[0], "2") == 0 && strcmp(oyuncuHamleleri[1], "1") == 0) ||
             (strcmp(oyuncuHamleleri[0], "3") == 0 && strcmp(oyuncuHamleleri[1], "2") == 0)) {
        sonuc = "Oyuncu 1 kazandi!";
    }
    else {
        sonuc = "Oyuncu 2 kazandi!";
    }

    // Sonucu oyunculara gönder
    for (int i = 0; i < MAX_OYUNCU; i++) {
        send(bagliOyuncular[i], sonuc, strlen(sonuc) + 1, 0);
    }

    // Sunucu ekranýna sonucu yazdýr
    printf("Oyun sonucu: %s\n", sonuc);

    // Soketleri kapat ve deðiþkenleri sýfýrla
    for (int i = 0; i < MAX_OYUNCU; i++) {
        closesocket(bagliOyuncular[i]);
    }
    bagliOyuncuSayisi = 0;
    alinanHamleSayisi = 0;
    memset(oyuncuHamleleri, 0, sizeof(oyuncuHamleleri));
}

int main() {
    WSADATA wsa;
    struct sockaddr_in adres;
    int adresBoyutu = sizeof(adres);

    // Kilit mekanizmasýný baþlat
    InitializeCriticalSection(&oyuncuKilidi);

    // Winsock baþlat
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup baþarýsýz oldu. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Sunucu soketi oluþtur
    if ((sunucuSoketi = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Soket oluþturma baþarýsýz. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Sunucu ayarlarýný yap
    adres.sin_family = AF_INET;
    adres.sin_addr.s_addr = INADDR_ANY;
    adres.sin_port = htons(PORT);

    int opt = 1;
    if (setsockopt(sunucuSoketi, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        printf("setsockopt baþarýsýz oldu. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Soketi baðla
    if (bind(sunucuSoketi, (struct sockaddr *)&adres, sizeof(adres)) == SOCKET_ERROR) {
        printf("Baðlama baþarýsýz. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Soketi dinlemeye baþla
    if (listen(sunucuSoketi, MAX_OYUNCU) == SOCKET_ERROR) {
        printf("Dinleme baþarýsýz. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Sunucu %d portunda dinliyor...\n", PORT);

    // Sonsuz döngü: Baðlanan oyuncularý kabul et
    while(1) {
        SOCKET yeniSoket;
        if ((yeniSoket = accept(sunucuSoketi, (struct sockaddr *)&adres, &adresBoyutu)) == INVALID_SOCKET) {
            printf("Kabul baþarýsýz. Hata Kodu: %d\n", WSAGetLastError());
            continue;
        }

        // Yeni bir thread oluþtur ve oyuncuyu iþle
        HANDLE thread = CreateThread(NULL, 0, oyuncuyuIsle, &yeniSoket, 0, NULL);
        if (thread == NULL) {
            printf("Thread oluþturulamadý.\n");
            closesocket(yeniSoket);
        } else {
            CloseHandle(thread);
        }
    }

    // Sunucuyu temizle
    closesocket(sunucuSoketi);
    WSACleanup();
    DeleteCriticalSection(&oyuncuKilidi);

    return 0;
}

