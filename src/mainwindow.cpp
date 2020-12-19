#include "mainwindow.h"

#include <d3d.h>
#include <ddraw.h>
#include <windows.h>
#include <wx/hyperlink.h>
#include <wx/menu.h>
#include <wx/notebook.h>
#include <wx/process.h>
#include <wx/propgrid/propgrid.h>
#include <wx/splitter.h>

#include "patcher.h"

MainWindow::MainWindow() :
  wxFrame(NULL, -1, _("LEGO Island Rebuilder"), wxDefaultPosition, wxSize(420, 420))
{
  // Set window icon to application icon
  SetIcon(wxICON(IDI_ICON1));

  // Initialize common padding value
  const int padding = 4;

  // Create central panel (Windows and/or wxWidgets insists on using a wxPanel)
  wxPanel* central_panel = new wxPanel(this);

  // Create layout/sizer
  wxBoxSizer* layout = new wxBoxSizer(wxVERTICAL);

  // Create header text
  wxStaticText* header_lbl = new wxStaticText(central_panel, wxID_ANY,
                                              _("LEGO Island Rebuilder\n"));
  MakeTextBold(header_lbl);
  layout->Add(header_lbl,
              0,
              wxTOP | wxALIGN_CENTER_HORIZONTAL,
              padding);

  // Create link
  layout->Add(new wxHyperlinkCtrl(central_panel, wxID_ANY,
                                  _("by MattKC (www.legoisland.org)"),
                                  wxT("https://www.legoisland.org/")),
              0, wxALIGN_CENTER_HORIZONTAL);

  // Create tabs
  wxNotebook* notebook = new wxNotebook(central_panel, wxID_ANY);
  layout->Add(notebook, 1, wxEXPAND | (wxALL & ~wxBOTTOM), padding);

  // Create patches tab
  wxPanel* patches_tab = new wxPanel(notebook);
  notebook->AddPage(patches_tab, _("Patches"));

  // Create patches property grid
  wxBoxSizer* patches_sizer = new wxBoxSizer(wxVERTICAL);

  wxSplitterWindow* splitter = new wxSplitterWindow(patches_tab, wxID_ANY, wxDefaultPosition,
                                                    wxDefaultSize, wxSP_NOBORDER);
  patches_sizer->Add(splitter, 1, wxEXPAND);
  patches_tab->SetSizer(patches_sizer);

  patches_grid_ = new wxPropertyGrid(splitter, ID_PATCHGRID, wxDefaultPosition, wxDefaultSize,
                                     wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER);
  wxFont patches_grid_font = patches_grid_->GetFont();
  patches_grid_font.SetPointSize(8);
  patches_grid_->SetFont(patches_grid_font);
  SetupPatchList();

  // Create patches description label
  wxPanel* description_panel = new wxPanel(splitter);
  wxBoxSizer* description_sizer = new wxBoxSizer(wxVERTICAL);
  description_panel->SetSizer(description_sizer);

  patches_title_lbl_ = new wxStaticText(description_panel, wxID_ANY, wxString());
  MakeTextBold(patches_title_lbl_);
  description_sizer->Add(patches_title_lbl_, 0, wxEXPAND);

  patches_description_lbl_ = new wxStaticText(description_panel, wxID_ANY, wxString(),
                                              wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
  patches_description_lbl_->SetMinSize(wxSize(0, patches_description_lbl_->GetFont().GetPixelSize().y * 3));
  description_sizer->Add(patches_description_lbl_, 1, wxEXPAND);

  splitter->SplitHorizontally(patches_grid_, description_panel);

  // Create music tab
  wxPanel* music_tab = new wxPanel(notebook);
  notebook->AddPage(music_tab, _("Music"));

  // Create launch buttons
  wxButton* run_btn = new wxButton(central_panel, ID_RUN, _("Run"));
  MakeTextBold(run_btn);
  layout->Add(run_btn, 0, wxEXPAND | wxALL, padding);

  central_panel->SetSizer(layout);

  patches_grid_->SetFocus();

  // Auto-size splitter
  splitter->SetSashPosition(200);
}

void MainWindow::AddDirect3DDevice(const wxString &name, const wxString &device_id)
{
  d3d_device_names_.push_back(name);
  d3d_device_entries_.push_back(device_id);
}

void MainWindow::SetupPatchList()
{
  patches_grid_->Append(new wxPropertyCategory(_("Controls")));

  AddPatch(new wxBoolProperty(_("Unhook Turning From Frame Rate"), wxT("UnhookTurnSpeed"), true),
           _("LEGO Island contains a bug where the turning speed is influenced by the frame rate. "
             "Enable this to make the turn speed independent of the frame rate."));

  AddPatch(new wxBoolProperty(_("Use Joystick"), wxT("UseJoystick"), false),
           _("Enables joystick functionality."));

  AddPatch(new wxIntProperty(_("Mouse Deadzone"), wxT("MouseDeadzone"), 40),
           _("Sets the radius from the center of the screen where the mouse will do nothing "
             "(40 = default)."));

  AddPatch(new wxFloatProperty(_("Movement: Min Acceleration"), wxT("MovementMinAcceleration"), 4.0f),
           _("Set the movement acceleration speed (i.e. how long it takes to go from not moving to "
             "top speed)"));

  AddPatch(new wxFloatProperty(_("Movement: Max Acceleration"), wxT("MovementMaxAcceleration"), 15.0f),
           _("Set the movement acceleration speed (i.e. how long it takes to go from not moving to "
             "top speed)"));

  AddPatch(new wxFloatProperty(_("Movement: Max Speed"), wxT("MovementMaxSpeed"), 40.0f),
           _("Set the movement maximum speed."));

  AddPatch(new wxFloatProperty(_("Movement: Deceleration"), wxT("MovementDeceleration"), 50.0f),
           _("Set the movement deceleration speed (i.e. how long it takes to slow to a stop after "
             "releasing the controls). "
             "Increase this value to stop faster, decrease it to stop slower. "
             "Usually this is set to a very high value making deceleration almost instant."));

  AddPatch(new wxFloatProperty(_("Turning: Min Acceleration"), wxT("TurnMinAcceleration"), 15.0f),
           _("Set the speed at which turning accelerates (requires 'Turning: Enable Velocity')"));

  AddPatch(new wxFloatProperty(_("Turning: Max Acceleration"), wxT("TurnMaxAcceleration"), 30.0f),
           _("Set the speed at which turning accelerates (requires 'Turning: Enable Velocity')"));

  AddPatch(new wxFloatProperty(_("Turning: Max Speed"), wxT("TurnMaxSpeed"), 20.0f),
           _("Set the maximum turning speed."));

  AddPatch(new wxFloatProperty(_("Turning: Deceleration"), wxT("TurnDeceleration"), 50.0f),
           _("Set the speed at which turning decelerates (requires 'Turning: Enable Velocity')"));

  AddPatch(new wxBoolProperty(_("Turning: Enable Velocity"), wxT("TurnUseVelocity"), false),
           _("By default, LEGO Island ignores the turning acceleration/deceleration values. "
             "Set this to TRUE to utilize them"));

  patches_grid_->Append(new wxPropertyCategory(_("Gameplay")));

  AddPatch(new wxBoolProperty(_("Disable Auto-Finish Building Section"), wxT("DisableAutoFinishBuilding"), false),
           _("In LEGO Island v1.1, placing the last block when building will automatically end the "
             "building section. While convenient, this prevents players from making any further "
             "changes after placing the last brick. It also notably defies what Bill Ding says - "
             "you don't hit the triangle when you're finished building.\n\nThis patch restores the "
             "functionality in v1.0 where placing the last block will not automatically finish the "
             "build section."));

  AddPatch(new wxBoolProperty(_("Play Music"), wxT("MusicToggle"), true),
           _("Turns in-game music on or off."));

  patches_grid_->Append(new wxPropertyCategory(_("Graphics")));

  GetDirect3DDevices();

  AddPatch(new wxEnumProperty(_("Direct3D Device"), wxT("D3DDevice"), d3d_device_names_),
           _("Sets the Direct3D device to use for rendering."));

  AddPatch(new wxBoolProperty(_("Draw Cursor"), wxT("DrawCursor"), false),
           _("Renders an in-game cursor, rather than a standard Windows pointer."));

  AddPatch(new wxFloatProperty(_("Field of View Adjustment"), wxT("FOVMultiplier"), 0.1f),
           _("Globally adjusts the field of view by a multiplier. "
             "Smaller than 0.1 is more zoomed in, larger than 0.1 is more zoomed out."));

  const wxChar* fps_cap_list[] = {_("Default"), _("Uncapped"), _("Limited"), NULL};
  AddPatch(new wxEnumProperty(_("FPS Cap"), wxT("FPSLimit"), fps_cap_list, NULL, 0),
           _("Modify LEGO Island's frame rate cap"));

  AddPatch(new wxFloatProperty(_("FPS Cap - Custom Limit"), wxT("CustomFPS"), 24.0f),
           _("Is 'FPS Cap' is set to 'Limited', this will be the frame rate used."));

  const wxChar* model_quality_list[] = {_("High"), _("Medium"), _("Low"), NULL};
  AddPatch(new wxEnumProperty(_("Model Quality"), wxT("ModelQuality"), model_quality_list, NULL, 1),
           _("Change LEGO Island's default model quality"));

  AddPatch(new wxBoolProperty(_("Run in Full Screen"), wxT("FullScreen"), true),
           _("Allows you to change modes without administrator privileges and registry editing."));

  patches_grid_->Append(new wxPropertyCategory(_("System")));

  AddPatch(new wxBoolProperty(_("Allow Multiple Instances"), wxT("DrawCursor"), false),
           _("Renders an in-game cursor, rather than a standard Windows pointer."));

  AddPatch(new wxBoolProperty(_("Redirect Save Files to %APPDATA%"), wxT("RedirectSaveData"), true),
           _("By default LEGO Island saves its game data in its Program Files folder. In newer "
             "versions of Windows, this folder is considered privileged access, necessitating "
             "running LEGO Island as administrator to save here. This patch sets LEGO Island's "
             "save location to %APPDATA% instead, which is an accessible and "
             "standard location that most modern games and apps save to."));

  AddPatch(new wxBoolProperty(_("Stay Active When Defocused"), wxT("StayActiveWhenDefocused"), false),
           _("By default, LEGO Island pauses when it's not the active window. "
             "This patch prevents that behavior."));

  patches_grid_->Append(new wxPropertyCategory(_("Experimental (Use at your own risk)")));

  AddPatch(new wxBoolProperty(_("Override Resolution"), wxT("OverrideResolution"), false),
           _("Override LEGO Island's hardcoded 640x480 resolution with a custom resolution. "
             "NOTE: This patch is currently incomplete and buggy."));

  AddPatch(new wxBoolProperty(_("Override Resolution - Bitmap Upscale"), wxT("UpscaleBitmaps"), false),
           _("WARNING: This doesn't upscale the bitmaps' hitboxes yet and can make 2D areas like "
             "the Information Center difficult to navigate."));

  AddPatch(new wxIntProperty(_("Override Resolution - Width"), wxT("ResolutionWidth"), 640),
           _("If 'Override Resolution' is enabled, this is the screen resolution width to use "
             "instead."));

  AddPatch(new wxIntProperty(_("Override Resolution - Height"), wxT("ResolutionHeight"), 480),
           _("If 'Override Resolution' is enabled, this is the screen resolution height to use "
             "instead."));
}

void MainWindow::AddPatch(wxPGProperty *property, const wxString &description)
{
  description_map_[property] = description;

  property->SetDefaultValue(property->GetValue());

  patches_grid_->Append(property);
}

void MainWindow::OnExit(wxCommandEvent &event)
{
  Close(true);
}

void MainWindow::OnRun(wxCommandEvent &event)
{
  // Create patch map
  Patcher::PatchMap patch_map;
  for (std::map<wxPGProperty*, wxString>::iterator it=description_map_.begin(); it!=description_map_.end(); ++it) {
    patch_map[it->first->GetName()] = it->first->GetValue();
  }

  // Manually inject D3D properties
  int d3d_device_index = patch_map[wxT("D3DDevice")].GetLong();
  patch_map[wxT("D3DDevice")] = d3d_device_entries_[d3d_device_index];

  // Create variables filled in by Patch()
  std::vector<wxString> incompatibilities;
  wxString exe_path;

  Patcher::Status result = Patcher::Patch(patch_map, incompatibilities, exe_path);

  if (result == Patcher::kSuccess) {
    // Run game
    wxProcess::Open(exe_path);
  }
}

void MainWindow::OnPatchGridSelected(wxPropertyGridEvent &event)
{
  wxPGProperty* prop = event.GetProperty();

  if (prop) {
    patches_title_lbl_->SetLabel(prop->GetLabel());
    patches_description_lbl_->SetLabel(description_map_[prop]);
    //patches_description_lbl_->SetLabel(wxString::Format(_("Default: %s"), prop->GetDefaultValue().MakeString()));
  }
}

void MainWindow::OnPatchGridChanged(wxPropertyGridEvent &event)
{
  wxPGProperty* prop = event.GetProperty();

  if (prop) {
    prop->SetWasModified((prop->GetValue() != prop->GetDefaultValue()));
  }
}

HRESULT CALLBACK EnumD3DDevices(GUID *lpGuid,
                                LPSTR lpDeviceDescription,
                                LPSTR lpDeviceName,
                                LPD3DDEVICEDESC,
                                LPD3DDEVICEDESC,
                                LPVOID context)
{
  std::pair<wxString, GUID> p;

  p.first = lpDeviceName;
  p.second = *lpGuid;

  static_cast<std::vector< std::pair<wxString, GUID> >*>(context)->push_back(p);

  return DDENUMRET_OK;
}

int device_count = 0;

BOOL CALLBACK EnumDisplayDrivers(GUID FAR* pGuid,
                                 LPSTR desc,
                                 LPSTR name,
                                 LPVOID context)
{
  LPDIRECTDRAW ddraw_device;

  DirectDrawCreate(pGuid, &ddraw_device, NULL);

  LPDIRECT3D2 d3d_device;

  ddraw_device->QueryInterface(IID_IDirect3D2, ( void** ) &d3d_device);

  std::vector< std::pair<wxString, GUID> > devices;

  d3d_device->EnumDevices(EnumD3DDevices, &devices);

  for (size_t i=0; i<devices.size(); i++) {
    const std::pair<wxString, GUID>& device = devices.at(i);

    wxString device_name = wxString::Format(wxT("%s (%s)"), device.first, desc);

    const GUID* lpGuid = &device.second;

    wxString device_id = wxString::Format(wxT("%i 0x%08x 0x%04x%04x 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x"),
                                          device_count, lpGuid->Data1, lpGuid->Data3, lpGuid->Data2,
                                          lpGuid->Data4[3], lpGuid->Data4[2],
        lpGuid->Data4[1], lpGuid->Data4[0],
        lpGuid->Data4[7], lpGuid->Data4[6],
        lpGuid->Data4[5], lpGuid->Data4[4]);

    static_cast<MainWindow*>(context)->AddDirect3DDevice(device_name, device_id);
  }

  device_count++;

  d3d_device->Release();

  ddraw_device->Release();

  return TRUE;
}

void MainWindow::GetDirect3DDevices()
{
  DirectDrawEnumerateA(EnumDisplayDrivers, this);
}

void MainWindow::MakeTextBold(wxWindow *window)
{
  wxFont f = window->GetFont();
  f.SetWeight(wxFONTWEIGHT_BOLD);
  window->SetFont(f);
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
EVT_MENU(wxID_EXIT, MainWindow::OnExit)
EVT_BUTTON(ID_RUN, MainWindow::OnRun)
EVT_PG_SELECTED(ID_PATCHGRID, MainWindow::OnPatchGridSelected)
EVT_PG_CHANGED(ID_PATCHGRID, MainWindow::OnPatchGridChanged)
END_EVENT_TABLE()
