#include <cmath>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <IXNetSystem.h>
#include <IXWebSocket.h>
#include <IXWebSocketServer.h>
#include <HCNetSDK.h>
#include <DecodeCardSdk.h>
#include "logs.h" 

BOOL encodeG711(LPVOID handle, unsigned char* pcmData, unsigned int pcmDataSize, unsigned char* g711Data, unsigned int& g711DataSize) {
    NET_DVR_AUDIOENC_PROCESS_PARAM encParam;
    encParam.in_buf = pcmData;               // PCM ma'lumotlarini kirish sifatida berish
    encParam.out_buf = g711Data;             // G711 uchun chiqish buferi
    encParam.out_frame_size = g711DataSize;  // Chiqish hajmi
    encParam.g711_type = 0;                  // Mu law kodlash (A law uchun 1)
    encParam.enc_mode = AMR_MR122_;          // G711 kodlash rejimi
    BOOL result = NET_DVR_EncodeG711Frame(handle, &encParam);
    if (result) {
        g711DataSize = encParam.out_frame_size;  // Kodlangan ma'lumotlar hajmini yangilash
        for (unsigned int i = 0; i < g711DataSize; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)g711Data[i] << " ";
        }
        std::cout << std::endl;
        delete[] encParam.out_buf;
        return TRUE; 
    }
    else {
        std::cerr << "G711 kodlashda xatolik: " << GetLastError() << std::endl;
        return FALSE;
    }
}

BOOL SendVoiceData(LONG lVoiceComHandle, unsigned char* g711Data, DWORD dwDataSize) {
    BOOL result = NET_DVR_VoiceComSendData(lVoiceComHandle, (char*)g711Data, dwDataSize);
    if (result) {
        std::cout << "G.711 ma'lumotlari muvaffaqiyatli yuborildi." << std::endl;
    }
    else {
        std::cerr << "Ma'lumot yuborishda xato: " << NET_DVR_GetLastError() << std::endl;
    }
    return result;
}

void CALLBACK VoiceDataCallBack(LONG lVoiceComHandle, char* pRecvDataBuffer, DWORD dwBufSize, BYTE byAudioFlag, void* pUser) {
    //std::cout << "Received audio data of size: " << dwBufSize << " bytes" << std::endl;
}

std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm bt;

    // Use localtime_s instead of localtime
    localtime_s(&bt, &in_time_t);

    std::ostringstream oss;
    oss << std::put_time(&bt, "%d_%m_%Y_%H_%M_%S");  // Format: DD_MM_YYYY_HH_MM_SS
    return oss.str();
}

bool createDirectoryIfNotExist(const std::string& path) {
    // Convert std::string to LPCWSTR (wide-character string)
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
    wchar_t* wpath = new wchar_t[size_needed];
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath, size_needed);

    DWORD ftyp = GetFileAttributesW(wpath); // Use GetFileAttributesW for wide-char string

    // Agar papka mavjud bo'lmasa, uni yaratish
    if (ftyp == INVALID_FILE_ATTRIBUTES || !(ftyp & FILE_ATTRIBUTE_DIRECTORY)) {
        if (CreateDirectoryW(wpath, NULL) == 0) {
            std::cerr << "Papka yaratishda xato!" << std::endl;
            delete[] wpath;
            return false;
        }
    }
    delete[] wpath;
    return true;
}

int main()
{    
    char logAddr[] = "./SDKlog";
    long lUserID; 

    if (NET_DVR_Init()) {
        printf("NET_DVR_Init called with success.");
    }
    else {
        printf("\nNET_DVR_Init error.");
    }
    NET_DVR_SetLogToFile(1, logAddr, FALSE);
    NET_DVR_USER_LOGIN_INFO struLoginInfo = { 0 };
    NET_DVR_DEVICEINFO_V40 deviceInfo = { 0 };
    struLoginInfo.bUseAsynLogin = 0;
    strcpy_s(struLoginInfo.sDeviceAddress, 129, "172.25.25.96");     //Device IP address
    struLoginInfo.wPort = 8000;                                      //Service port No.
    strcpy_s(struLoginInfo.sUserName, 64, "admin");                  //User name
    strcpy_s(struLoginInfo.sPassword, 64, "12345678a");              //Password
    lUserID = NET_DVR_Login_V40(&struLoginInfo, &deviceInfo);
    if (lUserID < 0){
        printf("\nLogin failed, error code: %d", NET_DVR_GetLastError());
        NET_DVR_Cleanup();
        return 1;
    }
    printf("\nLogin successfully");
    log_to_file("_Login succ IP=" + std::string(struLoginInfo.sDeviceAddress) + 
        " PORT="+ std::to_string(struLoginInfo.wPort) + 
        " username=" + std::string(struLoginInfo.sUserName) + 
        " password=" + std::string(struLoginInfo.sPassword)
    );
    /*LONG lVoiceComHandle = NET_DVR_StartVoiceCom_MR_V30(lUserID, 1, VoiceDataCallBack, NULL);
    if (lVoiceComHandle < 0) {
        std::cerr << "Failed to start voice communication, error: " << NET_DVR_GetLastError() << std::endl;
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        return -1;
    }
    std::cout << "Connected and broadcasting audio..." << std::endl;
    NET_DVR_COMPRESSION_AUDIO audioCompress;    
    BOOL a =  NET_DVR_GetCurrentAudioCompress(lUserID, &audioCompress);

    //ixWebsocket
    ix::initNetSystem();
    ix::WebSocketServer server(8080);
    server.setOnClientMessageCallback(
        [](std::shared_ptr<ix::ConnectionState> connectionState,
        ix::WebSocket& webSocket,
        const ix::WebSocketMessagePtr& msg) {
        std::cout << "Received message: " << msg->wireSize << " bytes" << std::endl;
        if (msg->type == ix::WebSocketMessageType::Message) {
            unsigned char* pcmData = (unsigned char*)msg->str.c_str(); 
            int pcmLength = msg->str.size();

            LPVOID hEncoderInstance = 0;
            NET_DVR_AUDIOENC_INFO enc_info;
            hEncoderInstance = NET_DVR_InitG711Encoder(&enc_info);
            NET_DVR_AUDIOENC_PROCESS_PARAM enc_param;
            enc_param.in_buf = pcmData;
            enc_param.out_buf = new unsigned char[320]; 
            enc_param.g711_type = 0;  
            enc_param.enc_mode = AMR_MR475_;                        
            BOOL result = NET_DVR_EncodeG711Frame(0, &enc_param);
            if (result == 0) {
                std::cerr << "G.711 kodlashda xato!" << std::endl;
            }
            else {
                std::cout << "Kodlangan G.711 ma'lumotlari: ";
                for (int i = 0; i < enc_param.out_frame_size; ++i) {
                    printf("%02X ", enc_param.out_buf[i]);
                }
                std::cout << std::endl;
                SendVoiceData(0, enc_param.out_buf,160);
            }
            // Xotirani tozalash
            delete[] enc_param.out_buf;
        }
    });
    auto res = server.listen();
    if (!res.first){
        return 1;
    }
    server.start();
    server.wait();*/

    // Get photo
    NET_DVR_JPEGPARA jpegPara = { 0 };
    jpegPara.wPicSize = 0xff;          // Ruxsatni avtomatik aniqlash
    jpegPara.wPicQuality = 2;          // Sifat: 2 - yuqori
    const std::string photosDir = "Photos";
    if (!createDirectoryIfNotExist(photosDir)) {
        std::cerr << "Photos papkasini yaratishda xatolik." << std::endl;
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        return -1;
    }
    std::string currentTime = getCurrentTimeString();
    std::string filePath = photosDir + "/Photo_" + currentTime + ".jpg";
    if (!NET_DVR_CaptureJPEGPicture(lUserID, 1, &jpegPara, const_cast<char*>(filePath.c_str()))) {
        std::cerr << "Suratni olishda xatolik: " << NET_DVR_GetLastError() << std::endl;
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        return -1;
    }
    std::cout << "Surat saqlandi: " << filePath << std::endl;

    
    // Get video 10sec intervals 
    NET_DVR_PREVIEWINFO previewInfo;
    previewInfo.hPlayWnd = NULL;   // Video oynasiga chiqish o'rnini belgilash (NULL - faqat saqlash)
    previewInfo.lChannel = 1;      // Kanal raqami (masalan, 1-chi kanal)
    previewInfo.dwStreamType = 0;  // Asosiy oqim (0 - asosiy, 1 - sub-oqim)
    previewInfo.dwLinkMode = 0;    // Ulanish turi (0 - TCP, 1 - UDP)
    LONG realHandle = NET_DVR_RealPlay_V40(lUserID, &previewInfo, NULL, NULL);
    if (realHandle == -1) {
        std::cerr << "Video oqimini olishda xato!" << std::endl;
        return 0;
    }
    std::string videoFolder = "./Videos";
    if (!createDirectoryIfNotExist(videoFolder)) {
        return 0;
    }
    for (int i = 0; i < 5; ++i) {  
        std::string filename = "video_" + getCurrentTimeString() + ".mp4";  // Fayl nomini yaratish
        std::string filePath = videoFolder + "\\" + filename;               // Faylni Videos papkasiga saqlash
        std::cout << "Saqlanmoqda: " << filePath << std::endl;
        if (!NET_DVR_SaveRealData(realHandle, const_cast<char*>(filePath.c_str()))) {
            std::cerr << "Video saqlashda xato!" << std::endl;
        }
        Sleep(10000);   
    }
    NET_DVR_StopSaveRealData(realHandle);
    NET_DVR_StopRealPlay(realHandle);
    NET_DVR_Logout(lUserID);
    NET_DVR_Cleanup();
    return 0;
}