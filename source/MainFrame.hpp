#pragma once

#include "interface_derived.h"
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <cstdint>

struct FileInfo {
	std::filesystem::path path;
	bool complete = false;
};

class MainFrame : public MainFrameBase {
public:
	MainFrame();
	void OnExit(wxCommandEvent&);

	void OnAddImages(wxCommandEvent&);
	void OnClear(wxCommandEvent&);
	void OnRemoveImages(wxCommandEvent&);
	void OnCompressAll(wxCommandEvent&);

	DECLARE_EVENT_TABLE()
private:
	uint64_t currentID = 0;
	std::unordered_map<decltype(currentID),FileInfo> currentFiles;
	bool isRunning = false;


	void DoFile(decltype(currentID) id);
	bool DoPNG(const FileInfo&, std::vector<uint8_t>&);

	std::vector<uint8_t> LoadPNG(const std::filesystem::path& path);
};

