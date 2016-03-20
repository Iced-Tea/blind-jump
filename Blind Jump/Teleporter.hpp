//
//  mapTower.hpp
//  Blind Jump
//
//  Created by Evan Bowman on 11/7/15.
//  Copyright © 2015 Evan Bowman. All rights reserved.
//

#ifndef Teleporter_hpp
#define Teleporter_hpp

#include <stdio.h>
#include "detailParent.hpp"
#include "effectsController.hpp"

class Teleporter : public detailParent {
public:
    Teleporter(float, float, sf::Sprite*, sf::Sprite, int, float, float);
    sf::Sprite* getShadow();
    unsigned char getCountdown();
    void initCountdown();
    sf::Sprite* getSprite();
    void update(float, float);
    sf::Sprite glowSprite;
    sf::Sprite* getGlow();
    
private:
    unsigned char smokeCountdown;
    sf::Sprite TeleporterSprites[2];
};

#endif /* mapTower_hpp */