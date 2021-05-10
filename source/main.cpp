#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

//#include "interface_derived.h"

class BatchCompress : public wxApp
{
public:
    virtual bool OnInit() {
       /* frame = new MainFrame();
        frame->Show(true);*/
        return true;
    }
    virtual int FilterEvent(wxEvent&);
    //MainFrame* frame;
};
wxIMPLEMENT_APP(BatchCompress);
