#pragma once

#include <set>
#include <string>

struct recorder_config
{
  std::string appId;
  std::string token;
  std::string ChannelName;
  std::string proxyServer;
  std::string hostUid;
  std::string guestUid;
  std::string callID;

  std::string UserId = "0";
  bool autoSubscribe = true;
  bool useStringUid = false;
  std::string subscribeVideoRegula;
  std::string subscribeAudioRegula;
  std::string subscribeVideoUids;
  std::string subscribeAudioUids;
  std::set<std::string> subscribeVideoUserList;
  std::set<std::string> subscribeAudioUserList;

  bool isAudioOnly = false; 
  bool isVideoOnly = false;


  bool isMix = false;

  int idleLimitSec = 300;
  int maxDuration = 86400;

  std::string recordFileRootDir;

  std::string decryptionMode;
  std::string secret;
  std::string salt;

  uint32_t streamType = 0; // 0: STREAM_HIGH, 1: STREAM_LOW
  bool enableCloudProxy = false;
  int audioIndicationInterval = 0; // in ms, 0 means disable audio indication
  
  struct {
      int sampleRate = 48000;
      int numOfChannels = 1;
  } audio;

  struct {
      int fps = 15;
      int height = 720;
      int width = 1280;
  } video;  // for mix

  int getVideoFrame = 0;
  int captureInterval = 5;
};
