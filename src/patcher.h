#ifndef PATCHER_H
#define PATCHER_H

#include <map>
#include <string>
#include <vector>

#include "stdint.h"
#include "wxutils.h"

class Patcher
{
public:
  enum Status {
    kSuccess = 0,
    kFailedToCopyCriticalFiles,
    kFailedToCreateTemporaryDir,
    kFailedToFindDiskPath,
    kFailedToOpenCopy,
    kFailedToDetectVersion,
    kFailedToCopyRegistryKey,
    kFailedToCreateSaveDir
  };

  typedef std::map<wxString, wxVariant> PatchMap;

  static Status Patch(const PatchMap& patch_map, std::vector<wxString>& incompatibilities, wxString &exe_path);

private:
  Patcher();

  enum Version {
    kUnknownVersion = -1,

    kEnglish10,
    kEnglish11,
    kGerman11,
    kDanish11,
    kSpanish11,

    kVersionCount
  };

  enum FPSCapMode {
    kFPSLimitDefault,
    kFPSLimitUncapped,
    kFPSLimitLimited
  };

  enum ModelQualityMode {
    kModelQualityHigh,
    kModelQualityMedium,
    kModelQualityLow
  };

  static const char* kVersionHashes[];

  static Version DetectVersion(const std::string &lego1_dll);

  static void ModifyFiles(const PatchMap& patch_map, Version version, wxFile& isle_exe, wxFile& lego1_dll, std::vector<wxString>& incompatibilities);

  template<typename T>
  static void WriteIfKeyExists(wxFile& file, const PatchMap& map, const wxString& key, uint32_t offset = -1);

  template<typename T>
  static void WriteData(wxFile& file, T value, int32_t offset = -1);

  static void WriteByte(wxFile& file, uint8_t value, int32_t offset = -1)
  {
    WriteManyBytes(file, value, 1, offset);
  }

  static void WriteManyBytes(wxFile& file, uint8_t value, int count, int32_t offset = -1);

  static inline bool GetValueFromConstMap(const PatchMap& map, const wxString& key, wxVariant& value)
  {
    PatchMap::const_iterator it = map.find(key);

    if (it != map.end()) {
      value = it->second;
      return true;
    }

    return false;
  }

  static void SetBooleanRegistryValue(wxRegKey& reg_key, const PatchMap& map, const wxString& key, const wxString& reg_entry);

};

#endif // PATCHER_H
