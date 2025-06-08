#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#if __linux__
#include "wxlin.xpm"
#endif

#include "MainFrame.hpp"

class BatchCompress : public wxApp
{
public:
    virtual bool OnInit() override{
#if _WIN32
        MSWEnableDarkMode();
        SetAppearance(Appearance::System);
#endif
        frame = new MainFrame();
#if __linux__
        frame->SetIcon(wxIcon(wxICON(wxlin)));
#endif

        frame->Show(true);
        return true;
    }
    MainFrame* frame;
};
wxIMPLEMENT_APP(BatchCompress);
