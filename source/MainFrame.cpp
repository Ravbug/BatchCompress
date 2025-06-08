#include "MainFrame.hpp"
#include <wx/filedlg.h>
#include <zopflipng_lib.h>
#include <fstream>
#include <array>
#include <format>
#include <fmt/format.h>
#include <wx/dirdlg.h>

#ifdef __APPLE__
#include "objc_bridge.h"
#endif

#ifdef _WIN32
#include <shobjidl_core.h>
#include <ShlObj_core.h>
#define COM_SAFE_RELEASE(x) if (x != NULL) x->Release(); x = NULL
#endif

#define DISPATCH_EVT 5000

constexpr static char* const version = "1.2";

wxDEFINE_EVENT(dispatchEvt, wxCommandEvent);

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(wxID_EXIT, MainFrame::OnExit)
EVT_MENU(wxID_OPEN, MainFrame::OnAddImages)
EVT_MENU(wxID_CLEAR, MainFrame::OnClear)
EVT_MENU(wxID_DELETE, MainFrame::OnRemoveImages)
EVT_MENU(ID_ADD_FROM_FOLDER, MainFrame::OnAddImagesFromFolder)
EVT_MENU(wxID_EXECUTE, MainFrame::OnCompressAll)
EVT_MENU(wxID_STOP, MainFrame::OnPause)
EVT_COMMAND(DISPATCH_EVT, dispatchEvt, MainFrame::OnDispatchUIUpdateMainThread)
EVT_DATAVIEW_ITEM_ACTIVATED(DATAVIEWLIST, MainFrame::OnSelectionActivated)
wxEND_EVENT_TABLE()

using namespace std;

static string sizeToString(const std::uintmax_t fileSize) {
	string formatted = "";
	int size = 1000;		//MB = 1000, MiB = 1024
	array<string, 5> suffix{ " bytes", " KB", " MB", " GB", " TB" };
	auto max = suffix.size();

	for (int i = 0; i < max; i++)
	{
		double compare = pow(size, i);
		if (fileSize <= compare || i == max - 1)
		{
			int minus = 0;
			if (i > 0)
			{
				minus = 1;
			}

			if (fileSize > compare) {
				minus = 0;
			}

			//round to 2 decimal places (except for bytes), then attach unit
			char buffer[10];
			const string format = (i - minus == 0) ? "%.0f" : "%.2f";
			sprintf(buffer, format.c_str(), fileSize / pow(size, i - minus));
			formatted = string(buffer) + suffix[i - minus];
			break;
		}
	}

	if (formatted == "") {
		return "??";
	}
	return formatted;
}


void MainFrame::OnExit(wxCommandEvent&)
{
	Close(true);
}

MainFrame::MainFrame() : MainFrameBase(nullptr), threadpool(std::thread::hardware_concurrency())
{
#ifdef _WIN32
	//name is the same as the one used in the resource file definition
	//the resource file definition must have the same ending as the name of the icon file itself
	//in this case, the icon file is wxwin.ico, so the definition is IDI_WXWIN
	SetIcon(wxIcon("IDI_WXWIN"));
#endif
	SetLabel(std::string("BatchCompress ") + version);

	DragAcceptFiles(true);	// enable drag and drop

	Bind(wxEVT_DROP_FILES, [this](wxDropFilesEvent& evt) {
			OnDropFiles(evt); 
		}
	);
}

void MainFrame::OnAddImages(wxCommandEvent&)
{
	wxFileDialog openFileDialog(this, _("Select images"), "", "",	"Image files (*.png)|*.png", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

	wxArrayString filenames;
	openFileDialog.GetFilenames(filenames);
	std::filesystem::path rootDir(openFileDialog.GetDirectory().ToStdString());
	for (auto& file : filenames) {
		file = (rootDir / std::filesystem::path(file.ToStdString())).string();
	}

	AddImagesImpl(filenames);

}

void MainFrame::AddImagesImpl(const wxArrayString& filenames)
{
	for (const auto& file : filenames) {
		wxVector<wxVariant> items(dataViewList->GetColumnCount());
		items[0] = file;
		items[5] = StatusToStr(Status::NotStarted);
		FileInfo entry{ std::filesystem::path(file.ToStdString()) };
		currentFiles.emplace(std::make_pair(currentID, entry));
		dataViewList->AppendItem(items, currentID);
		currentFiles.at(currentID).dataViewItem = dataViewList->RowToItem(dataViewList->GetItemCount() - 1);
		currentID++;
	}
	dataViewList->GetColumn(0)->SetWidth(wxCOL_WIDTH_AUTOSIZE);
}

void MainFrame::OnAddImagesFromFolder(wxCommandEvent&)
{
	wxDirDialog openFolderDialog{ this, "Choose root directory" };
	if (openFolderDialog.ShowModal() == wxID_CANCEL) {
		return;
	}
	const auto chosen_dir = std::filesystem::path{ openFolderDialog.GetPath().ToStdString() };

	wxArrayString filenames;

	for (const auto& file : std::filesystem::recursive_directory_iterator(chosen_dir)) {
		auto ext = file.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](const char c) {return std::tolower(c); });
		if (ext == ".png") {
			filenames.Add(file.path().string());
		}
	}
	AddImagesImpl(filenames);
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
	isPaused = false;
	// compress all

	for (auto& entry : currentFiles) {
		if (entry.second.status == Status::NotStarted) {
			auto id = entry.first;
			entry.second.status = Status::Queued;
			threadpool.enqueue([this, id]() { DoFile(id); });
		}
	}
}

void MainFrame::OnDispatchUIUpdateMainThread(wxCommandEvent& evt)
{
	//update the UI
	auto& fileid = currentFiles.at(evt.GetInt());
	auto row = dataViewList->ItemToRow(fileid.dataViewItem);
	dataViewList->SetTextValue(StatusToStr(fileid.status),row,5);

	switch (fileid.status) {
		case Status::NotStarted:
			break;
		case Status::Queued:
			break;
		case Status::InProgress:
			fileid.orig_size = std::filesystem::file_size(fileid.path);
			dataViewList->SetTextValue(sizeToString(fileid.orig_size), row, 2);
			break;
		case Status::Success:
			{
				auto newsize = std::filesystem::file_size(fileid.path);
				dataViewList->SetTextValue(sizeToString(newsize), row, 1);
				dataViewList->SetTextValue(fmt::format("{}%",static_cast<int>(100 - (static_cast<double>(newsize) / fileid.orig_size) * 100)),row,3);
				dataViewList->SetTextValue("Zopfli",row,4);	//TODO: when more compressors are added, set this properly
			}
			break;
		case Status::Failed:
			
		break;
		}
}

void MainFrame::OnPause(wxCommandEvent&)
{
	isPaused = true;
	threadpool.clear();
}

void MainFrame::OnSelectionActivated(wxDataViewEvent& e)
{
	auto idx = dataViewList->GetSelectedRow();
	auto& file = currentFiles[idx];

	//reveal the file in the platform's explorer

#ifdef _WIN32

	PIDLIST_ABSOLUTE pidl;
	SFGAOF attributes;
	HRESULT hr = SHParseDisplayName(file.path.c_str(), nullptr, &pidl, 0, &attributes);
	if (SUCCEEDED(hr)) {
		if (pidl) {
			LPITEMIDLIST pidlItem = ILFindLastID(pidl);
			HRESULT hr2 = SHOpenFolderAndSelectItems(pidl, 1, const_cast<LPCITEMIDLIST*>(&pidlItem), 0);
			ILFree(pidl);
			if (FAILED(hr2)) {

			}
		}
	}
#elif defined __APPLE__
    RevealFile(file.path);
#elif defined __linux__
	std::string str;
	if (std::filesystem::is_directory(file.path)) {
		str = file.path.string();
	}
	else {
		str = file.path.parent_path().string().ToStdString();
	}
	wxExecute(wxT("xdg-open \"" + str + "\""), wxEXEC_ASYNC);
#else
#error This platform's open-in-explorer is not supported. Implement its API here.'
#endif
}

void MainFrame::OnDropFiles(wxDropFilesEvent& evt)
{
	auto files = evt.GetFiles();
	auto nfiles = evt.GetNumberOfFiles();
	AddImagesImpl({ size_t(nfiles), files });
}

void MainFrame::DoFile(decltype(currentID) id)
{
	wxCommandEvent event(dispatchEvt);
	event.SetId(DISPATCH_EVT);
	event.SetInt(id);

	if (isPaused) {
		currentFiles.at(id).status = Status::NotStarted;
		wxPostEvent(this, event);
		return;
	}
	//get file object at ID
	auto& file = currentFiles.at(id);
	file.status = Status::InProgress;

	//dispatch to notify that processing has begun
	wxPostEvent(this, event);

	vector<uint8_t> result;
	try {
		
		if (file.path.extension() == ".png") {
			DoPNG(file, result);
		}
	}
	catch (runtime_error& e) {
		file.status = Status::Failed;
        wxPostEvent(this, event);
        return;
	}


	// if id is still in the dictionary
	if (currentFiles.find(id) != currentFiles.end()) {
		// move old file to trash
		if (MoveToRecycleBin(file.path)) {
			// write new compressed file 
			file.status = Status::Success;
			std::ofstream out(file.path, ios::out | ios::binary);
            out.write((char*)result.data(), result.size() * sizeof(result[0]));
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
#ifndef NDEBUG
	zop_opt.verbose = true;
#else
    zop_opt.verbose = false;
#endif
	zop_opt.keepchunks = { "acTL", "fcTL", "fdAT", "npTc" };

	// open the file:
	const vector<uint8_t> origpng = LoadPNG(file.path);

	vector<uint8_t> resultpng;

	if (!ZopfliPNGOptimize(origpng, zop_opt, zop_opt.verbose, &resultpng)) {
		if (resultpng.size() < origpng.size()) {
			result = resultpng;
		}
        else{
            //compression was not better than original, so use the original png
            result = origpng;
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
	// Initialize COM

	// Create COM instance of IFileOperation
	IFileOperation* pfo = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfo));
	if (SUCCEEDED(hr)) {
		assert(pfo != nullptr);

		// Set parameters for current operation
		hr = pfo->SetOperationFlags(
			FOF_SILENT |    // Don't display a progress dialog
			FOF_NOERRORUI  |  // Don't display any error messages to the user
			FOFX_RECYCLEONDELETE // send to the recycle bin
		);

		if (SUCCEEDED(hr)) {
			// Create IShellItem instance associated to file to delete
			IShellItem* psiFileToDelete = nullptr;
			hr = SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&psiFileToDelete));
			if (SUCCEEDED(hr)) {
				assert(psiFileToDelete != nullptr);

				// Declare this shell item (file) to be deleted
				hr = pfo->DeleteItem(psiFileToDelete, NULL);
			}

			// Cleanup file-to-delete shell item
			COM_SAFE_RELEASE(psiFileToDelete);

			if (SUCCEEDED(hr)) {
				// Perform the deleting operation
				hr = pfo->PerformOperations();
			}
		}
	}

	// Cleanup file operation object
	COM_SAFE_RELEASE(pfo);

	if (!SUCCEEDED(hr)) {
		return false;
	}

	return true;

#elif defined __APPLE__
	return trashItem(path);
#elif defined __linux__
	// there is no trash API on linux. delete the file instead. 
	std::filesystem::remove(path);
#else
#error This platform's trash is not supported -- implement this platform's trash API here
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
