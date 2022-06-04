/*
SSE NIF Optimizer
See the included LICENSE file
*/

#pragma once

#include <wx/cmdline.h>
#include <wx/dir.h>
#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

enum TargetGame { SSE, LE };

struct OptimizerOptions {
	wxArrayString files;
	wxString folder;
	bool recursive = true;
	bool smoothNormals = false;
	int smoothAngle = 60;
	bool smoothSeamNormals = true;
	bool headParts = false;
	bool cleanSkinning = true;
	bool calculateBounds = true;
	bool removeParallax = true;
	TargetGame targetGame = TargetGame::SSE;
	wxString logFilePath;
};

struct ScanOptions {
	wxString folder;
	bool recursive = true;
	TargetGame targetGame = TargetGame::SSE;
	bool checkMipmaps = true;
	bool writeLog = true;
};

class Optimizer;

class OptimizerApp : public wxApp {
public:
	virtual bool OnInit();
	virtual int OnExit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	void HandleCmdLine();
	void Optimize(const OptimizerOptions& options);
	void ScanTextures(const ScanOptions& options);

	void Log(wxFile& file, const wxString& msg = "") {
		if (file.IsOpened()) {
			file.Write(msg);
			file.Write("\r\n");
			file.Flush();
		}
	}

private:
	Optimizer* frame = nullptr;

	wxString cmdOptimize;
	wxString cmdLogPath;
	wxArrayString cmdPaths;
	bool cmdRecursive = false;
	bool cmdHeadparts = false;
};

static const wxCmdLineEntryDesc cmdLineDesc[]
	= {{wxCMD_LINE_OPTION, "opt", "optimize", "Optimize for given target", wxCMD_LINE_VAL_STRING},
	   {wxCMD_LINE_OPTION, "log", "log", "Path to log file", wxCMD_LINE_VAL_STRING},
	   {wxCMD_LINE_SWITCH, "recursive", "recursive", "Recursively parse all directories"},
	   {wxCMD_LINE_SWITCH, "headparts", "headparts", "Optimize files as headparts"},
	   {wxCMD_LINE_PARAM,
		"p",
		"path",
		"Paths to files and/or directories",
		wxCMD_LINE_VAL_STRING,
		wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE},
	   wxCMD_LINE_DESC_END};


class Optimizer : public wxFrame {
private:
	std::vector<std::pair<int, int>> progressStack;
	int progressVal = 0;

	wxStatusBar* statusBar = nullptr;
	wxGauge* progressBar = nullptr;

	wxDirPickerCtrl* dirCtrl = nullptr;
	wxCheckBox* cbRecursive = nullptr;
	wxCheckBox* cbSmoothNormals = nullptr;
	wxStaticText* lbSmoothAngle = nullptr;
	wxSpinCtrl* numSmoothAngle = nullptr;
	wxCheckBox* cbSmoothSeamNormals = nullptr;
	wxCheckBox* cbHeadParts = nullptr;
	wxCheckBox* cbCleanSkinning = nullptr;
	wxCheckBox* cbCalculateBounds = nullptr;
	wxCheckBox* cbRemoveParallax = nullptr;
	wxCheckBox* cbMipmapsCheck = nullptr;
	wxCheckBox* cbWriteLog = nullptr;
	wxRadioButton* rbSSE = nullptr;
	wxRadioButton* rbLE = nullptr;

	void onClose(wxCloseEvent& event);
	void dirCtrlChanged(wxFileDirPickerEvent& event);
	void cbSmoothNormalsChecked(wxCommandEvent& event);
	void btOptimizeClicked(wxCommandEvent& event);
	void btScanTexturesClicked(wxCommandEvent& event);

public:
	bool isProcessing = false;
	wxButton* btOptimize = nullptr;
	wxButton* btScanTextures = nullptr;

	Optimizer(wxWindow* parent,
			  wxWindowID id = wxID_ANY,
			  const wxString& title = "SSE NIF Optimizer v3.1.0",
			  const wxPoint& pos = wxDefaultPosition,
			  const wxSize& size = wxSize(475, 340),
			  long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);
	~Optimizer();

	void StartOptimize();
	void EndOptimize();

	void StartProgress(const wxString& msg = "");
	void EndProgress(const wxString& msg = "");
	void StartSubProgress(int min, int max);
	void UpdateProgress(int val, const wxString& msg = "");
};
