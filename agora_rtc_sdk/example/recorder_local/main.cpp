#include <atomic>
#include <csignal>
#include <sys/stat.h>
#include <sys/time.h>
#include <sstream>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <cstring>
#include <thread>
#include <mutex>
#include <iostream>

using json = nlohmann::json;

#include "opt_parser.h"
#include "config.h"
#include "AgoraBase.h"
#include "IAgoraService.h"
#include "IAgoraMediaComponentFactory.h"
#include "IAgoraMediaRtcRecorder.h"
#include "recorderEventHandler.h"
#include "time_util.h"
#include "log.h"

std::atomic<bool> g_bSignalStop(false);

void signal_handler(int signo) {
  (void)signo;
  g_bSignalStop = true;
}

void set_record_folder(std::string& recording_folder, std::string& cname, const recorder_config& config) {
  if (recording_folder.size() > 0 && recording_folder[recording_folder.size() - 1] != '/'){
    recording_folder += '/';
  }
  time_t rawtime;
  struct tm * timeinfo;
  char timebuffer[20];
  time(&rawtime);
  timeinfo = gmtime(&rawtime);
  strftime(timebuffer, 20, "%Y%m%d/", timeinfo);
  recording_folder += std::string(timebuffer);
  
  struct stat st;
  if (stat(recording_folder.c_str(), &st) == -1) {
    mkdir(recording_folder.c_str(), 0777);
  }
  
  // recording_folder += cname;
  // recording_folder += "_";
  // Time2UTCStrWithSlash_ns(timebuffer, 20);
  // recording_folder += std::string(timebuffer);
  recording_folder += config.callID;
  if (stat(recording_folder.c_str(), &st) == -1) {
    mkdir(recording_folder.c_str(), 0777);
  }
}

void init_subscribe_uids(recorder_config& config) {
  if (!config.subscribeVideoUids.empty()) {
    std::istringstream ss(config.subscribeVideoUids);
    std::string uid;
    while (std::getline(ss, uid, ',')) {
      config.subscribeVideoUserList.insert(uid);
    }
  }
  if (!config.subscribeAudioUids.empty()) {
    std::istringstream ss(config.subscribeAudioUids);
    std::string uid;
    while (std::getline(ss, uid, ',')) {
      config.subscribeAudioUserList.insert(uid);
    }
  }
}

// =========================================================================
// FUNGSI UPDATE LAYOUT (FINAL FIX - HAPUS RENDERMODE)
// =========================================================================
void applyLayout(agora::agora_refptr<agora::rtc::IAgoraMediaRtcRecorder>& recorder, 
                 const recorder_config& config, 
                 const std::string& currentGuestUid) 
{
    agora::rtc::VideoMixingLayout layout;
    
    // Alokasi memori heap untuk konfigurasi user (Host + Guest = max 2)
    agora::rtc::UserMixerLayout* userConfigs = new agora::rtc::UserMixerLayout[2];
    int rCount = 0;

    // 1. GUEST REGION (Background Penuh)
    if (!currentGuestUid.empty() && currentGuestUid != "0") {
        userConfigs[rCount].userId = currentGuestUid.c_str(); 
        userConfigs[rCount].config.x = 0.0;
        userConfigs[rCount].config.y = 0.0;
        userConfigs[rCount].config.width = 1.0;
        userConfigs[rCount].config.height = 1.0;
        userConfigs[rCount].config.zOrder = 0;
        userConfigs[rCount].config.alpha = 1.0;
        // renderMode tidak ada di versi ini, dihapus
        rCount++;
    }

    // 2. HOST REGION (Kotak Kecil Kanan Bawah)
    if (!config.hostUid.empty() && config.hostUid != "0") {
        userConfigs[rCount].userId = config.hostUid.c_str();
        userConfigs[rCount].config.x = 0.73;
        userConfigs[rCount].config.y = 0.70;
        userConfigs[rCount].config.width = 0.25;
        userConfigs[rCount].config.height = 0.25;
        userConfigs[rCount].config.zOrder = 1;
        userConfigs[rCount].config.alpha = 1.0;
        // renderMode tidak ada di versi ini, dihapus
        rCount++;
    }

    // Konfigurasi struktur utama VideoMixingLayout
    layout.canvasWidth = config.video.width;
    layout.canvasHeight = config.video.height;
    layout.canvasFps = config.video.fps;     
    layout.backgroundColor = 0xFF000000;     // Format hex ARGB (Hitam Pekat)
    layout.backgroundImage = NULL;           
    layout.userLayoutConfigNum = rCount;     
    layout.userLayoutConfigs = userConfigs;  

    // Terapkan ke Recorder SDK
    recorder->setVideoMixingLayout(layout);

    // Bebaskan memori dari heap dengan aman
    delete[] userConfigs; 
}

int main(int argc, char * const argv[]) 
{
  std::signal(SIGQUIT, signal_handler);
  std::signal(SIGABRT, signal_handler);
  std::signal(SIGINT, signal_handler);

  recorder_config config;
  agora::base::opt_parser parser;
  parser.add_long_opt("appId", &config.appId, "App Id/must", agora::base::opt_parser::require_argu);
  parser.add_long_opt("channel", &config.ChannelName, "Channel Id/must", agora::base::opt_parser::require_argu);
  parser.add_long_opt("uid", &config.UserId, "User Id default is 0/option");
  parser.add_long_opt("channelKey", &config.token, "channelKey/option");
  parser.add_long_opt("proxyServer", &config.proxyServer, "Proxy server IP:Port/option");
  parser.add_long_opt("isMixingEnabled", &config.isMix, "Mixing Enable? (0:1)/option");
  parser.add_long_opt("idle", &config.idleLimitSec, "Default 300s, should be above 3s/option");
  parser.add_long_opt("recordFileRootDir", &config.recordFileRootDir, "recording file root dir/option");
  parser.add_long_opt("autoSubscribe", &config.autoSubscribe, "Auto subscribe video/audio streams of each uid.");
  parser.add_long_opt("useStringUid", &config.useStringUid, "use string uid, defaut: false");
  parser.add_long_opt("subscribeVideoRegula", &config.subscribeVideoRegula, "subscribe video regula");
  parser.add_long_opt("subscribeAudioRegula", &config.subscribeAudioRegula, "subscribe audio regula");
  parser.add_long_opt("isAudioOnly", &config.isAudioOnly, "Default 0:A/V, 1:AudioOnly (0:1)/option");
  parser.add_long_opt("isVideoOnly", &config.isVideoOnly, "Default 0:A/V, 1:VideoOnly (0:1)/option");
  parser.add_long_opt("decryptionMode", &config.decryptionMode, "decryption Mode/option");
  parser.add_long_opt("secret", &config.secret, "input secret when enable decryptionMode/option");
  parser.add_long_opt("salt", &config.salt, "input salt when enable decryptionMode/option");
  parser.add_long_opt("streamType", &config.streamType, "remote video stream type");
  parser.add_long_opt("enableCloudProxy", &config.enableCloudProxy, "enable cloud proxy", agora::base::opt_parser::no_argu);
  parser.add_long_opt("audioIndicationInterval", &config.audioIndicationInterval, "audio indication interval");
  parser.add_long_opt("subscribeVideoUids", &config.subscribeVideoUids, "video stream of specific uids");
  parser.add_long_opt("subscribeAudioUids", &config.subscribeAudioUids, "audio stream of specific uids");
  parser.add_long_opt("getVideoFrame", &config.getVideoFrame, "getVideoFrame option");
  parser.add_long_opt("captureInterval", &config.captureInterval, "Video snapshot interval");
  parser.add_long_opt("hostUid", &config.hostUid, "UID of the agent/host");
  parser.add_long_opt("guestUid", &config.guestUid, "UID of the customer/guest");
  parser.add_long_opt("callID", &config.callID, "Call ID of the session");

  if (!parser.parse_opts(argc, argv) || config.appId.empty() || config.token.empty() || config.ChannelName.empty()) {
    std::ostringstream sout;
    parser.print_usage(argv[0], sout);
    std::cout << sout.str() << std::endl;
    return -1;
  }

  init_subscribe_uids(config);

  if(config.recordFileRootDir.empty()){
    config.recordFileRootDir = ".";
  }
  set_record_folder(config.recordFileRootDir, config.ChannelName, config);
  if (config.recordFileRootDir.back() != '/') {
    config.recordFileRootDir += '/';
  }

  std::string appLogPath = config.recordFileRootDir + "recorder_local.log";
  Logger::instance().open(appLogPath.c_str());
  
  std::string logPath = config.recordFileRootDir + "agorasdk.log";
  auto service = createAgoraService();
  agora::base::AgoraServiceConfiguration service_config;
  service_config.enableAudioDevice = false;
  service_config.enableAudioProcessor = true;
  service_config.enableVideo = true;
  service_config.appId = config.appId.c_str();
  service_config.useStringUid = config.useStringUid;
  service_config.logConfig.filePath = logPath.c_str();
  service_config.logConfig.fileSizeInKB = 1024 * 100;
  int ret = service->initialize(service_config);
  if(ret < 0) {
    AG_LOG(FATAL, "Failed to initialize Agora service, error code: %d", ret);
    Logger::instance().close();
    return -1;
  }

  auto agoraParameter = service->getAgoraParameter();
  if (config.enableCloudProxy) {
      agoraParameter->setBool("rtc.enable_proxy", true);
      agoraParameter->setInt("rtc.force_tcp", 1);
  }
  else if (!config.proxyServer.empty()) {
      agoraParameter->setString("rtc.proxy_server", config.proxyServer.c_str());
      agoraParameter->setInt("rtc.proxy_type", 1); 
      agoraParameter->setInt("rtc.force_tcp", 1);
  }
  else {
      agoraParameter->setInt("rtc.force_tcp", 1);
  }

  service->getAgoraParameter()->setParameters("{\"rtc.audio.enable_user_silence_packet\":true}");
  
  agora::rtc::IMediaComponentFactory* factory = createAgoraMediaComponentFactory();
  agora::agora_refptr<agora::rtc::IAgoraMediaRtcRecorder> recorder = factory->createMediaRtcRecorder();
  bool recordEncodedOnly = false;
  if(!config.isMix){
    recordEncodedOnly = true; 
  }
  recorder->initialize(service, config.isMix, recordEncodedOnly);
  
  std::unique_ptr<RecorderEventHandler> eventHandler{new RecorderEventHandler(recorder, config)};
  recorder->registerRecorderEventHandle(eventHandler.get());
  
  if(!config.decryptionMode.empty() && !config.secret.empty()) {
    agora::rtc::EncryptionConfig encryptConfig;
    if(config.decryptionMode == "AES_128_XTS") {
      encryptConfig.encryptionMode = agora::rtc::AES_128_XTS;
    } else if (config.decryptionMode == "AES_128_ECB") {
      encryptConfig.encryptionMode = agora::rtc::AES_128_ECB;
    } else if (config.decryptionMode == "AES_256_XTS") {
      encryptConfig.encryptionMode = agora::rtc::AES_256_XTS;
    } else if (config.decryptionMode == "SM4_128_ECB") {
      encryptConfig.encryptionMode = agora::rtc::SM4_128_ECB;
    } else if (config.decryptionMode == "AES_128_GCM") {
      encryptConfig.encryptionMode = agora::rtc::AES_128_GCM;
    } else if (config.decryptionMode == "AES_256_GCM") {
      encryptConfig.encryptionMode = agora::rtc::AES_256_GCM;
    } else if (config.decryptionMode == "AES_128_GCM2") {
      encryptConfig.encryptionMode = agora::rtc::AES_128_GCM2;
    } else if (config.decryptionMode == "AES_256_GCM2") {
      encryptConfig.encryptionMode = agora::rtc::AES_256_GCM2;
    }
    encryptConfig.encryptionKey = config.secret.c_str();
    if(!config.salt.empty()){
      memcpy(encryptConfig.encryptionKdfSalt, config.salt.data(), 32);
    }
    recorder->enableEncryption(true, encryptConfig);
  }
  
  if(config.autoSubscribe){
    recorder->subscribeAllAudio();
    agora::rtc::VideoSubscriptionOptions options;
    options.encodedFrameOnly = (!config.isMix && config.getVideoFrame <= 1);
    options.type = config.streamType == 0 ? agora::rtc::VIDEO_STREAM_HIGH : agora::rtc::VIDEO_STREAM_LOW;
    recorder->subscribeAllVideo(options);
  }

  std::unique_ptr<RecorderVideoFrameObserver> videoFrameObserver{new RecorderVideoFrameObserver()};
  agora::rtc::RecorderVideoFrameCaptureConfig videoFrameCaptureConfig;
  if(config.getVideoFrame != 0){
    videoFrameCaptureConfig.videoFrameType = static_cast<agora::rtc::VideoFrameCaptureType>(config.getVideoFrame-1);
    videoFrameCaptureConfig.jpgCaptureIntervalInSec = config.captureInterval;
    videoFrameCaptureConfig.jpgFileStorePath = config.recordFileRootDir.c_str();
    videoFrameCaptureConfig.observer = videoFrameObserver.get();
    recorder->enableRecorderVideoFrameCapture(true, videoFrameCaptureConfig);
  }

  if(config.audioIndicationInterval > 0) {
    recorder->setAudioVolumeIndicationParameters(config.audioIndicationInterval);
  }

  recorder->joinChannel(config.token.c_str(), config.ChannelName.c_str(), config.UserId.c_str());
  
  // Mutex global untuk mencegah race condition dari thread STDIN
  std::mutex layoutMutex;

  if(config.isMix){
      char fileName[512];
      char timeBuffer[80];
      Time2UTCStr(timeBuffer, 80);
      snprintf(fileName, 512, "%s_%s.mp4", config.callID.c_str(), timeBuffer);
      std::string storagePath = config.recordFileRootDir + fileName;

      agora::media::MediaRecorderStreamType streamType = agora::media::STREAM_TYPE_BOTH;
      if(config.isAudioOnly) {
          streamType = agora::media::STREAM_TYPE_AUDIO;
      } else if(config.isVideoOnly) {
          streamType = agora::media::STREAM_TYPE_VIDEO;
      }

      // Panggil helper function untuk inisialisasi layout pertama kali
      {
          std::lock_guard<std::mutex> lock(layoutMutex);
          applyLayout(recorder, config, config.guestUid);
      }

      agora::media::MediaRecorderConfiguration recorder_config;
      recorder_config.width = config.video.width;
      recorder_config.height = config.video.height;
      recorder_config.fps = config.video.fps;
      recorder_config.storagePath = storagePath.c_str();
      recorder_config.sample_rate = config.audio.sampleRate;
      recorder_config.channel_num = config.audio.numOfChannels;
      recorder_config.streamType = streamType;
      recorder_config.maxDurationMs = config.maxDuration * 1000;

      recorder->setRecorderConfig(recorder_config);
      recorder->startRecording();
      
      AG_LOG(INFO, "Recording started with PiP Layout. Host: %s, Guest: %s", 
            config.hostUid.c_str(), config.guestUid.c_str());
  }

  // =========================================================================
  // STDIN LISTENER (Update Layout Dinamis saat Reconnect)
  // =========================================================================
  std::thread stdinThread;
  if (config.isMix) {
      stdinThread = std::thread([recorder, &config, &layoutMutex]() {
          std::string line;
          AG_LOG(INFO, "[C++] Thread pembaca stdin aktif.");
          while (!g_bSignalStop && std::getline(std::cin, line)) {
              if (line.rfind("updateGuestUid:", 0) == 0) {
                  std::string newGuestUidStr = line.substr(15);
                  try {
                      // Validasi input angka (meski kita simpan sbg string)
                      std::stoull(newGuestUidStr); 
                      AG_LOG(INFO, "[C++] Stdin menerima update guestUid: %s", newGuestUidStr.c_str());

                      std::lock_guard<std::mutex> lock(layoutMutex);
                      // Panggil helper lagi untuk menerapkan layout baru
                      auto recPtr = recorder; 
                      applyLayout(recPtr, config, newGuestUidStr);
                      // Jangan ubah config.guestUid di thread ini secara langsung untuk menghindari masalah memory
                      // config.guestUid = newGuestUidStr; 

                      AG_LOG(INFO, "[C++] Layout berhasil diperbarui secara dinamis ke SDK.");
                  } catch (const std::exception& e) {
                      AG_LOG(ERROR, "[C++] Gagal memproses update layout dinamis: %s", e.what());
                  }
              }
          }
      });
      stdinThread.detach(); 
  }

  int idleLimitMs = config.idleLimitSec * 1000;
  int logCounter = 0;
  while (!g_bSignalStop) {
    usleep(100*1000);
    int userCount = eventHandler->getUidsCount();
    if (userCount == 0) {
      idleLimitMs -= 100;
      if (idleLimitMs <= 0) {
        AG_LOG(INFO, "No user joined for %d seconds, exiting...", config.idleLimitSec);
        break;
      }
    } else {
      idleLimitMs = config.idleLimitSec * 1000; 
      if (++logCounter >= 100) { 
            eventHandler->printActiveUsers();
            logCounter = 0;
      }
    }   
  }

  if(config.autoSubscribe){
    recorder->unsubscribeAllAudio();
    recorder->unsubscribeAllVideo();
  }
  if(config.isMix){
    recorder->stopRecording();
  } else {
    std::set<std::string> uids = eventHandler->getUids();
    for(auto& uid : uids){
      recorder->stopSingleRecordingByUid(uid.c_str());
    }
  }

  if(config.getVideoFrame != 0){
    recorder->enableRecorderVideoFrameCapture(false, videoFrameCaptureConfig);
  }
  recorder->unregisterRecorderEventHandle(eventHandler.get());
  eventHandler = nullptr;
  recorder->leaveChannel();
  recorder = nullptr;
  service->release();
  Logger::instance().close();
  return 0;
}
