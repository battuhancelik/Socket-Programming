#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define SUNUCU_ADRESI "127.0.0.1" // Sunucunun IP adresi
#define PORT 8080 // Sunucunun dinledi�i port

int main() {
    WSADATA wsa;
    SOCKET istemciSoketi; // �stemciye ait soket
    struct sockaddr_in sunucuBilgileri;
    char gonderilecekVeri[1024] = {0}; // G�nderilecek veriler
    char sunucudanGelenCevap[1024] = {0}; // Sunucudan al�nan cevaplar

    // Winsock ba�lat-A� ileti�imi tcp/ip
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup ba�ar�s�z oldu. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // �stemci soketi olu�tur 
    //SOCK_STREAM : soket t�r�(TCP soketi)
    //0 : TCP protokol�
    if ((istemciSoketi = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Soket olusturulamadi. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Sunucunun adres bilgilerini ayarla
    sunucuBilgileri.sin_family = AF_INET; //soket adresi IPv4 protokol� kullanacak.
    sunucuBilgileri.sin_addr.s_addr = inet_addr(SUNUCU_ADRESI); //string olarak al�nan IPv4 adresini binary format�na d�n��t�r�r.
    sunucuBilgileri.sin_port = htons(PORT); //ba�lant� kurulacak port numaras�n� tutan alan.

    // Sunucuya ba�lan
    if (connect(istemciSoketi, (struct sockaddr *)&sunucuBilgileri, sizeof(sunucuBilgileri)) == SOCKET_ERROR) {
        printf("Sunucuya baglanilamadi. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Sunucuya basariyla baglandiniz.\n");
    
    printf("Lutfen hamlenizi secin (1 = Tas, 2 = Kagit, 3 = Makas): ");
    while (1) {
	    scanf("%s", gonderilecekVeri);
	    
        // Girdi do�rulama 
        //strcmp: iki karakter dizisini kar��la�t�r�yor.
        if (strcmp(gonderilecekVeri, "1") == 0 || strcmp(gonderilecekVeri, "2") == 0 || strcmp(gonderilecekVeri, "3") == 0) {
        
		// Hamleyi sunucuya g�nder
        if (send(istemciSoketi, gonderilecekVeri, strlen(gonderilecekVeri), 0) == SOCKET_ERROR) //send(SOCKET s, const char *buf, int len, int flags);
		{
        printf("Hamle g�nderilemedi. Hata Kodu: %d\n", WSAGetLastError());
        break;
        return 1;
        }
        } else {
            printf("Hatali giris yaptiniz. Lutfen sadece 1, 2 veya 3 degerlerinden birini girin.\n");
        }
        }

    printf("Se�iminiz ba�ar�yla al�nd�: %s\n", gonderilecekVeri);

    // Sunucudan sonucu al
    int cevapBoyutu = recv(istemciSoketi, sunucudanGelenCevap, sizeof(sunucudanGelenCevap) - 1, 0); // recv: veri al 
	// -1:null terminator, buffer'�n sonuna bir null karakter ('\0') koyar. 
	// 0:flag,nromal veri alma
	
    if (cevapBoyutu > 0) {
        sunucudanGelenCevap[cevapBoyutu] = '\0'; // Cevab� string olarak sonland�r
        printf("Oyun sonucu: %s\n", sunucudanGelenCevap);
    } else {
        printf("Sunucudan cevap al�namad�.\n");
    }

    // Soketi kapat
    closesocket(istemciSoketi);
    WSACleanup(); //winsock k�t�phanesini kapat.

    return 0;
}

