float myState[12];
float initState[12];
float myVel[3];
float myAtt[3];
float zoneInfo[4];
float zone[3];
float vecZone[3];
float distanceZone;
float item[9][12];
float vec[9][3];
float distance[9];
float SPSpos[3];
float distanceSPS;
int t;
int step;
int ID;
int firstItem;

void init() {
    step = 0;
    api.getMyZRState(initState);
    game.dropSPS();
}

void loop() {
    api.getMyZRState(myState);
    t = api.getTime();
    for (int i = 0; i < 3; i++) {
        myVel[i] = myState[i+3];
        myAtt[i] = myState[i+6];
    }
    for (int i = 0; i < 9; i++) {
        game.getItemZRState(item[i], i);
        mathVecSubtract(vec[i], item[i], myState, 3);
        distance[i] = mathVecMagnitude(vec[i], 3);
    }
    if (game.getNumSPSHeld() == 0) {
        game.getZone(zoneInfo);
        for (int i = 0; i < 3; i++) {
            zone[i] = zoneInfo[i];
        }
        mathVecSubtract(vecZone, zone, myState, 3);
        distanceZone = mathVecMagnitude(vecZone, 3);
    }

    if (step == 0) {
        ID = firstItem = closestItem(2, 3);
        placeSPS();
    }
    else if (step == 1) {
        pickItem();
    }
    else {
        moveCapVelocity(zone);
    }

    //DEBUG(("step = %d; ID = %d", step, ID));
    DEBUG(("s = %d; i = %d", step, ID)); //gara
}

void placeSPS() {
    float newTarget[3];
    char x = 0;
    
    mathVecSubtract(newTarget, item[ID], initState, 3);
    mathVecNormalize(newTarget, 3);
    multVec(newTarget, 0.5f);
    mathVecAdd(newTarget, newTarget, initState, 3);
    calcSPS(item[ID]);
    if (fabsf(SPSpos[0]) > 0.64f || fabsf(SPSpos[1]) > 0.8f || fabsf(SPSpos[2]) > 0.64f) {
        x = 1;
        calcSPS(newTarget);
    }
    
    api.setAttitudeTarget(vec[ID]);
    if (game.getNumSPSHeld() == 2) {
        moveCapVelocity(SPSpos);
        if (dist(myState, SPSpos) < 0.05f) {
            game.dropSPS();
        }
    } 
    else if (x == 1 && game.getNumSPSHeld() == 1) {
        moveCapVelocity(newTarget);
        if (dist(myState, newTarget) < 0.05f) {
            game.dropSPS();
        }
    }
    else { 
        pickItem();
        if (game.hasItem(ID) == 1) {
            game.dropSPS();
        }
    }
    DEBUG(("x = %d", x)); //gara
}

void pickItem() {
    float dmax;
    float dmin;
    float vecAtt[3];
    float a[3];
    float target[3];
    float A[3];
    float cosalpha = 0;

    if (ID <= 1) {
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
        for (int i = 0; i < 3; i++) {
            vecAtt[i] = item[ID][i+6];
            vecAtt[i] *= (dmin+dmax)/2;
            a[i] = -vec[ID][i];
            target[i] = (a[i]+vecAtt[i])/2;
        }
        mathVecAdd(vecAtt, vecAtt, item[ID], 3);
        mathVecSubtract(A, vecAtt, myState, 3);
        mathVecNormalize(target, 3);
        for (int i = 0; i < 3; i++) {
            target[i] *= ((dmax+dmin)/2)+0.05f;
        }
        mathVecAdd(target, target, item[ID], 3);
        if (mathVecInner(vec[ID], A, 3) > mathVecMagnitude(A, 3)) {
            moveCapVelocity(target);
        } else {
            moveCapVelocity(vecAtt);
        }
        api.setAttitudeTarget(vec[ID]);
        cosalpha = mathVecInner(myAtt, vec[ID], 3)/(mathVecMagnitude(myAtt, 3)*mathVecMagnitude(vec[ID], 3));
        if (distance[ID] < dmax && mathVecMagnitude(myVel, 3) < 0.01f && cosalpha > 0.97f && game.isFacingCorrectItemSide(ID)) {
            game.dockItem(ID);
        }
        if (distance[ID] < dmin) {
            api.setForces(a);
        }
    } else {
        reachZone();
    }
}

void reachZone() {
    float vecZoneToMe[3];
    mathVecSubtract(vecZoneToMe, myState, zone, 3);
    mathVecNormalize(vecZoneToMe, 3);
    for (int i = 0; i < 3; i++){
        vecZoneToMe[i] *= dist(item[ID], myState);
    }
    mathVecAdd(vecZoneToMe, vecZoneToMe, zone, 3);
    moveCapVelocity(vecZoneToMe);
    api.setAttitudeTarget(vecZone);
    if (dist(item[ID], zone) < 0.04f) {
        game.dropItem();
        step++;
        if (step == 1) {
            for (int i = 5; i >= 0; i--) {
                if (game.hasItemBeenPickedUp(i) == 1 && i != firstItem) {
                    ID = i;
                }
            }
            if (ID == firstItem) {
                ID = closestItem(0, 1);
            }
        }
    }
}

int closestItem(int x, int y) {
    int id = -1;
    float d = 20;

    do {
        for (int i = y; i >= x; i--) {
            if (distance[i] < d && game.itemInZone(i) == 0 && game.hasItem(i) != 2) {
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

float dist(float *a, float *b) {
    float sub[3];
    mathVecSubtract(sub, a, b, 3);
    return mathVecMagnitude(sub,3);
}

void multVec(float* vec, float mult) {
    vec[0] *= mult;
    vec[1] *= mult;
    vec[2] *= mult;
}

void calcSPS(float target[]) {
    float startDist;
    float vecAtt[3];
    float vecy[3];
    float vecm[3];
    
    mathVecSubtract(vecm, target, initState, 3);
    startDist = mathVecMagnitude(vecm, 3);
    for (int i = 0; i < 3; i++) {
        vecm[i] = vecm[i]/2;
        vecAtt[i] = item[ID][i+6];
    }
    mathVecCross(vecy, vecAtt, vecm);
    mathVecCross(SPSpos, vecm, vecy);
    mathVecAdd(vecAtt, vecAtt, item[ID], 3);
    mathVecAdd(vecm, vecm, initState, 3);
    mathVecNormalize(SPSpos, 3);
    multVec(SPSpos, 0.18f/startDist + 0.2f);
    mathVecAdd(SPSpos, SPSpos, vecm, 3);
}

/*float distPR(float *pOnR, float *r, float *p) {
    float d = 0;
    d = -r[0]*p[0] - r[1]*p[1] - r[2]*p[2];
    float t = (-r[0]*pOnR[0]-r[1]*pOnR[1]-r[2]*pOnR[2] - d)/(r[0]*r[0] + r[1]*r[1] + r[2]*r[2]);
    float p_p[] = {pOnR[0] + r[0]*t, pOnR[1] + r[1]*t, pOnR[2] + r[2]*t };
    return sqrtf( (p_p[0] - p[0])*(p_p[0] - p[0])+ (p_p[1] - p[1])*(p_p[1] - p[1]) +(p_p[2] - p[2])*(p_p[2] - p[2]));
}*/
