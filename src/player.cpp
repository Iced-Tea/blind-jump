
//
//  player.cpp
//  BlindJump
//
//  Created by Evan Bowman on 10/9/15.
//  Copyright © 2015 Evan Bowman. All rights reserved.
//

#include "player.hpp"
#include "playerAnimationFunctions.hpp"
#include <cmath>
#include "playerCollisionFunctions.hpp"
#include "wall.hpp"
#include <tuple>
#include "scene.hpp"

Player::Player(float _xPos, float _yPos)
	: gun{},
	  health{4},
	  xPos{_xPos - 17}, // Magic number that puts the player in the direct center of the screen. Hmmm why does it work...
	  yPos{_yPos},
	  worldOffsetX{0.f},
	  worldOffsetY{0.f},
	  frameIndex{5},
	  sheetIndex{Sheet::stillDown},
	  cachedSheet{Sheet::stillDown},
	  lSpeed{0.f},
	  rSpeed{0.f},
	  uSpeed{0.f},
	  dSpeed{0.f},
	  animationTimer{0},
	  dashTimer{0},
	  invulnerableCounter{0},
	  invulnerable{false},
	  state{Player::State::nominal},
	  colorAmount{0.f},
	  colorTimer{0},
	  renderType{Rendertype::shadeDefault},
	  upPrevious{false},
	  downPrevious{false},
	  leftPrevious{false},
	  rightPrevious{false}
{
	scrShakeState = false;
	deathSheet.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	walkDown.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	walkUp.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	walkLeft.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	walkRight.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	walkDown.setPosition(xPos, yPos);
	walkUp.setPosition(xPos, yPos);
	walkLeft.setPosition(xPos, yPos);
	walkRight.setPosition(xPos, yPos);
	shadowSprite.setPosition(xPos + 7, yPos + 24);
	dashSheet.setPosition(xPos, yPos);
	deathSheet.setPosition(xPos - 13, yPos - 1);
	dashSheet.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	dashSheet.setOrigin(0, 1);
	shadowSprite.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	shadowSprite.setTextureRect(sf::IntRect(0, 100, 18, 16));
	gun.gunSpr.setPosition(xPos, yPos);
	gun.gunSpr.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
}

void Player::setPosition(float _xPos, float _yPos) {
	xPos = _xPos;
	yPos = _yPos;
	hitBox.setPosition({xPos, yPos});
	walkDown.setPosition(xPos, yPos);
	walkUp.setPosition(xPos, yPos);
	walkLeft.setPosition(xPos, yPos);
	walkRight.setPosition(xPos, yPos);
	deathSheet.setPosition(xPos - 13, yPos - 1);
	dashSheet.setPosition(xPos, yPos);
	shadowSprite.setPosition(xPos + 7, yPos + 24);
}

float Player::getXpos() const {
	return xPos;
}

float Player::getYpos() const {
	return yPos;
}

inline void compareSheetIndex(Player::Sheet & sheetIndex) {
	switch (sheetIndex) {
	case Player::Sheet::walkDown: sheetIndex = Player::Sheet::stillDown;
		break;
	case Player::Sheet::walkUp: sheetIndex = Player::Sheet::stillUp;
		break;
	case Player::Sheet::walkLeft: sheetIndex = Player::Sheet::stillLeft;
		break;
	case Player::Sheet::walkRight: sheetIndex = Player::Sheet::stillRight;
		break;
	default:
		break;
	}
}

template <Player::Sheet S>
void regKeyResponse(bool key1, bool key2, bool key3, bool key4, Player::Sheet & sheetIndex, float & speed, bool collision) {
	if (key1) {
		if (!key2 && !key3 && !key4 && sheetIndex != S) {
			sheetIndex = S;
		}
		if (!collision) {
			if (key3 || key4) {
				speed = 1.70f;
			} else {
				speed = 2.20f;
			}
		} else {
			speed = 0.f;
		}
	} else {
		speed = 0.f;
	}
}

template <Player::Sheet S>
void altKeyResponse(bool key1, bool key2, bool key3, Player::Sheet & sheetIndex, bool collision, float & speed) {
	if (key1) {
		sheetIndex = S;
		if (!collision) {
			if (key2 || key3) {
				speed = 1.40f;
			} else {
				speed = 1.80f;
			}
		} else {
			speed = 0.f;
		}
	} else {
		speed = 0.f;
	}
}

template <Player::Sheet S, uint8_t maxIndx>
void onKeyReleased(bool key1, bool key2, bool key3, bool key4, bool keyprev, bool x, Player::Sheet & shIndex, uint8_t & frmIndex) {
	if (!key1 && keyprev) {
		if (!key2 && !key3 && !key4) {
			if (!x) {
				shIndex = S;
				frmIndex = maxIndx;
			} else {
				compareSheetIndex(shIndex);
			}
		}
	}
}

template<int spd>
void setSpeed(bool key1, bool key2, bool key3, bool collision, float & speed) {
	if (key1 && !collision) {
		if (key2 || key3) {
			speed = spd * 0.8;
		} else {
			speed = spd;
		}
	} else {
		speed = 0;
	}
}

void Player::update(Scene * pGM, const sf::Time & elapsedTime) {
	InputController * pInput {pGM->getPInput()};
	tileController & tiles {pGM->getTileController()};
	detailController & details {pGM->getDetails()};
	EffectGroup & effects {pGM->getEffects()};
	FontController * pFonts {pGM->getPFonts()};
	bool x {pInput->xPressed()};
	bool z {pInput->zPressed()};
	bool up {pInput->upPressed()};
	bool down {pInput->downPressed()};
	bool left {pInput->leftPressed()};
	bool right {pInput->rightPressed()};
    bool collisionUp {false};
	bool collisionDown {false};
	bool collisionLeft {false};
	bool collisionRight {false};
	
	checkCollisionWall(tiles.walls, collisionDown, collisionUp, collisionRight, collisionLeft, yPos, xPos);
	checkCollisionChest(details.getChests(), collisionDown, collisionUp, collisionRight, collisionLeft, yPos, xPos);
	
	switch (state) {
	case State::deactivated:
		sheetIndex = Sheet::stillDown;
		frameIndex = 5;
		lSpeed = 0.f;
		rSpeed = 0.f;
		uSpeed = 0.f;
		dSpeed = 0.f;
		upPrevious = false;
		downPrevious = false;
		leftPrevious = false;
		rightPrevious = false;
		break;

	case State::nominal:
		updateGun(elapsedTime, x, effects, worldOffsetX, worldOffsetY);
		if (!x) {
			regKeyResponse<Sheet::walkUp>(up, down, left, right, sheetIndex, uSpeed, collisionUp);
			regKeyResponse<Sheet::walkDown>(down, up, left, right, sheetIndex, dSpeed, collisionDown);
			regKeyResponse<Sheet::walkLeft>(left, right, down, up, sheetIndex, lSpeed, collisionLeft);
			regKeyResponse<Sheet::walkRight>(right, left, down, up, sheetIndex, rSpeed, collisionRight);
		} else {
			if (sheetIndex == Sheet::walkUp || sheetIndex == Sheet::stillUp) {
				altKeyResponse<Sheet::walkUp>(up, left, right, sheetIndex, collisionUp, uSpeed);
				altKeyResponse<Sheet::walkUp>(down, left, right, sheetIndex, collisionDown, dSpeed);
				altKeyResponse<Sheet::walkUp>(left, up, down, sheetIndex, collisionLeft, lSpeed);
				altKeyResponse<Sheet::walkUp>(right, up, down, sheetIndex, collisionRight, rSpeed);
			}
			if (sheetIndex == Sheet::walkDown || sheetIndex == Sheet::stillDown) {
				altKeyResponse<Sheet::walkDown>(up, left, right, sheetIndex, collisionUp, uSpeed);
				altKeyResponse<Sheet::walkDown>(down, left, right, sheetIndex, collisionDown, dSpeed);
				altKeyResponse<Sheet::walkDown>(left, up, down, sheetIndex, collisionLeft, lSpeed);
				altKeyResponse<Sheet::walkDown>(right, up, down, sheetIndex, collisionRight, rSpeed);
			}
			if (sheetIndex == Sheet::walkRight || sheetIndex == Sheet::stillRight) {
				altKeyResponse<Sheet::walkRight>(up, left, right, sheetIndex, collisionUp, uSpeed);
				altKeyResponse<Sheet::walkRight>(down, left, right, sheetIndex, collisionDown, dSpeed);
				altKeyResponse<Sheet::walkRight>(left, up, down, sheetIndex, collisionLeft, lSpeed);
				altKeyResponse<Sheet::walkRight>(right, up, down, sheetIndex, collisionRight, rSpeed);
			}
			if (sheetIndex == Sheet::walkLeft || sheetIndex == Sheet::stillLeft) {
				altKeyResponse<Sheet::walkLeft>(up, left, right, sheetIndex, collisionUp, uSpeed);
				altKeyResponse<Sheet::walkLeft>(down, left, right, sheetIndex, collisionDown, dSpeed);
				altKeyResponse<Sheet::walkLeft>(left, up, down, sheetIndex, collisionLeft, lSpeed);
				altKeyResponse<Sheet::walkLeft>(right, up, down, sheetIndex, collisionRight, rSpeed);
			}
		}
		onKeyReleased<Player::Sheet::stillLeft, 5>(left, right, up, down, leftPrevious, x, sheetIndex, frameIndex);
		onKeyReleased<Player::Sheet::stillRight, 5>(right, left, up, down, rightPrevious, x, sheetIndex, frameIndex);
		onKeyReleased<Player::Sheet::stillUp, 4>(up, left, right, down, upPrevious, x, sheetIndex, frameIndex);
		onKeyReleased<Player::Sheet::stillDown, 4>(down, left, right, up, downPrevious, x, sheetIndex, frameIndex);
		
		if (z && !zPrevious && (left || right || up || down)) {
			state = State::prepdash;
			if (sheetIndex == Sheet::stillDown || sheetIndex == Sheet::walkDown) {
				frameIndex = 6;
			} else if (sheetIndex == Sheet::stillUp || sheetIndex == Sheet::walkUp) {
				frameIndex = 8;
			} else if (sheetIndex == Sheet::stillLeft || sheetIndex == Sheet::walkLeft) {
				frameIndex = 0;
			} else if (sheetIndex == Sheet::stillRight || sheetIndex == Sheet::walkRight) {
				frameIndex = 2;
			}
			cachedSheet = sheetIndex; // So we know what to go back to after dash
			sheetIndex = Sheet::dashSheet;
 		}
		
		zPrevious = z;
		upPrevious = up;
		downPrevious = down;
		leftPrevious = left;
		rightPrevious = right;	
		break;

	case State::prepdash:
		if (gun.timeout != 0) {
			gun.timeout = 40;
		}
		setSpeed<1>(left, up, down, collisionLeft, lSpeed);
		setSpeed<1>(right, up, down, collisionRight, rSpeed);
		setSpeed<1>(up, left, right, collisionUp, uSpeed);
		setSpeed<1>(down, left, right, collisionDown, dSpeed);
		dashTimer += elapsedTime.asMilliseconds();
		if (dashTimer > 60) {
			dashTimer = 0;
			state = State::dashing;
			switch (frameIndex) {
			case 6:
				if (uSpeed > 0.f || (uSpeed == 0.f && dSpeed == 0.f)) { // Sidestep
					if (lSpeed > 0.f) {
						frameIndex = 5;
					} else if (rSpeed > 0.f) {
						frameIndex = 4;
					} else {
						frameIndex = 7;
					}
				} else { // Forward dash
					frameIndex = 14;
				}
				break;

			case 8:
				if (dSpeed > 0.f || (uSpeed == 0.f && dSpeed == 0.f)) {
					if (lSpeed > 0.f) {
						frameIndex = 10;
					} else if (rSpeed > 0.f) {
						frameIndex = 11;
					} else {
						frameIndex = 9;
					}
				} else {
					frameIndex = 15;
				}
				break;

			case 0:
				if (rSpeed > 0.f) {
					frameIndex = 1;
				} else if (lSpeed == 0.f) {
					frameIndex = 1;
				} else {
					frameIndex = 12;
				}
				break;

			case 2:
				if (lSpeed > 0.f) {
					frameIndex = 3;
				} else if (rSpeed == 0.f) {
					frameIndex = 3;
				} else {
				    frameIndex = 13;
				}
				break;
				
			default:
				break;
			}
	    }
		break;
		
	case State::dashing:
		setSpeed<7>(leftPrevious, upPrevious, downPrevious, collisionLeft, lSpeed);
		setSpeed<7>(rightPrevious, upPrevious, downPrevious, collisionRight, rSpeed);
		setSpeed<7>(upPrevious, leftPrevious, rightPrevious, collisionUp, uSpeed);
		setSpeed<7>(downPrevious, leftPrevious, rightPrevious, collisionDown, dSpeed);
		dashTimer += elapsedTime.asMilliseconds();
		if (dashTimer > 88) {
			dashTimer = 0;
			state = State::cooldown;
		}
		break;
		
	case State::cooldown:
		setSpeed<1>(leftPrevious, upPrevious, downPrevious, collisionLeft, lSpeed);
		setSpeed<1>(rightPrevious, upPrevious, downPrevious, collisionRight, rSpeed);
		setSpeed<1>(upPrevious, leftPrevious, rightPrevious, collisionUp, uSpeed);
		setSpeed<1>(downPrevious, leftPrevious, rightPrevious, collisionDown, dSpeed);
		dashTimer += elapsedTime.asMilliseconds();
		if (dashTimer > 220) {
			dashTimer = 0;
			state = State::nominal;
			sheetIndex = cachedSheet;
		}
		break;
		
	case State::dead:
		lSpeed = 0.f;
		rSpeed = 0.f;
		uSpeed = 0.f;
		dSpeed = 0.f;
		break;
	}
	
	updateColor(elapsedTime);

	checkEffectCollisions(effects, pFonts);
	if (health <= 0 && state != Player::State::dead) {
		state = Player::State::dead;
		sheetIndex = Player::Sheet::deathSheet;
		frameIndex = 0;
	}
	
	worldOffsetX += (lSpeed + -rSpeed) * (elapsedTime.asMilliseconds() / 17.6);
	worldOffsetY += (uSpeed + -dSpeed) * (elapsedTime.asMilliseconds() / 17.6);
}

void Player::draw(drawableVec & gameObjects, drawableVec & gameShadows, const sf::Time & elapsedTime) {
	if (visible) {
		auto gunIndexOffset = [](int32_t timeout) {
			if (timeout < 1707 && timeout > 44) {
				return 1;
			} else {
				return 0;
			}
		};
		switch (sheetIndex) {
		case Sheet::stillDown:
			if (gun.timeout > 0) {
				gun.gunSpr.setPosition(xPos + 12, yPos + 15);
				gameObjects.emplace_back(gun.gunSpr[4 + gunIndexOffset(gun.timeout)],
										 gun.gunSpr.getYpos() - 14, renderType, colorAmount);
			}
			gameObjects.emplace_back(walkDown[5], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			break;

		case Sheet::stillUp:
			gameObjects.emplace_back(walkUp[5], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			break;

		case Sheet::stillLeft:
			if (gun.timeout > 0) {
				gun.gunSpr.setPosition(xPos + 2, yPos + 13);
				gameObjects.emplace_back(gun.gunSpr[2 + gunIndexOffset(gun.timeout)],
										 gun.gunSpr.getYpos() - 14, renderType, colorAmount);
			}
			gameObjects.emplace_back(walkLeft[6], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			break;

		case Sheet::stillRight:
			if (gun.timeout > 0) {
				gun.gunSpr.setPosition(xPos + 19, yPos + 13);
				gameObjects.emplace_back(gun.gunSpr[gunIndexOffset(gun.timeout)],
										 gun.gunSpr.getYpos() - 14, renderType, colorAmount);
			}
			gameObjects.emplace_back(walkRight[6], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			break;
			
		case Sheet::walkDown:
			if (gun.timeout > 0) {
				gun.gunSpr.setPosition(xPos + 12, yPos + 15);
				gameObjects.emplace_back(gun.gunSpr[4 + gunIndexOffset(gun.timeout)],
										 gun.gunSpr.getYpos() - 14, renderType, colorAmount);
			}
			updateAnimation(elapsedTime, 9, 100);
			gameObjects.emplace_back(walkDown[verticalAnimationDecoder(frameIndex)], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			break;

		case Sheet::walkUp:
			updateAnimation(elapsedTime, 9, 100);
			gameObjects.emplace_back(walkUp[verticalAnimationDecoder(frameIndex)], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			break;
			
		case Sheet::walkLeft:
		    if (gun.timeout > 0) {
				gun.gunSpr.setPosition(xPos + 2, yPos + 13);
				gameObjects.emplace_back(gun.gunSpr[2 + gunIndexOffset(gun.timeout)],
										 gun.gunSpr.getYpos() - 14, renderType, colorAmount);
			}
			updateAnimation(elapsedTime, 5, 100);
			gameObjects.emplace_back(walkLeft[frameIndex], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			break;
			
		case Sheet::walkRight:
			if (gun.timeout > 0) {
				gun.gunSpr.setPosition(xPos + 19, yPos + 13);
				gameObjects.emplace_back(gun.gunSpr[gunIndexOffset(gun.timeout)],
										 gun.gunSpr.getYpos() - 14, renderType, colorAmount);
			}
			updateAnimation(elapsedTime, 5, 100);
			gameObjects.emplace_back(walkRight[frameIndex], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			break;
			
		case Sheet::deathSheet:
			if (frameIndex < 10) {
				animationTimer += elapsedTime.asMilliseconds();
				if (animationTimer > 60) {
					animationTimer = 0;
					++frameIndex;
				}
			}
			gameObjects.emplace_back(deathSheet[frameIndex], yPos, renderType, colorAmount);
			break;
			
		case Sheet::dashSheet:
			gameObjects.emplace_back(dashSheet[frameIndex], yPos, renderType, colorAmount);
			gameShadows.emplace_back(shadowSprite, 0.f, Rendertype::shadeDefault, 0.f);
			animationTimer += elapsedTime.asMilliseconds();
			if (state == Player::State::dashing) {
				if (animationTimer > 20) {
					animationTimer = 0;
					blurs.emplace_back(&std::get<0>(gameObjects.back()), xPos - worldOffsetX, yPos - worldOffsetY);
				}
			}
			break;
		}
	}

	if (!blurs.empty()) {
		for (auto it = blurs.begin(); it != blurs.end();) {
			if (it->getKillFlag())
				it = blurs.erase(it);
			else {
				it->update(elapsedTime, worldOffsetX, worldOffsetY);
				gameObjects.emplace_back(*it->getSprite(), it->yInit, Rendertype::shadeDefault, 0.f);
				++it;
			}
		}
	}
}

void Player::updateAnimation(const sf::Time & elapsedTime, uint8_t maxIndex, uint8_t count) {
	animationTimer += elapsedTime.asMilliseconds();
	if (animationTimer > count) {
		frameIndex++;
		animationTimer -= count; 
	}
	if (frameIndex > maxIndex) {
		frameIndex = 0;
	}
}

float Player::getWorldOffsetX() const {
	return worldOffsetX;
}

float Player::getWorldOffsetY() const {
	return worldOffsetY;
}

void Player::setWorldOffsetX(float x) {
	worldOffsetX = x;
}

void Player::setWorldOffsetY(float y) {
	worldOffsetY = y;
}

void Player::setState(State _state) {
	state = _state;
}

Player::State Player::getState() const {
	return state;
}

Player::Health Player::getHealth() const {
	return health;
}

void Player::reset() {
	state = State::nominal;
	invulnerable = false;
	invulnerableCounter = 0;
	health = 4;
	sheetIndex = Sheet::walkDown;
}

void Player::setHealth(Health value) {
	health = value;
}

void Player::updateGun(const sf::Time & elapsedTime, const bool x, EffectGroup & effects, float xOffset, float yOffset) {
	gun.timeout -= elapsedTime.asMilliseconds();
	if (gun.bulletTimer != 0) {
		gun.bulletTimer -= elapsedTime.asMilliseconds();
		if (gun.bulletTimer < 0) {
			gun.bulletTimer = 0;
		}
	}
	if (gun.timeout <= 0) {
		gun.timeout = 0;
	}
	if (x) {
		if (gun.timeout == 0) {
			gun.timeout = 1760;
		} else if (gun.timeout < 1671) {
			gun.timeout = 1671;
			if (gun.bulletTimer == 0) {
				effects.add<9>(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects), globalResourceHandler.getTexture(ResourceHandler::Texture::whiteGlow), static_cast<int>(sheetIndex), xPos - xOffset, yPos - yOffset);
				gun.bulletTimer = 400;
			}
		}
	}
}

template<std::size_t indx, typename F >
void checkEffectCollision(EffectGroup & effects, Player & player, const F & policy) {
	for (auto & element : effects.get<indx>()) {
		if (player.getHitBox().overlapping(element.getHitBox())) {
			element.setKillFlag();
			policy();
		}
	}
}

void Player::checkEffectCollisions(EffectGroup & effects, FontController * pFonts) {
	auto hitPolicy = [&]() {
		pFonts->resetHPText();
		--health;
		renderType = Rendertype::shadeRed;
		colorAmount = 1.f;
		colorTimer = 0;
	};
	checkEffectCollision<8>(effects, *this, hitPolicy);
	checkEffectCollision<7>(effects, *this, hitPolicy);
	checkEffectCollision<6>(effects, *this, hitPolicy);
	checkEffectCollision<4>(effects, *this, [&]() {
			pFonts->updateScore(1);
			pFonts->resetSCText();
			renderType = Rendertype::shadeNeon;
			colorAmount = 1.f;
			colorTimer = 0;
		});
	checkEffectCollision<5>(effects, *this, [&]() {
			health = fmin(pFonts->getMaxHealth(), health + 1);
			renderType = Rendertype::shadeCrimson;
			colorAmount = 1.f;
			colorTimer = 0;
		});
}

const Player::HBox & Player::getHitBox() const {
	return hitBox;
}

void Player::updateColor(const sf::Time & elapsedTime) {
	if (colorAmount > 0.f) {
		colorTimer += elapsedTime.asMilliseconds();
		if (colorTimer > 40) {
			colorTimer -= 40;
			colorAmount -= 0.1f;
		}
	} else {
		renderType = Rendertype::shadeDefault;
	}
}
