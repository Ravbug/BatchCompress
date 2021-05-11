#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "MainFrame.hpp"

class BatchCompress : public wxApp
{
public:
    virtual bool OnInit() {
        
        frame = new MainFrame();
        frame->Show(true);
        return true;
    }
    MainFrame* frame;
};
wxIMPLEMENT_APP(BatchCompress);
