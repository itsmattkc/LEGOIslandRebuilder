#ifndef APP_H
#define APP_H

#include "wxutils.h"

class RebuilderApp : public wxApp
{
public:
  virtual bool OnInit();
};

DECLARE_APP(RebuilderApp)

#endif // APP_H
