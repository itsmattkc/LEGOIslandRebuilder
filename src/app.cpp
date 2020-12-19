#include "app.h"

#include "mainwindow.h"

IMPLEMENT_APP(RebuilderApp)

bool RebuilderApp::OnInit()
{
  MainWindow* m = new MainWindow();
  m->Show();
  SetTopWindow(m);
  return true;
}
