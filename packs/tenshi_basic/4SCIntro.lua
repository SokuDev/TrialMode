--
-- Created by PinkySmile on 18/08/2021.
--

local dialogs = {
	"lW Hi, this intro's goal is to explain how Tenshi's 4SC<br>Temperament Meteorological Revelation works. If you<br>already know it, feel free to skip the intro.",
        "lH Tenshi's Weather Laser is probably her most unique and<br>best spellcard. In fact, this spell gets buffed to<br>horrifying levels when any weather is active",
		"lC While buffed, it is able to do more than 4K damage<br>and up to 2.5 orbs of spirit damage on block while<br>giving you great frame advantage",
		"lA Combined with its fast start up, this spellcard<br>becomes a really potent pressure reset, crushing tool<br>and heavy damage dealer from almost every grounded move",
		"lE But it isn't perfect, be wary of throwing it in the<br>corner since most of its hits will whiff, also don't<br>do long combos with it since its proration is awful",
		"lc Also know that 4SC deletes the current weather. This can<br>be used as a weather manipulation tool to avoid, for<br>exemple, Typhoon at a life deficit",
		"lH But this is a combo trial so here's a small combo using<br>the card.",
}

local dialog  = StandDialog.new(dialogs)
local pressed = false
local side = false
local ctr = 60
local flashRect = RectangleShape.new()

dialog.hidden = false
flashRect.borderColor = enums.colors.Transparent
flashRect.fillColor = enums.colors.Transparent
flashRect.size = Vector2u.new(640, 480)

function update()
	local color = flashRect.fillColor

	battleMgr.leftCharacterManager:updateAnimation()
	if color.a and ctr == 0 and side then
		dialog:update()
		if pressed then
			if not dialog:onKeyPress() then
				return false
			end
			pressed = false
		end
	end
	if not side then
		color.a = color.a + 0x11;
		if color.a == 0xFF then
			side = true;
			camera.translate.x = -320
			camera.translate.y = 420
			camera.backgroundTranslate.x = 640
			camera.backgroundTranslate.y = 0

			battleMgr.leftCharacterManager.position.x = 400
			battleMgr.leftCharacterManager.position.y = 0
			battleMgr.leftCharacterManager.actionBlockId = 0
			battleMgr.leftCharacterManager.animationCounter = 0
			battleMgr.leftCharacterManager.animationSubFrame = 0
			battleMgr.leftCharacterManager.action = enums.actions.ACTION_IDLE
			battleMgr.leftCharacterManager:initAnimation()

			battleMgr.rightCharacterManager.position.x = 800
			battleMgr.rightCharacterManager.position.y = 0
			battleMgr.rightCharacterManager.animationSubFrame = 0
			battleMgr.rightCharacterManager.action = enums.actions.ACTION_KNOCKED_DOWN_STATIC
			battleMgr.rightCharacterManager:initAnimation()
			battleMgr.leftCharacterManager.direction = enums.directions.RIGHT
			battleMgr.rightCharacterManager.direction = enums.directions.LEFT
		end
	elseif flashRect.fillColor.a ~= 0 then
		color.a = color.a - 0x11;
	elseif ctr ~= 0 then
		ctr = ctr - 1
	end
	flashRect.fillColor = color
	return true;
end

function render()
	flashRect:draw()
	if flashRect.fillColor.a == 0 and ctr == 0 and side then
		dialog:render()
	end
end

function onKeyPressed()
	if flashRect.fillColor.a and ctr == 0 and side then
		pressed = true
	end
end