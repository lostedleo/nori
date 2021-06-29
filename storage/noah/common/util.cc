#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "common/entity.h"
#include "butil/logging.h"

#include "util.h"

namespace noah {
namespace common {

bool GetIpAddress(std::string* strIp, const std::string& strEth) {
  strIp->clear();

  struct ifaddrs* ifAddrStruct = NULL;
  struct ifaddrs* ifAddrStructOrig = NULL;
  int protoFamily;
  char addressBuffer[256] = {'\0'};

  if (strEth.empty())
    return false;

  if (getifaddrs(&ifAddrStruct))
    return false;

  ifAddrStructOrig = ifAddrStruct;

  while (ifAddrStruct) {
    protoFamily = ifAddrStruct->ifa_addr->sa_family;
    if (!strcmp(strEth.c_str(), ifAddrStruct->ifa_name) &&
        (AF_INET == protoFamily /* || AF_INET6 == protoFamily */))
      break;
    ifAddrStruct = ifAddrStruct->ifa_next;
  }

  if (!ifAddrStruct) {
    freeifaddrs(ifAddrStructOrig);
    return false;
  }

  if (AF_INET == protoFamily || AF_INET6 == protoFamily) {
    if (!inet_ntop(protoFamily,
        &((struct sockaddr_in*)ifAddrStruct->ifa_addr)->sin_addr,
        addressBuffer, 256)) {
      freeifaddrs(ifAddrStructOrig);
      return false;
    }
  }

  freeifaddrs(ifAddrStructOrig);
  strIp->append(addressBuffer);

  return true;
}

bool EncodeLbKey(const std::string& key, const std::string& table_name, int id, char key_type,
                 std::string* encode_key) {
  if (key.empty() || table_name.size() > 255) {
    return false;
  }

  *encode_key = "";
  char* data = {0};
  switch (key_type) {
    case common::KeyType::kMeta: {
                                   int data_size =  1 + key.size();
                                   data = new char[data_size];
                                   data[0] = key_type;
                                   memcpy(data + 1, key.c_str(), key.size());
                                   *encode_key = std::string(data);
                                   delete data;
                                   return true;
                                 }
    case common::KeyType::kKey: {
                                   uint32_t table_name_size = (table_name.size() & 0xFF);
                                   int int_size = sizeof(uint32_t);
                                   int data_size =  1 + 1 + table_name_size + int_size;
                                   data = new char[data_size];
                                   data[0] = key_type;
                                   data[1] = table_name_size;
                                   const char * table_name_ac = table_name.c_str();
                                   memcpy(data + 2, table_name_ac, table_name_size);
                                   memcpy(data + 2 + table_name_size, &id, int_size);
                                   *encode_key = std::string(data, data_size) + key;
                                   delete data;
                                   return true;
                                 }
    default: {
              return false;
            }
  }
  return true;
}

bool DecodeLbKey(const std::string& key, std::string* table_name, int* id, char* key_type,
                 std::string* decode_key) {
  *table_name = "";
  *id = -1;
  *decode_key = "";

  if (key.empty() || key.size() <= 1) {
    return false;
  }
  *key_type = key[0];
  int int_size = sizeof(int);
  switch (*key_type) {
    case common::KeyType::kMeta: {
                                   *decode_key = key.substr(1);
                                   if (*decode_key == "") return false;
                                   return true;
                                 }
    case common::KeyType::kKey: {
                                   if (key.size() < 2) return false;
                                   const char * ac_data = key.c_str();
                                   uint32_t table_name_size = (unsigned char)ac_data[1];
                                   if (key.size() < (2 + table_name_size + int_size)) return false;
                                   *table_name = key.substr(2, table_name_size);
                                   memcpy(id, ac_data + 2 + table_name_size, int_size);
                                   *decode_key = key.substr(2 + table_name_size + int_size);
                                   return true;
                                 }
    default: {
              return false;
            }
  }
  return true;
}

bool GetDirList(const std::string &strDir, std::vector<std::string>* vResultList) {
  vResultList->clear();

  struct dirent *ent = NULL;
  DIR *pDir;
  char dir[512];
  struct stat statbuf;

  if (!(pDir = opendir(strDir.c_str()))) {
    LOG(INFO) <<  "fail open dir: " <<  strDir;
    return false;
  }

  while ((ent = readdir(pDir))) {
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") ||
        strstr(ent->d_name, ".used"))
      continue;

    snprintf(dir, sizeof(dir), "%s/%s", strDir.c_str(), ent->d_name);

    lstat(dir, &statbuf);
    std::string strFilePath;
    strFilePath.append(strDir);
    strFilePath.append("/");
    strFilePath.append(ent->d_name);
    vResultList->push_back(ent->d_name);
  }

  closedir(pDir);
  pDir = NULL;

  return true;
}

}  // namespace common
}  // namespace noah
