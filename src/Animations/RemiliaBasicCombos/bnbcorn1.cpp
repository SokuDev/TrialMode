//
// Created by PinkySmile on 02/08/2021.
//

#include <Shlwapi.h>
#include <memory>
#include "../BattleAnimation.hpp"

#define TRANSLATE_MAX 120
#define REISEN_START_LOCATION 570
#define CAM_START_LOCATION (-20)
#define BG_START_LOCATION 850
#define HIT_STOP 7
#define PUSH_RATIO 2

#define spawnSubObject(chr, id, x, y) do {        \
	float something[3];                        \
	                                           \
	memset(something, 0, sizeof(something));   \
	something[2] = 1;                          \
	((void (__thiscall *)(SokuLib::CharacterManager &, int, float, float, char, int, float *, int))0x46EB30)(chr, id, x, y, 1, 1, something, 3); \
} while (0)
#define updateSubObjects(chr) ((void (__thiscall *)(SokuLib::ObjListManager &))0x633ce0)((chr).objects)

char profilePath[1024];
static const std::vector<std::string> dialogs{
	//Flandre walking happily
	//Remilia coming in
	"r SOh, hello",
	"r HI didn't know you were making a party!",
	"lEHWell, me neither.",
	"lCHLet me guess, you're here to \"play\" with the guests?",
	"rChOf course!",
	"lchCould you not involve yourself in this?",
	"lChIt is already hard to keep track of the situation",
	"rCEBut, sister...",
	"lhE*sigh* Fine, I will play with you.",
	"rhHReally?",
	"lHHOf course! I'm not a bad sister after all.",
	"lAHNow don't move."
	//Battle here
};

class Intro : public BattleAnimation {
private:
	SokuLib::DrawUtils::Sprite _stageBg;
	SokuLib::DrawUtils::Sprite _stageBottom;
	bool _falling = true;
	bool _remiIn = false;
	unsigned _fightCtr = 120;
	unsigned _fightStage = 0;
	unsigned _ctr = 240;
	unsigned _ctr2;
	unsigned _ctr3 = 0;
	unsigned _currentStage = 0;
	std::unique_ptr<SokuStand> _dialog;
	bool _keyPressed = false;
	bool _stop = false;
	SokuLib::PlayerInfo _playerInfo;
	int buffer[0xA];
	SokuLib::CharacterManager *_reisen = nullptr;

	void fightStage0(SokuLib::BattleManager &battleMgr)
	{
		this->_reisen->objectBase.position += this->_reisen->objectBase.speed;
		battleMgr.rightCharacterManager.objectBase.position += battleMgr.rightCharacterManager.objectBase.speed;
		this->_reisen->objectBase.speed.y -= 0.6;
		battleMgr.rightCharacterManager.objectBase.speed.y -= 0.6;
		if (this->_reisen->objectBase.position.y <= 300 && this->_reisen->objectBase.action != SokuLib::ACTION_j5A) {
			this->_reisen->objectBase.action = SokuLib::ACTION_j5A;
			this->_reisen->objectBase.animate();
		}
		if (battleMgr.rightCharacterManager.objectBase.position.y <= 200) {
			battleMgr.rightCharacterManager.objectBase.speed = {0, 0};
			battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_FORWARD_AIRDASH;
			battleMgr.rightCharacterManager.objectBase.animate();
			this->_fightStage++;
		}
		if (REISEN_START_LOCATION - this->_reisen->objectBase.position.x + CAM_START_LOCATION < TRANSLATE_MAX) {
			SokuLib::camera.translate.x = REISEN_START_LOCATION - this->_reisen->objectBase.position.x + CAM_START_LOCATION;
			SokuLib::camera.backgroundTranslate.x = this->_reisen->objectBase.position.x - REISEN_START_LOCATION + BG_START_LOCATION;
		} else {
			SokuLib::camera.translate.x = TRANSLATE_MAX;
			SokuLib::camera.backgroundTranslate.x = CAM_START_LOCATION - TRANSLATE_MAX + BG_START_LOCATION;
		}
	}

	void fightStage1(SokuLib::BattleManager &battleMgr)
	{
		this->_reisen->objectBase.position += this->_reisen->objectBase.speed;
		battleMgr.rightCharacterManager.objectBase.position += battleMgr.rightCharacterManager.objectBase.speed;
		if (this->_falling)
			this->_reisen->objectBase.speed.y -= 0.6;
		if (this->_reisen->objectBase.action == SokuLib::ACTION_j5A) {
			if (this->_reisen->objectBase.frameCount == 8) {
				SokuLib::playSEWaveBuffer(27);
			}
			if (this->_reisen->objectBase.position.y <= 0) {
				SokuLib::playSEWaveBuffer(30);
				this->_reisen->objectBase.speed.x = -4;
				this->_reisen->objectBase.speed.y = 0;
				this->_reisen->objectBase.position.y = 0;
				this->_reisen->objectBase.action = SokuLib::ACTION_WALK_BACKWARD;
				this->_reisen->objectBase.animate();
				this->_falling = false;
			}
		} else if (
			this->_reisen->objectBase.action == SokuLib::ACTION_RIGHTBLOCK_HIGH_MEDIUM_BLOCKSTUN ||
			this->_reisen->objectBase.action == SokuLib::ACTION_RIGHTBLOCK_LOW_BIG_BLOCKSTUN
		) {
			if (this->_reisen->objectBase.speed.x > 0)
				this->_reisen->objectBase.speed.x += 0.6;
			else
				this->_reisen->objectBase.speed.x = 0;
		} else if (this->_reisen->objectBase.action == SokuLib::ACTION_BE2) {
			if (this->_reisen->objectBase.actionBlockId == 1 && this->_reisen->objectBase.frameCount == 0) {
				this->_reisen->objectBase.speed.y = 20;
				SokuLib::playSEWaveBuffer(43);
			} else if (this->_reisen->objectBase.actionBlockId >= 1) {
				this->_reisen->objectBase.speed.y -= 0.75;
			}
		}

		if (this->_remiIn) {
			battleMgr.leftCharacterManager.objectBase.doAnimation();
			battleMgr.leftCharacterManager.objectBase.doAnimation();
			battleMgr.leftCharacterManager.objectBase.position = battleMgr.leftCharacterManager.objectBase.position + battleMgr.leftCharacterManager.objectBase.speed;
			if (battleMgr.leftCharacterManager.objectBase.position.y <= 0) {
				if (battleMgr.leftCharacterManager.objectBase.speed.y < 0) {
					battleMgr.leftCharacterManager.objectBase.speed.x = 20;
					battleMgr.leftCharacterManager.objectBase.speed.y = 0;
					battleMgr.leftCharacterManager.objectBase.animate2();
				}
				if (battleMgr.leftCharacterManager.objectBase.speed.x != 0)
					battleMgr.leftCharacterManager.objectBase.speed.x = battleMgr.leftCharacterManager.objectBase.speed.x - 2;
				else if (battleMgr.leftCharacterManager.objectBase.actionBlockId == 0) {
					this->_fightStage++;
					this->_dialog->setHidden(false);
					battleMgr.leftCharacterManager.objectBase.action = SokuLib::ACTION_IDLE;
					battleMgr.leftCharacterManager.objectBase.animate();
				} else if (battleMgr.leftCharacterManager.objectBase.actionBlockId != 2)
					battleMgr.leftCharacterManager.objectBase.animate2();
			} else
				battleMgr.leftCharacterManager.objectBase.speed.y = battleMgr.leftCharacterManager.objectBase.speed.y - 2;
		}

		if (this->_reisen->objectBase.action == SokuLib::ACTION_NEUTRAL_HIGH_JUMP) {
			if (this->_reisen->objectBase.actionBlockId == 1 && this->_reisen->objectBase.frameCount == 0) {
				this->_reisen->objectBase.speed.y = 20;
				SokuLib::playSEWaveBuffer(43);
			} else if (this->_reisen->objectBase.actionBlockId >= 1) {
				this->_reisen->objectBase.speed.y -= 0.75;
			}
		}

		if (battleMgr.rightCharacterManager.objectBase.position.x < this->_reisen->objectBase.position.x + 75) {
			this->_reisen->objectBase.position.x += battleMgr.rightCharacterManager.objectBase.speed.x / PUSH_RATIO;
			if (battleMgr.rightCharacterManager.objectBase.position.x < this->_reisen->objectBase.position.x + 75)
				battleMgr.rightCharacterManager.objectBase.position.x = this->_reisen->objectBase.position.x + 75;
		}

		if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_NEUTRAL_HIGH_JUMP) {
			if (battleMgr.rightCharacterManager.objectBase.actionBlockId == 1 && battleMgr.rightCharacterManager.objectBase.frameCount == 0) {
				battleMgr.rightCharacterManager.objectBase.speed.y = 23;
				SokuLib::playSEWaveBuffer(43);
			} else if (battleMgr.rightCharacterManager.objectBase.actionBlockId >= 1) {
				battleMgr.rightCharacterManager.objectBase.speed.y--;
			}
		} else if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_3A) {
			this->_ctr2++;
			if (this->_ctr2 == 15) {
				battleMgr.leftCharacterManager.objectBase.animate();
				battleMgr.leftCharacterManager.objectBase.animate2();
				battleMgr.leftCharacterManager.objectBase.speed = SokuLib::Vector2f{30, 12};
				battleMgr.leftCharacterManager.objectBase.position.y = 0;
				battleMgr.leftCharacterManager.playSE(10);
				this->_remiIn = true;
			}
			if (battleMgr.rightCharacterManager.objectBase.frameCount == 10) {
				SokuLib::playSEWaveBuffer(29);
				battleMgr.rightCharacterManager.objectBase.speed.x = -13;
			} else if (battleMgr.rightCharacterManager.objectBase.frameCount == 14) {
				SokuLib::playSEWaveBuffer(20);
				this->_fightCtr = HIT_STOP;
				this->_reisen->objectBase.speed.x = -13;
				this->_reisen->objectBase.action = SokuLib::ACTION_RIGHTBLOCK_LOW_BIG_BLOCKSTUN;
				this->_reisen->objectBase.animate();
			} else if (this->_ctr2 == 20) {
				this->_reisen->objectBase.speed.x = 0;
				this->_reisen->objectBase.action = SokuLib::ACTION_BE2;
				this->_reisen->objectBase.animate();
				SokuLib::playSEWaveBuffer(35);
			} else if (this->_ctr2 == 25) {
				this->_ctr2 = 0;
				battleMgr.rightCharacterManager.objectBase.speed.x = 0;
				battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_NEUTRAL_HIGH_JUMP;
				battleMgr.rightCharacterManager.objectBase.animate();
			}
		} else if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_2A) {
			if (battleMgr.rightCharacterManager.objectBase.frameCount == 8) {
				SokuLib::playSEWaveBuffer(27);
				SokuLib::playSEWaveBuffer(20);
				this->_fightCtr = HIT_STOP;
				this->_reisen->objectBase.action = SokuLib::ACTION_RIGHTBLOCK_LOW_SMALL_BLOCKSTUN;
				this->_reisen->objectBase.animate();
			} else if (battleMgr.rightCharacterManager.objectBase.frameCount == 9) {
				battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_3A;
				battleMgr.rightCharacterManager.objectBase.animate();
				this->_ctr2 = 0;
			}
		} else if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_j5AA) {
			if (battleMgr.rightCharacterManager.objectBase.frameCount == 5) {
				SokuLib::playSEWaveBuffer(27);
				SokuLib::playSEWaveBuffer(20);
				this->_fightCtr = HIT_STOP;
				this->_reisen->objectBase.speed.x = -10;
				this->_reisen->objectBase.action = SokuLib::ACTION_RIGHTBLOCK_HIGH_MEDIUM_BLOCKSTUN;
				this->_reisen->objectBase.animate();
			}
			if (battleMgr.rightCharacterManager.objectBase.position.y <= 0) {
				battleMgr.rightCharacterManager.objectBase.speed = {0, 0};
				battleMgr.rightCharacterManager.objectBase.position.y = 0;
				battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_2A;
				battleMgr.rightCharacterManager.objectBase.animate();
				SokuLib::playSEWaveBuffer(30);
			}
		} else if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_j5A) {
			if (battleMgr.rightCharacterManager.objectBase.frameCount == 8) {
				SokuLib::playSEWaveBuffer(27);
				SokuLib::playSEWaveBuffer(20);
				this->_fightCtr = HIT_STOP;
				this->_reisen->objectBase.speed.x = -10;
				this->_reisen->objectBase.action = SokuLib::ACTION_RIGHTBLOCK_HIGH_MEDIUM_BLOCKSTUN;
				this->_reisen->objectBase.animate();
			} else if (battleMgr.rightCharacterManager.objectBase.frameCount == 9) {
				battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_j5AA;
				battleMgr.rightCharacterManager.objectBase.animate();
			}
		} else {
			if (battleMgr.rightCharacterManager.objectBase.actionBlockId == 1 && battleMgr.rightCharacterManager.objectBase.frameCount == 0) {
				battleMgr.rightCharacterManager.objectBase.speed = {-30, -4};
				SokuLib::playSEWaveBuffer(31); // Dashing sound
			} else if (battleMgr.rightCharacterManager.objectBase.actionBlockId == 1 && battleMgr.rightCharacterManager.objectBase.frameCount == 8) {
				battleMgr.rightCharacterManager.objectBase.speed.x = -10;
			} else if (battleMgr.rightCharacterManager.objectBase.actionBlockId == 2) {
				battleMgr.rightCharacterManager.objectBase.speed.y = -4.5;
				battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_j5A;
				battleMgr.rightCharacterManager.objectBase.animate();
			}
		}
		if (REISEN_START_LOCATION - this->_reisen->objectBase.position.x + CAM_START_LOCATION < TRANSLATE_MAX) {
			SokuLib::camera.translate.x = REISEN_START_LOCATION - this->_reisen->objectBase.position.x + CAM_START_LOCATION;
			SokuLib::camera.backgroundTranslate.x = this->_reisen->objectBase.position.x - REISEN_START_LOCATION + BG_START_LOCATION;
		} else {
			SokuLib::camera.translate.x = TRANSLATE_MAX;
			SokuLib::camera.backgroundTranslate.x = CAM_START_LOCATION - TRANSLATE_MAX + BG_START_LOCATION;
		}
	}

	void fightStage2(SokuLib::BattleManager &battleMgr)
	{
		this->_reisen->objectBase.position += this->_reisen->objectBase.speed;
		battleMgr.rightCharacterManager.objectBase.position += battleMgr.rightCharacterManager.objectBase.speed;

		this->_ctr3++;
		if (this->_ctr3 == 5) {
			battleMgr.rightCharacterManager.objectBase.speed.y = 2.66;
			battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_j6A;
			battleMgr.rightCharacterManager.objectBase.animate();
			this->_reisen->objectBase.action = SokuLib::ACTION_j2B;
			this->_reisen->objectBase.animate();
		}

		if (this->_reisen->objectBase.action == SokuLib::ACTION_BE2) {
			if (this->_reisen->objectBase.actionBlockId == 1 && this->_reisen->objectBase.frameCount == 0) {
				this->_reisen->objectBase.speed.y = 20;
				SokuLib::playSEWaveBuffer(43);
			} else if (this->_reisen->objectBase.actionBlockId >= 1) {
				this->_reisen->objectBase.speed.y -= 0.75;
			}
		} else if (this->_reisen->objectBase.position.x < -500) {
			battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_FLY;
			battleMgr.rightCharacterManager.objectBase.animate();
			battleMgr.rightCharacterManager.objectBase.speed = {0, 0};
			this->_reisen->objectBase.speed = {0, 0};
			this->_reisen->objectBase.position.x = -490;
		}

		battleMgr.leftCharacterManager.objectBase.doAnimation();
		if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_NEUTRAL_HIGH_JUMP) {
			if (battleMgr.rightCharacterManager.objectBase.actionBlockId == 1 && battleMgr.rightCharacterManager.objectBase.frameCount == 0) {
				battleMgr.rightCharacterManager.objectBase.speed.y = 23;
				SokuLib::playSEWaveBuffer(43);
			} else if (battleMgr.rightCharacterManager.objectBase.actionBlockId >= 1) {
				battleMgr.rightCharacterManager.objectBase.speed.y--;
			}
		} else if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_3A) {
			this->_ctr2++;
			if (battleMgr.rightCharacterManager.objectBase.frameCount == 10) {
				SokuLib::playSEWaveBuffer(29);
				battleMgr.rightCharacterManager.objectBase.speed.x = -13;
			} else if (battleMgr.rightCharacterManager.objectBase.frameCount == 14) {
				SokuLib::playSEWaveBuffer(20);
				this->_fightCtr = HIT_STOP;
				this->_reisen->objectBase.speed.x = -13;
				this->_reisen->objectBase.action = SokuLib::ACTION_RIGHTBLOCK_LOW_BIG_BLOCKSTUN;
				this->_reisen->objectBase.animate();
			} else if (this->_ctr2 == 20) {
				this->_reisen->objectBase.speed.x = 0;
				this->_reisen->objectBase.action = SokuLib::ACTION_BE2;
				this->_reisen->objectBase.animate();
				SokuLib::playSEWaveBuffer(35);
			} else if (this->_ctr2 == 25) {
				this->_ctr2 = 0;
				battleMgr.rightCharacterManager.objectBase.speed.x = 0;
				battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_NEUTRAL_HIGH_JUMP;
				battleMgr.rightCharacterManager.objectBase.animate();
			}
		} else if (battleMgr.rightCharacterManager.objectBase.position.y < 0 && battleMgr.rightCharacterManager.objectBase.action != SokuLib::ACTION_LANDING) {
			battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_LANDING;
			battleMgr.rightCharacterManager.objectBase.animate();
			battleMgr.rightCharacterManager.objectBase.position.y = 0;
			battleMgr.rightCharacterManager.objectBase.speed = {0, 0};
			SokuLib::playSEWaveBuffer(30);
		} else if (
			battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_LANDING &&
			battleMgr.rightCharacterManager.objectBase.actionBlockId == 0 &&
			battleMgr.rightCharacterManager.objectBase.frameCount == 0
		) {
			battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_IDLE;
			battleMgr.rightCharacterManager.objectBase.animate();
			this->_dialog->onKeyPress();
			this->_fightStage++;
		} else if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_FALLING)
			battleMgr.rightCharacterManager.objectBase.speed.y -= 0.6;
		else if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_FLY) {
			if (battleMgr.rightCharacterManager.objectBase.actionBlockId == 1 && battleMgr.rightCharacterManager.objectBase.frameCount == 0) {
				battleMgr.rightCharacterManager.objectBase.speed.y = -25;
				SokuLib::playSEWaveBuffer(31);
			} else if (battleMgr.rightCharacterManager.objectBase.actionBlockId == 1 && battleMgr.rightCharacterManager.objectBase.frameCount == 12)
				battleMgr.rightCharacterManager.objectBase.speed.y = -4;
			else if (battleMgr.rightCharacterManager.objectBase.actionBlockId > 1 || battleMgr.rightCharacterManager.objectBase.frameCount == 14)
				battleMgr.rightCharacterManager.objectBase.speed.y -= 0.6;
		} else if (battleMgr.rightCharacterManager.objectBase.action == SokuLib::ACTION_j6A) {
			if (battleMgr.rightCharacterManager.objectBase.actionBlockId == 0 && battleMgr.rightCharacterManager.objectBase.frameCount == 12) {
				SokuLib::playSEWaveBuffer(29);
				SokuLib::playSEWaveBuffer(25);
				battleMgr.rightCharacterManager.objectBase.speed = {7.5, 9.86};
				this->_reisen->objectBase.action = SokuLib::ACTION_AIR_HIT_BIG_HITSTUN;
				this->_reisen->objectBase.animate();
				this->_reisen->damageLimited = true;
				this->_reisen->realLimit = 100;
				this->_reisen->objectBase.speed.x = -20;
				this->_fightCtr = HIT_STOP * 2;
			} else if (battleMgr.rightCharacterManager.objectBase.actionBlockId != 0 || battleMgr.rightCharacterManager.objectBase.frameCount > 12)
				battleMgr.rightCharacterManager.objectBase.speed.y -= 0.6;
		}
	}

	std::vector<void (Intro::*)(SokuLib::BattleManager &)> _fightStages{
		&Intro::fightStage0,
		&Intro::fightStage1,
		&Intro::fightStage2
	};

	bool updateFight()
	{
		auto &battleMgr = SokuLib::getBattleMgr();

		if (this->_fightCtr) {
			this->_fightCtr--;
			return true;
		}
		//if (this->_fightStage == 2 && !this->_keyPressed) {
		//	return true;
		//}
		this->_keyPressed = false;
		updateSubObjects(*this->_reisen);
		this->_reisen->objectBase.doAnimation();
		battleMgr.rightCharacterManager.objectBase.doAnimation();
		(this->*this->_fightStages[this->_fightStage])(battleMgr);
		return this->_fightStage < this->_fightStages.size();
	}

	void stage0()
	{
		auto &battleMgr = SokuLib::getBattleMgr();

		if (this->_ctr == 240) {
			this->_playerInfo.character = SokuLib::CHARACTER_REISEN;
			this->_playerInfo.isRight = false;
			this->_playerInfo.palette = 0;
			this->_playerInfo.padding2 = 0;
			this->_playerInfo.deck = 0;
			this->_playerInfo.effectiveDeck.unknown1 = 0;
			this->_playerInfo.effectiveDeck.data = nullptr;
			this->_playerInfo.effectiveDeck.chunkSize = 0;
			this->_playerInfo.effectiveDeck.counter = 0;
			this->_playerInfo.effectiveDeck.size = 0;
			this->_playerInfo.keyManager = nullptr;

			((void (__thiscall *)(int *, bool, SokuLib::PlayerInfo &))0x46da40)(this->buffer, false, this->_playerInfo);
			(*(void (__thiscall **)(SokuLib::CharacterManager *))(*(int *)this->_reisen + 0x44))(this->_reisen);
			*(SokuLib::CharacterManager **)&this->_reisen->objectBase.offset_0x168[8] = &battleMgr.rightCharacterManager;

			SokuLib::camera.scale = 0.8;
			SokuLib::camera.translate.x = CAM_START_LOCATION;
			SokuLib::camera.translate.y = 525;
			SokuLib::camera.backgroundTranslate.x = BG_START_LOCATION;

			battleMgr.leftCharacterManager.objectBase.position.x = -500;
			battleMgr.leftCharacterManager.objectBase.action = SokuLib::ACTION_DEFAULT_SKILL1_B;

			battleMgr.rightCharacterManager.objectBase.position.y = 525;
			battleMgr.rightCharacterManager.objectBase.action = SokuLib::ACTION_FALLING;
			battleMgr.rightCharacterManager.objectBase.animate();

			this->_reisen->objectBase.position.x = REISEN_START_LOCATION;
			this->_reisen->objectBase.position.y = 525;
			this->_reisen->objectBase.action = SokuLib::ACTION_FALLING;
			this->_reisen->objectBase.animate();

			((void (*)(const char *))0x43ff10)("data/bgm/st20.ogg");
		}

		if (this->_ctr < 60) {
			if (this->_stageBg.tint.a)
				this->_stageBg.tint.a -= 0xF;
			if (this->_stageBottom.tint.a)
				this->_stageBottom.tint.a -= 0xF;
		} else {
			if (this->_stageBg.tint.a != 0xFF)
				this->_stageBg.tint.a += 0xF;
			if (this->_stageBottom.tint.a != 0xFF)
				this->_stageBottom.tint.a += 0xF;
		}

		updateFight();
		if (this->_ctr % 2)
			this->_stageBottom.setPosition(this->_stageBottom.getPosition() + SokuLib::Vector2i{1, 0});
		this->_ctr--;
		if (!this->_ctr)
			this->_currentStage++;
	}

	void stage1()
	{
		if (!updateFight()) {
			this->_currentStage++;
			((void (*)(const char *))0x43ff10)("data/bgm/ta08.ogg");
		}
	}

	void stage2()
	{
		auto &battleMgr = SokuLib::getBattleMgr();

		battleMgr.leftCharacterManager.objectBase.doAnimation();
		battleMgr.rightCharacterManager.objectBase.doAnimation();
	}

	std::vector<void (Intro::*)()> anims{
		&Intro::stage0,
		&Intro::stage1,
		&Intro::stage2
	};

public:
	Intro()
	{
		puts("Init intro.");

		this->_stageBg.texture.loadFromFile((profilePath + std::string("\\bnbcorn1intro.png")).c_str());
		this->_stageBg.setSize(this->_stageBg.texture.getSize());
		this->_stageBg.setPosition({
			static_cast<int>(320 - this->_stageBg.texture.getSize().x / 2),
			-50
		});
		this->_stageBg.rect.width  = this->_stageBg.texture.getSize().x;
		this->_stageBg.rect.height = this->_stageBg.texture.getSize().y;
		this->_stageBg.tint.a = 0;

		this->_stageBottom.texture.loadFromGame("data/scenario/effect/Stage6.png");
		this->_stageBottom.setSize(this->_stageBottom.texture.getSize());
		this->_stageBottom.setPosition({
			static_cast<int>(320 - this->_stageBg.texture.getSize().x / 2),
			static_cast<int>(-50 + this->_stageBg.texture.getSize().y)
		});
		this->_stageBottom.rect.width  = this->_stageBottom.texture.getSize().x;
		this->_stageBottom.rect.height = this->_stageBottom.texture.getSize().y;
		this->_stageBottom.tint.a = 0;

		this->_dialog = std::make_unique<SokuStand>(dialogs);
	}

	~Intro()
	{
		if (this->_reisen) {
			(*(void (__thiscall **)(SokuLib::CharacterManager *, char))this->_reisen->objectBase.vtable)(this->_reisen, 0);
			SokuLib::Delete(this->_reisen);
		}
	}

	bool update() override
	{
		this->_dialog->update();
		if (this->_currentStage < this->anims.size())
			(this->*this->anims[this->_currentStage])();

		if (this->_keyPressed) {
			this->_stop |= !this->_dialog->onKeyPress();
			puts(this->_stop ? "Stop" : "Not stop");
			if (this->_stop)
				this->_dialog->setHidden(true);
			this->_keyPressed = false;
		}
		return !this->_stop || !this->_dialog->isAnimationFinished();
	}

	void render() const override
	{
		if (this->_currentStage <= 1 && this->_reisen) {
			// Display the CharacterManager
			((void (__thiscall *)(SokuLib::CharacterManager &))0x438d20)(*this->_reisen);

			// Display the CharacterManager subobjects
			((void (__thiscall *)(SokuLib::ObjListManager &, int))0x59be00)(this->_reisen->objects, -1);
			((void (__thiscall *)(SokuLib::ObjListManager &, int))0x59be00)(this->_reisen->objects, 1);

			// We redraw Remilia because Reisen's subobjects mess up the DirectX context and this will clean it up
			((void (__thiscall *)(SokuLib::CharacterManager &))0x438d20)(SokuLib::getBattleMgr().leftCharacterManager);
		}
		if (this->_ctr) {
			this->_stageBg.draw();
			this->_stageBottom.draw();
		}
		this->_dialog->render();
	}

	void onKeyPressed() override
	{
		this->_keyPressed = true;
	}
};

extern "C"
{
	__declspec(dllexport) BattleAnimation *getIntro()
	{
		return new Intro();
	}

	int APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
	{
		if (fdwReason != DLL_PROCESS_ATTACH)
			return TRUE;
		GetModuleFileName(hModule, profilePath, 1024);
		PathRemoveFileSpec(profilePath);
		return TRUE;
	}
}












//struct Node {
//	Node *higher;
//	Node *head;
//	Node *lower;
//	int a;
//	char _0x10[0x5];
//	bool isBottom;
//};
//
//struct BinTree {
//	Node *array2;
//	Node *array;
//};
//
//Node *** __thiscall FUN_0043f2f0(BinTree *This, Node ***param_1, int *search)
//{
//	Node **ppiVar1;
//	Node *currentNode;
//	Node *nextNode;
//	Node ***ppiVar4;
//	Node **local_10;
//	Node *firstNode;
//	Node **local_8;
//	Node *local_4;
//
//	firstNode = This->array->higher;
//	if (!firstNode->lower->isBottom) {
//		currentNode = firstNode->head;
//		do {
//			if (currentNode->a < *search) {
//				nextNode = currentNode->lower;
//			}
//			else {
//				nextNode = currentNode->higher;
//				firstNode = currentNode;
//			}
//			currentNode = nextNode;
//		} while (nextNode->isBottom);
//	}
//	local_10 = &This->array2;
//	if (firstNode == This->array->higher || *search < firstNode->a) {
//		local_8 = &This->array2;
//		local_4 = This->array;
//		ppiVar4 = &local_8;
//	}
//	else {
//		ppiVar4 = &local_10;
//	}
//	ppiVar1 = ppiVar4[1];
//	param_1[0] = *ppiVar4;
//	param_1[1] = ppiVar1;
//	return param_1;
//}