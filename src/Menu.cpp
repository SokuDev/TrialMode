//
// Created by PinkySmile on 19/07/2021.
//

#define _USE_MATH_DEFINES
#include <fstream>
#include <direct.h>
#include <dinput.h>
#include <process.h>
#include "Menu.hpp"
#include "Pack.hpp"
#include "Explorer.hpp"
#include "version.h"
#include "Trial/Trial.hpp"
#include "TrialEditor/TrialEditor.hpp"
#include "InputBox.hpp"
#include "ExplorerMenu.hpp"

#ifndef _DEBUG
#define puts(...)
#define printf(...)
#define fprintf(...)
#endif

#define FILTER_TEXT_SIZE 120

static SokuLib::Vector2i offsetTable[9] = {
	{-8, 8},
	{0, 10},
	{8, 8},
	{-10, 0},
	{0, 0},
	{10, 0},
	{-8, -8},
	{0, -10},
	{8, -8}
};
static int baseCursor = 0;
static bool expended = false;
static bool loaded = false;
static bool loadNextTrial = false;
static bool movingScenario = false;
static unsigned shownPack = 0;
static unsigned nameFilter = -1;
static unsigned modeFilter = -1;
static unsigned topicFilter = -1;
static unsigned nbPacks = 0;
static unsigned nbName = 0;
static unsigned nbMode = 0;
static unsigned nbTopic = 0;
static SokuLib::Vector2i versionStrSize;
static SokuLib::Vector2i nameFilterSize;
static SokuLib::Vector2i modeFilterSize;
static SokuLib::Vector2i topicFilterSize;
static SokuLib::DrawUtils::Sprite title;
static SokuLib::DrawUtils::Sprite score;
static SokuLib::DrawUtils::Sprite wrench;
static SokuLib::DrawUtils::Sprite nameFilterText;
static SokuLib::DrawUtils::Sprite modeFilterText;
static SokuLib::DrawUtils::Sprite topicFilterText;
static SokuLib::DrawUtils::Sprite lockedText;
static SokuLib::DrawUtils::Sprite version;
static SokuLib::DrawUtils::Sprite editSeat;
static SokuLib::DrawUtils::Sprite editScenarioSeat;
static SokuLib::DrawUtils::Sprite editSeatForeground;
static SokuLib::DrawUtils::RectangleShape rect;
static std::vector<std::unique_ptr<Guide>> noEditorGuides;
static std::vector<std::unique_ptr<Guide>> editorGuides;

static IDirect3DTexture9 **pphandle = nullptr;
static IDirect3DTexture9 **pphandle2 = nullptr;
static unsigned packStart = 0;
static unsigned entryStart = 0;
static unsigned band1Start = 0;
static unsigned band2Start = 0;

int currentPack = -3;
int currentEntry = -1;
SokuLib::DrawUtils::Sprite arrowSprite;
SokuLib::DrawUtils::Sprite lock;
SokuLib::DrawUtils::Sprite extraImg;
SokuLib::DrawUtils::Sprite missingIcon;
SokuLib::DrawUtils::Sprite questionMarks;
SokuLib::DrawUtils::Sprite packContainer;
SokuLib::DrawUtils::Sprite previewContainer;
SokuLib::DrawUtils::Sprite blackSilouettes;
SokuLib::DrawUtils::Sprite lockedImg;
SokuLib::DrawUtils::Sprite extraText;
SokuLib::DrawUtils::Sprite loadingGear;
SokuLib::DrawUtils::Sprite frame;
SokuLib::DrawUtils::Sprite lockedNoise;
SokuLib::DrawUtils::Sprite CRTBands;
SokuLib::DrawUtils::Sprite editSeatEmpty;
SokuLib::DrawUtils::Sprite stickTop;
SokuLib::DrawUtils::Sprite tickSprite;
SokuLib::DrawUtils::Sprite upArrow;
SokuLib::DrawUtils::Sprite downArrow;
std::map<unsigned int, std::string> chrs;
std::vector<std::string> orderChrs;
std::vector<std::unique_ptr<SokuLib::DrawUtils::Sprite>> charactersFaces;
std::unique_ptr<PackOutro> loadedOutro;
std::unique_ptr<TrialBase> loadedTrial;
bool loadRequest;
unsigned loading = false;
SokuLib::SWRFont defaultFont10;
SokuLib::SWRFont defaultFont12;
SokuLib::SWRFont defaultFont16;
HMODULE myModule;
bool editorMode = false;
char profilePath[1024 + MAX_PATH];
char profileFolderPath[1024 + MAX_PATH];
bool hasEnglishPatch;

static struct PackEditScenario {
	bool opened = false;
	unsigned int cursorPos = 0;
	SokuLib::DrawUtils::Sprite name;
	SokuLib::DrawUtils::Sprite filePath;
	SokuLib::DrawUtils::Sprite previewPath;

	void resetName()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		scenario->nameStr.clear();
		scenario->name.texture.createFromText(scenario->fileRel.c_str(), defaultFont10, {0x100, 30});
		scenario->name.rect = {
			0, 0,
			static_cast<int>(scenario->name.texture.getSize().x),
			static_cast<int>(scenario->name.texture.getSize().y),
		};
		scenario->name.setSize(scenario->name.texture.getSize());
		this->name.texture.createFromText("", defaultFont12, {153, 23});
		SokuLib::playSEWaveBuffer(0x29);
	}

	void resetFile()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		scenario->fileRel.clear();
		if (scenario->nameStr.empty()) {
			scenario->name.texture.createFromText("", defaultFont10, {0x100, 30});
			scenario->name.rect = {
				0, 0,
				static_cast<int>(scenario->name.texture.getSize().x),
				static_cast<int>(scenario->name.texture.getSize().y),
			};
			scenario->name.setSize(scenario->name.texture.getSize());
		}
		this->filePath.texture.createFromText("", defaultFont12, {153, 23});
		SokuLib::playSEWaveBuffer(0x29);
	}

	void resetPreviewPath()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		scenario->previewFile.clear();
		scenario->previewFileRel.clear();
		scenario->preview.reset();
		this->previewPath.texture.createFromText("", defaultFont12, {153, 23});
		SokuLib::playSEWaveBuffer(0x29);
	}

	void nothing()
	{
	}

	void resetDescription()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		scenario->descriptionStr.clear();
		scenario->description.texture.createFromText("No description provided", defaultFont12, {300, 150});
		scenario->description.rect = {
			0, 0,
			static_cast<int>(scenario->description.texture.getSize().x),
			static_cast<int>(scenario->description.texture.getSize().y),
		};
		scenario->description.setPosition({356, 286});
		scenario->description.setSize(scenario->description.texture.getSize());
		SokuLib::playSEWaveBuffer(0x29);
	}

	void setName()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		openInputDialog("Enter new name", scenario->nameStr.c_str());
		setInputBoxCallbacks([&scenario, this](const std::string &input){
			scenario->nameStr = input;
			scenario->name.texture.createFromText(scenario->nameStr.c_str(), defaultFont10, {0x100, 30});
			scenario->name.rect = {
				0, 0,
				static_cast<int>(scenario->name.texture.getSize().x),
				static_cast<int>(scenario->name.texture.getSize().y),
			};
			scenario->name.setSize(scenario->name.texture.getSize());
			this->name.texture.createFromText(scenario->nameStr.c_str(), defaultFont12, {153, 23});
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void setFile()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		openInputDialog("Enter new file path", scenario->fileRel.c_str());
		setInputBoxCallbacks([&scenario, this](const std::string &input){
			scenario->fileRel = input;
			scenario->file = scenario->folder + "/" + input;
			if (scenario->nameStr.empty()) {
				scenario->name.texture.createFromText(scenario->fileRel.c_str(), defaultFont10, {0x100, 30});
				scenario->name.rect = {
					0, 0,
					static_cast<int>(scenario->name.texture.getSize().x),
					static_cast<int>(scenario->name.texture.getSize().y),
				};
				scenario->name.setSize(scenario->name.texture.getSize());
			}
			this->filePath.texture.createFromText(scenario->fileRel.c_str(), defaultFont12, {153, 23});
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void setPreviewPath()
	{
		openFileDialog(
			"All files\0*.*\0"
			"Portable Network Graphics\0*.PNG\0"
			"Joint Photographic Experts Group\0*.JPG;*.JPEG;*.JIF;*.JFIF;.JFI;.JPE\0"
			"Windows Bitmap\0*.BMP\0"
			"Graphics Interchange Format\0*.GIF\0\0",
			loadedPacks[currentPack]->path,
			[this](const std::string &path){
				std::string input = path;
				auto &pack = loadedPacks[currentPack];
				auto &scenario = pack->scenarios[currentEntry];
				SokuLib::Vector2i size;
				char buffer[1024];      // buffer for file content
				char szDir[MAX_PATH];   // buffer for dir name
				char szPath[MAX_PATH];  // buffer for path name
				char szFile2[MAX_PATH];
				char *szFile;           // buffer for path file
				struct stat s;

				if (input.empty())
					return SokuLib::playSEWaveBuffer(0x29);
				GetFullPathNameA(pack->path.c_str(), sizeof(szDir), szDir, nullptr);
				GetFullPathNameA(input.c_str(), sizeof(szPath), szPath, &szFile);
				if (!szFile)
					return SokuLib::playSEWaveBuffer(0x29);
				if (strncmp(szPath, szDir, strlen(szDir)) == 0)
					input = (szPath + strlen(szDir));
				else {
					if (stat((pack->path + "/" + szFile).c_str(), &s) == 0) {
						char ext[40];

						strcpy(szFile2, szFile);
						szFile = szFile2;
						if (strchr(szFile, '.') && strlen(strchr(szFile, '.')) < 40) {
							strcpy(ext, strchr(szFile, '.'));
							*strchr(szFile, '.') = 0;
						}
						szFile[strlen(szFile) + 2] = 0;
						szFile[strlen(szFile) + 1] = '0';
						szFile[strlen(szFile) + 0] = '_';

						while (stat((pack->path + "/" + szFile + ext).c_str(), &s) == 0)
							szFile[strlen(szFile) - 1]++;
						strcat(szFile, ext);
					}

					std::ifstream istream{szPath, std::fstream::binary};

					if (istream.fail() || stat(szPath, &s))
						return SokuLib::playSEWaveBuffer(0x29);

					std::ofstream ostream{pack->path + "/" + szFile, std::fstream::binary};

					if (ostream.fail())
						return SokuLib::playSEWaveBuffer(0x29);

					while (s.st_size) {
						istream.read(buffer, sizeof(buffer));
						ostream.write(buffer, min(s.st_size, sizeof(buffer)));
						s.st_size -= min(s.st_size, sizeof(buffer));
					}
					if (ostream.fail())
						return SokuLib::playSEWaveBuffer(0x29);
					istream.close();
					ostream.close();
					input = szFile;
				}

				scenario->previewFileRel = input;
				scenario->previewFile = pack->path + "/" + scenario->previewFileRel;
				scenario->loadPreview(true);
				this->previewPath.texture.createFromText(scenario->previewFileRel.c_str(), defaultFont12, {153, 23});
				SokuLib::playSEWaveBuffer(0x28);
			}
		);
	}

	void switchExtra()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		scenario->extra = !scenario->extra;
		SokuLib::playSEWaveBuffer(0x28);
	}

	void switchLock()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		scenario->canBeLocked = !scenario->canBeLocked;
		SokuLib::playSEWaveBuffer(0x28);
	}

	void switchHidden()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		scenario->nameHiddenIfLocked = !scenario->nameHiddenIfLocked;
		SokuLib::playSEWaveBuffer(0x28);
	}

	void setDescription()
	{
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		openInputDialog("Enter new description", scenario->descriptionStr.c_str());
		setInputBoxCallbacks([&scenario](const std::string &input){
			scenario->descriptionStr = input;
			scenario->description.texture.createFromText(scenario->descriptionStr.c_str(), defaultFont12, {300, 150});
			scenario->description.rect = {
				0, 0,
				static_cast<int>(scenario->description.texture.getSize().x),
				static_cast<int>(scenario->description.texture.getSize().y),
			};
			scenario->description.setPosition({356, 286});
			scenario->description.setSize(scenario->description.texture.getSize());
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	static constexpr void (PackEditScenario::*resetHandlers[])() = {
		&PackEditScenario::resetName,        // Name
		&PackEditScenario::resetFile,        // File
		&PackEditScenario::resetPreviewPath, // Preview path
		&PackEditScenario::nothing,          // Extra
		&PackEditScenario::nothing,          // Can be locked
		&PackEditScenario::nothing,          // Hidden
		&PackEditScenario::resetDescription, // Description
	};
	static constexpr void (PackEditScenario::*handlers[])() = {
		&PackEditScenario::setName,        // Name
		&PackEditScenario::setFile,        // File
		&PackEditScenario::setPreviewPath, // Preview path
		&PackEditScenario::switchExtra,    // Extra
		&PackEditScenario::switchLock,     // Can be locked
		&PackEditScenario::switchHidden,   // Hidden
		&PackEditScenario::setDescription, // Description
	};
	static constexpr std::pair<SokuLib::Vector2i, SokuLib::Vector2u> cursorLocations[] = {
		{{182 + 7, 240}, {133, 23}},
		{{182 + 7, 270}, {133, 23}},
		{{182 + 7, 298}, {133, 23}},
		{{179 + 7, 334}, { 14, 14}},
		{{302 + 7, 333}, { 14, 14}},
		{{287 + 7, 363}, { 14, 14}},
		{{352 + 7, 286}, { 282, 121}},
	};
	static constexpr unsigned int leftTable[] = {
		6, 6, 6, 6, 3, 6, 0
	};
	static constexpr unsigned int rightTable[] = {
		6, 6, 6, 4, 6, 6, 0
	};
	static constexpr unsigned int upTable[] = {
		5, 0, 1, 2, 2, 3, 6
	};
	static constexpr unsigned int downTable[] = {
		1, 2, 3, 5, 5, 0, 6
	};
} packEditScenario;

static struct PackEditPage {
	unsigned cursorPos = 0;
	int chrCursorPos = 0;
	bool opened = false;
	bool selectingPos = false;
	bool selectingScale = false;
	bool selectingCharacters = false;
	std::string tmpModesStr;
	SokuLib::DrawUtils::Sprite name;
	SokuLib::DrawUtils::Sprite modes;
	SokuLib::DrawUtils::Sprite author;
	SokuLib::DrawUtils::Sprite version;
	SokuLib::DrawUtils::Sprite category;
	SokuLib::DrawUtils::Sprite iconPath;
	SokuLib::DrawUtils::Sprite endingPath;
	SokuLib::DrawUtils::Sprite characters;
	SokuLib::DrawUtils::Sprite description;
	SokuLib::DrawUtils::Sprite previewPath;
	SokuLib::DrawUtils::Sprite cursor;
	SokuLib::DrawUtils::Sprite cursorGear;
	SokuLib::DrawUtils::RectangleShape rect;

	void notImplemented()
	{
		MessageBox(SokuLib::window, "Not implemented", "Not implemented", MB_ICONINFORMATION);
	}

	void changeScale()
	{
		this->selectingScale = static_cast<bool>(loadedPacks[currentPack]->icon);
		editorGuides[5]->active = true;
		editorGuides[3]->active = false;
		SokuLib::playSEWaveBuffer(0x29 - this->selectingScale);
	}

	void changePos()
	{
		this->selectingPos = static_cast<bool>(loadedPacks[currentPack]->icon);
		editorGuides[4]->active = true;
		editorGuides[3]->active = false;
		SokuLib::playSEWaveBuffer(0x29 - this->selectingPos);
	}

	void changeCharacter()
	{
		this->chrCursorPos = 0;
		this->selectingCharacters = true;
		editorGuides[6]->active = true;
		editorGuides[3]->active = false;
		SokuLib::playSEWaveBuffer(0x29 - this->selectingCharacters);
	}

	void setName()
	{
		auto &pack = loadedPacks[currentPack];

		openInputDialog("Enter new name", pack->nameStr.c_str());
		setInputBoxCallbacks([&pack, this](const std::string &input){
			SokuLib::Vector2i size;

			pack->nameStr = input;
			pack->name.texture.createFromText(
				(pack->category + ": " + pack->nameStr).c_str(),
				defaultFont12, {0x100, 30}, &size
			);
			pack->name.rect = {0, 0, size.x, size.y};
			pack->name.setSize((size - 1).to<unsigned>());
			this->name.texture.createFromText(pack->nameStr.c_str(), defaultFont12, {153, 23}, &size);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void setCategory()
	{
		auto &pack = loadedPacks[currentPack];

		openInputDialog("Enter new category", pack->category.c_str());
		setInputBoxCallbacks([&pack, this](const std::string &input){
			SokuLib::Vector2i size;

			pack->category = input;
			pack->name.texture.createFromText(
				(pack->category + ": " + pack->nameStr).c_str(),
				defaultFont12, {0x100, 30}, &size
			);
			pack->name.rect = {0, 0, size.x, size.y};
			pack->name.setSize((size - 1).to<unsigned>());
			this->category.texture.createFromText(pack->category.c_str(), defaultFont12, {153, 23}, &size);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void setAuthor()
	{
		auto &pack = loadedPacks[currentPack];

		openInputDialog("Enter new author", pack->authorStr.c_str());
		setInputBoxCallbacks([&pack, this](const std::string &input){
			SokuLib::Vector2i size;

			pack->authorStr = input;
			if (pack->author.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER].b != 0xFF)
				return;
			pack->author.texture.createFromText(("By " + pack->authorStr).c_str(), defaultFont10, {0x100, 14}, &size);
			pack->author.rect = {
				0, 0,
				static_cast<int>(size.x),
				static_cast<int>(size.y),
			};
			pack->author.setSize((size - 1).to<unsigned>());
			this->author.texture.createFromText(pack->authorStr.c_str(), defaultFont12, {153, 23}, &size);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void setMinVersion()
	{
		auto &pack = loadedPacks[currentPack];

		openInputDialog("Enter new minimum mod version required", pack->minVersion.c_str());
		setInputBoxCallbacks([&pack, this](const std::string &input){
			SokuLib::Vector2i size;

			if (input.substr(0, strlen("beta ")) != "beta " && input.substr(0, strlen("alpha ")) != "alpha " && !std::isdigit(input[0]))
				return SokuLib::playSEWaveBuffer(0x29);
			try {
				Pack::getVersionFromStr(input);
			} catch (std::exception &e) {
				printf("Invalid version: %s\n", e.what());
				return SokuLib::playSEWaveBuffer(0x29);
			}
			pack->minVersion = input;
			this->version.texture.createFromText(pack->minVersion.c_str(), defaultFont12, {153, 23}, &size);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void setEnding()
	{
		auto &pack = loadedPacks[currentPack];

		openInputDialog("Enter new ending file", pack->outroRelPath.c_str());
		setInputBoxCallbacks([&pack, this](const std::string &input){
			SokuLib::Vector2i size;

			if (Pack::isInvalidPath(input))
				return SokuLib::playSEWaveBuffer(0x29);
			pack->outroRelPath = input;
			pack->outroPath = pack->path + "/" + input;
			this->endingPath.texture.createFromText(pack->outroRelPath.c_str(), defaultFont12, {153, 23}, &size);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void setDescription()
	{
		auto &pack = loadedPacks[currentPack];

		openInputDialog("Enter new description", pack->descriptionStr.c_str());
		setInputBoxCallbacks([&pack, this](const std::string &input){
			SokuLib::Vector2i size;

			pack->descriptionStr = input;
			pack->description.texture.createFromText(pack->descriptionStr.c_str(),defaultFont12, {300, 150});
			pack->description.rect = {
				0, 0,
				static_cast<int>(pack->description.texture.getSize().x),
				static_cast<int>(pack->description.texture.getSize().y),
			};
			pack->description.setPosition({356, 286});
			pack->description.setSize(pack->description.texture.getSize());
			this->description.texture.createFromText(pack->descriptionStr.c_str(), defaultFont16, {381, 111}, &size);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void switchMirrorX()
	{
		auto &pack = loadedPacks[currentPack];

		if (!pack->icon)
			return SokuLib::playSEWaveBuffer(0x29);
		pack->icon->mirror.x = !pack->icon->mirror.x;
		pack->icon->sprite.setMirroring(pack->icon->mirror.x, pack->icon->mirror.y);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void switchMirrorY()
	{
		auto &pack = loadedPacks[currentPack];

		if (!pack->icon)
			return SokuLib::playSEWaveBuffer(0x29);
		pack->icon->mirror.y = !pack->icon->mirror.y;
		pack->icon->sprite.setMirroring(pack->icon->mirror.x, pack->icon->mirror.y);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void nothing()
	{
	}

	void selectPreviewSoku()
	{
		auto &pack = loadedPacks[currentPack];

		if (!pack->previewFSAsset)
			loadExplorerFile(pack->previewPath, "cv2");
		else
			loadExplorerRoot("cv2");
		setExplorerDefaultMusic(nullptr);
		setExplorerCallback([this, &pack](std::string input){
			SokuLib::Vector2i size;

			if (input.empty())
				return SokuLib::playSEWaveBuffer(0x29);
			if (!pack->preview.texture.loadFromGame(input.c_str()))
				return SokuLib::playSEWaveBuffer(0x29);
			pack->previewPath = input;
			pack->previewFSAsset = false;
			pack->preview.rect = {
				0, 0,
				static_cast<int>(pack->preview.texture.getSize().x),
				static_cast<int>(pack->preview.texture.getSize().y),
			};
			pack->preview.setPosition({398, 128});
			pack->preview.setSize({200, 150});
			this->previewPath.texture.createFromText(pack->previewPath.c_str(), defaultFont12, {153, 23}, &size);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void selectPreviewFS()
	{
		openFileDialog(
			"All files\0*.*\0"
			"Portable Network Graphics\0*.PNG\0"
			"Joint Photographic Experts Group\0*.JPG;*.JPEG;*.JIF;*.JFIF;.JFI;.JPE\0"
			"Windows Bitmap\0*.BMP\0"
			"Graphics Interchange Format\0*.GIF\0\0",
			loadedPacks[currentPack]->path,
			[this](const std::string &path){
				std::string input = path;
				auto &pack = loadedPacks[currentPack];
				SokuLib::Vector2i size;
				char buffer[1024];      // buffer for file content
				char szDir[MAX_PATH];   // buffer for dir name
				char szPath[MAX_PATH];  // buffer for path name
				char szFile2[MAX_PATH];
				char *szFile;           // buffer for path file
				struct stat s;

				if (input.empty())
					return SokuLib::playSEWaveBuffer(0x29);
				GetFullPathNameA(pack->path.c_str(), sizeof(szDir), szDir, nullptr);
				GetFullPathNameA(input.c_str(), sizeof(szPath), szPath, &szFile);
				if (!szFile)
					return SokuLib::playSEWaveBuffer(0x29);
				if (strncmp(szPath, szDir, strlen(szDir)) == 0)
					input = (szPath + strlen(szDir));
				else {
					if (stat((pack->path + "/" + szFile).c_str(), &s) == 0) {
						char ext[40];

						strcpy(szFile2, szFile);
						szFile = szFile2;
						if (strchr(szFile, '.') && strlen(strchr(szFile, '.')) < 40) {
							strcpy(ext, strchr(szFile, '.'));
							*strchr(szFile, '.') = 0;
						}
						szFile[strlen(szFile) + 2] = 0;
						szFile[strlen(szFile) + 1] = '0';
						szFile[strlen(szFile) + 0] = '_';

						while (stat((pack->path + "/" + szFile + ext).c_str(), &s) == 0)
							szFile[strlen(szFile) - 1]++;
						strcat(szFile, ext);
					}

					std::ifstream istream{szPath, std::fstream::binary};

					if (istream.fail() || stat(szPath, &s))
						return SokuLib::playSEWaveBuffer(0x29);

					std::ofstream ostream{pack->path + "/" + szFile, std::fstream::binary};

					if (ostream.fail())
						return SokuLib::playSEWaveBuffer(0x29);

					while (s.st_size) {
						istream.read(buffer, sizeof(buffer));
						ostream.write(buffer, min(s.st_size, sizeof(buffer)));
						s.st_size -= min(s.st_size, sizeof(buffer));
					}
					if (ostream.fail())
						return SokuLib::playSEWaveBuffer(0x29);
					istream.close();
					ostream.close();
					input = szFile;
				}

				if (!pack->preview.texture.loadFromFile((pack->path + "/" + input).c_str()))
					return SokuLib::playSEWaveBuffer(0x29);
				pack->previewPath = input;
				pack->previewFSAsset = true;
				pack->preview.rect = {
					0, 0,
					static_cast<int>(pack->preview.texture.getSize().x),
					static_cast<int>(pack->preview.texture.getSize().y),
				};
				this->previewPath.texture.createFromText(pack->previewPath.c_str(), defaultFont12, {153, 23}, &size);
				SokuLib::playSEWaveBuffer(0x28);
			}
		);
	}

	void selectIconSoku()
	{
		auto &pack = loadedPacks[currentPack];

		if (pack->icon && !pack->icon->fsPath)
			loadExplorerFile(pack->icon->path, "cv2");
		else
			loadExplorerRoot("cv2");
		setExplorerDefaultMusic(nullptr);
		setExplorerCallback([this, &pack](std::string input){
			SokuLib::Vector2i size;

			if (input.empty())
				return SokuLib::playSEWaveBuffer(0x29);
			if (pack->icon) {
				if (!pack->icon->sprite.texture.loadFromGame(input.c_str()))
					return SokuLib::playSEWaveBuffer(0x29);
				pack->icon->path = input;
				pack->icon->fsPath = false;
				pack->icon->sprite.rect = {
					0, 0,
					static_cast<int>(pack->icon->sprite.texture.getSize().x),
					static_cast<int>(pack->icon->sprite.texture.getSize().y),
					};
			} else {
				pack->icon.reset(new Icon(pack->path, {
					{"path",   input},
					{"isPath", false}
				}));
				if (!pack->icon->sprite.texture.hasTexture())
					return SokuLib::playSEWaveBuffer(0x29);
			}

			pack->icon->untransformedRect.x = pack->icon->sprite.texture.getSize().x;
			pack->icon->untransformedRect.y = pack->icon->sprite.texture.getSize().y;
			pack->icon->rect.width = min(68 / pack->icon->scale, pack->icon->untransformedRect.x);
			pack->icon->rect.height = min(28 / pack->icon->scale, pack->icon->untransformedRect.y);
			pack->icon->sprite.setSize({
				static_cast<unsigned int>(pack->icon->rect.width * pack->icon->scale),
				static_cast<unsigned int>(pack->icon->rect.height * pack->icon->scale)
			});
			pack->icon->sprite.rect = pack->icon->rect;
			this->iconPath.texture.createFromText(pack->icon->path.c_str(), defaultFont12, {153, 23}, &size);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void selectIconFS()
	{
		openFileDialog(
			"All files\0*.*\0"
			"Portable Network Graphics\0*.PNG\0"
			"Joint Photographic Experts Group\0*.JPG;*.JPEG;*.JIF;*.JFIF;.JFI;.JPE\0"
			"Windows Bitmap\0*.BMP\0"
			"Graphics Interchange Format\0*.GIF\0\0",
			loadedPacks[currentPack]->path,
			[this](const std::string &path){
				std::string input = path;
				auto &pack = loadedPacks[currentPack];
				SokuLib::Vector2i size;
				char buffer[1024];      // buffer for file content
				char szDir[MAX_PATH];   // buffer for dir name
				char szPath[MAX_PATH];  // buffer for path name
				char szFile2[MAX_PATH];
				char *szFile;           // buffer for path file
				struct stat s;

				if (input.empty())
					return SokuLib::playSEWaveBuffer(0x29);
				GetFullPathNameA(pack->path.c_str(), sizeof(szDir), szDir, nullptr);
				GetFullPathNameA(input.c_str(), sizeof(szPath), szPath, &szFile);
				if (!szFile)
					return SokuLib::playSEWaveBuffer(0x29);
				if (strncmp(szPath, szDir, strlen(szDir)) == 0)
					input = (szPath + strlen(szDir));
				else {
					if (stat((pack->path + "/" + szFile).c_str(), &s) == 0) {
						char ext[40];

						strcpy(szFile2, szFile);
						szFile = szFile2;
						if (strchr(szFile, '.') && strlen(strchr(szFile, '.')) < 40) {
							strcpy(ext, strchr(szFile, '.'));
							*strchr(szFile, '.') = 0;
						}
						szFile[strlen(szFile) + 2] = 0;
						szFile[strlen(szFile) + 1] = '0';
						szFile[strlen(szFile) + 0] = '_';

						while (stat((pack->path + "/" + szFile + ext).c_str(), &s) == 0)
							szFile[strlen(szFile) - 1]++;
						strcat(szFile, ext);
					}

					std::ifstream istream{szPath, std::fstream::binary};

					if (istream.fail() || stat(szPath, &s))
						return SokuLib::playSEWaveBuffer(0x29);

					std::ofstream ostream{pack->path + "/" + szFile, std::fstream::binary};

					if (ostream.fail())
						return SokuLib::playSEWaveBuffer(0x29);

					while (s.st_size) {
						istream.read(buffer, sizeof(buffer));
						ostream.write(buffer, min(s.st_size, sizeof(buffer)));
						s.st_size -= min(s.st_size, sizeof(buffer));
					}
					if (ostream.fail())
						return SokuLib::playSEWaveBuffer(0x29);
					istream.close();
					ostream.close();
					input = szFile;
				}

				if (pack->icon) {
					if (!pack->icon->sprite.texture.loadFromFile((pack->path + "/" + input).c_str()))
						return SokuLib::playSEWaveBuffer(0x29);
					pack->icon->path = input;
					pack->icon->fsPath = true;
				} else
					pack->icon.reset(new Icon(pack->path, {
						{"path", input},
						{"isPath", true}
					}));

				pack->icon->untransformedRect.x = pack->icon->sprite.texture.getSize().x;
				pack->icon->untransformedRect.y = pack->icon->sprite.texture.getSize().y;
				pack->icon->rect.width = min(68 / pack->icon->scale, pack->icon->untransformedRect.x);
				pack->icon->rect.height = min(28 / pack->icon->scale, pack->icon->untransformedRect.y);
				pack->icon->sprite.setSize({
					static_cast<unsigned int>(pack->icon->rect.width * pack->icon->scale),
					static_cast<unsigned int>(pack->icon->rect.height * pack->icon->scale)
				});
				pack->icon->sprite.rect = pack->icon->rect;
				this->iconPath.texture.createFromText(pack->icon->path.c_str(), defaultFont12, {153, 23}, &size);
				SokuLib::playSEWaveBuffer(0x28);
			}
		);
	}

	void setModes()
	{
		auto &pack = loadedPacks[currentPack];

		openInputDialog("Enter new modes separated by commas", this->tmpModesStr.c_str());
		setInputBoxCallbacks([&pack, this](const std::string &i){
			unsigned allocSize = 0;
			std::string input = i;

			for (auto c : input)
				allocSize += c == ',';
			pack->modes.clear();
			pack->modes.reserve(allocSize + 1);
			for (auto pos = input.find(','); pos != std::string::npos; pos = input.find(',')) {
				auto str = input.substr(0, pos);

				input = input.substr(pos + 1);
				while (!str.empty() && std::isspace(str[0]))
					str.erase(str.begin());
				while (!str.empty() && std::isspace(str.back()))
					str.pop_back();
				if (str.empty())
					continue;
				str.front() ^= 32 * (std::islower(str.front()) != 0);
				if (std::find(pack->modes.begin(), pack->modes.end(), str) == pack->modes.end())
					pack->modes.push_back(str);
			}
			while (!input.empty() && std::isspace(input[0]))
				input.erase(input.begin());
			while (!input.empty() && std::isspace(input.back()))
				input.pop_back();
			if (!input.empty()) {
				input.front() ^= 32 * (std::islower(input.front()) != 0);
				if (std::find(pack->modes.begin(), pack->modes.end(), input) == pack->modes.end())
					pack->modes.push_back(input);
			}

			allocSize = 0;
			for (int i = 0; i < pack->modes.size(); i++)
				allocSize += pack->modes[i].size() + (i != 0);
			this->tmpModesStr.clear();
			this->tmpModesStr.reserve(allocSize);
			for (int i = 0; i < pack->modes.size(); i++) {
				if (i != 0)
					this->tmpModesStr += ',';
				this->tmpModesStr += pack->modes[i];
			}
			this->modes.texture.createFromText(this->tmpModesStr.c_str(), defaultFont12, {153, 23}, nullptr);
			SokuLib::playSEWaveBuffer(0x28);
		});
	}

	void resetName()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		pack->nameStr.clear();
		pack->name.texture.createFromText(
			(pack->category + ": " + pack->nameStr).c_str(),
			defaultFont12, {0x100, 30}, &size
		);
		pack->name.rect = {0, 0, size.x, size.y};
		pack->name.setSize((size - 1).to<unsigned>());
		this->name.texture.createFromText(pack->nameStr.c_str(), defaultFont12, {153, 23}, &size);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetCategory()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		pack->category.clear();
		pack->name.texture.createFromText(
			(": " + pack->nameStr).c_str(),
			defaultFont12, {0x100, 30}, &size
		);
		pack->name.rect = {0, 0, size.x, size.y};
		pack->name.setSize((size - 1).to<unsigned>());
		this->category.texture.createFromText("", defaultFont12, {153, 23}, &size);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetIconPath()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		pack->icon.reset();
		this->iconPath.texture.createFromText("", defaultFont12, {153, 23}, &size);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetIconScale()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		if (!pack->icon)
			return SokuLib::playSEWaveBuffer(0x29);
		pack->icon->scale = 1;
		this->cursor.setPosition({static_cast<int>(161 + (pack->icon ? 77 * pack->icon->scale : 77)), 136});
		this->cursorGear.setPosition({static_cast<int>(153 + (pack->icon ? 77 * pack->icon->scale : 77)), 141});
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetIconPos()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		if (!pack->icon)
			return SokuLib::playSEWaveBuffer(0x29);
		pack->icon->translate = {0, 0};
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetPreviewPath()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		pack->previewPath.clear();
		pack->preview.texture.destroy();
		this->previewPath.texture.createFromText("", defaultFont12, {153, 23}, &size);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetCharacters()
	{
		auto &pack = loadedPacks[currentPack];

		pack->characters.clear();
		this->characters.texture.createFromText("", defaultFont12, {381, 23}, nullptr);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetAuthor()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		pack->authorStr.clear();
		if (pack->author.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER].b != 0xFF)
			return;
		pack->author.texture.createFromText(("By " + authors[rand() % authors.size()]).c_str(), defaultFont10, {0x100, 14}, &size);
		pack->author.rect = {
			0, 0,
			static_cast<int>(size.x),
			static_cast<int>(size.y),
		};
		pack->author.setSize((size - 1).to<unsigned>());
		this->author.texture.createFromText("", defaultFont12, {153, 23}, &size);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetMinVersion()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		pack->minVersion.clear();
		this->version.texture.createFromText(pack->minVersion.c_str(), defaultFont12, {153, 23}, &size);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetDescription()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		pack->descriptionStr.clear();
		pack->description.texture.createFromText("No description provided", defaultFont12, {300, 150});
		this->description.texture.createFromText("", defaultFont16, {381, 111}, &size);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetEnding()
	{
		auto &pack = loadedPacks[currentPack];
		SokuLib::Vector2i size;

		pack->outroPath.clear();
		pack->outroRelPath.clear();
		this->endingPath.texture.createFromText(pack->outroRelPath.c_str(), defaultFont12, {153, 23}, &size);
		SokuLib::playSEWaveBuffer(0x28);
	}

	void resetModes()
	{
		auto &pack = loadedPacks[currentPack];

		pack->modes.clear();
		this->tmpModesStr.clear();
		this->modes.texture.createFromText(this->tmpModesStr.c_str(), defaultFont12, {153, 23}, nullptr);
		SokuLib::playSEWaveBuffer(0x28);
	}

	static constexpr void (PackEditPage::*resetHandlers[])() = {
		&PackEditPage::resetName,        // Name
		&PackEditPage::resetCategory,    // Category
		&PackEditPage::resetIconPath,    // Icon path
		&PackEditPage::nothing,          // Icon select file
		&PackEditPage::nothing,          // Icon select game asset
		&PackEditPage::resetIconScale,   // Icon change scale
		&PackEditPage::nothing,          // Icon mirror X
		&PackEditPage::nothing,          // Icon mirror Y
		&PackEditPage::resetIconPos,     // Icon change pos
		&PackEditPage::resetPreviewPath, // Preview path
		&PackEditPage::nothing,          // Preview select file
		&PackEditPage::nothing,          // Preview select game asset
		&PackEditPage::resetCharacters,  // Characters
		&PackEditPage::resetAuthor,      // Author
		&PackEditPage::resetMinVersion,  // Min version
		&PackEditPage::resetDescription, // Description
		&PackEditPage::resetEnding,      // Ending
		&PackEditPage::resetModes,       // Modes
	};
	static constexpr void (PackEditPage::*handlers[])() = {
		&PackEditPage::setName,          // Name
		&PackEditPage::setCategory,      // Category
		&PackEditPage::nothing,          // Icon path
		&PackEditPage::selectIconFS,     // Icon select file
		&PackEditPage::selectIconSoku,   // Icon select game asset
		&PackEditPage::changeScale,      // Icon change scale
		&PackEditPage::switchMirrorX,    // Icon mirror X
		&PackEditPage::switchMirrorY,    // Icon mirror Y
		&PackEditPage::changePos,        // Icon change pos
		&PackEditPage::nothing,          // Preview path
		&PackEditPage::selectPreviewFS,  // Preview select file
		&PackEditPage::selectPreviewSoku,// Preview select game asset
		&PackEditPage::changeCharacter,  // Characters
		&PackEditPage::setAuthor,        // Author
		&PackEditPage::setMinVersion,    // Min version
		&PackEditPage::setDescription,   // Description
		&PackEditPage::setEnding,        // Ending
		&PackEditPage::setModes,         // Modes
	};
	static constexpr std::pair<SokuLib::Vector2i, SokuLib::Vector2u> cursorLocations[] = {
		{{167 + 7,  62}, {133, 23}},
		{{395 + 7,  63}, {133, 23}},
		{{167 + 7,  99}, {133, 23}},
		{{316 + 7,  92}, { 95, 17}},
		{{316 + 7, 108}, { 95, 17}},
		{{165 + 7, 127}, {168, 17}},
		{{411 + 7, 129}, { 14, 14}},
		{{411 + 7, 147}, { 14, 14}},
		{{467 + 7, 100}, { 57, 56}},
		{{167 + 7, 173}, {133, 23}},
		{{316 + 7, 166}, { 95, 17}},
		{{316 + 7, 182}, { 95, 17}},
		{{167 + 7, 208}, {361, 23}},
		{{167 + 7, 245}, {133, 23}},
		{{395 + 7, 245}, {133, 23}},
		{{167 + 7, 281}, {361, 91}},
		{{167 + 7, 386}, {133, 23}},
		{{395 + 7, 386}, {133, 23}},
	};
	static constexpr unsigned int leftTable[] = {
		1, 0,
		8, 2, 2,
		8, 5, 5, 6,
		10, 9, 9,
		12,
		14, 13,
		15,
		17, 16
	};
	static constexpr unsigned int rightTable[] = {
		1, 0,
		3, 8, 8,
		6, 8, 8, 5,
		10, 9, 9,
		12,
		14, 13,
		15,
		17, 16
	};
	static constexpr unsigned int upTable[] = {
		16, 17,
		0, 1, 3,
		2, 4, 6, 1,
		5, 7, 10,
		9,
		12, 12,
		13,
		15, 15
	};
	static constexpr unsigned int downTable[] = {
		2, 3,
		5, 4, 6,
		9, 7, 10, 10,
		12, 11, 12,
		13,
		15, 15,
		16,
		0, 1
	};
} packEditPage;

#define CRenderer_Unknown1 ((void (__thiscall *)(int, int))0x404AF0)

extern "C" __declspec(dllexport) bool isInTrial()
{
	return loadedTrial.operator bool();
}

extern "C" __declspec(dllexport) const char *getTrialName()
{
	if (!loadedTrial)
		return "";
	return loadedPacks[currentPack]->scenarios[currentEntry]->nameStr.c_str();
}

static std::string tmp;

extern "C" __declspec(dllexport) const char *getTrialPack()
{
	if (!loadedTrial)
		return "";
	tmp = loadedPacks[currentPack]->category + ": " + loadedPacks[currentPack]->nameStr;
	return tmp.c_str();
}

extern "C" __declspec(dllexport) unsigned getCurrentAttempts()
{
	if (!loadedTrial)
		return 0;
	return loadedTrial->getAttempt() + 1;
}

extern "C" __declspec(dllexport) bool isEditor()
{
	return editorMode;
}

void displaySokuCursor(SokuLib::Vector2i pos, SokuLib::Vector2u size)
{
	SokuLib::Sprite (&CursorSprites)[3] = *(SokuLib::Sprite (*)[3])0x89A6C0;

	//0x443a50 -> Vanilla display cursor
	CursorSprites[0].scale.x = size.x * 0.00195313f;
	CursorSprites[0].scale.y = size.y / 16.f;
	pos.x -= 7;
	CursorSprites[0].render(pos.x, pos.y);
	CRenderer_Unknown1(0x896B4C, 2);
	CursorSprites[1].rotation = *(float *)0x89A450 * 4.00000000f;
	CursorSprites[1].render(pos.x, pos.y + 8.00000000f);
	CursorSprites[2].rotation = -*(float *)0x89A450 * 4.00000000f;
	CursorSprites[2].render(pos.x - 14.00000000f, pos.y - 1.00000000f);
	CRenderer_Unknown1(0x896B4C, 1);
}

static void loadAllExistingCards()
{
	if (!characterCards.empty())
		return;

	char buffer[] = "data/csv/000000000000/spellcard.csv";
	char bufferCards[] = "data/card/000000000000/card000.bmp";
	auto chrs = validCharacters;

	if (!SokuLib::SWRUnlinked)
		for (auto &chr : swrCharacters)
			chrs.emplace(chr);
	puts("Loading cards");
	for (auto &[id, codeName] : chrs) {
		sprintf(buffer, "data/csv/%s/spellcard.csv", codeName.c_str());
		printf("Loading cards from %s\n", buffer);

		SokuLib::CSVParser parser{buffer};
		int i = 0;

		do {
			auto str = parser.getNextCell();
			unsigned short cardId;

			i++;
			try {
				auto str2 = parser.getNextCell();

				printf("New card %s: \"%s\" ->", str.c_str(), str2.c_str());
				cardId = std::stoul(str);
				while (str2.front() >= 0 && str2.front() < 127 && std::isspace(str2.front()))
					str2.erase(str2.begin());
				while (str2.back() >= 0 && str2.back() < 127 && std::isspace(str2.back()))
					str2.pop_back();
				printf(" \"%s\"\n", str2.c_str());
				characterCards[id][cardId].first = str2;
			} catch (std::exception &e) {
				MessageBoxA(
					SokuLib::window,
					(
						"Fatal error: Cannot load cards list for " + codeName + ":\n" +
						"In file " + buffer + ": Cannot parse cell #1 at line #" + std::to_string(i) +
						" \"" + str + "\": " + e.what()
					).c_str(),
					"Loading card names failed",
					MB_ICONERROR
				);
				abort();
			}
			parser.getNextCell();
			try {
				auto str2 = parser.getNextCell();

				printf("Card %s has cost: %s\n", str.c_str(), str2.c_str());
				characterCards[id][cardId].second = std::stoul(str2);
			} catch (std::exception &e) {
				MessageBoxA(
					SokuLib::window,
					(
						"Fatal error: Cannot load cards list for " + codeName + ":\n" +
						"In file " + buffer + ": Cannot parse cell #3 at line #" + std::to_string(i) +
						" \"" + str + "\": " + e.what()
					).c_str(),
					"Loading card costs failed",
					MB_ICONERROR
				);
				abort();
			}
		} while (parser.goToNextLine());
	}
}

void saveScores()
{
	std::ofstream stream{loadedPacks[currentPack]->scorePath, std::ofstream::binary};

	for (auto &scenario : loadedPacks[currentPack]->scenarios)
		stream.write(&scenario->score, sizeof(scenario->score));
}

std::vector<unsigned char> getScores()
{
	std::vector<unsigned char> scores;

	for (auto &scenario : loadedPacks[currentPack]->scenarios)
		scores.push_back(scenario->score);
	return scores;
}

bool checkField(const std::string &field, const nlohmann::json &value, bool (nlohmann::json::*fct)() const noexcept)
{
	if (!value.contains(field) || !(value[field].*fct)()) {
		MessageBox(
			SokuLib::window,
			("The field \"" + field + "\" is not valid but is mandatory.").c_str(),
			"Loading error",
			MB_ICONERROR
		);
		return false;
	}
	return true;
}

inline bool isCompleted(int entry)
{
	if (entry < 0)
		return true;
	return loadedPacks[currentPack]->scenarios[entry]->score != -1;
}

inline int isLocked(const Pack &pack)
{
	if (pack.requirement.empty())
		return 0;

	auto arr = packsByName[pack.nameStr];
	auto it = std::find_if(arr.begin(), arr.end(), [&pack](const std::shared_ptr<Pack> &a){
		return pack.requirement == a->category;
	});

	if (it == arr.end())
		return -1;
	if ((*it)->scenarios.empty())
		return -1;
	for (size_t i = (*it)->scenarios.size(); i; i--)
		if (!(*it)->scenarios[i - 1]->extra)
			return (*it)->scenarios[i - 1]->score < 0;
	return -1;
}

inline bool isLocked(int entry)
{
	if (entry <= 0)
		return false;
	if (!loadedPacks[currentPack]->scenarios[entry]->canBeLocked)
		return false;
	if (!loadedPacks[currentPack]->scenarios[entry]->extra)
		return !isCompleted(entry - 1);
	for (auto &scenario : loadedPacks[currentPack]->scenarios)
		if (scenario->score != 3 && !scenario->extra)
			return true;
	return false;
}

void addCharacterToBufferEditor(const std::string &name, const nlohmann::json &json, SokuLib::PlayerInfo &info, bool isRight)
{
	std::string str = getField<std::string>(json, "reimu", &nlohmann::json::is_string, "character", name);
	auto it = std::find_if(
		validCharacters.begin(),
		validCharacters.end(),
		[str](const std::pair<unsigned, std::string> &s) {
			return s.second == str;
		}
	);
	auto itSWR = std::find_if(
		swrCharacters.begin(),
		swrCharacters.end(),
		[str](const std::pair<unsigned, std::string> &s) {
			return s.second == str;
		}
	);

	if (it == validCharacters.end() && (SokuLib::SWRUnlinked || itSWR == swrCharacters.end())) {
		MessageBox(
			SokuLib::window,
			("Error in field \"" + name + "\": " + str + " is not a valid character.").c_str(),
			"Loading error",
			MB_ICONERROR
		);
		it = validCharacters.begin();
	}

	std::vector<int> deck;

	try {
		std::map<unsigned short, int> cards;

		deck = getField<std::vector<int>>(json, {}, &nlohmann::json::is_array, "deck", name);
		if (deck.size() != 20 && !deck.empty())
			throw std::out_of_range("Deck must either be empty or have 20 cards");
		for (int card : deck) {
			cards[card]++;
			if (cards[card] > 4)
				throw std::out_of_range("More than 4 card " + std::to_string(card) + " are in this deck.");
		}
	} catch (std::exception &e) {
		MessageBox(
			SokuLib::window,
			("Error in field \"" + name + "\": Deck is not valid: " + std::string()).c_str(),
			"Loading error",
			MB_ICONERROR
		);
		deck.clear();
	}

	info.character = static_cast<SokuLib::Character>(it == validCharacters.end() ? itSWR->first : it->first);
	info.palette = getField(json, 0, &nlohmann::json::is_number, "palette", name);
	info.isRight = isRight;

	auto FUN_00434bf0 = reinterpret_cast<void (__thiscall *)(SokuLib::Profile *, char)>(0x434bf0);

	//:magic_wand:
	if (isRight) {
		*((SokuLib::KeyManager **)0x00898684) = reinterpret_cast<SokuLib::KeyManager *>(0x8986a8);
		FUN_00434bf0(&SokuLib::profile2, -1);
		info.keyManager = (SokuLib::KeyManager *)0x0089918c;
	} else {
		*((SokuLib::KeyManager **)0x00898680) = *(char *)0x898678 < '\0' ?
			reinterpret_cast<SokuLib::KeyManager *>(0x8986a8) :
			((SokuLib::KeyManager *(__thiscall *)(int, char))0x43e3b0)(0x899cec, *(char *)0x898678);
		FUN_00434bf0(&SokuLib::profile1, *(char *)0x898678);
		info.keyManager = (SokuLib::KeyManager *)0x008989A0;
	}

	info.effectiveDeck.clear();
	for (auto card : deck)
		info.effectiveDeck.push_back(card);
}

bool addCharacterToBuffer(const std::string &name, const nlohmann::json &chr, SokuLib::PlayerInfo &info, bool isRight)
{
	if (!checkField("character", chr, &nlohmann::json::is_string))
		return false;
	if (!checkField("palette", chr, &nlohmann::json::is_number))
		return false;
	if (!checkField("deck", chr, &nlohmann::json::is_array))
		return false;

	std::string str = chr["character"];
	auto it = std::find_if(
		validCharacters.begin(),
		validCharacters.end(),
		[str](const std::pair<unsigned, std::string> &s) {
			return s.second == str;
		}
	);
	auto itSWR = std::find_if(
		swrCharacters.begin(),
		swrCharacters.end(),
		[str](const std::pair<unsigned, std::string> &s) {
			return s.second == str;
		}
	);

	if (it == validCharacters.end() && (SokuLib::SWRUnlinked || itSWR == swrCharacters.end())) {
		MessageBox(
			SokuLib::window,
			("Error in field \"" + name + "\": " + str + " is not a valid character.").c_str(),
			"Loading error",
			MB_ICONERROR
		);
		return false;
	}

	std::vector<int> deck;

	try {
		std::map<unsigned short, int> cards;

		deck = chr["deck"].get<std::vector<int>>();
		if (deck.size() != 20 && !deck.empty())
			throw std::out_of_range("Deck must either be empty or have 20 cards");
		for (int card : deck) {
			cards[card]++;
			if (cards[card] > 4)
				throw std::out_of_range("More than 4 card " + std::to_string(card) + " are in this deck.");
		}
	} catch (std::exception &e) {
		MessageBox(
			SokuLib::window,
			("Error in field \"" + name + "\": Deck is not valid: " + std::string()).c_str(),
			"Loading error",
			MB_ICONERROR
		);
		return false;
	}

	info.character = static_cast<SokuLib::Character>(it == validCharacters.end() ? itSWR->first : it->first);
	info.palette = chr["palette"].get<int>();
	info.isRight = isRight;

	auto FUN_00434bf0 = reinterpret_cast<void (__thiscall *)(SokuLib::Profile *, char)>(0x434bf0);

	if (isRight) {
		*((SokuLib::KeyManager **)0x00898684) = reinterpret_cast<SokuLib::KeyManager *>(0x8986a8);
		FUN_00434bf0(&SokuLib::profile2, -1);
		info.keyManager = (SokuLib::KeyManager *)0x0089918c;
	} else {
		*((SokuLib::KeyManager **)0x00898680) = *(char *)0x898678 < '\0' ?
			reinterpret_cast<SokuLib::KeyManager *>(0x8986a8) :
			((SokuLib::KeyManager *(__thiscall *)(int, char))0x43e3b0)(0x899cec, *(char *)0x898678);
		FUN_00434bf0(&SokuLib::profile1, *(char *)0x898678);
		info.keyManager = (SokuLib::KeyManager *)0x008989A0;
	}

	info.effectiveDeck.clear();
	for (auto card : deck)
		info.effectiveDeck.push_back(card);
	return true;
}

bool prepareReplayBuffer(const std::string &path, const char *folder)
{
	std::ifstream stream{path};
	nlohmann::json value;

	if (stream.fail()) {
		MessageBox(
			SokuLib::window,
			("Cannot load file " + path + ": " + strerror(errno)).c_str(),
			"Trial loading error",
			MB_ICONERROR
		);
		return false;
	}

	try {
		stream >> value;
	} catch (std::exception &e) {
		MessageBox(
			SokuLib::window,
			("File " + path + " is not valid: " + e.what() + ".\n").c_str(),
			"Trial loading error",
			MB_ICONERROR
		);
		return false;
	}

	if (!checkField("player", value, &nlohmann::json::is_object))
		return false;
	if (!checkField("dummy", value, &nlohmann::json::is_object))
		return false;
	if (!checkField("stage", value, &nlohmann::json::is_number))
		return false;
	if (!value.contains("music") || (!value["music"].is_number() && !value["music"].is_string())) {
		MessageBox(
			SokuLib::window,
			"The field \"music\" is not valid but is mandatory.",
			"Loading error",
			MB_ICONERROR
		);
		return false;
	}
	if (!checkField("type", value, &nlohmann::json::is_string))
		return false;

	if (!addCharacterToBuffer("player", value["player"], SokuLib::leftPlayerInfo, false))
		return false;
	if (!addCharacterToBuffer("dummy", value["dummy"], SokuLib::rightPlayerInfo, true))
		return false;

	*(char *)0x899D0C = value["stage"].get<char>();
	if (value["music"].is_number())
		*(char *)0x899D0D = value["music"].get<char>();
	else {
		char nb = 6;
		std::string str = value["music"];

		if (str.size() == strlen("data/bgm/st00.ogg") && str.substr(0, 11) == "data/bgm/st" && str.substr(13) == ".ogg")
			try {
				nb = std::stoul(str.substr(11, 2));
			} catch (...) {}
		*(char *)0x899D0D = nb;
	}
	try {
		loadedTrial.reset(Trial::create(folder, SokuLib::leftPlayerInfo.character, value));
	} catch (std::exception &e) {
		MessageBox(
			SokuLib::window,
			("File " + path + " is not valid: " + e.what() + ".\n").c_str(),
			"Trial loading error",
			MB_ICONERROR
		);
		return false;
	}
	return true;
}

bool prepareReplayBufferEditor(const std::string &path, const char *folder)
{
	std::ifstream stream{path};
	nlohmann::json value;

	if (stream.fail()) {
		MessageBox(
			SokuLib::window,
			("Cannot load file " + path + ": " + strerror(errno)).c_str(),
			"Trial loading error",
			MB_ICONERROR
		);
		return false;
	}

	try {
		stream >> value;
	} catch (std::exception &e) {
		MessageBox(
			SokuLib::window,
			("File " + path + " is not valid: " + e.what() + ".\n").c_str(),
			"Trial loading error",
			MB_ICONERROR
		);
		return false;
	}

	if (!checkField("type", value, &nlohmann::json::is_string))
		return false;

	addCharacterToBufferEditor("player", value, SokuLib::leftPlayerInfo, false);
	addCharacterToBufferEditor("dummy", value, SokuLib::rightPlayerInfo, true);

	*(char *)0x899D0C = getField(value, 6, &nlohmann::json::is_number, "stage");
	if (value.contains("music")) {
		if (value["music"].is_number())
			*(char *)0x899D0D = value["music"].get<char>();
		else {
			char nb = 6;
			std::string str = value["music"];

			if (str.size() == strlen("data/bgm/st00.ogg") && str.substr(0, 11) == "data/bgm/st" && str.substr(13) == ".ogg")
				try {
					nb = std::stoul(str.substr(11, 2));
				} catch (...) {}
			*(char *)0x899D0D = nb;
		}
	} else
		*(char *)0x899D0D = 6;

	try {
		loadedTrial.reset(TrialEditor::create(folder, path.c_str(), SokuLib::leftPlayerInfo.character, value));
	} catch (std::exception &e) {
		MessageBox(
			SokuLib::window,
			("File " + path + " is not valid: " + e.what() + ".\n").c_str(),
			"Trial loading error",
			MB_ICONERROR
		);
		return false;
	}
	return true;
}

void prepareGameLoading(const char *folder, const std::string &path)
{
	SokuLib::setBattleMode(SokuLib::BATTLE_MODE_VSPLAYER, SokuLib::BATTLE_SUBMODE_PLAYING1);
	if ((editorMode && !prepareReplayBufferEditor(path, folder)) || (!editorMode && !prepareReplayBuffer(path, folder)))
		return;
	loadRequest = true;
}

std::vector<unsigned> getCurrentPackScores()
{
	std::vector<unsigned> scores;

	scores.reserve(loadedPacks[currentPack]->scenarios.size());
	for (auto &scenario : loadedPacks[currentPack]->scenarios)
		scores.push_back(static_cast<unsigned char>(scenario->score));
	return scores;
}

ResultMenu::ResultMenu(int score)
{
	loadedPacks[currentPack]->scenarios[currentEntry]->setScore(max(loadedPacks[currentPack]->scenarios[currentEntry]->score, score));
	saveScores();

	this->_resultTop.texture.loadFromGame("data/infoeffect/result/resultTitle.bmp");
	this->_resultTop.setPosition({128, 94});
	this->_resultTop.setSize(this->_resultTop.texture.getSize());
	this->_resultTop.rect.width = this->_resultTop.texture.getSize().x;
	this->_resultTop.rect.height = this->_resultTop.texture.getSize().y;

	this->_title.texture.loadFromGame("data/menu/result/result.bmp");
	this->_title.setSize(this->_title.texture.getSize());
	this->_title.rect.width = this->_title.texture.getSize().x;
	this->_title.rect.height = this->_title.texture.getSize().y;

	this->_score.texture.loadFromGame("data/infoeffect/result/rankFont.bmp");
	this->_score.setPosition({378, 164});
	this->_score.setSize({128, 128});
	this->_score.rect.left = score * this->_score.texture.getSize().x / 4;
	this->_score.rect.width = this->_score.texture.getSize().x / 4;
	this->_score.rect.height = this->_score.texture.getSize().y;

	for (int i = currentEntry + 1; i < loadedPacks[currentPack]->scenarios.size(); i++)
		if (!loadedPacks[currentPack]->scenarios[i]->extra) {
			this->_done = false;
			break;
		}

	this->_done &= !loadedPacks[currentPack]->scenarios[currentEntry]->extra;
	for (int i = 0; i < TrialBase::menuActionText.size(); i++) {
		auto &sprite = this->_text[i];

		sprite.texture.createFromText(i == 0 && this->_done && !loadedPacks[currentPack]->outroPath.empty() ? "Watch pack outro" : TrialBase::menuActionText[i].c_str(), defaultFont16, {230, 24});
		sprite.setPosition({128, 182 + i * 24});
		sprite.setSize(sprite.texture.getSize());
		sprite.rect.width = sprite.texture.getSize().x;
		sprite.rect.height = sprite.texture.getSize().y;
	}
	if (!this->_done || loadedPacks[currentPack]->outroPath.empty())
		if (currentEntry == loadedPacks[currentPack]->scenarios.size() - 1 || isLocked(currentEntry + 1)) {
			this->_text[0].tint = SokuLib::DrawUtils::DxSokuColor{0x40, 0x40, 0x40};
			this->_disabled = true;
		}
	this->_selected = this->_disabled;
}

void ResultMenu::_()
{
	puts("_ !");
	*(int *)0x882a94 = 0x16;
}

int ResultMenu::onProcess()
{
	if (SokuLib::inputMgrs.input.b == 1 || SokuLib::checkKeyOneshot(DIK_ESCAPE, 0, 0, 0)) {
		SokuLib::playSEWaveBuffer(0x29);
		this->_selected = TrialBase::RETURN_TO_TITLE_SCREEN;
	}
	if (SokuLib::inputMgrs.input.a == 1) {
		if (this->_selected == TrialBase::GO_TO_NEXT_TRIAL) {
			if (this->_disabled) {
				SokuLib::playSEWaveBuffer(0x29);
				return true;
			}

			auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

			if (!loadedPacks[currentPack]->outroPath.empty() && !scenario->extra && this->_done) {
				try {
					loadedOutro.reset(new PackOutro(loadedPacks[currentPack]->path, loadedPacks[currentPack]->outroPath));
				} catch (std::exception &e) {
					SokuLib::playSEWaveBuffer(0x29);
					MessageBox(SokuLib::window, ("Error when loading pack outro: " + std::string(e.what())).c_str(), "Pack outro loading error", MB_ICONERROR);
					return true;
				}
			} else
				loadNextTrial = true;
		}
		SokuLib::playSEWaveBuffer(0x28);
		loadedTrial->onMenuClosed(static_cast<TrialBase::MenuAction>(this->_selected));
		return false;
	}
	if (SokuLib::inputMgrs.input.verticalAxis == -1 || (SokuLib::inputMgrs.input.verticalAxis <= -36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
		SokuLib::playSEWaveBuffer(0x27);
		this->_selected--;
		this->_selected += TrialBase::NB_MENU_ACTION;
		this->_selected %= TrialBase::NB_MENU_ACTION;
	} else if (SokuLib::inputMgrs.input.verticalAxis == 1 || (SokuLib::inputMgrs.input.verticalAxis >= 36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
		SokuLib::playSEWaveBuffer(0x27);
		this->_selected++;
		this->_selected %= TrialBase::NB_MENU_ACTION;
	}
	return true;
}

int ResultMenu::onRender()
{
	this->_title.draw();
	this->_resultTop.draw();
	this->_score.draw();
	//Display the green gradiant cursor bar
	displaySokuCursor(
		{128, 183 + this->_selected * 24},
		{300, 16}
	);
	for (auto &sprite : this->_text)
		sprite.draw();
	return 0;
}


void loadFont()
{
	SokuLib::FontDescription desc;

	desc.r1 = 255;
	desc.r2 = 255;
	desc.g1 = 255;
	desc.g2 = 255;
	desc.b1 = 255;
	desc.b2 = 255;
	desc.height = 10 + hasEnglishPatch * 2;
	desc.weight = FW_NORMAL;
	desc.italic = 0;
	desc.shadow = 1;
	desc.bufferSize = 1000000;
	desc.charSpaceX = hasEnglishPatch * -1;
	desc.charSpaceY = hasEnglishPatch * -2;
	desc.offsetX = 0;
	desc.offsetY = 0;
	desc.useOffset = 0;
	strcpy(desc.faceName, "MonoSpatialModSWR");
	desc.weight = FW_REGULAR;
	defaultFont10.create();
	defaultFont10.setIndirect(desc);

	desc.height = 12 + hasEnglishPatch * 2;
	defaultFont12.create();
	defaultFont12.setIndirect(desc);

	desc.height = 16 + hasEnglishPatch * 2;
	defaultFont16.create();
	defaultFont16.setIndirect(desc);
}

void menuLoadAssets()
{
	if (loaded)
		return;
	loaded = true;
	loadAllExistingCards();
	puts("Loading assets");
	hasEnglishPatch = (*(int *)0x411c64 == 1);
	loadFont();

	rect.setSize({640, 480});
	rect.setFillColor({0x00, 0x00, 0x00, 0x00});
	rect.setBorderColor({0x00, 0x00, 0x00, 0x00});
	previewContainer.texture.loadFromGame("data/menu/profile_list_seat.bmp");
	previewContainer.rect = {
		0, 0,
		static_cast<int>(previewContainer.texture.getSize().x),
		static_cast<int>(previewContainer.texture.getSize().y),
	};
	previewContainer.setPosition({310, 92});
	previewContainer.setSize({365, 345});

	packContainer.texture.loadFromResource(myModule, MAKEINTRESOURCE(4));
	packContainer.rect = {
		0, 0,
		static_cast<int>(packContainer.texture.getSize().x),
		static_cast<int>(packContainer.texture.getSize().y),
	};
	packContainer.setSize(packContainer.texture.getSize());

	missingIcon.texture.loadFromResource(myModule, MAKEINTRESOURCE(8));
	missingIcon.rect = {
		0, 0,
		static_cast<int>(missingIcon.texture.getSize().x),
		static_cast<int>(missingIcon.texture.getSize().y),
	};
	missingIcon.setSize({32, 32});
	missingIcon.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor::White * 0.25;
	missingIcon.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor::White * 0.25;

	upArrow.texture.loadFromGame("data/scene/select/bg/bg_cursor0.bmp");
	upArrow.setSize(upArrow.texture.getSize());
	upArrow.rect.width = upArrow.texture.getSize().x;
	upArrow.rect.height = upArrow.texture.getSize().y;

	downArrow.texture.loadFromGame("data/scene/select/bg/bg_cursor1.bmp");
	downArrow.setSize(downArrow.texture.getSize());
	downArrow.rect.width = downArrow.texture.getSize().x;
	downArrow.rect.height = downArrow.texture.getSize().y;

	score.texture.loadFromGame("data/infoeffect/result/rankFont.bmp");
	score.setSize({32, 32});
	score.rect.width = score.texture.getSize().x / 4;
	score.rect.height = score.texture.getSize().y;

	extraImg.texture.loadFromGame("data/infoeffect/weatherOrbWhite.bmp");
	extraImg.setSize({32, 24});
	extraImg.rect.width = extraImg.texture.getSize().x;
	extraImg.rect.height = extraImg.texture.getSize().y;

	packEditPage.cursor.texture.loadFromGame("data/scene/title/2_menu_hari.bmp");
	packEditPage.cursor.setSize({13, 19});
	packEditPage.cursor.rect.width = packEditPage.cursor.texture.getSize().x;
	packEditPage.cursor.rect.height = packEditPage.cursor.texture.getSize().y;
	packEditPage.cursor.setRotation(-2.039668);

	packEditPage.cursorGear.texture.loadFromGame("data/scene/title/2_menu_gear.bmp");
	packEditPage.cursorGear.setSize({22, 22});
	packEditPage.cursorGear.rect.width = packEditPage.cursorGear.texture.getSize().x;
	packEditPage.cursorGear.rect.height = packEditPage.cursorGear.texture.getSize().y;

	title.texture.loadFromResource(myModule, MAKEINTRESOURCE(24));
	title.setSize(title.texture.getSize());
	title.rect.width = title.texture.getSize().x;
	title.rect.height = title.texture.getSize().y;

	lock.texture.loadFromResource(myModule, MAKEINTRESOURCE(28));
	lock.setSize({32, 32});
	lock.rect.width = lock.texture.getSize().x;
	lock.rect.height = lock.texture.getSize().y;

	lockedImg.texture.loadFromResource(myModule, MAKEINTRESOURCE(32));
	lockedImg.setSize({200, 150});
	lockedImg.rect.width = lockedImg.texture.getSize().x;
	lockedImg.rect.height = lockedImg.texture.getSize().y;
	lockedImg.setPosition({398, 128});

	frame.texture.loadFromResource(myModule, MAKEINTRESOURCE(36));
	frame.setSize({212, 162});
	frame.rect.width = frame.texture.getSize().x;
	frame.rect.height = frame.texture.getSize().y;
	frame.setPosition({392, 122});

	blackSilouettes.texture.loadFromResource(myModule, MAKEINTRESOURCE(40));
	blackSilouettes.setSize({200, 150});
	blackSilouettes.rect.width = blackSilouettes.texture.getSize().x;
	blackSilouettes.rect.height = blackSilouettes.texture.getSize().y;
	blackSilouettes.setPosition({398, 128});

	wrench.texture.loadFromResource(myModule, MAKEINTRESOURCE(44));
	wrench.setSize({32, 32});
	wrench.rect.width = wrench.texture.getSize().x;
	wrench.rect.height = wrench.texture.getSize().y;

	editSeat.texture.loadFromResource(myModule, MAKEINTRESOURCE(56));
	editSeat.setSize(editSeat.texture.getSize());
	editSeat.setPosition({
		(640 - static_cast<int>(editSeat.getSize().x)) / 2,
		(480 - static_cast<int>(editSeat.getSize().y)) / 2
	});
	editSeat.rect.width = editSeat.texture.getSize().x;
	editSeat.rect.height = editSeat.texture.getSize().y;

	editSeatForeground.texture.loadFromResource(myModule, MAKEINTRESOURCE(60));
	editSeatForeground.setSize(editSeatForeground.texture.getSize());
	editSeatForeground.setPosition({
		(640 - static_cast<int>(editSeatForeground.getSize().x)) / 2,
		(480 - static_cast<int>(editSeatForeground.getSize().y)) / 2
	});
	editSeatForeground.rect.width = editSeatForeground.texture.getSize().x;
	editSeatForeground.rect.height = editSeatForeground.texture.getSize().y;

	stickTop.texture.loadFromResource(myModule, MAKEINTRESOURCE(64));
	stickTop.setPosition({464, 96});
	stickTop.setSize({64, 64});
	stickTop.rect.width = stickTop.texture.getSize().x;
	stickTop.rect.height = stickTop.texture.getSize().y;

	tickSprite.texture.loadFromResource(myModule, MAKEINTRESOURCE(68));
	tickSprite.setSize(tickSprite.texture.getSize());
	tickSprite.rect.width = tickSprite.texture.getSize().x;
	tickSprite.rect.height = tickSprite.texture.getSize().y;

	editSeatEmpty.texture.loadFromResource(myModule, MAKEINTRESOURCE(72));
	editSeatEmpty.setSize(editSeat.texture.getSize());
	editSeatEmpty.setPosition({
		(640 - static_cast<int>(editSeatEmpty.getSize().x)) / 2,
		(480 - static_cast<int>(editSeatEmpty.getSize().y)) / 2
	});
	editSeatEmpty.rect.width = editSeatEmpty.texture.getSize().x;
	editSeatEmpty.rect.height = editSeatEmpty.texture.getSize().y;

	editScenarioSeat.texture.loadFromResource(myModule, MAKEINTRESOURCE(76));
	editScenarioSeat.setSize(editScenarioSeat.texture.getSize());
	editScenarioSeat.setPosition({122, 226});
	editScenarioSeat.rect.width = editScenarioSeat.texture.getSize().x;
	editScenarioSeat.rect.height = editScenarioSeat.texture.getSize().y;

	explorerLoadAssets();
	inputBoxLoadAssets();

	lockedText.setSize({300, 150});
	lockedText.rect.width = 300;
	lockedText.rect.height = 150;
	lockedText.setPosition({356, 286});

	extraText.texture.createFromText("Unlocked by getting a perfect rank for each<br>trial of this pack", defaultFont12, {300, 150});
	extraText.setSize({300, 150});
	extraText.rect.width = 300;
	extraText.rect.height = 150;
	extraText.setPosition({356, 286});

	questionMarks.texture.createFromText("????????????????", defaultFont12, {0x100, 15});
	questionMarks.setSize(questionMarks.texture.getSize());
	questionMarks.rect.width = questionMarks.texture.getSize().x;
	questionMarks.rect.height = questionMarks.texture.getSize().y;
	questionMarks.tint = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0x80};

	arrowSprite.texture.loadFromGame("data/profile/deck2/sayuu.bmp");
	arrowSprite.setSize({
		arrowSprite.texture.getSize().x / 2,
		arrowSprite.texture.getSize().y
	});
	arrowSprite.rect.width = arrowSprite.texture.getSize().x / 2;
	arrowSprite.rect.height = arrowSprite.texture.getSize().y;

	loadingGear.texture.loadFromGame("data/scene/logo/gear.bmp");
	loadingGear.setSize({
		loadingGear.texture.getSize().x,
		loadingGear.texture.getSize().y
	});
	loadingGear.rect.width = loadingGear.texture.getSize().x;
	loadingGear.rect.height = loadingGear.texture.getSize().y;

	version.texture.createFromText(
#ifdef _DEBUG
		"Version " VERSION_STR " DEBUG",
#else
		"Version " VERSION_STR,
#endif
		defaultFont10, {300, 20}, &versionStrSize
	);
	version.setSize({
		static_cast<unsigned int>(versionStrSize.x),
		static_cast<unsigned int>(versionStrSize.y)
	});
	version.rect.width = versionStrSize.x;
	version.rect.height = versionStrSize.y;

	nameFilterText.texture.createFromText("Any name",  defaultFont12, {300, 20}, &nameFilterSize);
	nameFilterText.setSize({
		nameFilterText.texture.getSize().x,
		nameFilterText.texture.getSize().y
	});
	nameFilterText.rect.width = nameFilterText.texture.getSize().x;
	nameFilterText.rect.height = nameFilterText.texture.getSize().y;

	modeFilterText.texture.createFromText("Any mode",  defaultFont12, {300, 20}, &modeFilterSize);
	modeFilterText.setSize({
		modeFilterText.texture.getSize().x,
		modeFilterText.texture.getSize().y
	});
	modeFilterText.rect.width = modeFilterText.texture.getSize().x;
	modeFilterText.rect.height = modeFilterText.texture.getSize().y;

	topicFilterText.texture.createFromText("Any topic", defaultFont12, {300, 20}, &topicFilterSize);
	topicFilterText.setSize({
		topicFilterText.texture.getSize().x,
		topicFilterText.texture.getSize().y
	});
	topicFilterText.rect.width = topicFilterText.texture.getSize().x;
	topicFilterText.rect.height = topicFilterText.texture.getSize().y;

	int id;
	HRESULT ret;

	pphandle = SokuLib::textureMgr.allocate(&id);
	if (FAILED(ret = D3DXCreateTexture(SokuLib::pd3dDev, 200, 150, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, pphandle))) {
		pphandle = nullptr;
		SokuLib::textureMgr.deallocate(id);
		fprintf(stderr, "D3DXCreateTexture(SokuLib::pd3dDev, 200, 150, D3DX_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, %p) failed with code %li\n", pphandle, ret);
		goto failed;
	}
	lockedNoise.texture.setHandle(id, {200, 150});
	lockedNoise.setSize({200, 150});
	lockedNoise.setPosition({398, 128});
	lockedNoise.rect.width = 200;
	lockedNoise.rect.height = 150;

failed:
	pphandle2 = SokuLib::textureMgr.allocate(&id);
	if (FAILED(ret = D3DXCreateTexture(SokuLib::pd3dDev, 200, 150, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, pphandle2))) {
		pphandle = nullptr;
		SokuLib::textureMgr.deallocate(id);
		fprintf(stderr, "D3DXCreateTexture(SokuLib::pd3dDev, 200, 150, D3DX_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, %p) failed with code %li\n", pphandle2, ret);
		goto failed2;
	}
	CRTBands.texture.setHandle(id, {200, 150});
	CRTBands.setSize({200, 150});
	CRTBands.setPosition({398, 128});
	CRTBands.rect.width = 200;
	CRTBands.rect.height = 150;

failed2:
	loadPacks();

	noEditorGuides.resize(3);
	noEditorGuides[0] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(300));
	noEditorGuides[1] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(304));
	noEditorGuides[2] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(308));
	editorGuides.resize(8);
	editorGuides[0] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(312));
	editorGuides[1] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(316));
	editorGuides[2] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(320));
	editorGuides[3] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(324));
	editorGuides[4] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(328));
	editorGuides[5] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(332));
	editorGuides[6] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(336));
	editorGuides[7] = std::make_unique<Guide>(myModule, MAKEINTRESOURCE(340));

	if (
		nbPacks != loadedPacks.size() ||
		nbName != uniqueNames.size() ||
		nbMode != uniqueModes.size() ||
		nbTopic != uniqueCategories.size()
	) {
		currentPack = -loadedPacks.empty();
		noEditorGuides[loadedPacks.empty()]->active = true;
		for (auto &guide : noEditorGuides)
			guide->reset();
		for (auto &guide : editorGuides)
			guide->reset();
		currentEntry = -1;
		shownPack = 0;
		nameFilter = -1;
		modeFilter = -1;
		topicFilter = -1;
		packStart = 0;
		entryStart = 0;
		nbPacks = loadedPacks.size();
		nbName = uniqueNames.size();
		nbMode = uniqueModes.size();
		nbTopic = uniqueCategories.size();
		expended = false;
	} else {
		if (expended)
			noEditorGuides[2]->active = true;
		else
			noEditorGuides[currentPack < 0]->active = true;
		for (auto &guide : noEditorGuides)
			guide->reset();
		for (auto &guide : editorGuides)
			guide->reset();
		nameFilterText.texture.createFromText( nameFilter == -1  ? "Any name" : uniqueNames[nameFilter].c_str(), defaultFont12, {300, 20}, &nameFilterSize);
		modeFilterText.texture.createFromText( modeFilter == -1  ? "Any mode" : uniqueModes[modeFilter].c_str(), defaultFont12, {300, 20}, &modeFilterSize);
		topicFilterText.texture.createFromText(topicFilter == -1 ? "Any topic" : uniqueCategories[topicFilter].c_str(), defaultFont12, {300, 20}, &topicFilterSize);
	}

	if (currentEntry != -1 && isLocked(currentEntry)) {
		auto &other = loadedPacks[currentPack]->scenarios[currentEntry - 1];

		lockedText.texture.createFromText(("Unlocked by completing " + (isLocked(currentEntry - 1) && other->nameHiddenIfLocked ? std::string("????????????????") : other->nameStr)).c_str(), defaultFont12, {300, 150});
	} else if (currentEntry == -1 && loadedPacks.size() < shownPack && isLocked(*loadedPacks[shownPack]))
		lockedText.texture.createFromText(("Unlocked by completing " + loadedPacks[shownPack]->requirement + "'s<br>episode").c_str(), defaultFont12, {300, 150});

	if (chrs.empty()) {
		if (!SokuLib::SWRUnlinked)
			for (auto &infos: swrCharacters)
				chrs[infos.first] = infos.second;
		for (auto &infos: validCharacters)
			chrs[infos.first] = infos.second;
	}

	int i = 0;

	charactersFaces.reserve(chrs.size());
	for (auto &chr : chrs) {
		auto *sprite = new SokuLib::DrawUtils::Sprite();

		charactersFaces.emplace_back(sprite);
		orderChrs.push_back(chr.second);
		sprite->texture.loadFromGame(("data/character/" + chr.second + "/face/face000.bmp").c_str());
		sprite->rect.width = sprite->texture.getSize().x;
		sprite->rect.height = sprite->texture.getSize().y;
		sprite->setSize({80, 32});
		sprite->setPosition({
			140 + 100 * (i / 9),
			60 + 40 * (i % 9),
		});
		i++;
	}
}

#define NOISE_DELTA 50
#define RANDOM_VAL rand() % ((NOISE_DELTA) * 2 + 1)
#define RANDOM(r) ((r) + (RANDOM_VAL) - (NOISE_DELTA))

static void updateNoiseTexture(SokuLib::DrawUtils::DxSokuColor *array)
{
	unsigned char r = rand() % 206;

	for (int y = 0; y < 150; y++)
		for (int x = 0; x < 200; x++) {
			int g = RANDOM((int)r);

			if (g > 205)
				r = 205;
			else if (g < 0)
				r = 0;
			else
				r = g;
			array[x + y * 200] = SokuLib::DrawUtils::DxSokuColor{r, r, r};
		}
}

static void updateBandTexture(SokuLib::DrawUtils::DxSokuColor *array)
{
	static bool b = false;

	b = !b;
	for (int y = 0; y < 150; y++)
		for (int x = 0; x < 200; x++)
			array[x + y * 200] = SokuLib::DrawUtils::DxSokuColor{0xFF, 0xFF, 0xFF, 0x00};
	band1Start += 1 + b;
	band2Start += 1;
	if (band1Start > 220)
		band1Start = 0;
	if (band2Start > 255)
		band2Start = 0;

	for (int y = -10; y; y++) {
		if (band1Start < -y)
			continue;
		if (band1Start + y >= 150)
			break;
		for (int x = 0; x < 200; x++)
			array[x + (band1Start + y) * 200].a += 0x55;
	}
	for (int y = -20; y; y++) {
		if (band2Start < -y)
			continue;
		if (band2Start + y >= 150)
			break;
		for (int x = 0; x < 200; x++)
			array[x + (band2Start + y) * 200].a += 0x80;
	}
}

void updateNoiseTexture()
{
	HRESULT ret;
	D3DLOCKED_RECT r;

	if (!pphandle)
		return;
	if (FAILED(ret = (*pphandle)->LockRect(0, &r, nullptr, 0))) {
		fprintf(stderr, "(*pphandle)->LockRect(0, &r, nullptr, D3DLOCK_DISCARD) failed with code %li\n", ret);
		return;
	}
	updateNoiseTexture(reinterpret_cast<SokuLib::DrawUtils::DxSokuColor *>(r.pBits));
	if (FAILED(ret = (*pphandle)->UnlockRect(0)))
		fprintf(stderr, "(*pphandle)->UnlockRect(0) failed with code %li\n", ret);
}

void updateBandTexture()
{
	HRESULT ret;
	D3DLOCKED_RECT r;

	if (!pphandle2)
		return;
	if (FAILED(ret = (*pphandle2)->LockRect(0, &r, nullptr, 0))) {
		fprintf(stderr, "(*pphandle2)->LockRect(0, &r, nullptr, D3DLOCK_DISCARD) failed with code %li\n", ret);
		return;
	}
	updateBandTexture(reinterpret_cast<SokuLib::DrawUtils::DxSokuColor *>(r.pBits));
	if (FAILED(ret = (*pphandle2)->UnlockRect(0)))
		fprintf(stderr, "(*pphandle2)->UnlockRect(0) failed with code %li\n", ret);
}

void menuUnloadAssets()
{
	if (!loaded)
		return;
	loaded = false;
	puts("Unloading assets");

	defaultFont10.destruct();
	defaultFont12.destruct();
	defaultFont16.destruct();

	lock.texture.destroy();
	upArrow.texture.destroy();
	downArrow.texture.destroy();
	title.texture.destroy();
	score.texture.destroy();
	arrowSprite.texture.destroy();
	missingIcon.texture.destroy();
	packContainer.texture.destroy();
	questionMarks.texture.destroy();
	nameFilterText.texture.destroy();
	modeFilterText.texture.destroy();
	topicFilterText.texture.destroy();
	previewContainer.texture.destroy();
	lockedNoise.texture.destroy();
	lockedText.texture.destroy();
	lockedImg.texture.destroy();
	extraText.texture.destroy();
	extraImg.texture.destroy();
	frame.texture.destroy();
	CRTBands.texture.destroy();
	loadingGear.texture.destroy();
	blackSilouettes.texture.destroy();
	editSeat.texture.destroy();
	editScenarioSeat.texture.destroy();
	editSeatEmpty.texture.destroy();
	stickTop.texture.destroy();
	packEditPage.name.texture.destroy();
	packEditPage.category.texture.destroy();
	packEditPage.description.texture.destroy();
	packEditPage.previewPath.texture.destroy();
	packEditPage.iconPath.texture.destroy();
	packEditPage.modes.texture.destroy();
	packEditPage.characters.texture.destroy();
	packEditPage.author.texture.destroy();
	packEditPage.version.texture.destroy();
	packEditPage.endingPath.texture.destroy();
	packEditPage.cursor.texture.destroy();
	packEditPage.cursorGear.texture.destroy();
	packEditScenario.name.texture.destroy();
	packEditScenario.filePath.texture.destroy();
	packEditScenario.previewPath.texture.destroy();

	charactersFaces.clear();
	loadedPacks.clear();
	uniqueNames.clear();
	uniqueCategories.clear();
	uniqueModes.clear();
	packsByName.clear();
	packsByCategory.clear();
	noEditorGuides.clear();
	editorGuides.clear();

	explorerUnloadAssets();
	inputBoxUnloadAssets();
	editorMode = false;
	packEditScenario.opened = false;
	packEditPage.opened = false;
	packEditPage.selectingPos = false;
	packEditPage.selectingScale = false;
}

static void displayFilters()
{
	arrowSprite.rect.left = 0;
	arrowSprite.setPosition({
		static_cast<int>(100 - FILTER_TEXT_SIZE / 2 - arrowSprite.getSize().x),
		70
	});
	arrowSprite.draw();
	arrowSprite.setPosition({
		static_cast<int>(320 - FILTER_TEXT_SIZE / 2 - arrowSprite.getSize().x),
		80
	});
	arrowSprite.draw();
	arrowSprite.setPosition({
		static_cast<int>(540 - FILTER_TEXT_SIZE / 2 - arrowSprite.getSize().x),
		90
	});
	arrowSprite.draw();

	arrowSprite.rect.left = arrowSprite.texture.getSize().x / 2;
	arrowSprite.setPosition({
		static_cast<int>(100 + FILTER_TEXT_SIZE / 2),
		70
	});
	arrowSprite.draw();
	arrowSprite.setPosition({
		static_cast<int>(320 + FILTER_TEXT_SIZE / 2),
		80
	});
	arrowSprite.draw();
	arrowSprite.setPosition({
		static_cast<int>(540 + FILTER_TEXT_SIZE / 2),
		90
	});
	arrowSprite.draw();

	if (currentPack == -1)
		displaySokuCursor(
			{540 - FILTER_TEXT_SIZE / 2, static_cast<int>(90 + arrowSprite.getSize().y / 2 - 7)},
			{FILTER_TEXT_SIZE + FILTER_TEXT_SIZE / 2, 16}
		);
	else if (currentPack == -2)
		displaySokuCursor(
			{320 - FILTER_TEXT_SIZE / 2, static_cast<int>(80 + arrowSprite.getSize().y / 2 - 7)},
			{FILTER_TEXT_SIZE + FILTER_TEXT_SIZE / 2, 16}
		);
	else if (currentPack == -3)
		displaySokuCursor(
			{100 - FILTER_TEXT_SIZE / 2, static_cast<int>(70 + arrowSprite.getSize().y / 2 - 7)},
			{FILTER_TEXT_SIZE + FILTER_TEXT_SIZE / 2, 16}
		);

	nameFilterText.setPosition({
		static_cast<int>(100 - nameFilterSize.x / 2),
		static_cast<int>(70 + arrowSprite.getSize().y / 2 - nameFilterSize.y / 2)
	});
	nameFilterText.draw();
	modeFilterText.setPosition({
		static_cast<int>(320 - modeFilterSize.x / 2),
		static_cast<int>(80 + arrowSprite.getSize().y / 2 - modeFilterSize.y / 2)
	});
	modeFilterText.draw();
	topicFilterText.setPosition({
		static_cast<int>(540 - topicFilterSize.x / 2),
		static_cast<int>(90 + arrowSprite.getSize().y / 2 - topicFilterSize.y / 2)
	});
	topicFilterText.draw();
}

static void switchEditorMode()
{
	if (movingScenario)
		return;
	editorMode = !editorMode;

	auto sound = editorMode ? 48 : 41;

	if (currentPack >= 0 && isLocked(*loadedPacks[currentPack])) {
		expended = false;
		currentEntry = -1;
	}
	SokuLib::playSEWaveBuffer(sound);
	packEditScenario.opened = false;
	packEditPage.opened = false;
	packEditPage.selectingPos = false;
	packEditPage.selectingScale = false;
	if (editorMode)
		for (int i = 0 ; i < noEditorGuides.size(); i++) {
			editorGuides[i]->active = noEditorGuides[i]->active;
			noEditorGuides[i]->active = false;
		}
	else {
		for (int i = 0; i < noEditorGuides.size(); i++) {
			noEditorGuides[i]->active = editorGuides[i]->active;
			editorGuides[i]->active = false;
		}
		for (auto &guide : editorGuides)
			guide->active = false;

		if (currentEntry != -1 && isLocked(currentEntry)) {
			auto &other = loadedPacks[currentPack]->scenarios[currentEntry - 1];

			lockedText.texture.createFromText(("Unlocked by completing " + (isLocked(currentEntry - 1) && other->nameHiddenIfLocked ? std::string("????????????????") : other->nameStr)).c_str(), defaultFont12, {300, 150});
		} else if (currentEntry == -1 && loadedPacks.size() < shownPack && isLocked(*loadedPacks[shownPack]))
			lockedText.texture.createFromText(("Unlocked by completing " + loadedPacks[shownPack]->requirement + "'s<br>episode").c_str(), defaultFont12, {300, 150});
	}
}

void checkScrollDown()
{
	if (currentPack < 0) {
		packStart = 0;
		entryStart = 0;
		return;
	}
	if (currentPack >= 0 && currentEntry == -1) {
		if (expended) {
			auto newStart = max(0, min(currentPack, 1.f * currentPack - static_cast<int>(264 - (currentPack == loadedPacks.size() - 1 ? 0 : 20) - 25 - (expended ? 15.f * loadedPacks[currentPack]->scenarios.size() : 0)) / 35));

			packStart = max(packStart, newStart);
		} else if (currentPack - packStart > 6)
			packStart = currentPack - 6;
		entryStart = 0;
		return;
	}
	if (currentEntry - entryStart > 15)
		entryStart = currentEntry - 15;
}

void checkScrollUp()
{
	if (currentPack < 0) {
		entryStart = 0;
		return;
	}
	if (currentEntry == -1) {
		if (currentPack < packStart)
			packStart = currentPack;
		else if (currentPack - packStart > 6)
			packStart = currentPack - 6;
		return;
	}
	if (currentEntry == loadedPacks[currentPack]->scenarios.size() - 1) {
		packStart = max(0, min(currentPack, 1.f * currentPack - static_cast<int>(264 - (currentPack == loadedPacks.size() - 1 ? 0 : 20) - 35 - 15.f * loadedPacks[currentPack]->scenarios.size()) / 35));
		if (currentEntry > 15)
			entryStart = currentEntry - 15;
		return;
	}
	if (currentEntry < entryStart)
		entryStart = currentEntry;
}

static void handleGoLeft()
{
	if (currentPack >= 0)
		return;
	SokuLib::playSEWaveBuffer(0x27);
	switch (currentPack) {
	case -3:
		nameFilter--;
		if (nameFilter == -2)
			nameFilter = uniqueNames.size() - 1;
		nameFilterText.texture.createFromText( nameFilter == -1  ? "Any name" : uniqueNames[nameFilter].c_str(),  defaultFont12, {300, 20}, &nameFilterSize);
		return;
	case -2:
		modeFilter--;
		if (modeFilter == -2)
			modeFilter = uniqueModes.size() - 1;
		modeFilterText.texture.createFromText( modeFilter == -1  ? "Any mode" : uniqueModes[modeFilter].c_str(),  defaultFont12, {300, 20}, &modeFilterSize);
		return;
	case -1:
		topicFilter--;
		if (topicFilter == -2)
			topicFilter = uniqueCategories.size() - 1;
		topicFilterText.texture.createFromText(topicFilter == -1 ? "Any topic" : uniqueCategories[topicFilter].c_str(), defaultFont12, {300, 20}, &topicFilterSize);
		return;
	default:
		return;
	}
}

static void handleGoRight()
{
	if (currentPack >= 0)
		return;
	SokuLib::playSEWaveBuffer(0x27);
	switch (currentPack) {
	case -3:
		nameFilter++;
		if (nameFilter == uniqueNames.size())
			nameFilter = -1;
		nameFilterText.texture.createFromText( nameFilter == -1  ? "Any name" : uniqueNames[nameFilter].c_str(),  defaultFont12, {300, 20}, &nameFilterSize);
		return;
	case -2:
		modeFilter++;
		if (modeFilter == uniqueModes.size())
			modeFilter = -1;
		modeFilterText.texture.createFromText( modeFilter == -1  ? "Any mode" : uniqueModes[modeFilter].c_str(),  defaultFont12, {300, 20}, &modeFilterSize);
		return;
	case -1:
		topicFilter++;
		if (topicFilter == uniqueCategories.size())
			topicFilter = -1;
		topicFilterText.texture.createFromText(topicFilter == -1 ? "Any topic" : uniqueCategories[topicFilter].c_str(), defaultFont12, {300, 20}, &topicFilterSize);
		return;
	default:
		return;
	}
}

static void handleGoUp()
{
	SokuLib::playSEWaveBuffer(0x27);
	if (currentEntry == -1) {
		do {
			currentEntry = -1;
			currentPack--;
			if (currentPack == -4)
				currentPack += loadedPacks.size() + 3;
			if (currentPack >= 0) {
				//currentEntry += loadedPacks[currentPack]->scenarios.size();
				shownPack = currentPack;
			}
			if (currentPack < 0)
				break;

			auto &pack = loadedPacks[currentPack];

			if (nameFilter != -1 && pack->nameStr != uniqueNames[nameFilter])
				continue;
			if (modeFilter != -1 && std::find(pack->modes.begin(), pack->modes.end(), uniqueModes[modeFilter]) == pack->modes.end())
				continue;
			if (topicFilter != -1 && pack->category != uniqueCategories[topicFilter])
				continue;
			//if (!loadedPacks[currentPack]->scenarios[currentEntry]->loading && loadedPacks[currentPack]->scenarios[currentEntry]->preview)
			//	loadedPacks[currentPack]->scenarios[currentEntry]->preview->reset();
			//else
			//	loadedPacks[currentPack]->scenarios[currentEntry]->loadPreview();
			break;
		} while (true);
		for (auto &guide : (editorMode ? editorGuides : noEditorGuides))
			guide->active = false;
		(editorMode ? editorGuides : noEditorGuides)[currentPack < 0]->active = true;
	} else {
		auto oldEntry = currentEntry;

		currentEntry--;
		if (currentEntry == -1)
			currentEntry = loadedPacks[currentPack]->scenarios.size() - 1;
		if (!movingScenario) {
			loadedPacks[currentPack]->scenarios[oldEntry]->loaded = false;
			if (!loadedPacks[currentPack]->scenarios[oldEntry]->loading && loadedPacks[currentPack]->scenarios[oldEntry]->preview)
				loadedPacks[currentPack]->scenarios[oldEntry]->preview.reset();
			loadedPacks[currentPack]->scenarios[currentEntry]->loaded = true;
			loadedPacks[currentPack]->scenarios[currentEntry]->loadPreview();
		}
	}
	checkScrollUp();
	printf("Pack: %i, Entry %i, Shown %i\n", currentPack, currentEntry, shownPack);
	if (currentEntry != -1 && isLocked(currentEntry) && !editorMode) {
		auto &other = loadedPacks[currentPack]->scenarios[currentEntry - 1];

		lockedText.texture.createFromText(("Unlocked by completing " + (isLocked(currentEntry - 1) && other->nameHiddenIfLocked ? std::string("????????????????") : other->nameStr)).c_str(), defaultFont12, {300, 150});
	} else if (currentEntry == -1 && loadedPacks.size() < shownPack && isLocked(*loadedPacks[shownPack]) && !editorMode)
		lockedText.texture.createFromText(("Unlocked by completing " + loadedPacks[shownPack]->requirement + "'s<br>episode").c_str(), defaultFont12, {300, 150});
}

static void handleGoDown()
{
	SokuLib::playSEWaveBuffer(0x27);
	if (currentPack < 0 || !expended) {
		do {
			currentPack++;
			if (currentPack == loadedPacks.size())
				currentPack = -3;
			shownPack = max(0, currentPack);
			currentEntry = -1;
			if (currentPack < 0)
				break;

			auto &pack = loadedPacks[currentPack];

			if (nameFilter != -1 && pack->nameStr != uniqueNames[nameFilter])
				continue;
			if (modeFilter != -1 && std::find(pack->modes.begin(), pack->modes.end(), uniqueModes[modeFilter]) == pack->modes.end())
				continue;
			if (topicFilter != -1 && pack->category != uniqueCategories[topicFilter])
				continue;
			break;
		} while (true);
		for (auto &guide : (editorMode ? editorGuides : noEditorGuides))
			guide->active = false;
		(editorMode ? editorGuides : noEditorGuides)[currentPack < 0]->active = true;
	} else {
		auto oldEntry = currentEntry;

		currentEntry++;
		if (currentEntry == loadedPacks[currentPack]->scenarios.size()) {
			currentEntry = 0;
			if (expended) {
				auto newStart = max(0, min(currentPack, 1.f * currentPack - static_cast<int>(264 - (currentPack == loadedPacks.size() - 1 ? 0 : 20) - 25 - (expended ? 15.f * loadedPacks[currentPack]->scenarios.size() : 0)) / 35));

				packStart = max(packStart, newStart);
			} else if (currentPack - packStart > 6)
				packStart = currentPack - 6;
			entryStart = 0;
		}
		if (!movingScenario) {
			loadedPacks[currentPack]->scenarios[oldEntry]->loaded = false;
			if (!loadedPacks[currentPack]->scenarios[oldEntry]->loading && loadedPacks[currentPack]->scenarios[oldEntry]->preview)
				loadedPacks[currentPack]->scenarios[oldEntry]->preview.reset();
			loadedPacks[currentPack]->scenarios[currentEntry]->loaded = true;
			loadedPacks[currentPack]->scenarios[currentEntry]->loadPreview();
		}
	}
	checkScrollDown();
	printf("Pack: %i, Entry %i, Shown %i\n", currentPack, currentEntry, shownPack);
	if (currentEntry != -1 && isLocked(currentEntry) && !editorMode) {
		auto &other = loadedPacks[currentPack]->scenarios[currentEntry - 1];

		lockedText.texture.createFromText(("Unlocked by completing " + (isLocked(currentEntry - 1) && other->nameHiddenIfLocked ? std::string("????????????????") : other->nameStr)).c_str(), defaultFont12, {300, 150});
	} else if (currentEntry == -1 && loadedPacks.size() < shownPack && isLocked(*loadedPacks[shownPack]))
		lockedText.texture.createFromText(("Unlocked by completing " + loadedPacks[shownPack]->requirement + "'s<br>episode").c_str(), defaultFont12, {300, 150});
}

void openPackEditPage()
{
	auto &pack = loadedPacks[currentPack];
	SokuLib::Vector2i size;
	std::string result;
	size_t allocSize;

	SokuLib::playSEWaveBuffer(0x28);
	packEditPage.opened = true;
	for (auto &guide : editorGuides)
		guide->active = false;
	editorGuides[3]->active = true;

	packEditPage.name.texture.createFromText(pack->nameStr.c_str(), defaultFont12, {153, 23}, &size);
	packEditPage.name.setSize({133, 13});
	packEditPage.name.rect.width  = 133;
	packEditPage.name.rect.height = 13;
	packEditPage.name.setPosition({167, 66});
	packEditPage.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditPage.category.texture.createFromText(pack->category.c_str(), defaultFont12, {153, 23}, &size);
	packEditPage.category.setSize({133, 13});
	packEditPage.category.rect.width  = 133;
	packEditPage.category.rect.height = 13;
	packEditPage.category.setPosition({395, 66});
	packEditPage.category.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.category.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditPage.description.texture.createFromText(pack->descriptionStr.c_str(), defaultFont16, {381, 111}, &size);
	packEditPage.description.setSize({361, 91});
	packEditPage.description.rect.width  = 361;
	packEditPage.description.rect.height = 91;
	packEditPage.description.setPosition({167, 284});
	packEditPage.description.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.description.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditPage.previewPath.texture.createFromText(pack->previewPath.c_str(), defaultFont12, {153, 23}, &size);
	packEditPage.previewPath.setSize({133, 13});
	packEditPage.previewPath.rect.width  = 133;
	packEditPage.previewPath.rect.height = 13;
	packEditPage.previewPath.setPosition({167, 178});
	packEditPage.previewPath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.previewPath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditPage.iconPath.texture.createFromText(pack->icon ? pack->icon->path.c_str() : "", defaultFont12, {153, 23}, &size);
	packEditPage.iconPath.setSize({133, 13});
	packEditPage.iconPath.rect.width  = 133;
	packEditPage.iconPath.rect.height = 13;
	packEditPage.iconPath.setPosition({167, 105});
	packEditPage.iconPath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.iconPath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	allocSize = 0;
	for (int i = 0; i < pack->modes.size(); i++)
		allocSize += pack->modes[i].size() + (i != 0);
	packEditPage.tmpModesStr.clear();
	packEditPage.tmpModesStr.reserve(allocSize);
	for (int i = 0; i < pack->modes.size(); i++) {
		if (i != 0)
			packEditPage.tmpModesStr += ',';
		packEditPage.tmpModesStr += pack->modes[i];
	}
	packEditPage.modes.texture.createFromText(packEditPage.tmpModesStr.c_str(), defaultFont12, {153, 23}, &size);
	packEditPage.modes.setSize({133, 13});
	packEditPage.modes.rect.width  = 133;
	packEditPage.modes.rect.height = 13;
	packEditPage.modes.setPosition({395, 391});
	packEditPage.modes.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.modes.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	allocSize = 0;
	for (int i = 0; i < pack->characters.size(); i++)
		allocSize += pack->characters[i].size() + (i != 0);
	result.reserve(allocSize);
	for (int i = 0; i < pack->characters.size(); i++) {
		if (i != 0)
			result += ',';
		result += pack->characters[i];
	}
	packEditPage.characters.texture.createFromText(result.c_str(), defaultFont12, {381, 23}, &size);
	packEditPage.characters.setSize({361, 13});
	packEditPage.characters.rect.width  = 361;
	packEditPage.characters.rect.height = 13;
	packEditPage.characters.setPosition({167, 212});
	packEditPage.characters.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.characters.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditPage.author.texture.createFromText(pack->authorStr.c_str(), defaultFont12, {153, 23}, &size);
	packEditPage.author.setSize({133, 13});
	packEditPage.author.rect.width  = 133;
	packEditPage.author.rect.height = 13;
	packEditPage.author.setPosition({167, 249});
	packEditPage.author.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.author.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditPage.version.texture.createFromText(pack->minVersion.c_str(), defaultFont12, {153, 23}, &size);
	packEditPage.version.setSize({133, 13});
	packEditPage.version.rect.width  = 133;
	packEditPage.version.rect.height = 13;
	packEditPage.version.setPosition({395, 249});
	packEditPage.version.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.version.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditPage.endingPath.texture.createFromText(pack->outroRelPath.c_str(), defaultFont12, {153, 23}, &size);
	packEditPage.endingPath.setSize({133, 13});
	packEditPage.endingPath.rect.width  = 133;
	packEditPage.endingPath.rect.height = 13;
	packEditPage.endingPath.setPosition({167, 391});
	packEditPage.endingPath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditPage.endingPath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditPage.cursor.setPosition({static_cast<int>(161 + (pack->icon ? 77 * pack->icon->scale : 77)), 136});
	packEditPage.cursorGear.setPosition({static_cast<int>(153 + (pack->icon ? 77 * pack->icon->scale : 77)), 141});

	packEditPage.rect.setSize({14, 14});
	packEditPage.rect.setFillColor(SokuLib::Color::White);
	packEditPage.rect.setBorderColor(SokuLib::Color::Black);
}

void openTrialEditPage()
{
	auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

	for (auto &guide : editorGuides)
		guide->active = false;
	editorGuides[3]->active = true;
	SokuLib::playSEWaveBuffer(0x28);
	packEditScenario.opened = true;

	packEditScenario.name.texture.createFromText(scenario->nameStr.c_str(), defaultFont12, {153, 23}, nullptr);
	packEditScenario.name.setSize({133, 13});
	packEditScenario.name.rect.width  = 133;
	packEditScenario.name.rect.height = 13;
	packEditScenario.name.setPosition({182, 244});
	packEditScenario.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditScenario.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditScenario.filePath.texture.createFromText(scenario->fileRel.c_str(), defaultFont12, {153, 23}, nullptr);
	packEditScenario.filePath.setSize({133, 13});
	packEditScenario.filePath.rect.width  = 133;
	packEditScenario.filePath.rect.height = 13;
	packEditScenario.filePath.setPosition({182, 274});
	packEditScenario.filePath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditScenario.filePath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};

	packEditScenario.previewPath.texture.createFromText(scenario->previewFileRel.c_str(), defaultFont12, {153, 23}, nullptr);
	packEditScenario.previewPath.setSize({133, 13});
	packEditScenario.previewPath.rect.width  = 133;
	packEditScenario.previewPath.rect.height = 13;
	packEditScenario.previewPath.setPosition({182, 302});
	packEditScenario.previewPath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
	packEditScenario.previewPath.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0xFF};
}

static std::string removeShiftJISChars(const char *str)
{
	std::string result;

	result.reserve(strlen(str));
	for (int i = 0; str[i]; i++) {
		if (!(str[i] & 0x80)) {
			result += str[i];
			continue;
		}
		i += 2;
		result += '?';
		if (!str[i - 1])
			return result;
	}
	return result;
}

void createBasicPack(const std::string &path)
{
	printf("Creating pack at location %s\n", path.c_str());

	std::ofstream stream{path + "\\pack.json"};
	nlohmann::json json;

	if (stream.fail()) {
		MessageBox(
			SokuLib::window,
			("Cannot open " + path + "\\pack.json for writing: " + strerror(errno)).c_str(),
			"Pack creation error",
			MB_ICONERROR
		);
		return;
	}
	json["min_version"] = VERSION_STR;
	json["author"] = removeShiftJISChars(SokuLib::profile1.name);
	json["characters"] = nlohmann::json::array();
	json["scenarios"] = nlohmann::json::array();
	stream << json.dump(4);

	auto pack = std::make_shared<Pack>(path, json);

	loadedPacks.push_back(pack);
	packsByName[pack->nameStr].push_back(loadedPacks.back());
	packsByCategory[pack->category].push_back(loadedPacks.back());
	if (std::find(uniqueCategories.begin(), uniqueCategories.end(), pack->category) == uniqueCategories.end())
		uniqueCategories.push_back(pack->category);
	if (std::find(uniqueNames.begin(), uniqueNames.end(), pack->nameStr) == uniqueNames.end())
		uniqueNames.push_back(pack->nameStr);
	for (auto &mode : pack->modes) {
		packsByName[mode].push_back(loadedPacks.back());
		if (std::find(uniqueModes.begin(), uniqueModes.end(), mode) == uniqueModes.end())
			uniqueModes.push_back(mode);
	}
	std::sort(loadedPacks.begin(), loadedPacks.end(), [](std::shared_ptr<Pack> pack1, std::shared_ptr<Pack> pack2){
		if (pack1->error.texture.hasTexture() != pack2->error.texture.hasTexture())
			return pack2->error.texture.hasTexture();
		return pack1->category < pack2->category;
	});
	std::sort(uniqueCategories.begin(), uniqueCategories.end());
	std::sort(uniqueNames.begin(), uniqueNames.end());
	std::sort(uniqueModes.begin(), uniqueModes.end());

	auto it = std::find(loadedPacks.begin(), loadedPacks.end(), pack);

	currentPack = it - loadedPacks.begin();
	shownPack = currentPack;
	checkScrollDown();
	modeFilter = -1;
	nameFilter = -1;
	topicFilter = -1;
	currentEntry = -1;
	pack->name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::DrawUtils::DxSokuColor{0x80, 0xFF, 0x80};
	pack->name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER]= SokuLib::DrawUtils::DxSokuColor{0x80, 0xFF, 0x80};
	openPackEditPage();
}

bool saveCurrentPack()
{
	auto &pack = loadedPacks[currentPack];
	nlohmann::json json;
	std::vector<nlohmann::json> scenarios;
	std::string str;
	std::ofstream stream;

	json["modes"] = pack->modes;
	json["characters"] = pack->characters;
	if (!pack->previewPath.empty()) {
		json["stand"] = {
			{"isPath", pack->previewFSAsset},
			{"path",   pack->previewPath}
		};
	}
	if (!pack->nameStr.empty())
		json["name"] = pack->nameStr;
	if (!pack->category.empty())
		json["category"] = pack->category;
	if (!pack->category.empty())
		json["requirement"] = pack->requirement;
	if (!pack->minVersion.empty())
		json["min_version"] = pack->minVersion;
	if (!pack->outroRelPath.empty())
		json["outro"] = pack->outroRelPath;
	if (!pack->descriptionStr.empty())
		json["description"] = pack->descriptionStr;
	if (!pack->authorStr.empty())
		json["author"] = pack->authorStr;
	if (pack->icon)
		json["icon"] = {
			{"isPath", pack->icon->fsPath},
			{"path", pack->icon->path},
			{"offset", {
				{"x", pack->icon->translate.x},
				{"y", pack->icon->translate.y}
			}},
			{"scale", pack->icon->scale},
			{"xMirror", pack->icon->mirror.x},
			{"yMirror", pack->icon->mirror.y},
			{"rect", {
				{"left", pack->icon->rect.left},
				{"top", pack->icon->rect.top},
				{"width", pack->icon->rect.width},
				{"height", pack->icon->rect.height},
			}}
		};
	json["scenarios"] = nlohmann::json::array();
	for (auto &scenario : pack->scenarios) {
		json["scenarios"].push_back({
			{"name", scenario->nameStr},
			{"file", scenario->fileRel},
			{"preview", scenario->previewFileRel},
			{"description", scenario->descriptionStr},
			{"may_be_locked", scenario->canBeLocked},
			{"name_hidden_when_locked", scenario->nameHiddenIfLocked},
			{"extra", scenario->extra},
		});
	}
	try {
		str = json.dump(4);
	} catch (std::exception &e) {
		MessageBox(SokuLib::window, ("Error: Cannot save pack: " + std::string(e.what())).c_str(), "Saving error", MB_ICONERROR);
		return false;
	}
	if (rename(
		(pack->path + "/pack.json").c_str(),
		(pack->path + "/pack_backup.json").c_str()
	)) {
		MessageBox(
			SokuLib::window,
			("Cannot rename " + pack->path + "/pack.json to " + pack->path + "/pack_backup.json: " + strerror(errno)).c_str(),
			"Saving error",
			MB_ICONERROR
		);
		return false;
	}
	stream.open(pack->path + "/pack.json");
	if (stream.fail()) {
		MessageBox(
			SokuLib::window,
			("Cannot open " + pack->path + "/pack.json for writing: " + strerror(errno)).c_str(),
			"Saving error",
			MB_ICONERROR
		);
		rename(
			(pack->path + "/pack_backup.json").c_str(),
			(pack->path + "/pack.json").c_str()
		);
		return false;
	}
	stream << str;
	if (stream.fail()) {
		MessageBox(
			SokuLib::window,
			("Cannot write to " + pack->path + "/pack.json: " + strerror(errno)).c_str(),
			"Saving error",
			MB_ICONERROR
		);
		unlink((pack->path + "/pack.json").c_str());
		rename(
			(pack->path + "/pack_backup.json").c_str(),
			(pack->path + "/pack.json").c_str()
		);
		return false;
	}
	unlink((pack->path + "/pack_backup.json").c_str());
	return true;
}

bool checkEditorKeys(const SokuLib::KeyInput &input)
{
	if (packEditScenario.opened) {
		if (SokuLib::inputMgrs.input.b == 1 || SokuLib::checkKeyOneshot(DIK_ESCAPE, false, false, false)) {
			SokuLib::playSEWaveBuffer(0x29);
			if (saveCurrentPack()) {
				packEditScenario.opened = false;
				editorGuides[3]->active = false;
				editorGuides[2]->active = true;
			}
			return false;
		}
		if (SokuLib::inputMgrs.input.horizontalAxis == -1 || (SokuLib::inputMgrs.input.horizontalAxis < -36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
			SokuLib::playSEWaveBuffer(0x27);
			packEditScenario.cursorPos = PackEditScenario::leftTable[packEditScenario.cursorPos];
		} else if (SokuLib::inputMgrs.input.horizontalAxis == 1 || (SokuLib::inputMgrs.input.horizontalAxis > 36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
			SokuLib::playSEWaveBuffer(0x27);
			packEditScenario.cursorPos = PackEditScenario::rightTable[packEditScenario.cursorPos];
		}
		if (SokuLib::inputMgrs.input.verticalAxis == -1 || (SokuLib::inputMgrs.input.verticalAxis < -36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
			SokuLib::playSEWaveBuffer(0x27);
			packEditScenario.cursorPos = PackEditScenario::upTable[packEditScenario.cursorPos];
		} else if (SokuLib::inputMgrs.input.verticalAxis == 1 || (SokuLib::inputMgrs.input.verticalAxis > 36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
			SokuLib::playSEWaveBuffer(0x27);
			packEditScenario.cursorPos = PackEditScenario::downTable[packEditScenario.cursorPos];
		}
		if (SokuLib::inputMgrs.input.a == 1)
			(packEditScenario.*PackEditScenario::handlers[packEditScenario.cursorPos])();
		if (SokuLib::inputMgrs.input.c == 1)
			(packEditScenario.*PackEditScenario::resetHandlers[packEditScenario.cursorPos])();
		return true;
	} else if (packEditPage.opened) {
		auto &pack = loadedPacks[currentPack];
		bool cond = SokuLib::inputMgrs.input.b == 1 || SokuLib::checkKeyOneshot(DIK_ESCAPE, false, false, false);
		bool playSound = false;

		if (packEditPage.selectingCharacters) {
			if (cond) {
				unsigned allocSize = 0;
				std::string result;

				for (int i = 0; i < pack->characters.size(); i++)
					allocSize += pack->characters[i].size() + (i != 0);
				result.reserve(allocSize);
				for (int i = 0; i < pack->characters.size(); i++) {
					if (i != 0)
						result += ',';
					result += pack->characters[i];
				}
				packEditPage.characters.texture.createFromText(result.c_str(), defaultFont12, {381, 23}, nullptr);
				SokuLib::playSEWaveBuffer(0x29);
				packEditPage.selectingCharacters = false;
				editorGuides[6]->active = false;
				editorGuides[3]->active = true;
				return false;
			}
			if (SokuLib::inputMgrs.input.a == 1) {
				auto it = std::find(pack->characters.begin(), pack->characters.end(), orderChrs[packEditPage.chrCursorPos]);

				if (it == pack->characters.end())
					pack->characters.push_back(orderChrs[packEditPage.chrCursorPos]);
				else
					pack->characters.erase(it);
				SokuLib::playSEWaveBuffer(0x28);
			}
			if (SokuLib::inputMgrs.input.verticalAxis == -1 || (SokuLib::inputMgrs.input.verticalAxis < -36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
				playSound = true;
				do {
					if (packEditPage.chrCursorPos % 9 == 0)
						packEditPage.chrCursorPos += 8;
					else
						packEditPage.chrCursorPos -= 1;
				} while (packEditPage.chrCursorPos >= orderChrs.size());
				if (SokuLib::inputMgrs.input.horizontalAxis == -1 || (SokuLib::inputMgrs.input.horizontalAxis < -36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0))  {
					if (packEditPage.chrCursorPos < 9)
						while (packEditPage.chrCursorPos + 9 < orderChrs.size())
							packEditPage.chrCursorPos += 9;
					else
						packEditPage.chrCursorPos -= 9;
				} else if (SokuLib::inputMgrs.input.horizontalAxis == 1 || (SokuLib::inputMgrs.input.horizontalAxis > 36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
					packEditPage.chrCursorPos += 9;
					if (packEditPage.chrCursorPos >= orderChrs.size())
						packEditPage.chrCursorPos %= 9;
				}
			} else if (SokuLib::inputMgrs.input.verticalAxis == 1 || (SokuLib::inputMgrs.input.verticalAxis > 36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
				playSound = true;
				do {
					if (packEditPage.chrCursorPos % 9 == 8)
						packEditPage.chrCursorPos -= 8;
					else
						packEditPage.chrCursorPos += 1;
				} while (packEditPage.chrCursorPos >= orderChrs.size());
				if (SokuLib::inputMgrs.input.horizontalAxis == -1 || (SokuLib::inputMgrs.input.horizontalAxis < -36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0))  {
					if (packEditPage.chrCursorPos < 9)
						while (packEditPage.chrCursorPos + 9 < orderChrs.size())
							packEditPage.chrCursorPos += 9;
					else
						packEditPage.chrCursorPos -= 9;
				} else if (SokuLib::inputMgrs.input.horizontalAxis == 1 || (SokuLib::inputMgrs.input.horizontalAxis > 36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
					packEditPage.chrCursorPos += 9;
					if (packEditPage.chrCursorPos >= orderChrs.size())
						packEditPage.chrCursorPos %= 9;
				}
			} else if (SokuLib::inputMgrs.input.horizontalAxis == -1 || (SokuLib::inputMgrs.input.horizontalAxis < -36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
				playSound = true;
				if (packEditPage.chrCursorPos < 9)
					while (packEditPage.chrCursorPos + 9 < orderChrs.size())
						packEditPage.chrCursorPos += 9;
				else
					packEditPage.chrCursorPos -= 9;
			} else if (SokuLib::inputMgrs.input.horizontalAxis == 1 || (SokuLib::inputMgrs.input.horizontalAxis > 36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
				playSound = true;
				packEditPage.chrCursorPos += 9;
				if (packEditPage.chrCursorPos >= orderChrs.size())
					packEditPage.chrCursorPos %= 9;
			}
			if (playSound)
				SokuLib::playSEWaveBuffer(0x27);
		} else if (packEditPage.selectingPos) {
			if (SokuLib::inputMgrs.input.a == 1 || cond) {
				SokuLib::playSEWaveBuffer(0x28 + cond);
				packEditPage.selectingPos = false;
				editorGuides[4]->active = false;
				editorGuides[3]->active = true;
				return true;
			}
			if (SokuLib::inputMgrs.input.horizontalAxis == -1 || (SokuLib::inputMgrs.input.horizontalAxis < -36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
				playSound = true;
				pack->icon->translate.x -= 1;
				if (SokuLib::inputMgrs.input.verticalAxis == -1 || (SokuLib::inputMgrs.input.verticalAxis < -36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0))
					pack->icon->translate.y -= 1;
				else if (SokuLib::inputMgrs.input.verticalAxis == 1 || (SokuLib::inputMgrs.input.verticalAxis > 36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0))
					pack->icon->translate.y += 1;
			} else if (SokuLib::inputMgrs.input.horizontalAxis == 1 || (SokuLib::inputMgrs.input.horizontalAxis > 36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
				playSound = true;
				pack->icon->translate.x += 1;
				if (SokuLib::inputMgrs.input.verticalAxis == -1 || (SokuLib::inputMgrs.input.verticalAxis < -36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0))
					pack->icon->translate.y -= 1;
				else if (SokuLib::inputMgrs.input.verticalAxis == 1 || (SokuLib::inputMgrs.input.verticalAxis > 36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0))
					pack->icon->translate.y += 1;
			} else if (SokuLib::inputMgrs.input.verticalAxis == -1 || (SokuLib::inputMgrs.input.verticalAxis < -36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
				playSound = true;
				pack->icon->translate.y -= 1;
			} else if (SokuLib::inputMgrs.input.verticalAxis == 1 || (SokuLib::inputMgrs.input.verticalAxis > 36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
				playSound = true;
				pack->icon->translate.y += 1;
			}
			if (playSound)
				SokuLib::playSEWaveBuffer(0x27);
		} else if (packEditPage.selectingScale) {
			if (SokuLib::inputMgrs.input.a == 1 || cond) {
				SokuLib::playSEWaveBuffer(0x28 + cond);
				packEditPage.selectingScale = false;
				editorGuides[5]->active = false;
				editorGuides[3]->active = true;
				return true;
			}
			if (SokuLib::inputMgrs.input.horizontalAxis == -1 || (SokuLib::inputMgrs.input.horizontalAxis < -36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
				SokuLib::playSEWaveBuffer(0x27);
				if (pack->icon->scale >= 0.02) {
					pack->icon->scale -= 0.01;
					packEditPage.cursor.setPosition({static_cast<int>(161 + (pack->icon ? 77 * pack->icon->scale : 77)), 136});
					packEditPage.cursorGear.setPosition({static_cast<int>(153 + (pack->icon ? 77 * pack->icon->scale : 77)), 141});
					packEditPage.cursorGear.setRotation(packEditPage.cursorGear.getRotation() - 0.1);
				}
				pack->icon->rect.width = min(68 / pack->icon->scale, pack->icon->untransformedRect.x);
				pack->icon->rect.height = min(28 / pack->icon->scale, pack->icon->untransformedRect.y);
				pack->icon->sprite.setSize({
					static_cast<unsigned int>(pack->icon->rect.width * pack->icon->scale),
					static_cast<unsigned int>(pack->icon->rect.height * pack->icon->scale)
				});
				pack->icon->sprite.rect = pack->icon->rect;
			} else if (SokuLib::inputMgrs.input.horizontalAxis == 1 || (SokuLib::inputMgrs.input.horizontalAxis > 36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
				SokuLib::playSEWaveBuffer(0x27);
				if (pack->icon->scale < 2) {
					pack->icon->scale += 0.01;
					packEditPage.cursor.setPosition({static_cast<int>(161 + (pack->icon ? 77 * pack->icon->scale : 77)), 136});
					packEditPage.cursorGear.setPosition({static_cast<int>(153 + (pack->icon ? 77 * pack->icon->scale : 77)), 141});
					packEditPage.cursorGear.setRotation(packEditPage.cursorGear.getRotation() + 0.1);
				}
				pack->icon->rect.width = min(68 / pack->icon->scale, pack->icon->untransformedRect.x);
				pack->icon->rect.height = min(28 / pack->icon->scale, pack->icon->untransformedRect.y);
				pack->icon->sprite.setSize({
					static_cast<unsigned int>(pack->icon->rect.width * pack->icon->scale),
					static_cast<unsigned int>(pack->icon->rect.height * pack->icon->scale)
				});
				pack->icon->sprite.rect = pack->icon->rect;
			}
		} else {
			if (cond) {
				SokuLib::playSEWaveBuffer(0x29);
				if (saveCurrentPack()) {
					packEditPage.opened = false;
					editorGuides[2]->active = true;
					editorGuides[3]->active = false;
				}
				return false;
			}
			if (SokuLib::inputMgrs.input.horizontalAxis == -1 || (SokuLib::inputMgrs.input.horizontalAxis < -36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
				SokuLib::playSEWaveBuffer(0x27);
				packEditPage.cursorPos = PackEditPage::leftTable[packEditPage.cursorPos];
			} else if (SokuLib::inputMgrs.input.horizontalAxis == 1 || (SokuLib::inputMgrs.input.horizontalAxis > 36 && SokuLib::inputMgrs.input.horizontalAxis % 6 == 0)) {
				SokuLib::playSEWaveBuffer(0x27);
				packEditPage.cursorPos = PackEditPage::rightTable[packEditPage.cursorPos];
			}
			if (SokuLib::inputMgrs.input.verticalAxis == -1 || (SokuLib::inputMgrs.input.verticalAxis < -36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
				SokuLib::playSEWaveBuffer(0x27);
				packEditPage.cursorPos = PackEditPage::upTable[packEditPage.cursorPos];
			} else if (SokuLib::inputMgrs.input.verticalAxis == 1 || (SokuLib::inputMgrs.input.verticalAxis > 36 && SokuLib::inputMgrs.input.verticalAxis % 6 == 0)) {
				SokuLib::playSEWaveBuffer(0x27);
				packEditPage.cursorPos = PackEditPage::downTable[packEditPage.cursorPos];
			}
			if (SokuLib::inputMgrs.input.a == 1)
				(packEditPage.*PackEditPage::handlers[packEditPage.cursorPos])();
			if (SokuLib::inputMgrs.input.c == 1)
				(packEditPage.*PackEditPage::resetHandlers[packEditPage.cursorPos])();
		}
		return true;
	} else if (movingScenario) {
		if (SokuLib::inputMgrs.input.b == 1 || SokuLib::checkKeyOneshot(DIK_ESCAPE, false, false, false)) {
			SokuLib::playSEWaveBuffer(0x29);
			movingScenario = false;
			editorGuides[7]->active = false;
			editorGuides[2]->active = true;
			currentEntry = baseCursor;
			return false;
		}
		if (SokuLib::inputMgrs.input.a == 1) {
			SokuLib::playSEWaveBuffer(0x28);
			movingScenario = false;
			editorGuides[7]->active = false;
			editorGuides[2]->active = true;
			if (currentEntry < baseCursor) {
				for (int i = currentEntry + 1; i <= baseCursor; i++)
					loadedPacks[currentPack]->scenarios[currentEntry].swap(loadedPacks[currentPack]->scenarios[i]);
			} else if (baseCursor < currentEntry) {
				for (int i = currentEntry; i > baseCursor; i--)
					loadedPacks[currentPack]->scenarios[baseCursor].swap(loadedPacks[currentPack]->scenarios[i]);
			}
			saveCurrentPack();
			return false;
		}
		return true;
	}
	if (SokuLib::inputMgrs.input.changeCard == 1 && currentPack >= 0) {
		if (!expended) {
			try {
				loadedOutro.reset(new PackOutro(loadedPacks[currentPack]->path, loadedPacks[currentPack]->outroPath));
			} catch (std::exception &e) {
				SokuLib::playSEWaveBuffer(0x29);
				MessageBox(SokuLib::window, ("Error when loading pack outro: " + std::string(e.what())).c_str(), "Pack outro loading error", MB_ICONERROR);
			}
		} else {
			SokuLib::playSEWaveBuffer(0x28);
			movingScenario = true;
			editorGuides[7]->active = true;
			editorGuides[2]->active = false;
			baseCursor = currentEntry;
		}
		return false;
	}
	if (SokuLib::inputMgrs.input.c == 1 && !expended) {
		std::string folder{packsLocation, packsLocation + strlen(packsLocation) - 1};

		openInputDialog("Enter a folder name", nullptr);
		setInputBoxCallbacks([folder](const std::string &result){
			auto path = folder + result;

			if (_mkdir(path.c_str()) != 0) {
				MessageBox(
					SokuLib::window,
					(path + ": " + strerror(errno)).c_str(),
					"Pack creation error",
					MB_ICONERROR
				);
				return;
			}
			createBasicPack(path);
		});
		return true;
	}
	if (SokuLib::inputMgrs.input.horizontalAxis == 1 && currentPack >= 0) {
		if (currentEntry == -1)
			openPackEditPage();
		else
			openTrialEditPage();
	}
	if (SokuLib::inputMgrs.input.c == 1 && expended) {
		openInputDialog("Enter file name.", nullptr);
		setInputBoxCallbacks([](const std::string &jsonfile){
			auto &pack = loadedPacks[currentPack];
			struct stat s;
			std::ifstream from;
			std::ofstream to;
			char buffer[1024];

			if (stat((pack->path + "/" + jsonfile).c_str(), &s) == 0) {
				MessageBox(SokuLib::window, (pack->path + "/" + jsonfile + " already exists.").c_str(), "Copy error", MB_ICONERROR);
				return;
			}
			stat(pack->scenarios[currentEntry]->file.c_str(), &s);
			from.open(pack->scenarios[currentEntry]->file);
			if (from.fail()) {
				MessageBox(SokuLib::window, ("Cannot open file " + pack->scenarios[currentEntry]->file + " to read.").c_str(), "Copy error", MB_ICONERROR);
				return;
			}
			to.open(pack->path + "/" + jsonfile);
			if (to.fail()) {
				MessageBox(SokuLib::window, ("Cannot open file " + pack->path + "/" + jsonfile + " to write.").c_str(), "Copy error", MB_ICONERROR);
				return;
			}
			while (s.st_size) {
				auto size = min(sizeof(buffer), s.st_size);

				from.read(buffer, size);
				to.write(buffer, size);
				s.st_size -= size;
			}

			pack->scenarios.emplace_back(new Scenario(-1, pack->scenarios.size(), pack->path, {{"file", jsonfile}}));
			currentEntry = pack->scenarios.size() - 1;
			packStart = max(0, min(currentPack, 1.f * currentPack - static_cast<int>(264 - (currentPack == loadedPacks.size() - 1 ? 0 : 20) - 35 - 15.f * loadedPacks[currentPack]->scenarios.size()) / 35));
			if (currentEntry > 15)
				entryStart = currentEntry - 15;
			for (auto &guide : (editorMode ? editorGuides : noEditorGuides))
				guide->active = false;
			(editorMode ? editorGuides : noEditorGuides)[2]->active = true;
			expended = true;
			SokuLib::playSEWaveBuffer(0x28);
			saveCurrentPack();
		});
		return true;
	}
	if (SokuLib::inputMgrs.input.d == 1 && currentPack >= 0) {
		auto &pack = loadedPacks[currentPack];

		if (currentEntry >= 0) {
			if (MessageBoxA(
				SokuLib::window,
				("Are you sure you want to delete scenario named \"" + pack->scenarios[currentEntry]->nameStr + "\"?").c_str(),
				"Delete scenario?",
				MB_ICONQUESTION | MB_YESNO
			) == IDNO)
				return SokuLib::playSEWaveBuffer(0x29), true;
			pack->scenarios.erase(pack->scenarios.begin() + currentEntry);
			currentEntry -= currentEntry == pack->scenarios.size();
			expended = !pack->scenarios.empty();
		} else {
			openInputDialog("Enter file name. If the file doesn't<br>exist, it will be created.", nullptr);
			setInputBoxCallbacks([&pack](const std::string &jsonfile){
				struct stat s;

				if (stat((pack->path + "/" + jsonfile).c_str(), &s) != 0) {
					openInputDialog("Enter trial type<br>Valid types are <color A0FFA0>combo</color>", nullptr);
					setInputBoxCallbacks([&pack, jsonfile](const std::string &type){
						if (type.empty())
							return SokuLib::playSEWaveBuffer(0x29);
						if (!Trial::isTypeValid(type)) {
							MessageBox(SokuLib::window, (type + " is not a valid trial type.").c_str(), "Error", MB_ICONERROR);
							return SokuLib::playSEWaveBuffer(0x29);
						}

						std::ofstream stream{pack->path + "/" + jsonfile};

						if (stream.fail()) {
							MessageBox(SokuLib::window, ("Cannot open " + pack->path + "/" + jsonfile + " for writing: " + strerror(errno)).c_str(), "Error", MB_ICONERROR);
							return SokuLib::playSEWaveBuffer(0x29);
						}
						stream << "{" << std::endl;
						stream << R"(    "type": ")" << type << "\"" << std::endl;
						stream << "}" << std::endl;
						if (stream.fail()) {
							MessageBox(SokuLib::window, ("Cannot write to " + pack->path + "/" + jsonfile + ": " + strerror(errno)).c_str(), "Error", MB_ICONERROR);
							return SokuLib::playSEWaveBuffer(0x29);
						}
						pack->scenarios.emplace_back(new Scenario(-1, pack->scenarios.size(), pack->path, {{"file", jsonfile}}));
						currentEntry = pack->scenarios.size() - 1;
						packStart = max(0, min(currentPack, 1.f * currentPack - static_cast<int>(264 - (currentPack == loadedPacks.size() - 1 ? 0 : 20) - 35 - 15.f * loadedPacks[currentPack]->scenarios.size()) / 35));
						if (currentEntry > 15)
							entryStart = currentEntry - 15;
						for (auto &guide : (editorMode ? editorGuides : noEditorGuides))
							guide->active = false;
						(editorMode ? editorGuides : noEditorGuides)[2]->active = true;
						expended = true;
					});
					return;
				}
				pack->scenarios.emplace_back(new Scenario(-1, pack->scenarios.size(), pack->path, {{"file", jsonfile}}));
				currentEntry = pack->scenarios.size() - 1;
				packStart = max(0, min(currentPack, 1.f * currentPack - static_cast<int>(264 - (currentPack == loadedPacks.size() - 1 ? 0 : 20) - 35 - 15.f * loadedPacks[currentPack]->scenarios.size()) / 35));
				if (currentEntry > 15)
					entryStart = currentEntry - 15;
				for (auto &guide : (editorMode ? editorGuides : noEditorGuides))
					guide->active = false;
				(editorMode ? editorGuides : noEditorGuides)[2]->active = true;
				expended = true;
			});
			return true;
		}
		SokuLib::playSEWaveBuffer(0x28);
		saveCurrentPack();
	}
	return true;
}

bool editorUpdate()
{
	if (inputBoxShown)
		return inputBoxUpdate(), false;
	if (explorerShown)
		return explorerUpdate(), false;
	if (!checkEditorKeys(SokuLib::inputMgrs.input))
		return false;
	if (packEditScenario.opened)
		return false;
	if (packEditPage.opened) {
		auto &inputs = SokuLib::inputMgrs.input;
		int dir = packEditPage.selectingPos * ((inputs.horizontalAxis != 0) * std::copysign(1, inputs.horizontalAxis) + (inputs.verticalAxis != 0) * std::copysign(3, -inputs.verticalAxis)) + 4;

		stickTop.setPosition({
			463 + offsetTable[dir].x,
			96 + offsetTable[dir].y
		});
		return false;
	}
	return true;
}

void editorRender()
{
	if (packEditScenario.opened) {
		auto &scenario = loadedPacks[currentPack]->scenarios[currentEntry];

		editScenarioSeat.draw();
		displaySokuCursor(PackEditScenario::cursorLocations[packEditScenario.cursorPos].first, PackEditScenario::cursorLocations[packEditScenario.cursorPos].second);
		packEditScenario.name.draw();
		packEditScenario.filePath.draw();
		packEditScenario.previewPath.draw();
		if (scenario->extra) {
			tickSprite.setPosition({177, 329});
			tickSprite.draw();
		}
		if (scenario->canBeLocked) {
			tickSprite.setPosition({300, 328});
			tickSprite.draw();
		}
		if (scenario->nameHiddenIfLocked) {
			tickSprite.setPosition({285, 357});
			tickSprite.draw();
		}
		if (scenario->description.texture.hasTexture())
			scenario->description.draw();
	} else if (packEditPage.opened) {
		auto &pack = loadedPacks[currentPack];

		editSeat.draw();
		displaySokuCursor(PackEditPage::cursorLocations[packEditPage.cursorPos].first, PackEditPage::cursorLocations[packEditPage.cursorPos].second);
		editSeatForeground.draw();
		stickTop.draw();
		packEditPage.name.draw();
		packEditPage.modes.draw();
		packEditPage.author.draw();
		packEditPage.version.draw();
		packEditPage.category.draw();
		packEditPage.iconPath.draw();
		packEditPage.characters.draw();
		packEditPage.endingPath.draw();
		packEditPage.description.draw();
		packEditPage.previewPath.draw();
		packEditPage.cursorGear.draw();
		packEditPage.cursor.draw();
		if (pack->icon) {
			if (pack->icon->mirror.x) {
				tickSprite.setPosition({409, 127});
				tickSprite.draw();
			}
			if (pack->icon->mirror.y) {
				tickSprite.setPosition({409, 145});
				tickSprite.draw();
			}
		}
		if (packEditPage.selectingCharacters) {
			int i = 0;

			editSeatEmpty.draw();
			for (auto &chr : chrs) {
				auto &sprite = *charactersFaces[i];

				packEditPage.rect.setPosition({
					120 + 100 * (i / 9),
					69 + 40 * (i % 9),
				});
				sprite.draw();
				packEditPage.rect.draw();
				if (packEditPage.chrCursorPos == i)
					displaySokuCursor({
						127 + 100 * (i / 9),
						69 + 40 * (i % 9),
					}, {16, 16});
				if (std::find(pack->characters.begin(), pack->characters.end(), chr.second) != pack->characters.end()) {
					tickSprite.setPosition({
						118 + 100 * (i / 9),
						67 + 40 * (i % 9),
					});
					tickSprite.draw();
				}
				i++;
			}
		}
	}
}

void handlePlayerInputs(const SokuLib::KeyInput &input)
{
	if (SokuLib::inputMgrs.input.d == 1) {
		loadExplorerRoot("All files");
		setExplorerDefaultMusic(nullptr);
		setExplorerCallback([](std::string){});
		return;
	}
	if (SokuLib::inputMgrs.input.spellcard == 1)
		switchEditorMode();
	if (SokuLib::inputMgrs.input.changeCard == 1) {
		SokuLib::playSEWaveBuffer(0x28);
		SokuLib::activateMenu(new ExplorerMenu());
		return;
	}
	if (input.a == 1 && SokuLib::newSceneId == SokuLib::SCENE_TITLE && currentPack >= 0) {
		if (loadedPacks[currentPack]->scenarios.empty())
			SokuLib::playSEWaveBuffer(0x29);
		else if (currentEntry == -1) {
			if (isLocked(*loadedPacks[currentPack]) && !editorMode)
				return SokuLib::playSEWaveBuffer(0x29);
			SokuLib::playSEWaveBuffer(0x28);
			expended = true;
			currentEntry = 0;
			packStart = max(0, min(currentPack, 1.f * currentPack - static_cast<int>(264 - (currentPack == loadedPacks.size() - 1 ? 0 : 20) - 35 - 15.f * loadedPacks[currentPack]->scenarios.size()) / 35));
			if (currentEntry > 15)
				entryStart = currentEntry - 15;
			for (auto &guide : (editorMode ? editorGuides : noEditorGuides))
				guide->active = false;
			(editorMode ? editorGuides : noEditorGuides)[2]->active = true;
		} else {
			if (!isLocked(currentEntry) || editorMode) {
				puts("Start game !");
				SokuLib::playSEWaveBuffer(0x28);
				prepareGameLoading(
					loadedPacks[currentPack]->scenarios[currentEntry]->folder.c_str(),
					loadedPacks[currentPack]->scenarios[currentEntry]->file
				);
				return;
			}
			SokuLib::playSEWaveBuffer(0x29);
		}
	}
	if (input.verticalAxis == -1 || (input.verticalAxis <= -36 && input.verticalAxis % 6 == 0))
		handleGoUp();
	else if (input.verticalAxis == 1 || (input.verticalAxis >= 36 && input.verticalAxis % 6 == 0))
		handleGoDown();
	if (input.horizontalAxis == -1 || (input.horizontalAxis <= -36 && input.horizontalAxis % 6 == 0))
		handleGoLeft();
	else if (input.horizontalAxis == 1 || (input.horizontalAxis >= 36 && input.horizontalAxis % 6 == 0))
		handleGoRight();
}

int menuOnProcess(SokuLib::MenuResult *This)
{
	if (SokuLib::newSceneId != SokuLib::sceneId)
		return true;

	if (reloadRequest) {
		reloadRequest = false;
		return loadRequest = true;
	}
	if (loadNextTrial) {
		loadNextTrial = false;
		loadedPacks[currentPack]->scenarios[currentEntry]->loaded = false;
		if (!loadedPacks[currentPack]->scenarios[currentEntry]->loading && loadedPacks[currentPack]->scenarios[currentEntry]->preview)
			loadedPacks[currentPack]->scenarios[currentEntry]->preview.reset();
		++currentEntry;
		loadedPacks[currentPack]->scenarios[currentEntry]->loaded = true;
		loadedPacks[currentPack]->scenarios[currentEntry]->loadPreview();
		prepareGameLoading(
			loadedPacks[currentPack]->scenarios[currentEntry]->folder.c_str(),
			loadedPacks[currentPack]->scenarios[currentEntry]->file
		);
		if (loadRequest)
			return true;
	}
	menuLoadAssets();
	if (currentEntry >= 0) {
		auto curr = movingScenario ? baseCursor : currentEntry;

		if (!loadedPacks[currentPack]->scenarios[curr]->loading && loadedPacks[currentPack]->scenarios[curr]->preview)
			loadedPacks[shownPack]->scenarios[curr]->preview->update();
		else {
			loadedPacks[currentPack]->scenarios[curr]->loaded = true;
			if (!loadedPacks[currentPack]->scenarios[curr]->loading)
				loadedPacks[currentPack]->scenarios[curr]->loadPreview();
			loadingGear.setRotation(loadingGear.getRotation() + 0.1);
		}
	}
	updateNoiseTexture();
	updateBandTexture();
	for (auto &guide : noEditorGuides)
		if (guide)
			guide->update();
	for (auto &guide : editorGuides)
		if (guide)
			guide->update();
	if (!loadedOutro) {
		if (editorMode && !editorUpdate())
			return true;
		else if (inputBoxShown)
			inputBoxUpdate();
		else if (explorerShown)
			explorerUpdate();
		else {
			if (SokuLib::inputMgrs.input.b == 1) {
				SokuLib::playSEWaveBuffer(0x29);
				if (expended) {
					expended = false;
					currentEntry = -1;
					for (auto &guide : (editorMode ? editorGuides : noEditorGuides))
						guide->active = false;
					(editorMode ? editorGuides : noEditorGuides)[currentPack < 0]->active = true;
				} else {
					puts("Quit");
					printf("Still loading %i images\n", loading);
					return loading;
				}
			}
			if (SokuLib::checkKeyOneshot(DIK_ESCAPE, 0, 0, 0)) {
				SokuLib::playSEWaveBuffer(0x29);
				printf("Still loading %i images\n", loading);
				return loading;
			}
			handlePlayerInputs(SokuLib::inputMgrs.input);
		}
	}
	SokuLib::currentScene->to<SokuLib::Title>().menuInputHandler.pos = 8;
	SokuLib::currentScene->to<SokuLib::Title>().menuInputHandler.posCopy = 8;

	if (loadedOutro) {
		if (!rect.getFillColor().a)
			try {
				if (SokuLib::checkKeyOneshot(DIK_ESCAPE, false, false, false))
					SokuLib::playSEWaveBuffer(0x29);
				else if (loadedOutro->update())
					return true;
			} catch (std::exception &e) {
				MessageBox(SokuLib::window, ("Error when updating pack outro: " + std::string(e.what())).c_str(), "Pack outro loading error", MB_ICONERROR);
			}
		else
			loadedOutro->update();
		rect.setFillColor(static_cast<unsigned int>(rect.getFillColor()) + 0x0F000000);
		if (rect.getFillColor().a == 0xFF) {
			loadedOutro.reset();
			SokuLib::playBGM("data/bgm/op2.ogg");
		}
	} else if (rect.getFillColor().a)
		rect.setFillColor(static_cast<unsigned int>(rect.getFillColor()) - 0x0F000000);
	return true;
}

void renderOnePackBack(Pack &pack, SokuLib::Vector2<float> &pos, bool deployed)
{
	if (nameFilter != -1 && pack.nameStr != uniqueNames[nameFilter])
		return;
	if (modeFilter != -1 && std::find(pack.modes.begin(), pack.modes.end(), uniqueModes[modeFilter]) == pack.modes.end())
		return;
	if (topicFilter != -1 && pack.category != uniqueCategories[topicFilter])
		return;

	if (pos.y >= 100) {
		packContainer.setPosition({
			static_cast<int>(pos.x),
			static_cast<int>(pos.y)
		});
		packContainer.draw();
	}
	pos.y += 35;
	if (deployed && expended) {
		for (int i = entryStart; i < pack.scenarios.size(); i++) {
			pos.y += 15;
			if (pos.y > 379)
				break;
		}
	} else
		pos.y += 5;
}

void renderOnePack(Pack &pack, SokuLib::Vector2<float> &pos, bool deployed)
{
	int i;
	auto p = pos;
	auto &sprite = pack.error.texture.hasTexture() ? pack.error : pack.author;

	if (nameFilter != -1 && pack.nameStr != uniqueNames[nameFilter])
		return;
	if (modeFilter != -1 && std::find(pack.modes.begin(), pack.modes.end(), uniqueModes[modeFilter]) == pack.modes.end())
		return;
	if (topicFilter != -1 && pack.category != uniqueCategories[topicFilter])
		return;

	//100 <= y <= 406
	if (pos.y >= 100) {
		auto sumScore = 0;
		bool hasScore = false;

		for (auto &scenario : pack.scenarios) {
			hasScore |= scenario->score != -1;
			sumScore += scenario->score;
		}

		if (!editorMode && isLocked(pack));
		else if (pack.icon) {
			pack.icon->sprite.setPosition(SokuLib::Vector2i{
				static_cast<int>(pos.x + 4),
				static_cast<int>(pos.y + 2)
			} + pack.icon->translate);
			pack.icon->sprite.draw();
		} else {
			missingIcon.setPosition({
				static_cast<int>(pos.x + 34),
				static_cast<int>(pos.y - 1)
			});
			missingIcon.draw();
		}
		if (isLocked(pack)) {
			lock.setPosition({
				static_cast<int>(pos.x + 18),
				static_cast<int>(pos.y - 14)
			});
			lock.setSize({64, 64});
			lock.draw();
		}

		sprite.setPosition({
			static_cast<int>(pos.x + 75),
			static_cast<int>(pos.y + 17)
		});
		sprite.draw();

		if (hasScore) {
			int val = sumScore / static_cast<int>(pack.scenarios.size());

			score.rect.left = max(0, val) * score.texture.getSize().x / 4;
			score.setPosition({
				static_cast<int>(pos.x + 296),
				static_cast<int>(pos.y - 8)
			});
			score.draw();
		}
	}

	if (currentEntry != -1) {
		p.x += 25;
		p.y += (currentEntry - entryStart) * 15 + 33;
	}
	if (deployed)
		displaySokuCursor(
			{static_cast<int>(p.x + 70), static_cast<int>(p.y + 1)},
			{300, 16}
		);

	if (pos.y >= 100 && pos.y <= 406) {
		if (pack.error.texture.hasTexture()) {
			pack.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::Color{0xFF, 0xA0, 0xA0};
			pack.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER] = SokuLib::Color{0xFF, 0xA0, 0xA0};
		} else if (editorMode) {
			pack.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::Color{0x80, 0xFF, 0x80};
			pack.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER] = SokuLib::Color{0x80, 0xFF, 0x80};
		} else if (isLocked(pack)) {
			pack.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::Color{0x40, 0x40, 0x40};
			pack.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER] = SokuLib::Color{0x40, 0x40, 0x40};
		} else {
			pack.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_LEFT_CORNER] = SokuLib::Color{0xA0, 0xA0, 0xFF};
			pack.name.fillColors[SokuLib::DrawUtils::GradiantRect::RECT_BOTTOM_RIGHT_CORNER] = SokuLib::Color{0xA0, 0xA0, 0xFF};
		}
		pack.name.setPosition({
			static_cast<int>(pos.x + 74),
			static_cast<int>(pos.y + 2)
		});
		pack.name.draw();
	}
	pos.y += 35;

	if (!deployed || !expended) {
		pos.y += 5;
		return;
	}

	for (i = entryStart; i < pack.scenarios.size(); i++) {
		auto curr = i;

		if (movingScenario) {
			if (curr == currentEntry)
				curr = baseCursor;
			else if (curr <= baseCursor && curr > currentEntry)
				curr--;
			else if (curr >= baseCursor && curr < currentEntry)
				curr++;
		}

		auto &scenario = pack.scenarios[curr];

		if (pos.y >= 100) {
			if (scenario->extra && scenario->score == -1) {
				extraImg.setPosition({
					static_cast<int>(pos.x + 271),
					static_cast<int>(pos.y - 5)
				});
				extraImg.draw();
			}
			if (isLocked(i)) {
				lock.setPosition({
					static_cast<int>(pos.x + 271),
					static_cast<int>(pos.y - 10)
				});
				lock.setSize({32, 32});
				lock.draw();
				if (scenario->nameHiddenIfLocked && !editorMode) {
					questionMarks.setPosition({
						static_cast<int>(pos.x + 100),
						static_cast<int>(pos.y)
					});
					for (int j = 0; j < 4; j++)
						questionMarks.fillColors[j] = scenario->name.fillColors[j];
					questionMarks.draw();
				} else {
					scenario->name.setPosition({
						static_cast<int>(pos.x + 100),
						static_cast<int>(pos.y)
					});
					if (!editorMode)
						scenario->name.tint = SokuLib::DrawUtils::DxSokuColor{0x80, 0x80, 0x80};
					else
						scenario->name.tint = SokuLib::DrawUtils::DxSokuColor::White;
					scenario->name.draw();
				}
			} else {
				scenario->name.setPosition({
					static_cast<int>(pos.x + 100),
					static_cast<int>(pos.y)
				});
				scenario->name.tint = SokuLib::DrawUtils::DxSokuColor::White;
				scenario->name.draw();
				if (scenario->score != -1) {
					scenario->scoreSprite.setPosition({
						static_cast<int>(pos.x + 271),
						static_cast<int>(pos.y - 10)
					});
					scenario->scoreSprite.draw();
				}
			}
			if (editorMode) {
				wrench.setPosition({
					static_cast<int>(pos.x + 287),
					static_cast<int>(pos.y - 10)
				});
				wrench.draw();
			}
		}
		pos.y += 15;
		if (pos.y > 379)
			break;
	}
	if (entryStart) {
		upArrow.setPosition({72, 148});
		upArrow.setSize({16, 16});
		upArrow.draw();
	}
	if (i < pack.scenarios.size() - 1) {
		downArrow.setPosition({72, 372});
		downArrow.setSize({16, 16});
		downArrow.draw();
	}
}

void menuOnRender(SokuLib::MenuResult *This)
{
	SokuLib::Vector2<float> pos = {16, 116};

	if (!loaded)
		goto displayOutro;

	displayFilters();
	title.draw();
	for (unsigned i = packStart; i < loadedPacks.size(); i++) {
		// 100 <= y <= 364
		renderOnePackBack(*loadedPacks[i], pos, i == currentPack);
		if (pos.y > 394)
			break;
	}
	pos = {16, 116};
	for (unsigned i = packStart; i < loadedPacks.size(); i++) {
		// 100 <= y <= 364
		renderOnePack(*loadedPacks[i], pos, i == currentPack);
		if (pos.y > 394)
			break;
	}

	Guide *guide = nullptr;

	for (auto &g : noEditorGuides)
		if (g) {
			g->render();
			if (g->active)
				guide = &*g;
		}
	for (auto &g : editorGuides)
		if (g) {
			g->render();
			if (g->active)
				guide = &*g;
		}

	previewContainer.draw();
	version.setPosition({
		639 - versionStrSize.x,
		(guide ? guide->getPosition().y : 480) - versionStrSize.y - 1
	});
	version.draw();
	if (loadedPacks.empty()) {
		goto displayOutro;
	}

	if (currentEntry < 0) {
		if (loadedPacks[shownPack]->preview.texture.hasTexture())
			loadedPacks[shownPack]->preview.draw();
		if (isLocked(*loadedPacks[shownPack]) && !editorMode) {
			lockedText.draw();
			lockedImg.draw();
		} else if (loadedPacks[shownPack]->description.texture.hasTexture())
			loadedPacks[shownPack]->description.draw();
		goto displayOutro;
	}
	if (!isLocked(currentEntry) || editorMode) {
		auto curr = movingScenario ? baseCursor : currentEntry;

		if (!loadedPacks[shownPack]->scenarios[curr]->loading) {
			if (loadedPacks[shownPack]->scenarios[curr]->preview && loadedPacks[shownPack]->scenarios[curr]->preview->isValid())
				loadedPacks[shownPack]->scenarios[curr]->preview->render();
			else {
				lockedNoise.draw();
				blackSilouettes.draw();
			}
		} else {
			lockedNoise.draw();
			loadingGear.setRotation(-loadingGear.getRotation());
			loadingGear.setPosition({540, 243});
			loadingGear.draw();
			loadingGear.setRotation(-loadingGear.getRotation());
			loadingGear.setPosition({563, 225});
			loadingGear.draw();
		}
		if (loadedPacks[shownPack]->scenarios[curr]->description.texture.hasTexture())
			loadedPacks[shownPack]->scenarios[curr]->description.draw();
		CRTBands.draw();
	} else {
		lockedNoise.draw();
		CRTBands.draw();
		if (loadedPacks[shownPack]->scenarios[currentEntry]->extra)
			extraText.draw();
		else
			lockedText.draw();
		lockedImg.draw();
	}
	frame.draw();
displayOutro:
	if (loadedOutro)
		loadedOutro->draw();
	if (inputBoxShown)
		return inputBoxRender();
	else if (explorerShown)
		return explorerRender();
	if (editorMode)
		editorRender();
	rect.draw();
}

void Guide::_init()
{
	this->_sprite.setPosition({0, static_cast<int>(480 - this->_sprite.texture.getSize().y)});
	this->_sprite.setSize({640, this->_sprite.texture.getSize().y});
	this->_sprite.rect.width = this->_sprite.getSize().x;
	this->_sprite.rect.height = this->_sprite.getSize().y;
	this->_sprite.tint.a = 0;
}

Guide::Guide(const char *gamePath)
{
	this->_sprite.texture.loadFromGame(gamePath);
	this->_init();
}

Guide::Guide(HMODULE module, const char *resource)
{
	this->_sprite.texture.loadFromResource(module, resource);
	this->_init();
}

#define GUIDE_SCROLL_TIME_START 60
#define GUIDE_SCROLL_TIME_END 120

void Guide::update()
{
	if (this->active && this->_sprite.tint.a != 255)
		this->_sprite.tint.a += 15;
	else if (!this->active && this->_sprite.tint.a)
		this->_sprite.tint.a -= 15;
	if ((!this->active && !this->_sprite.tint.a) || this->_timer >= GUIDE_SCROLL_TIME_START + GUIDE_SCROLL_TIME_END) {
		this->_sprite.rect.left = 0;
		this->_timer = 0;
	} else if (
		this->_timer < GUIDE_SCROLL_TIME_START ||
		(this->_sprite.texture.getSize().x - 1 <= this->_sprite.getSize().x + this->_sprite.rect.left && this->_timer == GUIDE_SCROLL_TIME_START) ||
		(this->_timer < GUIDE_SCROLL_TIME_START + GUIDE_SCROLL_TIME_END && this->_timer != GUIDE_SCROLL_TIME_START)
	)
		this->_timer++;
	else
		this->_sprite.rect.left++;
}

void Guide::render() const
{
	this->_sprite.draw();
}

SokuLib::Vector2i Guide::getPosition() const
{
	return this->_sprite.getPosition();
}

void Guide::reset()
{
	this->_timer = 0;
	this->_sprite.rect.left = 0;
	this->_sprite.tint.a = this->active * 255;
}
