#include "GameObject.h"

class Artifact : public GameObject {
public:
    uint8_t bonusType = 0;
    Artifact(uint32_t id, Vector2 pos, cpSpace* space) : GameObject(id, EntityType::ARTIFACT) {
        spaceRef = space;
        bonusType = rand() % 4;
        cpFloat radius = 15.0f;
        body = cpSpaceAddBody(space, cpBodyNew(1.0f, cpMomentForCircle(1.0f, 0, radius, cpvzero)));
        cpBodySetPosition(body, ToCp(pos));
        shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
        cpShapeSetSensor(shape, true);
        cpShapeSetUserData(shape, (void*)this);
    }
    void Update(float dt) override {}
};