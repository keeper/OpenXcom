/*
 * Copyright 2010-2013 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ManageAlienContainmentState.h"
#include <sstream>
#include <climits>
#include <cmath>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Armor.h"
#include "../Engine/Timer.h"
#include "../Engine/Options.h"
#include "../Menu/ErrorMessageState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Manage Alien Containment screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param origin Game section that originated this state.
 */
ManageAlienContainmentState::ManageAlienContainmentState(Game *game, Base *base, OptionsOrigin origin) : State(game), _base(base), _qtys(), _aliens(), _sel(0), _aliensSold(0), _researchedAliens(0)
{
	_changeValueByMouseWheel = Options::getInt("changeValueByMouseWheel");
	_allowChangeListValuesByMouseWheel = (Options::getBool("allowChangeListValuesByMouseWheel") && _changeValueByMouseWheel);
	_containmentLimit = Options::getBool("alienContainmentLimitEnforced");
	_overCrowded = _containmentLimit && _base->getAvailableContainment() < _base->getUsedContainment();

	for(std::vector<ResearchProject*>::const_iterator iter = _base->getResearch().begin (); iter != _base->getResearch().end (); ++iter)
	{
		const RuleResearch *research = (*iter)->getRules();
		if (_game->getRuleset()->getUnit(research->getName()))
		{
			++_researchedAliens;
		}
	}

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(_overCrowded? 288:148, 16, _overCrowded? 16:8, 176);
	_btnCancel = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtAvailable =  new Text(190, 9, 10, 24);
	_txtUsed = new Text(110, 9, 136, 24);
	_txtItem = new Text(130, 9, 10, 32);
	_txtLiveAliens = new Text(54, 9, 136, 32);
	_txtDeadAliens = new Text(96, 9, 220, 32);
	_lstAliens = new TextList(280, 120, 8, 44);

	// Set palette
	if (origin == OPT_BATTLESCAPE)
	{
		_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)), Palette::backPos, 16);
		_color  = Palette::blockOffset(15)-1;
		_color2 = Palette::blockOffset(8)+10;
	}
	else
	{
		_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(1)), Palette::backPos, 16);
		_color  = Palette::blockOffset(13)+10;
		_color2 = Palette::blockOffset(13);
	}

	add(_window);
	add(_btnOk);
	add(_btnCancel);
	add(_txtTitle);
	add(_txtAvailable);
	add(_txtUsed);
	add(_txtItem);
	add(_txtLiveAliens);
	add(_txtDeadAliens);
	add(_lstAliens);

	centerAllSurfaces();

	// Set up objects
	_window->setColor(_color);
	_window->setBackground(_game->getResourcePack()->getSurface((origin == OPT_BATTLESCAPE)? "BACK01.SCR" : "BACK05.SCR"));

	_btnOk->setColor(_color);
	_btnOk->setText(tr("STR_REMOVE_SELECTED"));
	_btnOk->onMouseClick((ActionHandler)&ManageAlienContainmentState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ManageAlienContainmentState::btnOkClick, (SDLKey)Options::getInt("keyOk"));

	_btnCancel->setColor(_color);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&ManageAlienContainmentState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&ManageAlienContainmentState::btnCancelClick, (SDLKey)Options::getInt("keyCancel"));

	if (_overCrowded)
	{
		_btnCancel->setVisible(false);
		_btnOk->setVisible(false);
	}

	_txtTitle->setColor((origin == OPT_BATTLESCAPE)? Palette::blockOffset(8)+5 : _color);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_MANAGE_CONTAINMENT"));

	_txtItem->setColor(_color);
	_txtItem->setText(tr("STR_ALIEN"));

	_txtLiveAliens->setColor(_color);
	_txtLiveAliens->setText(tr("STR_LIVE_ALIENS"));

	_txtDeadAliens->setColor(_color);
	_txtDeadAliens->setText(tr("STR_DEAD_ALIENS"));

	_txtAvailable->setColor(_color);
	_txtAvailable->setSecondaryColor(_color2);
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(_base->getAvailableContainment() - _base->getUsedContainment()));

	_txtUsed->setColor(_color);
	_txtUsed->setSecondaryColor(_color2);
	_txtUsed->setText(tr("STR_SPACE_USED").arg( _base->getUsedContainment() - _researchedAliens));

	_lstAliens->setColor(_color2);
	_lstAliens->setArrowColor(_color);
	_lstAliens->setArrowColumn(184, ARROW_HORIZONTAL);
	_lstAliens->setColumns(3, 150, 84, 46);
	_lstAliens->setSelectable(true);
	_lstAliens->setBackground(_window);
	_lstAliens->setMargin(2);
	_lstAliens->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
	_lstAliens->onLeftArrowPress((ActionHandler)&ManageAlienContainmentState::lstItemsLeftArrowPress);
	_lstAliens->onLeftArrowRelease((ActionHandler)&ManageAlienContainmentState::lstItemsLeftArrowRelease);
	_lstAliens->onLeftArrowClick((ActionHandler)&ManageAlienContainmentState::lstItemsLeftArrowClick);
	_lstAliens->onRightArrowPress((ActionHandler)&ManageAlienContainmentState::lstItemsRightArrowPress);
	_lstAliens->onRightArrowRelease((ActionHandler)&ManageAlienContainmentState::lstItemsRightArrowRelease);
	_lstAliens->onRightArrowClick((ActionHandler)&ManageAlienContainmentState::lstItemsRightArrowClick);
	_lstAliens->onMousePress((ActionHandler)&ManageAlienContainmentState::lstItemsMousePress);

	const std::vector<std::string> &items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		int qty = _base->getItems()->getItem(*i);
		if (qty > 0 && _game->getRuleset()->getItem(*i)->getAlien())
		{
			_qtys.push_back(0);
			_aliens.push_back(*i);
			std::wstringstream ss;
			ss << qty;
			_lstAliens->addRow(3, tr(*i).c_str(), ss.str().c_str(), L"0");
		}
	}

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)&ManageAlienContainmentState::increase);
	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)&ManageAlienContainmentState::decrease);
}

/**
 *
 */
ManageAlienContainmentState::~ManageAlienContainmentState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void ManageAlienContainmentState::think()
{
	State::think();

	_timerInc->think(this, 0);
	_timerDec->think(this, 0);
}

/**
 * Deals with the selected aliens.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::btnOkClick(Action *)
{
	for (unsigned int i = 0; i < _qtys.size(); ++i)
	{
		if (_qtys[i] > 0)
		{
			// remove the aliens
			_base->getItems()->removeItem(_aliens[i], _qtys[i]);

			// add the corpses
			_base->getItems()->addItem(
				_game->getRuleset()->getArmor(
					_game->getRuleset()->getUnit(
						_aliens[i]
					)->getArmor()
				)->getCorpseItem(), _qtys[i]
			); // ;)
		}
	}
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Starts increasing the alien count.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsRightArrowPress(Action *action)
{
	_sel = _lstAliens->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerInc->isRunning()) _timerInc->start();
}

/**
 * Stops increasing the alien count.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsRightArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the selected alien count;
 * by one on left-click, to max on right-click.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsRightArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) increaseByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		increaseByValue(1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the alien count.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsLeftArrowPress(Action *action)
{
	_sel = _lstAliens->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerDec->isRunning()) _timerDec->start();
}

/**
 * Stops decreasing the alien count.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsLeftArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the selected alien count;
 * by one on left-click, to 0 on right-click.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsLeftArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) decreaseByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		decreaseByValue(1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsMousePress(Action *action)
{
	_sel = _lstAliens->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (_allowChangeListValuesByMouseWheel
			&& action->getAbsoluteXMouse() >= _lstAliens->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstAliens->getArrowsRightEdge())
		{
			increaseByValue(_changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (_allowChangeListValuesByMouseWheel
			&& action->getAbsoluteXMouse() >= _lstAliens->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstAliens->getArrowsRightEdge())
		{
			decreaseByValue(_changeValueByMouseWheel);
		}
	}
}

/**
 * Gets the quantity of the currently selected alien on the base.
 * @return Quantity of selected alien on the base.
 */
int ManageAlienContainmentState::getQuantity()
{
	return _base->getItems()->getItem(_aliens[_sel]);
}

/**
 * Increases the quantity of the selected alien to exterminate by one.
 */
void ManageAlienContainmentState::increase()
{
	_timerDec->setInterval(50);
	_timerInc->setInterval(50);
	increaseByValue(1);
}

/**
 * Increases the quantity of the selected alien to exterminate by "change".
 * @param change How much we want to add.
 */
void ManageAlienContainmentState::increaseByValue(int change)
{
	int qty = getQuantity() - _qtys[_sel];
	if (0 >= change || qty <= 0) return;

	change = std::min(qty, change);
	_qtys[_sel] += change;
	_aliensSold += change;
	updateStrings();
}

/**
 * Decreases the quantity of the selected alien to exterminate by one.
 */
void ManageAlienContainmentState::decrease()
{
	_timerInc->setInterval(50);
	_timerDec->setInterval(50);
	decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected alien to exterminate by "change".
 * @param change How much we want to remove.
 */
void ManageAlienContainmentState::decreaseByValue(int change)
{
	if (0 >= change || 0 >= _qtys[_sel]) return;
	change = std::min(_qtys[_sel], change);
	_qtys[_sel] -= change;
	_aliensSold -= change;
	updateStrings();
}

/**
 * Updates the quantity-strings of the selected alien.
 */
void ManageAlienContainmentState::updateStrings()
{
	std::wstringstream ss, ss2, ss3;
	int qty = getQuantity() - _qtys[_sel];
	ss << qty;
	ss2 << _qtys[_sel];

	_lstAliens->setRowColor(_sel, (qty != 0)? _color2 : _color);
	_lstAliens->setCellText(_sel, 1, ss.str());
	_lstAliens->setCellText(_sel, 2, ss2.str());

	int aliens = _base->getUsedContainment() - _aliensSold - _researchedAliens;
	int spaces = _base->getAvailableContainment() - _base->getUsedContainment() + _aliensSold;
	bool enoughSpace = _containmentLimit? spaces >= 0 : true;

	_btnCancel->setVisible(enoughSpace && !_overCrowded);
	_btnOk->setVisible(enoughSpace);
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(spaces));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(aliens));
}

}
