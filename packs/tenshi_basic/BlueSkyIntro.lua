--
-- Created by PinkySmile on 18/08/2021.
--

local dialogs = {
	"lW Hi, this intro's goal is to explain how the Blue Sky<br>weather works. If you already know it, feel free to skip<br>the intro.",
        "lH Blue Sky is the 4th weather on the cycle, it's a<br>combo-based weather that lets you cancel skills into<br>skills for half of their spirit cost",
        "lC This unlocks new pressure strings for a lot of<br>characters by making commital skills safe on block,<br>which is mostly useful on melee skills to catch grazing",
        "lc In Tenshi's kit, Blue Sky isn't that great since most<br>of her popular skills are really slow (Quake, d22) but<br>she can still use it with skills such as a236 Heaven",
        "lA It may be situational, but Tenshi can also use the<br>weather to unlock new combos, like after hitting an<br>opponent in the ceiling with d22",
        "lH Here's a fancy combo that uses Blue Sky to its complete<br>potential by only being composed of Skill Cancels",
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