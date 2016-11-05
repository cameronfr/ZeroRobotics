float myState[12];
float myPos[3];
float myVel[3];
float myAtt[3];
float zoneInfo[4];
float zone[3];
float vecZone[3];
float distanceZone;
float item[6][3];
float vec[6][3];
float distance[6];
float oppVec[3];
float SPSpos[3];
float vecSPS[3];
float distanceSPS;
int t;
int step;
int ID;
char c;

void init() {
    step = 0;
    api.getMyZRState(myState);
    if (myState[1] > 0) {
        c = 1;
    } else {
        c = -1;
    }
    game.dropSPS();
    SPSpos[0] = -0.3f*c;
    SPSpos[1] = 0.65f*c;
    SPSpos[2] = 0;
    DEBUG(("TFFT_36er"));
}

void loop() {
    api.getMyZRState(myState);
    t = api.getTime();
    for (int i = 0; i < 3; i++) {
        myPos[i] = myState[i];
        myVel[i] = myState[i+3];
        myAtt[i] = myState[i+6];
    }
    for (int i = 0; i < 6; i++) {
        game.getItemLoc(item[i], i);
        mathVecSubtract(vec[i], item[i], myPos, 3);
        distance[i] = mathVecMagnitude(vec[i], 3);
    }
    if (game.getNumSPSHeld() == 0) {
        game.getZone(zoneInfo);
        for (int i = 0; i < 3; i++) {
            zone[i] = zoneInfo[i];
        }
        mathVecSubtract(vecZone, zone, myPos, 3);
        distanceZone = mathVecMagnitude(vecZone, 3);
    }
    mathVecSubtract(vecSPS, SPSpos, myPos, 3);
    distanceSPS = mathVecMagnitude(vecSPS, 3);
    
    switch (step) {
        case 0:
            ID = closestItem(0, 1);
            api.setAttitudeTarget(vec[ID]);
            moveSPS(SPSpos);
            if (distanceSPS < 0.05f || t == 15) {
                game.dropSPS();
                step++;
            }
            break;
        case 1:
            if (game.hasItem(0) != 1 && game.hasItem(1) != 1) {
                ID = closestItem(0, 1);
            }
            pickItem();
            if (game.hasItem(0) == 1 || game.hasItem(1) == 1) {
                game.dropSPS();
            }
            break;
        case 2:
            if (dist(myPos, item[closestItem(2, 5)]) <= 0.5f) {
                ID = closestItem(2, 5);
                pickItem();
            } else {
                step++;
            }
            break;
        case 3:
            if (game.hasItem(0) != 1 && game.hasItem(1) != 1) {
                ID = closestItem(0, 1);
            }
            pickItem();
            break;
        case 4:
            pickItem();
            break;
        default:
            ID = closestItem(0, 5);
            pickItem();
    }
    DEBUG(("%d", step));
}

void pickItem() {
    float dmax;
    float dmin;
    float vectx[3];
    float target[3];
    float x;
    float cosalpha = 0;
    
    if (ID == 0 || ID == 1) {
        dmax = 0.173f;
        dmin = 0.151f;
    }
    else if (ID == 2 || ID == 3) {
        dmax = 0.160f;
        dmin = 0.138f;
    }
    else {
        dmax = 0.146f;
        dmin = 0.124f;
    }
    if (game.hasItem(ID) == 0) {
        if (step == 4) {
            for (int i = 5; i >= 0; i--) {
                if (game.hasItemBeenPickedUp(i) == 1 && game.itemInZone(i) == 0) {
                    ID = i;
                }
            }
        }
        x = calcVec(ID);
        for (int i = 0; i < 3; i++) {
            vectx[i] = vec[ID][i] * x;
        }
        mathVecAdd(target, vectx, myPos, 3);
        moveCapVelocity(target);
        api.setAttitudeTarget(vec[ID]);
        cosalpha = mathVecInner(myAtt, vec[ID], 3)/(mathVecMagnitude(myAtt, 3)*mathVecMagnitude(vec[ID], 3));
        if (distance[ID] < dmax && mathVecMagnitude(myVel, 3) < 0.01f && cosalpha > 0.97f) {
            game.dockItem(ID);
        }
        if (distance[ID] < dmin) {
            mathVecSubtract(oppVec, myPos, item[ID], 3);
            api.setForces(oppVec);
        }
    } else {
        reachZone();
    }
}

void reachZone() {
    float vecZoneToMe[3];
    mathVecSubtract(vecZoneToMe, myPos, zone, 3);
    mathVecNormalize(vecZoneToMe, 3);
    for(int i = 0; i < 3; i++){
        vecZoneToMe[i] *= dist(item[ID], myPos);
    }
    mathVecAdd(vecZoneToMe, vecZoneToMe, zone, 3);
    moveCapVelocity(vecZoneToMe);
    api.setAttitudeTarget(vecZone);
    if (dist(item[ID], zone) < 0.04f) {
        game.dropItem();
        step++;
    }
}

int closestItem(int x, int y) {
    int id = -1;
    float d = 20;

    do {
        for (int i = x; i <= y; i++) {
            if (distance[i] < d && game.itemInZone(i) == 0 && (game.hasItem(i) != 2 || step == 3)) {
                d = distance[i];
                id = i;
            }
        }
        x = 0;
        y = 5;
        d = 20;
    } while (id == -1 && step != 3);
    
    return id;
}

float calcVec(int id) {
    float c;
    float x;
    if (id == 0 || id == 1) {
        c = 0.151f;
    }
    else if (id == 2 || id == 3 || id == 10) {
        c = 0.138f;
    }
    else {
        c = 0.124f;
    }
    
    if (id == 10) {
        x = (distanceZone - c) / distanceZone;
    } else {
        x = (distance[id] - c) / distance[id];
    }
    return x;
}

float dist(float *a, float *b) {
    float sub[3];
    mathVecSubtract(sub, a, b, 3);
    return mathVecMagnitude(sub,3);
}

void moveSPS(float *whereTo) {
    float v[3];
    mathVecSubtract(v, whereTo, myState, 3);
    mathVecNormalize(v, 3);
    if (dist(myPos, SPSpos) > 0.3f)
    multVec(v, 0.45f);
    else
    multVec(v, -0.45f);
    api.setForces(v);
    
}

void moveCapVelocity(float *whereTo){
    if (dist(myState, whereTo) < 0.15f){
        api.setPositionTarget(whereTo);
    }
    else{
        float v[3];
        mathVecSubtract(v, whereTo, myState, 3);
        mathVecNormalize(v, 3);
        multVec(v, 0.041f);
        api.setVelocityTarget(v);
    }
}

void multVec(float* vec, float mult) {
    vec[0] *= mult;
    vec[1] *= mult;
    vec[2] *= mult;
}
