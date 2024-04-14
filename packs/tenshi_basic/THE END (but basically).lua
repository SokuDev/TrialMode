--
-- Created by PinkySmile on 18/08/2021.
--

local dialogs = {
	"lW Thanks for playing through the trial!<br>If you have any feedback feel free to DM or ping me on<br>your average Soku Discord",
	"lH Just remember that this trial is mainly composed of<br>unoptimal combos but before you go attempt the<br>3 frame link optimals make sure to master these ones",
	"lA Having highly damaging combos is nice but if you drop<br>the limit constantly once you're in-game it's pretty<br>close to useless",
	"lC Anyway, I'll see you in the hostlist!"
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