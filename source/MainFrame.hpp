#pragma once

#include "interface_derived.h"
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <cstdint>
#include "ThreadPool.h"

enum class Status : uint8_t {
	NotStarted,
	Queued,
	InProgress,
	Success,
	Failed
};

struct FileInfo {
	std::filesystem::path path;
	Status status = Status::NotStarted;
	wxDataViewItem dataViewItem;
	std::uintmax_t orig_size;
};

class MainFrame : public MainFrameBase {
public:
	MainFrame();
	void OnExit(wxCommandEvent&);

	void OnAddImages(wxCommandEvent&);
	void OnAddImagesFromFolder(wxCommandEvent&);
	void OnClear(wxCommandEvent&);
	void OnRemoveImages(wxCommandEvent&);
	void OnCompressAll(wxCommandEvent&);
	void OnDispatchUIUpdateMainThread(wxCommandEvent&);
	void OnPause(wxCommandEvent&);
	void OnSelectionActivated(wxDataViewEvent&);

	void OnDropFiles(wxDropFilesEvent&);

	DECLARE_EVENT_TABLE()
private:
	uint64_t currentID = 0;
	std::unordered_map<decltype(currentID),FileInfo> currentFiles;
	bool isPaused = false;

	void AddImagesImpl(const wxArrayString& filenames);
	void DoFile(decltype(currentID));
	bool DoPNG(const FileInfo&, std::vector<uint8_t>&);

	bool MoveToRecycleBin(const std::filesystem::path&);

	const char* StatusToStr(Status);

	ThreadPool threadpool;

	std::vector<uint8_t> LoadPNG(const std::filesystem::path&);
};

