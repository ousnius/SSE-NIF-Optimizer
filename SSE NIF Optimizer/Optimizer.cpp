/*
SSE NIF Optimizer
See the included LICENSE file
*/

#include "Optimizer.h"
#include "PlatformUtil.h"
#include "lib/DDS.h"


IMPLEMENT_APP(OptimizerApp)
wxDECLARE_APP(OptimizerApp);

bool OptimizerApp::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	frame = new Optimizer(nullptr);
	frame->Show(true);
	SetTopWindow(frame);

	return true;
}

int OptimizerApp::OnExit()
{
	return 0;
}

void OptimizerApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.SetDesc(cmdLineDesc);
}

bool OptimizerApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	cmdFiles.Clear();

	for (size_t i = 0; i < parser.GetParamCount(); i++)
		cmdFiles.Add(parser.GetParam(i));

	return true;
}

void OptimizerApp::Optimize(const OptimizerOptions& options)
{
	frame->StartProgress();

	int folderFlags = wxDIR_FILES | wxDIR_HIDDEN;
	if (options.recursive)
		folderFlags |= wxDIR_DIRS;

	wxArrayString files;
	wxDir::GetAllFiles(options.folder, &files, "*.nif", folderFlags);
	wxDir::GetAllFiles(options.folder, &files, "*.btr", folderFlags);
	wxDir::GetAllFiles(options.folder, &files, "*.bto", folderFlags);

	wxFile logFile;
	if (options.writeLog)
		logFile.Open("SSE NIF Optimizer.txt", wxFile::OpenMode::write);

	Log(logFile, "==== SSE NIF Optimizer v3.0.12 by ousnius ====");
	Log(logFile, "----------------------------------------------------------------------");

	Log(logFile, "[INFO] Options:");
	Log(logFile, wxString::Format("- Folder: '%s'", options.folder));
	Log(logFile, wxString::Format("- Sub Directories: %s", options.recursive ? "Yes" : "No"));
	Log(logFile, wxString::Format("- Head Parts Only: %s", options.headParts ? "Yes" : "No"));
	Log(logFile, wxString::Format("- Clean Skinning: %s", options.cleanSkinning ? "Yes" : "No"));
	Log(logFile, wxString::Format("- Calculate Bounds: %s", options.calculateBounds ? "Yes" : "No"));
	Log(logFile, wxString::Format("- Remove Parallax: %s", options.removeParallax ? "Yes" : "No"));
	Log(logFile, wxString::Format("- Smooth Normals: %s", options.smoothNormals ? "Yes" : "No"));
	if (options.smoothNormals)
	{
		Log(logFile, wxString::Format("- Smooth Angle: %d", options.smoothAngle));
		Log(logFile, wxString::Format("- Smooth Seam Normals: %s", options.smoothSeamNormals ? "Yes" : "No"));
	}
	Log(logFile);

	size_t fileCount = files.GetCount();
	Log(logFile, wxString::Format("[INFO] %d file(s) were found.", fileCount));
	Log(logFile, "----------------------------------------------------------------------");

	float prog = 0.0f;
	float step = 100.0f;

	if (fileCount > 0)
		step /= fileCount;

	for (auto &file : files)
	{
		wxFileName fileName(file);
		wxString fileExt = fileName.GetExt().MakeLower();
		frame->UpdateProgress(prog += step, wxString::Format("'%s'...", fileName.GetFullName()));

		Log(logFile, wxString::Format("Loading '%s'...", file));

		std::fstream fsOpen;
		PlatformUtil::OpenFileStream(fsOpen, file.ToUTF8().data(), std::ios::in | std::ios::binary);

		NifLoadOptions loadOptions;
		loadOptions.isTerrain = (fileExt == "btr" || fileExt == "bto");

		NifFile nif;
		if (nif.Load(fsOpen, loadOptions) == 0)
		{
			OptOptions optOptions;
			optOptions.headParts = options.headParts;
			optOptions.calcBounds = options.calculateBounds;
			optOptions.removeParallax = options.removeParallax;

			NiVersion version;
			if (options.targetGame == TargetGame::SSE)
			{
				version.SetFile(NiFileVersion::V20_2_0_7);
				version.SetUser(12);
				version.SetStream(100);
			}
			else
			{
				version.SetFile(NiFileVersion::V20_2_0_7);
				version.SetUser(12);
				version.SetStream(83);
			}
			optOptions.targetVersion = version;

			OptResult result = nif.OptimizeFor(optOptions);
			if (result.versionMismatch)
			{
				Log(logFile, "[INFO] NIF version can't be saved with the target version (or already was). Skipping conversion.");
			}

			if (result.dupesRenamed)
			{
				Log(logFile, "[INFO] Renamed at least one shape with duplicate names.\r\n");
			}

			if (!result.shapesVColorsRemoved.empty())
			{
				wxString shapeList = "[INFO] Removed vertex colors from shapes:\r\n";
				for (auto &s : result.shapesVColorsRemoved)
					shapeList.Append(wxString::Format("- %s\r\n", s));

				Log(logFile, shapeList);
			}

			if (!result.shapesNormalsRemoved.empty())
			{
				wxString shapeList = "[INFO] Removed unnecessary normals and tangents from shapes:\r\n";
				for (auto &s : result.shapesNormalsRemoved)
					shapeList.Append(wxString::Format("- %s\r\n", s));

				Log(logFile, shapeList);
			}

			if (!result.shapesPartTriangulated.empty())
			{
				wxString shapeList = "[INFO] Triangulated skin partitions of shapes:\r\n";
				for (auto &s : result.shapesPartTriangulated)
					shapeList.Append(wxString::Format("- %s\r\n", s));

				Log(logFile, shapeList);
			}

			if (!result.shapesTangentsAdded.empty())
			{
				wxString shapeList = "[INFO] Added tangents to shapes:\r\n";
				for (auto &s : result.shapesTangentsAdded)
					shapeList.Append(wxString::Format("- %s\r\n", s));

				Log(logFile, shapeList);
			}

			if (!result.shapesParallaxRemoved.empty())
			{
				wxString shapeList = "[INFO] Removed parallax from shapes:\r\n";
				for (auto &s : result.shapesParallaxRemoved)
					shapeList.Append(wxString::Format("- %s\r\n", s));

				Log(logFile, shapeList);
			}

			if (options.cleanSkinning)
			{
				AnimSkeleton::getInstance().DisableCustomTransforms();

				AnimInfo anim;
				anim.LoadFromNif(&nif);

				if (!anim.shapeBones.empty())
				{
					Log(logFile, "[INFO] Skinned mesh: Cleaning up skin data and calculating bounds.");
				}

				anim.WriteToNif(&nif);
			}

			if (options.smoothNormals)
			{
				for (auto &s : nif.GetShapes())
				{
					nif.CalcNormalsForShape(s, options.smoothSeamNormals, options.smoothAngle);
					nif.CalcTangentsForShape(s);
				}
			}

			nif.GetHeader().SetExportInfo("Optimized with SSE NIF Optimizer v3.0.12.");
			nif.FinalizeData();

			NifSaveOptions saveOptions;
			saveOptions.optimize = false;
			saveOptions.sortBlocks = false;

			std::fstream fsSave;
			PlatformUtil::OpenFileStream(fsSave, file.ToUTF8().data(), std::ios::out | std::ios::binary);

			if (nif.Save(fsSave, saveOptions) == 0)
			{
				Log(logFile, "[SUCCESS] Saved file.");
			}
			else
			{
				Log(logFile, "[ERROR] Failed to save file.");
			}
		}
		else
		{
			Log(logFile, wxString::Format("[ERROR] Failed to load '%s'.", file));
		}

		Log(logFile, "----------------------------------------------------------------------");

		wxSafeYield(frame);

		if (!frame->isProcessing)
			break;
	}

	Log(logFile, "Program finished.");

	frame->EndProgress();
}

void OptimizerApp::ScanTextures(const ScanOptions& options)
{
	frame->StartProgress();

	int folderFlags = wxDIR_FILES | wxDIR_HIDDEN;
	if (options.recursive)
		folderFlags |= wxDIR_DIRS;

	wxArrayString files;
	wxDir::GetAllFiles(options.folder, &files, "*.dds", folderFlags);
	wxDir::GetAllFiles(options.folder, &files, "*.tga", folderFlags);

	wxFile logFile;
	if (options.writeLog)
		logFile.Open("SSE NIF Optimizer (Texture Scan).txt", wxFile::OpenMode::write);

	Log(logFile, "==== SSE NIF Optimizer v3.0.12 (Texture Scan) by ousnius ====");
	Log(logFile, "----------------------------------------------------------------------");

	Log(logFile, "[INFO] Options:");
	Log(logFile, wxString::Format("- Folder: '%s'", options.folder));
	Log(logFile, wxString::Format("- Sub Directories: %s", options.recursive ? "Yes" : "No"));
	Log(logFile);

	size_t fileCount = files.GetCount();
	Log(logFile, wxString::Format("[INFO] %d file(s) were found.", fileCount));
	Log(logFile, "----------------------------------------------------------------------");

	float prog = 0.0f;
	float step = 100.0f;

	if (fileCount > 0)
		step /= fileCount;

	wxArrayString logResult;

	for (auto &file : files)
	{
		wxFileName fileName(file);
		wxString fileExt = fileName.GetExt().MakeLower();
		frame->UpdateProgress(prog += step, wxString::Format("'%s'...", fileName.GetFullName()));

		wxArrayString fileLog;
		if (fileExt == "tga")
		{
			if (!file.Lower().Contains("facegendata"))
			{
				fileLog.Add("TGA texture files are not supported.");
			}
		}
		else
		{
			std::fstream fsOpen;
			PlatformUtil::OpenFileStream(fsOpen, file.ToUTF8().data(), std::ios::in | std::ios::binary);

			if (fsOpen)
			{
				char ddsMagic[4];
				DDS_HEADER dds;
				DDS_HEADER_DXT10 dds10;

				fsOpen.read(ddsMagic, sizeof(ddsMagic));
				if (std::strncmp(ddsMagic, "DDS ", sizeof(ddsMagic)) == 0)
				{
					fsOpen.read((char*)&dds, sizeof(DDS_HEADER));

					if (fsOpen)
					{
						if (dds.dwWidth % 4 != 0 || dds.dwHeight % 4 != 0)
						{
							fileLog.Add(wxString::Format("Dimensions must be divisible by 4 (currently %dx%d).", dds.dwWidth, dds.dwHeight));
						}

						if (dds.dwCaps2 & DDS_CUBEMAP && std::memcmp(&dds.ddspf, &DDSPF_R8G8B8, sizeof(DDS_PIXELFORMAT)) == 0)
						{
							fileLog.Add("Uncompressed cubemaps require an alpha channel. Use ARGB8 instead of RGB8 or compress them with DXT1/BC1.");
						}

						if (std::memcmp(&dds.ddspf, &DDSPF_L8, sizeof(DDS_PIXELFORMAT)) == 0)
						{
							fileLog.Add("Unsupported L8 format (one channel with luminance flag). Use R8 or BC4 instead.");
						}

						if (std::memcmp(&dds.ddspf, &DDSPF_L16, sizeof(DDS_PIXELFORMAT)) == 0)
						{
							fileLog.Add("Unsupported L16 format (one channel with luminance flag). Use R8 or BC4 instead.");
						}

						if (std::memcmp(&dds.ddspf, &DDSPF_A8L8, sizeof(DDS_PIXELFORMAT)) == 0)
						{
							fileLog.Add("Unsupported A8L8 format (two channels with luminance flag). Use BC7 instead.");
						}

						if (std::memcmp(&dds.ddspf, &DDSPF_DX10, sizeof(DDS_PIXELFORMAT)) == 0)
						{
							if (options.targetGame == TargetGame::LE)
							{
								fileLog.Add("DX10+ DDS formats are not supported.");
							}

							fsOpen.read((char*)&dds10, sizeof(DDS_HEADER_DXT10));

							if (fsOpen)
							{
								if (dds10.dxgiFormat == DXGI_FORMAT_BC1_UNORM_SRGB ||
									dds10.dxgiFormat == DXGI_FORMAT_BC2_UNORM_SRGB ||
									dds10.dxgiFormat == DXGI_FORMAT_BC3_UNORM_SRGB ||
									dds10.dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB ||
									dds10.dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
								{
									if (file.Lower().EndsWith("_n.dds"))
									{
										fileLog.Add("sRGB color space detected. Use linear color space for normal maps or lighting will be incorrect.");
									}
									else
									{
										fileLog.Add("sRGB color space detected. Use linear color space instead if this was unintentional.");
									}
								}

								if (dds10.dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM ||
									dds10.dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM ||
									dds10.dxgiFormat == DXGI_FORMAT_B4G4R4A4_UNORM)
								{
									fileLog.Add("This format will cause the game to crash on Windows 7.");
								}
							}
							else
							{
								fileLog.Add("File is flagged as DX10 but isn't a valid DX10 DDS header.");
							}
						}
						else
						{
							if (std::memcmp(&dds.ddspf, &DDSPF_R5G6B5, sizeof(DDS_PIXELFORMAT)) == 0 ||
								std::memcmp(&dds.ddspf, &DDSPF_A1R5G5B5, sizeof(DDS_PIXELFORMAT)) == 0 ||
								std::memcmp(&dds.ddspf, &DDSPF_A4R4G4B4, sizeof(DDS_PIXELFORMAT)) == 0)
							{
								fileLog.Add("This format will cause the game to crash on Windows 7.");
							}
						}
					}
					else
					{
						fileLog.Add("File header isn't a valid DDS header.");
					}
				}
			}
			else
			{
				Log(logFile, wxString::Format("[ERROR] Failed to load '%s'.", file));
			}
		}

		if (!fileLog.IsEmpty())
		{
			Log(logFile, file);
			logResult.Add(file);

			for (auto &fl : fileLog)
			{
				Log(logFile, "- " + fl);
				logResult.Add("- " + fl);
			}
		}

		wxSafeYield(frame);

		if (!frame->isProcessing)
			break;
	}

	Log(logFile, "Program finished.");

	frame->EndProgress();

	if (!logResult.IsEmpty())
	{
		wxSingleChoiceDialog resultDialog(frame, "", "Texture Scan Result", logResult, nullptr, wxDEFAULT_DIALOG_STYLE | wxOK | wxCENTRE | wxRESIZE_BORDER);
		resultDialog.ShowModal();
	}
	else
	{
		wxMessageBox("No errors were detected in the texture scan.", "Texture Scan");
	}
}


Optimizer::Optimizer(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	SetSizeHints(size);
	SetBackgroundColour(wxSystemSettings::GetColour(wxSystemColour::wxSYS_COLOUR_WINDOW));

	auto sizer = new wxBoxSizer(wxVERTICAL);
	auto sizerDir = new wxBoxSizer(wxHORIZONTAL);

	dirCtrl = new wxDirPickerCtrl(this, wxID_ANY, wxGetCwd(), "Select a folder...", wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE | wxDIRP_DIR_MUST_EXIST);
	sizerDir->Add(dirCtrl, 1, wxALL, 5);

	cbRecursive = new wxCheckBox(this, wxID_ANY, "Sub Directories");
	cbRecursive->SetValue(true);
	cbRecursive->SetToolTip("All files in the sub directories of the selected folder are processed as well.");
	sizerDir->Add(cbRecursive, 0, wxALL | wxEXPAND, 5);

	sizer->Add(sizerDir, 0, wxEXPAND, 5);

	auto sbOptions = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Options"), wxVERTICAL);

	auto sizerOptions = new wxFlexGridSizer(0, 2, 0, 0);
	sizerOptions->AddGrowableCol(0);
	sizerOptions->AddGrowableCol(1);
	sizerOptions->SetFlexibleDirection(wxBOTH);
	sizerOptions->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	cbHeadParts = new wxCheckBox(sbOptions->GetStaticBox(), wxID_ANY, "Head Parts Only");
	cbHeadParts->SetForegroundColour(wxColour(255, 0, 0));
	cbHeadParts->SetToolTip("Use this for ONLY head parts, such as head, ear, mouth and hair meshes.");
	sizerOptions->Add(cbHeadParts, 0, wxALL | wxEXPAND, 5);

	cbCleanSkinning = new wxCheckBox(sbOptions->GetStaticBox(), wxID_ANY, "Clean Skinning");
	cbCleanSkinning->SetToolTip("Resave the skin data to perform some cleanup.");
	cbCleanSkinning->SetValue(true);
	sizerOptions->Add(cbCleanSkinning, 0, wxALL, 5);

	cbCalculateBounds = new wxCheckBox(sbOptions->GetStaticBox(), wxID_ANY, "Calculate Bounds");
	cbCalculateBounds->SetToolTip("Calculate new bounding spheres for static meshes.");
	cbCalculateBounds->SetValue(true);
	sizerOptions->Add(cbCalculateBounds, 0, wxALL | wxEXPAND, 5);

	cbRemoveParallax = new wxCheckBox(sbOptions->GetStaticBox(), wxID_ANY, "Remove Parallax");
	cbRemoveParallax->SetToolTip("Removes parallax flags and texture paths.");
	cbRemoveParallax->SetValue(true);
	sizerOptions->Add(cbRemoveParallax, 0, wxALL, 5);

	sbOptions->Add(sizerOptions, 1, wxEXPAND, 5);

	sizer->Add(sbOptions, 0, wxALL | wxEXPAND, 5);

	auto sbExtras = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Extras - don't blindly use for everything"), wxVERTICAL);

	auto sizerExtras = new wxFlexGridSizer(0, 2, 0, 0);
	sizerExtras->AddGrowableCol(1);
	sizerExtras->SetFlexibleDirection(wxBOTH);
	sizerExtras->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	cbSmoothNormals = new wxCheckBox(sbExtras->GetStaticBox(), wxID_ANY, "Smooth Normals");
	sizerExtras->Add(cbSmoothNormals, 0, wxALL | wxEXPAND, 5);

	auto sizerNormals = new wxBoxSizer(wxHORIZONTAL);

	lbSmoothAngle = new wxStaticText(sbExtras->GetStaticBox(), wxID_ANY, "Max. Degrees");
	lbSmoothAngle->Wrap(-1);
	sizerNormals->Add(lbSmoothAngle, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	numSmoothAngle = new wxSpinCtrl(sbExtras->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 180, 60);
	numSmoothAngle->Enable(false);
	sizerNormals->Add(numSmoothAngle, 1, wxALL, 5);

	sizerExtras->Add(sizerNormals, 1, wxEXPAND, 5);

	cbSmoothSeamNormals = new wxCheckBox(sbExtras->GetStaticBox(), wxID_ANY, "Smooth Seam Normals");
	cbSmoothSeamNormals->SetValue(true);
	cbSmoothSeamNormals->Enable(false);
	sizerExtras->Add(cbSmoothSeamNormals, 0, wxALL, 5);

	sbExtras->Add(sizerExtras, 1, wxEXPAND, 5);

	sizer->Add(sbExtras, 0, wxALL | wxEXPAND, 5);

	sizer->Add(0, 0, 1, wxEXPAND, 5);

	auto sizerBottom = new wxBoxSizer(wxHORIZONTAL);

	cbWriteLog = new wxCheckBox(this, wxID_ANY, "Write Log");
	cbWriteLog->SetValue(true);
	cbWriteLog->SetToolTip("Toggles writing of a log file with information about the optimization process.");
	sizerBottom->Add(cbWriteLog, 0, wxALL, 5);

	rbSSE = new wxRadioButton(this, wxID_ANY, "SSE");
	rbSSE->SetValue(true);
	rbSSE->SetToolTip("Choose SSE as the target version.");
	sizerBottom->Add(rbSSE, 0, wxALL, 5);

	rbLE = new wxRadioButton(this, wxID_ANY, "LE");
	rbLE->SetToolTip("Choose LE as the target version.");
	sizerBottom->Add(rbLE, 0, wxALL, 5);

	sizer->Add(sizerBottom, 0, wxALIGN_RIGHT, 5);

	auto sizerButtons = new wxBoxSizer(wxHORIZONTAL);

	btScanTextures = new wxButton(this, wxID_ANY, "Scan Textures");
	sizerButtons->Add(btScanTextures, 0, wxALL, 5);

	btOptimize = new wxButton(this, wxID_ANY, "Optimize");
	sizerButtons->Add(btOptimize, 1, wxALL | wxEXPAND, 5);

	sizer->Add(sizerButtons, 0, wxEXPAND, 5);

	SetSizer(sizer);
	Layout();
	statusBar = CreateStatusBar(2, wxSTB_DEFAULT_STYLE);
	statusBar->SetStatusText("Ready!");

	Centre(wxBOTH);

	// Bind Events
	Bind(wxEVT_CLOSE_WINDOW, &Optimizer::onClose, this);
	dirCtrl->Bind(wxEVT_DIRPICKER_CHANGED, &Optimizer::dirCtrlChanged, this);
	cbSmoothNormals->Bind(wxEVT_CHECKBOX, &Optimizer::cbSmoothNormalsChecked, this);
	btOptimize->Bind(wxEVT_BUTTON, &Optimizer::btOptimizeClicked, this);
	btScanTextures->Bind(wxEVT_BUTTON, &Optimizer::btScanTexturesClicked, this);
}

Optimizer::~Optimizer()
{
	// Unbind Events
	Unbind(wxEVT_CLOSE_WINDOW, &Optimizer::onClose, this);
	dirCtrl->Unbind(wxEVT_DIRPICKER_CHANGED, &Optimizer::dirCtrlChanged, this);
	cbSmoothNormals->Unbind(wxEVT_CHECKBOX, &Optimizer::cbSmoothNormalsChecked, this);
	btOptimize->Unbind(wxEVT_BUTTON, &Optimizer::btOptimizeClicked, this);
	btScanTextures->Unbind(wxEVT_BUTTON, &Optimizer::btScanTexturesClicked, this);
}

void Optimizer::onClose(wxCloseEvent& event)
{
	if (isProcessing)
	{
		isProcessing = false;
		event.Veto();
	}
	else
	{
		Destroy();
	}
}

void Optimizer::dirCtrlChanged(wxFileDirPickerEvent& event)
{
	btOptimize->Enable(!event.GetPath().IsEmpty());
}

void Optimizer::cbSmoothNormalsChecked(wxCommandEvent& event)
{
	numSmoothAngle->Enable(event.IsChecked());
	cbSmoothSeamNormals->Enable(event.IsChecked());
}

void Optimizer::btOptimizeClicked(wxCommandEvent& event)
{
	if (isProcessing)
	{
		isProcessing = false;
		return;
	}

	btOptimize->SetLabel("Cancel");
	btScanTextures->Disable();
	isProcessing = true;

	OptimizerOptions options;
	options.folder = dirCtrl->GetPath();
	options.recursive = cbRecursive->GetValue();
	options.smoothNormals = cbSmoothNormals->GetValue();
	options.smoothAngle = numSmoothAngle->GetValue();
	options.smoothSeamNormals = cbSmoothSeamNormals->GetValue();
	options.headParts = cbHeadParts->GetValue();
	options.cleanSkinning = cbCleanSkinning->GetValue();
	options.calculateBounds = cbCalculateBounds->GetValue();
	options.removeParallax = cbRemoveParallax->GetValue();
	options.targetGame = rbSSE->GetValue() ? TargetGame::SSE : TargetGame::LE;
	options.writeLog = cbWriteLog->GetValue();

	wxGetApp().Optimize(options);

	btOptimize->SetLabel("Optimize");
	btScanTextures->Enable();
	isProcessing = false;
}

void Optimizer::btScanTexturesClicked(wxCommandEvent& event)
{
	if (isProcessing)
	{
		isProcessing = false;
		return;
	}

	btScanTextures->SetLabel("Cancel");
	btOptimize->Disable();
	isProcessing = true;

	ScanOptions options;
	options.folder = dirCtrl->GetPath();
	options.recursive = cbRecursive->GetValue();
	options.targetGame = rbSSE->GetValue() ? TargetGame::SSE : TargetGame::LE;
	options.writeLog = cbWriteLog->GetValue();

	wxGetApp().ScanTextures(options);

	btScanTextures->SetLabel("Scan Textures");
	btOptimize->Enable();
	isProcessing = false;
}

void Optimizer::StartProgress(const wxString& msg)
{
	if (progressStack.empty()) {
		progressVal = 0;
		progressStack.emplace_back(0, 10000);

		wxRect rect;
		statusBar->GetFieldRect(1, rect);
		progressBar = new wxGauge(statusBar, wxID_ANY, 10000, rect.GetPosition(), rect.GetSize());

		if (msg.IsEmpty())
			statusBar->SetStatusText("Processing...");
		else
			statusBar->SetStatusText(msg);
	}
}

void Optimizer::StartSubProgress(int min, int max)
{
	int range = progressStack.back().second - progressStack.front().first;
	float mindiv = min / 100.0f;
	float maxdiv = max / 100.0f;
	int minoff = mindiv * range + 1;
	int maxoff = maxdiv * range + 1;
	progressStack.emplace_back(progressStack.front().first + minoff, progressStack.front().first + maxoff);
}

void Optimizer::UpdateProgress(int val, const wxString& msg)
{
	if (progressStack.empty())
		return;

	int range = progressStack.back().second - progressStack.back().first;
	float div = val / 100.0f;
	int offset = range * div + 1;

	progressVal = progressStack.back().first + offset;
	if (progressVal > 10000)
		progressVal = 10000;

	statusBar->SetStatusText(msg);
	progressBar->SetValue(progressVal);
}

void Optimizer::EndProgress(const wxString& msg)
{
	if (progressStack.empty())
		return;

	progressBar->SetValue(progressStack.back().second);
	progressStack.pop_back();

	if (progressStack.empty()) {
		if (msg.IsEmpty())
			statusBar->SetStatusText("Ready!");
		else
			statusBar->SetStatusText(msg);

		delete progressBar;
		progressBar = nullptr;
	}
}
