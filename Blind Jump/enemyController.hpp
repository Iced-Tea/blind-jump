//
//  enemyController.hpp
//  Blind Jump
//
//  Created by Evan Bowman on 10/18/15.
//  Copyright © 2015 Evan Bowman. All rights reserved.
//

#ifndef enemyController_hpp
#define enemyController_hpp

#include <stdio.h>
#include "SFML/graphics.hpp"
#include <iostream>
#include "turret.hpp"
#include "effectsController.hpp"
#include "scoot.hpp"
#include "dasher.hpp"
#include "critter.hpp"
#include "heavyBot.hpp"

class ScreenShakeController;
class detailController;
class tileController;

class enemyController {
private:
    sf::Sprite heavyBotSprites[17];
    sf::Texture heavyBotTextures[17];
    sf::Sprite turretSprites[10];
    sf::Texture turretTextures[10];
    // A vector to hold turret objects
    std::vector<turret> turrets;
    sf::Sprite droneSprites[5];
    sf::Texture droneTextures[5];
    sf::Sprite scootSprites[3];
    sf::Texture scootTexture[3];
    sf::Texture dasherTexture[6];
    sf::Sprite dasherSprite[21];
    sf::Texture dasherDeathSeq[15];
    sf::Sprite chaserSprites[4];
    sf::Texture chaserTextures[4];
    // Vectors to hold enemy objects
    std::vector<Scoot> scoots;
    std::vector<Dasher> dashers;
    std::vector<Critter> critters;
    std::vector<HeavyBot> heavyBots;
    float windowW;
    float windowH;
    
public:
    enemyController();
    void updateEnemies(std::vector<std::tuple<sf::Sprite, float, int>>&, std::vector<std::tuple<sf::Sprite, float, int>>&, float, float, effectsController&, std::vector<wall>, bool, detailController*, tileController*, ScreenShakeController*, FontController&);
    void clear();
    sf::Sprite* getTurretSprites();
    sf::Sprite* getGuardianSprites();
    sf::Sprite* getScootSprites();
    sf::Sprite* getDasherSprites();
    sf::Sprite* getHeavyBotSprites();
    sf::Sprite* getChaserSprites();
    void addTurret(turret);
    void addScoot(Scoot);
    void addHeavyBot(HeavyBot);
    void addDasher(Dasher);
    void addCritter(Critter);
    void setWindowSize(float, float);
};
#endif /* enemyController_hpp */