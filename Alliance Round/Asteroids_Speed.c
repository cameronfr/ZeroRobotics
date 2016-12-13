typedef struct {
  float initState[12];
  float myState[12];
  float myVel[3];
  float myAtt[3];
  float zoneInfo[4];
  float zonePos[3];
  float vecToZone[3];
  float distanceToZone;
  float itemPos[9][12];
  float vecToItem[9][3];
  float distanceToItem[9];
  int numSPSHeld;
  //int step;
  char startSide;
} gameState;

int targetItemID;
float secondSPSPos[3];

gameState state;

void init() {
    //step = 0;
    api.getMyZRState(state.myState);
    api.getMyZRState(state.initState);
    state.startSide = sign(state.myState[1]);
    game.dropSPS();
    targetItemID = chooseItem(0,1);
}

void loop() {

    //Fetching latest variable values
    api.getMyZRState(state.myState);
    state.numSPSHeld = game.getNumSPSHeld();
    for (int i = 0; i < 3; i++) {
        state.myVel[i] = state.myState[i+3];
        state.myAtt[i] = state.myState[i+6];
    }
    for (int i = 0; i < 9; i++) {
        game.getItemZRState(state.itemPos[i], i);
        mathVecSubtract(state.vecToItem[i], state.itemPos[i], state.myState, 3);
        state.distanceToItem[i] = mathVecMagnitude(state.vecToItem[i],3);
    }
    if (state.numSPSHeld == 0) {
        game.getZone(state.zoneInfo);
        for (int i = 0; i < 3; i++) {
            state.zonePos[i] = state.zoneInfo[i];
        }
        mathVecSubtract(state.vecToZone, state.zonePos, state.myState, 3);
        state.distanceToZone = mathVecMagnitude(state.vecToZone, 3);
    }

    //Strategy:
    if (state.numSPSHeld == 2) {
      calcSPSPosition(secondSPSPos);
      //targetItemID = chooseItem(); already in init code so not needed here
      placeSPSUnits();
    }
    else {
      if (game.hasItem(targetItemID) == 0) {
        targetItemID = chooseItem(0,1);
        pickupItem(targetItemID);
      }
      else if (game.hasItem(targetItemID) == 1) {
        if (state.numSPSHeld == 1) game.dropSPS();
        reachZone();
        //TODO: also check if attitude is lined up
        if (dist(state.itemPos[targetItemID],state.zonePos) < 0.04f) {
            game.dropItem();
            //TODO: if less than 10% fuel, stay here
        }
      }
    }

    //Old strategy code

    /*if (step == 0) {
        ID = closestItem(0, 1);
        placeSPS();
    }
    else if (step == 1) {
        ID = closestItem(0, 1);
        pickItem();
    }
    else {
        ID = closestItem(0, 5);
        pickItem();
    }
    DEBUG(("step = %d; ID = %d", step, ID));*/
}

void placeSPSUnits() {

    if(state.numSPSHeld == 2) {
      DEBUG(("SECOND SPS POS %f %f %f",secondSPSPos[0],secondSPSPos[1],secondSPSPos[2]));
      /*secondSPSPos[0] = 0.5;
      secondSPSPos[1] = 0.5;
      secondSPSPos[2] = 0.5;*/
      moveCapVelocity(secondSPSPos);
      if (dist(state.myState,secondSPSPos) < 0.05f) {
        game.dropSPS();
      }
    }

}

//TODO: more efficient SPS positioning: don't need 0.05 error
void calcSPSPosition(float SPSpos[3]) {
    float startDist;
    float vecAtt[3];
    float vecy[3];
    float vecm[3];

    mathVecSubtract(vecm, state.itemPos[targetItemID], state.initState, 3);
    startDist = mathVecMagnitude(vecm, 3);
    for (int i = 0; i < 3; i++) {
        vecAtt[i] = state.itemPos[targetItemID][i+6];
    }
    multVec(vecm,0.5f);
    mathVecCross(vecy, vecAtt, vecm);
    mathVecCross(SPSpos, vecm, vecy);
    mathVecAdd(vecAtt, vecAtt, state.itemPos[targetItemID], 3);
    mathVecAdd(vecm, vecm, state.initState, 3);
    mathVecNormalize(SPSpos, 3);
    multVec(SPSpos, 0.18f/startDist + 0.2f);
    mathVecAdd(SPSpos, SPSpos, vecm, 3);
}


void pickupItem(int ID) {
    float dmax;
    float dmin;
    float targetPos[3];
    float vecToTarget[3];
    float itemAtt[3];
    float orthagComponent[3];

    //dmax = dimn = 0 if ID == 7 || ID == 8
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

    for (int i = 0; i < 3; i++) {
        targetPos[i] = state.itemPos[ID][i+6];
        itemAtt[i] = state.itemPos[ID][i+6];
        orthagComponent[i] = state.itemPos[ID][i+6];
    }

    multVec(targetPos,((dmin+dmax)/2));
    mathVecAdd(targetPos,targetPos,state.itemPos[ID],3);
    mathVecSubtract(vecToTarget,state.myState,targetPos,3);

    //this value if is positive if we need to go around the item
    //distance between position and plane of correct face of item
    float D = mathVecInner(itemAtt,targetPos,3);
    float pointPlaneDist = mathVecInner(itemAtt,state.myState,3)-D;
    //DEBUG(("point plane dist %f",pointPlaneDist));

    if (pointPlaneDist < -0.2f) {
        float target[3];
        //projection of the distance vec (sattelite -> item) onto plane of correct face of item
        //http://maplecloud.maplesoft.com/application.jsp?appId=5641983852806144
        float projVecPlane[3];
        multVec(orthagComponent,mathVecInner(itemAtt,vecToTarget,3)/(mathVecMagnitude(itemAtt,3)));
        mathVecSubtract(projVecPlane,vecToTarget,orthagComponent,3);
        mathVecNormalize(projVecPlane,3);
        multVec(projVecPlane,dmax * 1.02f);
        mathVecAdd(target,targetPos,projVecPlane,3);
        moveCapVelocity(target);
    }
    else {
        moveCapVelocity(targetPos);
    }
    multVec(itemAtt,-1);
    api.setAttitudeTarget(itemAtt);

    float cosalpha = mathVecInner(state.myAtt, state.vecToItem[ID], 3)/(mathVecMagnitude(state.myAtt, 3)*state.distanceToItem[ID]);
    if (state.distanceToItem[ID] < dmax && mathVecMagnitude(state.myVel, 3) < 0.01f && cosalpha > 0.97f && game.isFacingCorrectItemSide(ID)) {
        game.dockItem(ID);
    }

}

/*
void pickItemOld() {
    float dmax;
    float dmin;
    float vecAtt[3];
    float a[3];
    float target[3];
    float A[3];
    float cosalpha = 0;

    //dmax = dimn = 0 if ID == 7 || ID == 8
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

    //has case for ID == 6 in special item strategy
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
        if (mathVecMagnitude(A, 3) > 0.22f) {
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
*/

void reachZone() {
    float zoneTargetPos[3];
    mathVecSubtract(zoneTargetPos, state.myState, state.zonePos, 3);
    mathVecNormalize(zoneTargetPos, 3);
    multVec(zoneTargetPos,state.distanceToItem[targetItemID]);
    mathVecAdd(zoneTargetPos, zoneTargetPos, state.zonePos, 3);
    moveCapVelocity(zoneTargetPos);
    api.setAttitudeTarget(state.vecToZone);
}

//TODO: if vel of item too high, don't go for it
//also implement going for smaller items later
//general strategy of not stealing: getting in items as fast as possible
int chooseItem(int x, int y) {
    int id = -1;
    float d = 20;

    do {
        for (int i = x; i <= y; i++) {
            if (state.distanceToItem[i] < d && game.itemInZone(i) == 0 && game.hasItem(i) != 2) {
                d = state.distanceToItem[i];
                id = i;
            }
        }
        x = 0;
        y = 5;
        d = 20;
    } while (id == -1 /*&& step != 3*/);

    return id;
}


void moveCapVelocityOld(float *whereTo){
    if (dist(state.myState, whereTo) < 0.15f){
        api.setPositionTarget(whereTo);
    }
    else{
        float v[3];
        mathVecSubtract(v, whereTo, state.myState, 3);
        mathVecNormalize(v, 3);
        multVec(v, 0.041f);
        api.setVelocityTarget(v);
    }
}


void moveCapVelocity(float destPosition[3]) {
    float MAX_SPEED = 0.065;//0.041;// 0.04; //max speed that should be obtained
    float ACCELERATION_FACTOR = 1;//1; //what percentage of max accel should we accel at
    float SAT_MASS = 4.75 * currentMassFactor(); //normal mass in kg -- used for finding force to apply
    float SAT_MAX_ACCEL = 0.008 / currentMassFactor(); //how fast the sat can decelerate at normal mass
    float MIN_MIN_DIST = 0.01;          //to prevent random movement from effecting things
    float ZERO_VEL_DIST_THRESH = 0.005; //when to switch to zeroing velocity instead of accelerating

    float directionVec[3];
    float directionVecNorm[3];
    mathVecSubtract(directionVec, destPosition, state.myState,3);
    mathVecSubtract(directionVecNorm, destPosition, state.myState,3);
    mathVecNormalize(directionVecNorm,3);

    float force[3];
    for (int i = 0; i < 3; i++) {

        //Factor that describes speed/accel in relation to other dimensions (i.e., keep vel / accel prop to dist)
        float PROPORTIONAL_FACTOR = fabsf(directionVecNorm[i]);
        float DIM_MAX_SPEED = MAX_SPEED * PROPORTIONAL_FACTOR;

        //Calculate both what we woud accel and decel the sattelite at (which we use depends on min_dist)
        float est_accel = sign(directionVec[i])*ACCELERATION_FACTOR*PROPORTIONAL_FACTOR*SAT_MAX_ACCEL;
        float est_decel = - sign(state.myVel[i]) * (mathSquare(state.myVel[i])/(2*fabsf(directionVec[i])));

        //Calculate the distance at which we need to start decelerating
        float naive_min_dist = mathSquare(fabsf(state.myVel[i]) + fabsf(est_accel)) / (2 * SAT_MAX_ACCEL); //v = v_current + a*(1 timestep ahead)
        float MIN_DIST = (naive_min_dist)+(fabsf(state.myVel[i])+0.5*fabsf(est_accel));// dist + v_o*t + 0.5*a*t^2
        MIN_DIST = MIN_MIN_DIST > MIN_DIST ? MIN_MIN_DIST : MIN_DIST; //take min of min_dist and min_min_dist

        float distance = fabsf(directionVec[i]);
        if (distance < ZERO_VEL_DIST_THRESH) {
            force[i] = - state.myVel[i];
        } else if (distance < MIN_DIST) {
            force[i] = est_decel;
        } else if (fabsf(state.myVel[i]) < DIM_MAX_SPEED || sign(state.myVel[i])!=sign(directionVec[i])) {
            force[i] = est_accel;
        } else {
            force[i] = 0;
        }

        force[i] *= SAT_MASS;
    }

    api.setForces(force);

}

/*void sliceToArray(float *toArray,float*fromArray,int from, int len) {
    for (int i =0; i< len; i++) {
        toArray[i] = fromArray[from+i];
    }
}*/

float currentMassFactor() {
    switch(game.getNumSPSHeld()) {
        case 0:
            if (game.hasItem(targetItemID)!=1)
                return 1.0;

            switch (targetItemID) {
                case 0:
                case 1:
                    return (11.0/8.0);
                    break;
                case 2:
                case 3:
                    return (5.0/4.0);
                    break;
                case 4:
                case 5:
                    return (9.0/8.0);
                    break;
                default:
                    return (1.0);
                    break;
            }

            break;
        case 1:
            return (9.0/8.0);
            break;
        case 2:
            return (5.0/4.0);
            break;
        case 3:
            return (11.0/8.0);
            break;
    }
    return 1;
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

int sign(float n) {
    return n > 0 ? 1 : -1;
}

float max(float a, float b){
    return (a>b ? a : b);
}

float distPR(float *pOnR, float *r, float *p) {
    float d = 0;
    d = -r[0]*p[0] - r[1]*p[1] - r[2]*p[2];
    float t = (-r[0]*pOnR[0]-r[1]*pOnR[1]-r[2]*pOnR[2] - d)/(r[0]*r[0] + r[1]*r[1] + r[2]*r[2]);
    float p_p[] = {pOnR[0] + r[0]*t, pOnR[1] + r[1]*t, pOnR[2] + r[2]*t };
    return sqrtf( (p_p[0] - p[0])*(p_p[0] - p[0])+ (p_p[1] - p[1])*(p_p[1] - p[1]) +(p_p[2] - p[2])*(p_p[2] - p[2]));
}
