///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "interface_derived.h"

///////////////////////////////////////////////////////////////////////////

MainFrameBase::MainFrameBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* MainPane;
	MainPane = new wxBoxSizer( wxVERTICAL );

	dataViewList = new wxDataViewListCtrl( this, DATAVIEWLIST, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE|wxDV_ROW_LINES|wxDV_VERT_RULES );
	nameCol = dataViewList->AppendTextColumn( wxT("Name"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	sizeCol = dataViewList->AppendTextColumn( wxT("Size"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	origSizeCol = dataViewList->AppendTextColumn( wxT("Orig Size"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	savingsCol = dataViewList->AppendTextColumn( wxT("Savings"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	bestToolCol = dataViewList->AppendTextColumn( wxT("Best Tool"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	statusCol = dataViewList->AppendTextColumn( wxT("Status"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	MainPane->Add( dataViewList, 1, wxEXPAND, 5 );


	this->SetSizer( MainPane );
	this->Layout();
	menuBat = new wxMenuBar( 0 );
	fileMenu = new wxMenu();
	wxMenuItem* addImagesMenu;
	addImagesMenu = new wxMenuItem( fileMenu, wxID_OPEN, wxString( wxT("Add Images") ) + wxT('\t') + wxT("CTRL+O"), wxEmptyString, wxITEM_NORMAL );
	fileMenu->Append( addImagesMenu );

	wxMenuItem* clearMenu;
	clearMenu = new wxMenuItem( fileMenu, wxID_CLEAR, wxString( wxT("Clear Image List") ) , wxEmptyString, wxITEM_NORMAL );
	fileMenu->Append( clearMenu );

	wxMenuItem* delMenu;
	delMenu = new wxMenuItem( fileMenu, wxID_DELETE, wxString( wxT("Remove Selected Images") ) , wxEmptyString, wxITEM_NORMAL );
	fileMenu->Append( delMenu );

	wxMenuItem* compressMenu;
	compressMenu = new wxMenuItem( fileMenu, wxID_EXECUTE, wxString( wxT("Compress Listed Files") ) , wxEmptyString, wxITEM_NORMAL );
	fileMenu->Append( compressMenu );

	menuBat->Append( fileMenu, wxT("File") );

	windowMenu = new wxMenu();
	wxMenuItem* quitMenu;
	quitMenu = new wxMenuItem( windowMenu, wxID_EXIT, wxString( wxT("Quit") ) , wxEmptyString, wxITEM_NORMAL );
	windowMenu->Append( quitMenu );

	menuBat->Append( windowMenu, wxT("Window") );

	this->SetMenuBar( menuBat );

	statusBar = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

	this->Centre( wxBOTH );
}

MainFrameBase::~MainFrameBase()
{
}
