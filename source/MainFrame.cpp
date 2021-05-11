#include "MainFrame.hpp"
#include <wx/filedlg.h>
#include <zopflipng_lib.h>
#include <fstream>

#define DISPATCH_EVT 5000

wxDEFINE_EVENT(dispatchEvt, wxCommandEvent);

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(wxID_EXIT, MainFrame::OnExit)
EVT_MENU(wxID_OPEN, MainFrame::OnAddImages)
EVT_MENU(wxID_CLEAR, MainFrame::OnClear)
EVT_MENU(wxID_DELETE, MainFrame::OnRemoveImages)
EVT_MENU(wxID_EXECUTE, MainFrame::OnCompressAll)
EVT_COMMAND(DISPATCH_EVT, dispatchEvt, MainFrame::OnDispatchUIUpdateMainThread)
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
		items[5] = StatusToStr(Status::NotStarted);
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
	alltasks.clear();
	isRunning = true;
	// compress all
	for (auto& entry : currentFiles) {
		if (entry.second.status == Status::NotStarted || entry.second.status == Status::Failed) {
			auto id = entry.first;
			entry.second.status = Status::Queued;
			alltasks.emplace([this, id]() { DoFile(id); });
		}
	}
	executor.run(alltasks);	//don't block here
}

void MainFrame::OnDispatchUIUpdateMainThread(wxCommandEvent& evt)
{
	//update the UI
	const auto& fileid = currentFiles.at(evt.GetInt());

}

void MainFrame::DoFile(decltype(currentID) id)
{
	if (!isRunning) {
		return;
	}
	//get file object at ID
	auto& file = currentFiles.at(id);
	file.status = Status::InProgress;

	//dispatch to notify that processing has begun
	wxCommandEvent event(dispatchEvt);
	event.SetId(DISPATCH_EVT);
	event.SetInt(id);
	wxPostEvent(this, event);

	vector<uint8_t> result;
	try {
		
		if (file.path.extension() == ".png") {
			DoPNG(file, result);
		}
	}
	catch (runtime_error& e) {
		file.status = Status::Failed;
	}

	if (!isRunning) {
		wxPostEvent(this, event);
		return;
	}

	// if id is still in the dictionary
	if (currentFiles.find(id) != currentFiles.end()) {
		// move old file to trash
		if (MoveToRecycleBin(file.path)) {
			// write new compressed file 
			file.status = Status::Success;
			std::ofstream out(file.path);
			std::copy(result.rbegin(), result.rend(), std::ostream_iterator<int>(out));
		}
		else {
			file.status = Status::Failed;
		}
		//dispatch to main thread for update
		//notify that processing has completed
		wxPostEvent(this, event);
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

bool MainFrame::MoveToRecycleBin(const std::filesystem::path& path)
{
#ifdef _WIN32
	SHFILEOPSTRUCT shfos;
	memset(&shfos, 0, sizeof(shfos));
	
	auto wpath = path.wstring() + "\0\0";

	shfos.wFunc = FO_DELETE;
	shfos.pFrom = wpath.wchar_str();
	shfos.fFlags = FOF_ALLOWUNDO | FOF_SILENT | FOF_NOCONFIRMATION;

	auto result = SHFileOperation(&shfos);
	return !result;

#elif defined __APPLE__
	
#else
#error This platform is not supported
#endif

}

const char* MainFrame::StatusToStr(Status s)
{
	switch (s) {
	case Status::Failed:
		return "Failed";
	case Status::InProgress:
		return "In Progress";
	case Status::NotStarted:
		return "Not Started";
	case Status::Queued:
		return "Queued";
	case Status::Success:
		return "Complete";
	}
}
