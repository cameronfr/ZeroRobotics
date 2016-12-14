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
bool guardZone; //triggered when fuel is low at zone

gameState state;

void init() {
    //step = 0;
    api.getMyZRState(state.myState);
    api.getMyZRState(state.initState);
    state.startSide = sign(state.myState[1]);
    game.dropSPS();
    guardZone = false;
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
      targetItemID = chooseItem(true);
      calcSPSPosition(secondSPSPos);
      placeSPSUnits();
    }
    else {
      if (game.hasItem(targetItemID) == 0) {
        if(!guardZone) {
            pickupItem(targetItemID);
        }
        else {
            api.setPositionTarget(state.zonePos);
        }
      }
      else if (game.hasItem(targetItemID) == 1) {
        if (state.numSPSHeld == 1) game.dropSPS();
        //TODO: if cube is not held by opponent or in a zone, go by the initial position not the current position (i.e., dont chase moving cubes)
        reachZone();
        
        //TODO: also check if attitude is lined up
        if (dist(state.itemPos[targetItemID],state.zonePos) < 0.03f) {
            game.dropItem();
            targetItemID = chooseItem(false);
            DEBUG(("FUEL: %f",game.getFuelRemaining()));
            if(game.getFuelRemaining() < 10 || game.itemInZone(2) || game.itemInZone(3)) {
                guardZone = true;
            }
            //TODO: if less than 10% fuel, stay here
        }
      }
    }

}

void placeSPSUnits() {

    if(state.numSPSHeld == 2) {
      DEBUG(("SECOND SPS POS %f %f %f",secondSPSPos[0],secondSPSPos[1],secondSPSPos[2]));
      /*secondSPSPos[0] = 0.5;
      secondSPSPos[1] = 0.5;
      secondSPSPos[2] = 0.5;*/
      moveCapVelocity(secondSPSPos);
      if (dist(state.myState,secondSPSPos) < 0.03f) {
        game.dropSPS();
      }
    }

}


//TODO: this SPS error isn't constant and I can't figure out why
//TODO: check if location is out of bounds, try negative 
void calcSPSPositionOld(float SPSpos[3]) {
    
    //note: really olny needs to be orthagonal to path vector
    
    float targetPos[3];
    float itemAtt[3];
    float vecToTarget[3];
    
     for (int i = 0; i < 3; i++) {
        targetPos[i] = state.itemPos[targetItemID][i+6];
        itemAtt[i] = state.itemPos[targetItemID][i+6];
    }
    
    multVec(targetPos,0.15*1.1f);
    mathVecAdd(targetPos,targetPos,state.itemPos[targetItemID],3);
    mathVecSubtract(vecToTarget,targetPos,state.initState,3);
    mathVecCross(SPSpos,vecToTarget,itemAtt);
    float distToTarget = mathVecMagnitude(vecToTarget,3);
    //DEBUG(("TARGET POS %f %f %f",targetPos[0],targetPos[1],targetPos[2]));
    //DEBUG(("VEC TO TARGET %f %f %f",vecToTarget[0],vecToTarget[1],vecToTarget[2]));
    //DEBUG(("ITEM ATT %f %f %f",itemAtt[0],itemAtt[1],itemAtt[2]));
    //DEBUG(("CROSS PROD %f %f %f",SPSpos[0],SPSpos[1],SPSpos[2]));
    mathVecNormalize(SPSpos,3);
    multVec(SPSpos,(0.2f/distToTarget));
    mathVecAdd(SPSpos,SPSpos,targetPos,3);
    
}

void calcSPSPosition(float SPSpos[]) {
    
    /*float vecAtt[3];
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
    mathVecAdd(SPSpos, SPSpos, vecm, 3);*/
    
    
    float startDist;
    float vecAtt[3];
    float target[3];
    float vecy[3];
    float vecm[3];
    
    for (int i = 0; i < 3; i++) {
        target[i] = state.itemPos[targetItemID][i+6];
        vecAtt[i] = state.itemPos[targetItemID][i+6];
    }
    
    multVec(target,0.15*1.1f);
    mathVecAdd(target,target,state.itemPos[targetItemID],3);
    mathVecSubtract(vecm,target,state.initState,3);
    startDist = mathVecMagnitude(vecm, 3);
    multVec(vecm,0.5f);
    mathVecCross(vecy, vecAtt, vecm);
    mathVecCross(SPSpos, vecm, vecy);
    //mathVecAdd(vecAtt, vecAtt, state.itemPos[targetItemID], 3);
    mathVecAdd(vecm, vecm, state.initState, 3);
    mathVecNormalize(SPSpos, 3);
    multVec(SPSpos, 0.2f/startDist);
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

   // multVec(targetPos,((dmin+dmax)/2));
    multVec(targetPos,dmin*1.1f);
    mathVecAdd(targetPos,targetPos,state.itemPos[ID],3);
    mathVecSubtract(vecToTarget,state.myState,targetPos,3);

    //this value if is positive if we need to go around the item
    //distance between position and plane of correct face of item
    float D = mathVecInner(itemAtt,targetPos,3);
    float pointPlaneDist = mathVecInner(itemAtt,state.myState,3)-D;
    DEBUG(("point plane dist %f",pointPlaneDist));

    if (pointPlaneDist < -0.05f/*-0.2f*/) {
        float target[3];
        //projection of the distance vec (sattelite -> item) onto plane of correct face of item
        //http://maplecloud.maplesoft.com/application.jsp?appId=5641983852806144
        float projVecPlane[3];
        multVec(orthagComponent,mathVecInner(itemAtt,vecToTarget,3)/(mathVecMagnitude(itemAtt,3)));
        mathVecSubtract(projVecPlane,vecToTarget,orthagComponent,3);
        mathVecNormalize(projVecPlane,3);
        multVec(projVecPlane,dmin * 1.15f);
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
//TODO: use the distance to the position we have to get to, not the position of the center of the cube
//TODO:HEURISTIC:if initial yellow cube is too close, go for red cube (bcz SPS placement wont work)
int chooseItem(bool isStart) {
    float CLOSE_CUBE_THRESH = 0.3;
    //1. check if open yellow cube (not in anyones zone or docked)
    //2. check for close red cubes
    //3. yellow cube in other zone or docked
    
    //1. Go for the closer yellow cube at the start
    if (isStart) {
        int item;
        float d0 = state.distanceToItem[0];
        float d1 = state.distanceToItem[1];
        
        //DEBUG(("D0 %f D1 %f",d0,d1));
        if (d0 < d1) {item = 0;}
        else {item = 1;}
        DEBUG(("DIST TO YELLOW CUBE %f",state.distanceToItem[item]));
        if (!(state.distanceToItem[item]<0.2)) {
            return item;
        }
    }
    
    //2. If there are any medium cubes close to the zone, go for those
    for (int i =0;i<2;i++) {
        DEBUG(("DISTANCE TO RED CUBE %f",state.distanceToItem[i+2]));
        if (state.distanceToItem[i+2] < CLOSE_CUBE_THRESH && game.itemInZone(i+2) == 0 && game.hasItem(i+2)!=2) {
            return i+2;
        }
    }
    
    //3. Go for yellow cube in opponents zone
    for (int i =0;i<2;i++) {
        if (game.itemInZone(i) == 0) {
            return i;
        }
    }
    
    return 5;
    
    
    /*float d = 20;

    for (int i = x; i <= y; i++) {
        if (state.distanceToItem[i] < d && game.itemInZone(i) == 0 && game.hasItem(i) != 2) {
            d = state.distanceToItem[i];
            id = i;
        }
    }

    return id;*/
}


void moveCapVelocitySlow(float *whereTo){
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


//Note: tested with and without currentMassFactor, doesn't make a huge difference
void moveCapVelocity(float destPosition[3]) {
    float MAX_SPEED = 0.065;//0.041;// 0.04; //max speed that should be obtained
    float ACCELERATION_FACTOR = 1;//1; //what percentage of max accel should we accel at
    float SAT_MASS = 4.75;// * currentMassFactor(); //normal mass in kg -- used for finding force to apply
    float SAT_MAX_ACCEL = 0.008;// / currentMassFactor(); //how fast the sat can decelerate at normal mass
    float SAT_MAX_DECEL = 0.008*0.7; // because we don't use mass factor anymore
    float MIN_MIN_DIST = 0.01;          //to prevent random movement from effecting things: don't accelerate if we're a little bit off
    //float ZERO_VEL_DIST_THRESH = 0.005; //when to switch to zeroing velocity instead of accelerating

    float directionVec[3];
    mathVecSubtract(directionVec, destPosition, state.myState,3);

    float force[3];
    for (int i = 0; i < 3; i++) {

        //Factor that describes speed/accel in relation to other dimensions (i.e., keep vel / accel prop to dist)
        float PROPORTIONAL_FACTOR = fabsf(directionVec[i]) / mathVecMagnitude(directionVec,3);//fabsf(directionVecNorm[i]);

        //Calculate both what we woud accel and decel the sattelite at (which we use depends on min_dist)
        float est_accel = sign(directionVec[i])*ACCELERATION_FACTOR*PROPORTIONAL_FACTOR*SAT_MAX_ACCEL;
        float est_decel = - sign(state.myVel[i]) * (mathSquare(state.myVel[i])/(2*fabsf(directionVec[i])));

        //Calculate the distance at which we need to start decelerating
        float naive_min_dist = mathSquare(fabsf(state.myVel[i]) + fabsf(est_accel)) / (2 * SAT_MAX_DECEL); //v = v_current + a*(1 timestep ahead)
        float min_dist = (naive_min_dist)+(fabsf(state.myVel[i])+0.5*fabsf(est_accel));// dist + v_o*t + 0.5*a*t^2
        min_dist = MIN_MIN_DIST > min_dist ? MIN_MIN_DIST : min_dist; //take min of min_dist and min_min_dist

        
        /*if (fabsf(directionVec[i]) < ZERO_VEL_DIST_THRESH) {
            force[i] = - state.myVel[i];
        } else */if (fabsf(directionVec[i]) < min_dist) {
            force[i] = est_decel;
        } else if (fabsf(state.myVel[i]) < MAX_SPEED * PROPORTIONAL_FACTOR || sign(state.myVel[i])!=sign(directionVec[i])) {
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
