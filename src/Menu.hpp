//
// Created by PinkySmile on 19/07/2021.
//

#ifndef SWRSTOYS_MENU_HPP
#define SWRSTOYS_MENU_HPP


#include <memory>
#include <SokuLib.hpp>
#include <nlohmann/json.hpp>
#include "TrialBase.hpp"
#include "PackOutro.hpp"

class Guide {
private:
	SokuLib::DrawUtils::Sprite _sprite;
	unsigned _timer = 0;

	void _init();

public:
	bool active = false;

	Guide(const char *gamePath);
	Guide(HMODULE module, const char *resource);
	SokuLib::Vector2i getPosition() const;
	void update();
	void render() const;
	void reset();
};

class ResultMenu : public SokuLib::IMenu {
private:
	bool _done = true;
	int _selected = 0;
	SokuLib::DrawUtils::Sprite _title;
	SokuLib::DrawUtils::Sprite _score;
	SokuLib::DrawUtils::Sprite _resultTop;
	std::array<SokuLib::DrawUtils::Sprite, TrialBase::NB_MENU_ACTION> _text;

protected:
	bool _disabled = false;

public:
	ResultMenu(int score);
	~ResultMenu() override = default;
	void _() override;
	int onProcess() override;
	int onRender() override;
};

extern char profilePath[1024 + MAX_PATH];
extern char profileFolderPath[1024 + MAX_PATH];
extern HMODULE myModule;
extern SokuLib::SWRFont defaultFont10;
extern SokuLib::SWRFont defaultFont12;
extern SokuLib::SWRFont defaultFont16;
extern bool loadRequest;
extern std::unique_ptr<TrialBase> loadedTrial;
extern std::unique_ptr<PackOutro> loadedOutro;
extern bool editorMode;
extern unsigned loading;
extern SokuLib::DrawUtils::Sprite stickTop;
extern SokuLib::DrawUtils::Sprite tickSprite;
extern std::map<unsigned int, std::string> chrs;
extern std::vector<std::string> orderChrs;
extern std::vector<std::unique_ptr<SokuLib::DrawUtils::Sprite>> charactersFaces;
extern SokuLib::DrawUtils::Sprite arrowSprite;
extern SokuLib::DrawUtils::Sprite editSeatEmpty;
extern SokuLib::DrawUtils::Sprite upArrow;
extern SokuLib::DrawUtils::Sprite downArrow;
extern SokuLib::DrawUtils::Sprite lockedNoise;
extern SokuLib::DrawUtils::Sprite CRTBands;
extern SokuLib::DrawUtils::Sprite frame;
extern SokuLib::DrawUtils::Sprite loadingGear;
extern SokuLib::DrawUtils::Sprite previewContainer;
extern SokuLib::DrawUtils::Sprite blackSilouettes;
extern SokuLib::DrawUtils::Sprite lockedImg;
extern SokuLib::DrawUtils::Sprite extraText;
extern SokuLib::DrawUtils::Sprite packContainer;
extern SokuLib::DrawUtils::Sprite lock;
extern SokuLib::DrawUtils::Sprite extraImg;
extern SokuLib::DrawUtils::Sprite missingIcon;
extern SokuLib::DrawUtils::Sprite questionMarks;
extern int currentPack;
extern int currentEntry;
extern bool hasEnglishPatch;

void menuLoadAssets();
int menuOnProcess(SokuLib::MenuResult *This);
void menuOnRender(SokuLib::MenuResult *This);
void menuUnloadAssets();
std::vector<unsigned> getCurrentPackScores();
void displaySokuCursor(SokuLib::Vector2i pos, SokuLib::Vector2u size);
void updateNoiseTexture();
void updateBandTexture();

template<typename T, typename ...Args>
T getField(nlohmann::json json, T defaultValue, bool (nlohmann::json::*checker)() const, std::string field, Args... fields)
{
	std::vector<std::string> vec{fields...};

	if (!json.is_object())
		return defaultValue;
	for (auto &val : vec) {
		if (!json.contains(val) || !json[val].is_object())
			return defaultValue;
		json = json[val];
	}
	if (!json.contains(field) || !(json[field].*checker)())
		return defaultValue;
	return json[field];
}

#endif //SWRSTOYS_MENU_HPP
