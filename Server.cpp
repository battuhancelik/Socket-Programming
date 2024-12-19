#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080 // Kullan�lacak port numaras�
#define MAX_OYUNCU 2 // Maksimum oyuncu say�s�

SOCKET sunucuSoketi; // Sunucuya ait soket
SOCKET bagliOyuncular[MAX_OYUNCU]; // Ba�l� oyuncular�n soketlerini tutar
int bagliOyuncuSayisi = 0; // Ba�l� oyuncu say�s�n� takip eder
CRITICAL_SECTION oyuncuKilidi; // Ayn� anda eri�im i�in kilit mekanizmas�

char oyuncuHamleleri[MAX_OYUNCU][1024] = {0}; // Oyuncular�n hamlelerini saklar
int alinanHamleSayisi = 0; // Gelen hamle say�s�n� takip eder

// Kazanan� belirleme fonksiyonu
void kazananBelirle();

DWORD WINAPI oyuncuyuIsle(LPVOID arg) { // �� par�ac��� bitince, i� par�ac���n�n durumu hakk�nda bilgi vermek i�in kullan�l�r
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
        gelenVeri[gelenVeriBoyutu] = '\0'; // Gelen veriyi string sonland�r
        EnterCriticalSection(&oyuncuKilidi); 
        strcpy(oyuncuHamleleri[oyuncuIndex], gelenVeri);
        printf("Oyuncu %d'nin hamlesi: %s\n", oyuncuIndex + 1, gelenVeri);

        // Gelen hamle say�s�n� art�r
        alinanHamleSayisi++;

        // T�m hamleler al�nd�ysa kazanan� belirle
        if (alinanHamleSayisi == MAX_OYUNCU) {
            kazananBelirle();
        }
        LeaveCriticalSection(&oyuncuKilidi);
    } else {
        printf("Oyuncu %d baglantiyi kesti veya gecersiz veri gonderdi.\n", oyuncuIndex + 1);
    }

    return 0;
}

// Kazanan� belirler ve sonucu oyunculara g�nderir
void kazananBelirle() {
    char *sonuc;

    // Hamlelere g�re sonucu hesapla
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

    // Sonucu oyunculara g�nder
    for (int i = 0; i < MAX_OYUNCU; i++) {
        send(bagliOyuncular[i], sonuc, strlen(sonuc) + 1, 0);
    }

    // Sunucu ekran�na sonucu yazd�r
    printf("Oyun sonucu: %s\n", sonuc);

    // Soketleri kapat ve de�i�kenleri s�f�rla
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

    // Kilit mekanizmas�n� ba�lat
    InitializeCriticalSection(&oyuncuKilidi);

    // Winsock ba�lat
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup ba�ar�s�z oldu. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Sunucu soketi olu�tur
    if ((sunucuSoketi = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Soket olu�turma ba�ar�s�z. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Sunucu ayarlar�n� yap
    adres.sin_family = AF_INET;
    adres.sin_addr.s_addr = INADDR_ANY;
    adres.sin_port = htons(PORT);

    int opt = 1;
    if (setsockopt(sunucuSoketi, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        printf("setsockopt ba�ar�s�z oldu. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Soketi ba�la
    if (bind(sunucuSoketi, (struct sockaddr *)&adres, sizeof(adres)) == SOCKET_ERROR) {
        printf("Ba�lama ba�ar�s�z. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Soketi dinlemeye ba�la
    if (listen(sunucuSoketi, MAX_OYUNCU) == SOCKET_ERROR) {
        printf("Dinleme ba�ar�s�z. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Sunucu %d portunda dinliyor...\n", PORT);

    // Sonsuz d�ng�: Ba�lanan oyuncular� kabul et
    while(1) {
        SOCKET yeniSoket;
        if ((yeniSoket = accept(sunucuSoketi, (struct sockaddr *)&adres, &adresBoyutu)) == INVALID_SOCKET) {
            printf("Kabul ba�ar�s�z. Hata Kodu: %d\n", WSAGetLastError());
            continue;
        }

        // Yeni bir thread olu�tur ve oyuncuyu i�le
        HANDLE thread = CreateThread(NULL, 0, oyuncuyuIsle, &yeniSoket, 0, NULL);
        if (thread == NULL) {
            printf("Thread olu�turulamad�.\n");
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

