#include "patcher.h"

#include <string.h>

#include "sha1.hpp"

const char* Patcher::kVersionHashes[] = {
  "58fcf0f6500614e9f743712d1dd4d340088123de",
  "bbe289e89e5a39949d272174162711ea5cff522c",
  "96a6bae8345aa04c21f1b319a632caecfee22443",
  "8dfd3e5fdde8c95c61013069795171163c9a4821",
  "47ee50fc1ec5f6c54f465eb296d2f1b7ca25d5d2"
};

int round(double x)
{
  if (x < 0.0)
    return (int)(x - 0.5);
  else
    return (int)(x + 0.5);
}

Patcher::Status Patcher::Patch(const PatchMap &patch_map, std::vector<wxString> &incompatibilities, wxString& exe_path)
{
  // Create temporary dir where we rebuild
  wxString rebuild_dir = wxString::Format(wxT("%s/LEGOIslandRebuilder"), wxStandardPaths::Get().GetTempDir());
  if (!wxDirExists(rebuild_dir) && !wxMkdir(rebuild_dir)) {
    return kFailedToCreateTemporaryDir;
  }

  // Find registry entry
  wxRegKey reg_key(wxRegKey::HKLM, wxT("SOFTWARE\\Mindscape\\LEGO Island"));

  // Retrieve diskpath from registry
  wxString install_dir;
  reg_key.QueryValue(wxT("diskpath"), install_dir);

  // Handle error retrieving diskpath
  if (install_dir.IsEmpty()) {
    return kFailedToFindDiskPath;
  }

  // Copy files from install directory to rebuild directory
  wxString isle_exe_filename = wxT("/ISLE.EXE");
  wxString lego1_dll_filename = wxT("/LEGO1.DLL");
  wxString isle_exe_src_path = install_dir;
  isle_exe_src_path.Append(isle_exe_filename);
  wxString isle_exe_dst_path = rebuild_dir;
  isle_exe_dst_path.Append(isle_exe_filename);
  wxString lego1_dll_src_path = install_dir;
  lego1_dll_src_path.Append(lego1_dll_filename);
  wxString lego1_dll_dst_path = rebuild_dir;
  lego1_dll_dst_path.Append(lego1_dll_filename);

  if (!wxCopyFile(isle_exe_src_path, isle_exe_dst_path, true)) {
    return kFailedToCopyCriticalFiles;
  }

  if (!wxCopyFile(lego1_dll_src_path, lego1_dll_dst_path, true)) {
    return kFailedToCopyCriticalFiles;
  }

  // Detect which version this is
  Version version = DetectVersion(std::string(lego1_dll_dst_path.c_str()));

  if (version == kUnknownVersion) {
    return kFailedToDetectVersion;
  }

  // Open our copies, which we should have full access over
  wxFile isle_exe, lego1_dll;

  if (!isle_exe.Open(isle_exe_dst_path, wxFile::read_write)) {
    return kFailedToOpenCopy;
  }

  if (!lego1_dll.Open(lego1_dll_dst_path, wxFile::read_write)) {
    return kFailedToOpenCopy;
  }

  ModifyFiles(patch_map, version, isle_exe, lego1_dll, incompatibilities);

  wxRegKey destination_key(wxRegKey::HKCU, wxT("SOFTWARE\\Mindscape\\LEGO Island"));

  if (!reg_key.Copy(destination_key)) {
    return kFailedToCopyRegistryKey;
  }

  // Set full screen value
  SetBooleanRegistryValue(destination_key, patch_map, wxT("FullScreen"), wxT("Full Screen"));

  // Set draw cursor value
  SetBooleanRegistryValue(destination_key, patch_map, wxT("DrawCursor"), wxT("Draw Cursor"));

  // Set Joystick toggle
  SetBooleanRegistryValue(destination_key, patch_map, wxT("UseJoystick"), wxT("UseJoystick"));

  // Toggle music on or off
  SetBooleanRegistryValue(destination_key, patch_map, wxT("MusicToggle"), wxT("Music"));

  // Redirect save path
  wxVariant value;
  if (GetValueFromConstMap(patch_map, wxT("RedirectSaveData"), value) && value.GetBool()) {
    wxString save_dir = wxString::Format(wxT("%s/LEGO Island"), wxStandardPaths::Get().GetUserConfigDir());

    if (!wxDirExists(save_dir) && !wxMkdir(save_dir)) {
      return kFailedToCreateSaveDir;
    }
    destination_key.SetValue(wxT("savepath"), save_dir);
  }

  // Set Direct3D device
  if (GetValueFromConstMap(patch_map, wxT("D3DDevice"), value)) {
    destination_key.SetValue(wxT("3D Device ID"), value.GetString());
  }

  exe_path = isle_exe_dst_path;

  return kSuccess;
}

Patcher::Version Patcher::DetectVersion(const std::string &lego1_dll)
{
  std::string hash = SHA1::from_file(lego1_dll);

  for (int i=0; i<kVersionCount; i++) {
    if (strcmp(kVersionHashes[i], hash.c_str()) == 0) {
      return static_cast<Version>(i);
    }
  }

  return kUnknownVersion;
}

template<typename T>
void Patcher::WriteIfKeyExists(wxFile &file, const PatchMap &map, const wxString &key, uint32_t offset)
{
  wxVariant v;

  if (GetValueFromConstMap(map, key)) {
    WriteData<T>(file, GetValueFromConstMap(map, key).As<T>(), offset);
  }
}

void Patcher::ModifyFiles(const PatchMap& patch_map, Patcher::Version version, wxFile &isle_exe, wxFile &lego1_dll, std::vector<wxString>& incompatibilities)
{
  uint32_t nav_offset, fov_offset_1, fov_offset_2, turn_speed_routine_loc, dsoundoffs1,
      dsoundoffs2, dsoundoffs3, remove_fps_limit, jukebox_path_offset, model_quality_offset;

  switch (version) {
  case kEnglish10:
    nav_offset = 0xF2C28;
    fov_offset_1 = 0xA1D67;
    fov_offset_2 = 0xA1D32;
    turn_speed_routine_loc = 0x54258;
    dsoundoffs1 = 0xB48FB;
    dsoundoffs2 = 0xB48F1;
    dsoundoffs3 = 0xAD7D3;
    remove_fps_limit = 0x7A68B;
    jukebox_path_offset = 0xD28F6;
    model_quality_offset = 0xFF028;
    break;
  case kEnglish11:
  default:
    nav_offset = 0xF3228;
    fov_offset_1 = 0xA22D7;
    fov_offset_2 = 0xA22A2;
    turn_speed_routine_loc = 0x544F8;
    dsoundoffs1 = 0xB120B;
    dsoundoffs2 = 0xB1201;
    dsoundoffs3 = 0xADD43;
    remove_fps_limit = 0x7ABAB;
    jukebox_path_offset = 0xD2E66;
    model_quality_offset = 0xFF648;
    break;
  case kGerman11:
    nav_offset = 0xF3428;
    fov_offset_1 = 0xA2517;
    fov_offset_2 = 0xA24E2;
    turn_speed_routine_loc = 0x544F8;
    dsoundoffs1 = 0xB144B;
    dsoundoffs2 = 0xB1441;
    dsoundoffs3 = 0xADF83;
    remove_fps_limit = 0x7AD9B;
    jukebox_path_offset = 0xD30A6;
    model_quality_offset = 0xFF878;
    break;
  case kDanish11:
    nav_offset = 0xF3428;
    fov_offset_1 = 0xA24C7;
    fov_offset_2 = 0xA2492;
    turn_speed_routine_loc = 0x544F8;
    dsoundoffs1 = 0xB13FB;
    dsoundoffs2 = 0xB13F1;
    dsoundoffs3 = 0xADF33;
    remove_fps_limit = 0x7AD5B;
    jukebox_path_offset = 0xD3056;
    model_quality_offset = 0xFF868;
    break;
  case kSpanish11:
    nav_offset = 0xF3228;
    fov_offset_1 = 0xA2407;
    fov_offset_2 = 0xA23D2;
    turn_speed_routine_loc = 0x544F8;
    dsoundoffs1 = 0xB133B;
    dsoundoffs2 = 0xB1331;
    dsoundoffs3 = 0xADE73;
    remove_fps_limit = 0x7ACBB;
    jukebox_path_offset = 0xD2F96;
    model_quality_offset = 0xFF658;
    break;
  }

  wxVariant value;

  if (GetValueFromConstMap(patch_map, wxT("MouseDeadzone"), value)) {
    WriteData<int32_t>(lego1_dll, value.GetLong(), nav_offset);
  }

  // Skip zero threshold
  lego1_dll.Seek(4, wxFromCurrent);

  if (GetValueFromConstMap(patch_map, wxT("MovementMaxSpeed"), value)) {
    WriteData<float>(lego1_dll, value.GetDouble());
  }

  if (GetValueFromConstMap(patch_map, wxT("TurnMaxSpeed"), value)) {
    WriteData<float>(lego1_dll, value.GetDouble());
  }

  if (GetValueFromConstMap(patch_map, wxT("MovementMaxAcceleration"), value)) {
    WriteData<float>(lego1_dll, value.GetDouble());
  }

  if (GetValueFromConstMap(patch_map, wxT("TurnMaxAcceleration"), value)) {
    WriteData<float>(lego1_dll, value.GetDouble());
  }

  if (GetValueFromConstMap(patch_map, wxT("MovementMinAcceleration"), value)) {
    WriteData<float>(lego1_dll, value.GetDouble());
  }

  if (GetValueFromConstMap(patch_map, wxT("TurnMinAcceleration"), value)) {
    WriteData<float>(lego1_dll, value.GetDouble());
  }

  if (GetValueFromConstMap(patch_map, wxT("MovementDeceleration"), value)) {
    WriteData<float>(lego1_dll, value.GetDouble());
  }

  if (GetValueFromConstMap(patch_map, wxT("TurnDeceleration"), value)) {
    WriteData<float>(lego1_dll, value.GetDouble());
  }

  // Skip 0.4 value that we don't know yet
  lego1_dll.Seek(4, wxFromCurrent);

  if (GetValueFromConstMap(patch_map, wxT("TurnUseVelocity"), value)) {
    WriteData<int32_t>(lego1_dll, value.GetLong());
  }

  // Patch EXE to read from HKCU instead of HKLM
  WriteData<uint8_t>(isle_exe, 0x01, 0x1B5F);

  if (GetValueFromConstMap(patch_map, wxT("UnhookTurnSpeed"), value) && value.GetBool()) {
    // Write routine to use frame delta time to adjust the turn speed
    WriteByte(lego1_dll, 0xD9, turn_speed_routine_loc);
    WriteByte(lego1_dll, 0x46);
    WriteByte(lego1_dll, 0x24);
    WriteByte(lego1_dll, 0xD8);
    WriteByte(lego1_dll, 0x4C);
    WriteByte(lego1_dll, 0x24);
    WriteByte(lego1_dll, 0x14);
    WriteByte(lego1_dll, 0xD8);
    WriteByte(lego1_dll, 0x4E);
    WriteByte(lego1_dll, 0x34);

    // Frees up 26 bytes
    WriteManyBytes(lego1_dll, 0x90, 26);
  }

  if (GetValueFromConstMap(patch_map, wxT("StayActiveWhenDefocused"), value) && value.GetBool()) {
    // Remove code that writes focus value to memory, effectively keeping it always true - frees up 3 bytes
    WriteManyBytes(isle_exe, 0x90, 3, 0x1363);

    // Write DirectSound flags to allow audio to play while the window is defocused
    WriteByte(lego1_dll, 0x80, dsoundoffs1);
    WriteByte(lego1_dll, 0x80, 0x5B96);
    WriteByte(lego1_dll, 0x80, dsoundoffs2);
    WriteByte(lego1_dll, 0x80, dsoundoffs3);
  }

  if (GetValueFromConstMap(patch_map, wxT("MultipleInstances"), value) && value.GetBool()) {
    // LEGO Island uses FindWindowA in user32.dll to determine if it's already running, here we replace the call with moving 0x0 into EAX, simulating a NULL response from FindWindowA
    WriteByte(isle_exe, 0xEB, 0x10B5);
  }

  // Redirect JUKEBOX.SI if we're inserting music
  /*if (music_injector.ReplaceCount() > 0)
  {
    Uri uri1 = new Uri(jukebox_output.Substring(0, jukebox_output.LastIndexOf(".")));

    Uri uri2 = new Uri(Path.Combine(source_dir, "ISLE.EXE"));
    Uri relative = uri2.MakeRelativeUri(uri1);
    string jukebox_path = "\\" + Uri.UnescapeDataString(relative.ToString()).Replace("/", "\\");

    switch (version) {
    case kEnglish10:
      WriteByte(lego1_dll, 0xF6, 0x51EF5);
      WriteByte(lego1_dll, 0x34);
      WriteByte(lego1_dll, 0x0D);
      WriteByte(lego1_dll, 0x10);
      break;
    case kEnglish11:
    default:
      WriteByte(lego1_dll, 0x66, 0x52195);
      WriteByte(lego1_dll, 0x3A);
      WriteByte(lego1_dll, 0x0D);
      WriteByte(lego1_dll, 0x10);
      break;
    case kGerman11:
      WriteByte(lego1_dll, 0xA6, 0x52195);
      WriteByte(lego1_dll, 0x3C);
      WriteByte(lego1_dll, 0x0D);
      WriteByte(lego1_dll, 0x10);
      break;
    case kDanish11:
      WriteByte(lego1_dll, 0x56, 0x52195);
      WriteByte(lego1_dll, 0x3C);
      WriteByte(lego1_dll, 0x0D);
      WriteByte(lego1_dll, 0x10);
      break;
    case kSpanish11:
      WriteByte(lego1_dll, 0x96, 0x52195);
      WriteByte(lego1_dll, 0x3B);
      WriteByte(lego1_dll, 0x0D);
      WriteByte(lego1_dll, 0x10);
      break;
    }

    WriteString(lego1_dll, jukebox_path, jukebox_path_offset);
  }*/

  // FOV Patch
  if (GetValueFromConstMap(patch_map, wxT("FOVMultiplier"), value)) {
    WriteByte(lego1_dll, 0xEB, fov_offset_1);
    WriteByte(lego1_dll, 0xC9);
    //WriteByte(lego1_dll, 0x90);
    //WriteByte(lego1_dll, 0x90);

    WriteByte(lego1_dll, 0x68, fov_offset_2);
    WriteData<float>(lego1_dll, value.GetDouble());
    WriteByte(lego1_dll, 0xD8);
    WriteByte(lego1_dll, 0x0C);
    WriteByte(lego1_dll, 0x24);
    WriteByte(lego1_dll, 0x5E);
    WriteByte(lego1_dll, 0xEB);
    WriteByte(lego1_dll, 0x2E);
  }

  // FPS Patch
  if (GetValueFromConstMap(patch_map, wxT("FPSLimit"), value)) {
    FPSCapMode fps_limit = (FPSCapMode)value.GetLong();
    if (fps_limit == kFPSLimitUncapped) {
      // Write zero frame delay resulting in uncapped frame rate
      WriteData<int32_t>(isle_exe, 0, 0x4B4);
    } else if (fps_limit == kFPSLimitLimited && GetValueFromConstMap(patch_map, wxT("CustomFPS"), value)) {
      // Calculate frame delay and write new limit
      uint32_t delay = round(1000.0 / value.GetDouble());

      WriteData<int32_t>(isle_exe, delay, 0x4B4);
    }

    if (fps_limit != kFPSLimitDefault) {
      // Disables 30 FPS limit in Information Center when using software mode
      WriteManyBytes(lego1_dll, 0x90, 8, remove_fps_limit);
    }
  }

  if (GetValueFromConstMap(patch_map, wxT("ModelQuality"), value)) {
    ModelQualityMode model_quality = (ModelQualityMode)value.GetLong();

    switch (model_quality) {
    case kModelQualityLow:
      WriteData<float>(lego1_dll, 0.0f, model_quality_offset);
      break;
    case kModelQualityMedium:
      WriteData<float>(lego1_dll, 3.6f, model_quality_offset);
      break;
    case kModelQualityHigh:
      WriteData<float>(lego1_dll, 5.0f, model_quality_offset);
      break;
    }
  }

  // INCOMPLETE: Resolution hack:
  if (GetValueFromConstMap(patch_map, wxT("OverrideResolution"), value) && value.GetBool()) {
    GetValueFromConstMap(patch_map, wxT("ResolutionWidth"), value);
    int32_t res_width = value.GetLong();
    GetValueFromConstMap(patch_map, wxT("ResolutionHeight"), value);
    int32_t res_height = value.GetLong();

    // Changes window size
    WriteData<int32_t>(isle_exe, res_width, 0xE848);
    WriteData<int32_t>(isle_exe, res_height, 0xE84C);

    // Changes D3D render size
    WriteData<int32_t>(isle_exe, res_width - 1, 0x4D0);
    WriteData<int32_t>(isle_exe, res_height - 1, 0x4D7);

    // Write code to upscale the bitmaps
    if (GetValueFromConstMap(patch_map, wxT("UpscaleBitmaps"), value) && value.GetBool()) {
      WriteByte(lego1_dll, 0xE9, 0xB20E9);
      WriteByte(lego1_dll, 0x2D);
      WriteByte(lego1_dll, 0x01);
      WriteByte(lego1_dll, 0x00);
      WriteByte(lego1_dll, 0x00);
      WriteByte(lego1_dll, 0x8B);
      WriteByte(lego1_dll, 0x56);
      WriteByte(lego1_dll, 0x1C);
      WriteByte(lego1_dll, 0x6A);
      WriteByte(lego1_dll, 0x00);
      WriteByte(lego1_dll, 0x8D);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xE4);
      WriteByte(lego1_dll, 0xF6);
      WriteByte(lego1_dll, 0x42);
      WriteByte(lego1_dll, 0x30);
      WriteByte(lego1_dll, 0x08);
      WriteByte(lego1_dll, 0x74);
      WriteByte(lego1_dll, 0x07);
      WriteByte(lego1_dll, 0x68);
      WriteByte(lego1_dll, 0x00);
      WriteByte(lego1_dll, 0x80);
      WriteByte(lego1_dll, 0x00);
      WriteByte(lego1_dll, 0x00);
      WriteByte(lego1_dll, 0xEB);
      WriteByte(lego1_dll, 0x02);
      WriteByte(lego1_dll, 0x6A);
      WriteByte(lego1_dll, 0x00);
      WriteByte(lego1_dll, 0x8B);
      WriteByte(lego1_dll, 0x3B);
      WriteByte(lego1_dll, 0x50);
      WriteByte(lego1_dll, 0x51);
      WriteByte(lego1_dll, 0x8D);
      WriteByte(lego1_dll, 0x4D);
      WriteByte(lego1_dll, 0xD4);
      WriteByte(lego1_dll, 0x51);
      WriteByte(lego1_dll, 0x53);
      WriteByte(lego1_dll, 0x53);
      WriteByte(lego1_dll, 0x50);
      WriteByte(lego1_dll, 0x68);

      WriteData<float>(lego1_dll, (float)res_height / 480.0f);

      int32_t x_offset = round((res_width - (res_height / 3.0 * 4.0))/2.0);

      WriteByte(lego1_dll, 0xDB);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xD4);
      WriteByte(lego1_dll, 0xD8);
      WriteByte(lego1_dll, 0x0C);
      WriteByte(lego1_dll, 0x24);
      WriteByte(lego1_dll, 0xDB);
      WriteByte(lego1_dll, 0x5D);
      WriteByte(lego1_dll, 0xD4);
      WriteByte(lego1_dll, 0xDB);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xD8);
      WriteByte(lego1_dll, 0xD8);
      WriteByte(lego1_dll, 0x0C);
      WriteByte(lego1_dll, 0x24);
      WriteByte(lego1_dll, 0xDB);
      WriteByte(lego1_dll, 0x5D);
      WriteByte(lego1_dll, 0xD8);
      WriteByte(lego1_dll, 0xDB);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xDC);
      WriteByte(lego1_dll, 0xD8);
      WriteByte(lego1_dll, 0x0C);
      WriteByte(lego1_dll, 0x24);
      WriteByte(lego1_dll, 0xDB);
      WriteByte(lego1_dll, 0x5D);
      WriteByte(lego1_dll, 0xDC);
      WriteByte(lego1_dll, 0xDB);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xE0);
      WriteByte(lego1_dll, 0xD8);
      WriteByte(lego1_dll, 0x0C);
      WriteByte(lego1_dll, 0x24);
      WriteByte(lego1_dll, 0xDB);
      WriteByte(lego1_dll, 0x5D);
      WriteByte(lego1_dll, 0xE0);
      WriteByte(lego1_dll, 0x58);
      WriteByte(lego1_dll, 0x8B);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xD4);
      WriteByte(lego1_dll, 0x05);
      WriteData<int32_t>(lego1_dll, x_offset);
      WriteByte(lego1_dll, 0x89);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xD4);
      WriteByte(lego1_dll, 0x8B);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xDC);
      WriteByte(lego1_dll, 0x05);
      WriteData<int32_t>(lego1_dll, x_offset);
      WriteByte(lego1_dll, 0x89);
      WriteByte(lego1_dll, 0x45);
      WriteByte(lego1_dll, 0xDC);

      // Frees up 143 bytes of NOPs
      WriteManyBytes(lego1_dll, 0x90, 143);

      WriteByte(lego1_dll, 0x58);
      WriteByte(lego1_dll, 0x5B);

      WriteByte(lego1_dll, 0xE9, 0xB22F3);
      WriteByte(lego1_dll, 0xF6);
      WriteByte(lego1_dll, 0xFD);
      WriteByte(lego1_dll, 0xFF);
      WriteByte(lego1_dll, 0xFF);

      // Frees up 19 bytes of NOPs
      WriteManyBytes(lego1_dll, 0x90, 19);
    }
  }

  if (GetValueFromConstMap(patch_map, wxT("DisableAutoFinishBuilding"), value) && value.GetBool()) {
    if (version == kEnglish10) {
      incompatibilities.push_back(wxT("DisableAutoFinishBuilding"));
    } else {
      // Disables cutscene/exit code
      WriteManyBytes(lego1_dll, 0x90, 5, 0x22C0B);

      // Disables flag that freezes the UI on completion
      WriteManyBytes(lego1_dll, 0x90, 7, 0x22C6A);
    }
  }
}

void Patcher::WriteManyBytes(wxFile &file, uint8_t value, int count, int32_t offset)
{
  if (offset > -1) {
    file.Seek(offset, wxFromStart);
  }

  uint8_t* buffer = new uint8_t[count];
  for (int i=0; i<count; i++) {
    buffer[i] = value;
  }

  file.Write(buffer, count);

  delete [] buffer;
}

void Patcher::SetBooleanRegistryValue(wxRegKey &reg_key, const PatchMap &map, const wxString &key, const wxString &reg_entry)
{
  wxVariant value;

  if (GetValueFromConstMap(map, key, value)) {
    reg_key.SetValue(reg_entry, value.GetBool() ? wxT("YES") : wxT("NO"));
  }
}

template<typename T>
void Patcher::WriteData(wxFile &file, T value, int32_t offset)
{
  if (offset > -1) {
    file.Seek(offset, wxFromStart);
  }

  file.Write(static_cast<const void*>(&value), sizeof(value));
}
