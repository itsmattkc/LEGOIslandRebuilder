#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <map>
#include <vector>

#include "wxutils.h"

class MainWindow : public wxFrame
{
public:
  MainWindow();

  void AddDirect3DDevice(const wxString& name, const wxString& device_id);

private:
  void SetupPatchList();

  void AddPatch(wxPGProperty* property, const wxString& description);

  void OnExit(wxCommandEvent& event);

  void OnRun(wxCommandEvent& event);

  void OnPatchGridSelected(wxPropertyGridEvent &event);

  void OnPatchGridChanged(wxPropertyGridEvent &event);

  void GetDirect3DDevices();

  enum {
    ID_RUN = 1,
    ID_PATCHGRID
  };

  wxArrayString d3d_device_names_;
  wxArrayString d3d_device_entries_;

  wxDECLARE_EVENT_TABLE();

  wxPropertyGrid* patches_grid_;
  wxStaticText* patches_title_lbl_;
  wxStaticText* patches_description_lbl_;

  std::map<wxPGProperty*, wxString> description_map_;

};

#endif // MAINWINDOW_H
