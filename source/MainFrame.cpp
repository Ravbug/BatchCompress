#include "MainFrame.hpp"
#include <wx/filedlg.h>
#include <zopflipng_lib.h>
#include <fstream>

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(wxID_EXIT, MainFrame::OnExit)
EVT_MENU(wxID_OPEN, MainFrame::OnAddImages)
EVT_MENU(wxID_CLEAR, MainFrame::OnClear)
EVT_MENU(wxID_DELETE, MainFrame::OnRemoveImages)
EVT_MENU(wxID_EXECUTE, MainFrame::OnCompressAll)
wxEND_EVENT_TABLE()

using namespace std;

void MainFrame::OnExit(wxCommandEvent&)
{
	Close(true);
}

MainFrame::MainFrame() : MainFrameBase(nullptr)
{
#ifdef _WIN32
	//name is the same as the one used in the resource file definition
	//the resource file definition must have the same ending as the name of the icon file itself
	//in this case, the icon file is wxwin.ico, so the definition is IDI_WXWIN
	SetIcon(wxIcon("IDI_WXWIN"));
#endif
}

void MainFrame::OnAddImages(wxCommandEvent&)
{
	wxFileDialog openFileDialog(this, _("Select images"), "", "",	"Image files (*.png)|*.png", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

	wxArrayString filenames;
	openFileDialog.GetFilenames(filenames);

	for (const auto& file : filenames) {
		wxVector<wxVariant> items(dataViewList->GetColumnCount());
		items[0] = file;
		currentFiles.emplace(std::make_pair(currentID, FileInfo{ std::filesystem::path(openFileDialog.GetDirectory().ToStdString()) / std::filesystem::path(file.ToStdString()) }));
		dataViewList->AppendItem(items,currentID);
		currentID++;
	}

	dataViewList->GetColumn(0)->SetWidth(wxCOL_WIDTH_AUTOSIZE);
}

void MainFrame::OnClear(wxCommandEvent&)
{
	dataViewList->DeleteAllItems();
	currentFiles.clear();
}

void MainFrame::OnRemoveImages(wxCommandEvent&)
{
	//get the selected rows
	wxDataViewItemArray items;
	dataViewList->GetSelections(items);

	//remove the row and remove the associated filedata from the hashmap by looking up ID in userdata
	for (auto& item : items) {
		auto data = dataViewList->GetItemData(item);
		currentFiles.erase(static_cast<decltype(currentID)>(data));
		dataViewList->DeleteItem(dataViewList->ItemToRow(item));
	}
}

void MainFrame::OnCompressAll(wxCommandEvent&)
{
	// compress all
	for (const auto& entry : currentFiles) {
		DoFile(entry.first);
	}
}

void MainFrame::DoFile(decltype(currentID) id)
{
	//get file object at ID
	auto& file = currentFiles.at(id);

	vector<uint8_t> result;
	if (file.path.extension() == ".png") {
		DoPNG(file, result);
	}

	file.complete = true;

	// if id is still in the dictionary
	if (currentFiles.find(id) != currentFiles.end()) {
		// move old file to trash

		// write new compressed file 
		std::ofstream out(file.path);
		//std::copy(result.rbegin(), result.rend(), std::ostream_iterator<int>(out));
	}
	
}

bool MainFrame::DoPNG(const FileInfo& file, vector<uint8_t>& result)
{

	// compress in different ways
	ZopfliPNGOptions zop_opt;
	zop_opt.use_zopfli = true;
	zop_opt.lossy_transparent = false;
	zop_opt.lossy_8bit = false;
	zop_opt.num_iterations = zop_opt.num_iterations_large = 15;
	zop_opt.verbose = true;
	zop_opt.keepchunks = { "acTL", "fcTL", "fdAT", "npTc" };

	// open the file:
	const vector<uint8_t> origpng = LoadPNG(file.path);

	vector<uint8_t> resultpng;

	if (!ZopfliPNGOptimize(origpng, zop_opt, true, &resultpng)) {
		if (resultpng.size() < origpng.size()) {
			result = resultpng;
		}
	}
	else {
		throw runtime_error("Zopfli failed");
		return false;
	}

	return true;
}
