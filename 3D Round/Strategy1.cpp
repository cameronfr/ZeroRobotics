//Strategy:
/*
* Navigation:
* 1. Place SPS units, with configurable tolerance, so that the last one is near cube
* 2. Dock with cube and return it to zone
* 3. Choose best next cube to go for
* 4. put cube in zone
* 5. repeat # 2 and # 3
*
* Movement Strategy: function(initial_state,desired_end_state)
* 
*
*/
//questions: how fuel use calculated, how accel calculated.
/*
* 
* from actual sphere: mass: 4kg, max accel: 0.17 m / s/s
* from tests: sphere: mass: ~4.6 - 4.8 kg, max accel: 0.008m/s^2
* fuel use is related to accel, most efficient at max accel
*/

typedef struct position {
    float X,Y,Z;
    int itemID;
} position;

typedef enum state {
    SPS_UNITS,
    GATHERING,
} sphereState;

typedef struct {
    float zone[4];
    sphereState state;
    float zrState[12];
} sphere;

sphere Sphere;

position firstSPS;
position secondSPS;
position target;

//Declare any variables shared between functions here
void init(){
    Sphere.state = SPS_UNITS;
    api.getMyZRState(Sphere.zrState);
    
    firstSPS.X = 0.75;
    firstSPS.Y = 0.75;
    firstSPS.Z = 0.75;
    secondSPS.X = 0.75;
    secondSPS.Y = -0.75;
    secondSPS.Z = 0.75;
}

void unwrapPosition (float pos[3], position p) {
    pos[0] = p.X;
    pos[1] = p.Y;
    pos[2] = p.Z;
}

void loop() {
    /*game.dropSPS();
    game.dropSPS();
    game.dropSPS();
    float forces[3];
    forces[0] = 200;
    forces[1] = 0;
    forces[2] = 0;
    api.setForces(forces);
    */
    api.getMyZRState(Sphere.zrState);
    
    int c = game.getNumSPSHeld();
    int i = 3 - c;
    
    float position[3];
    position[0] = Sphere.zrState[0];
    position[1] = Sphere.zrState[1];
    position[2] = Sphere.zrState[2];
    
    float velocity[3];
    velocity[0] = Sphere.zrState[3];
    velocity[1] = Sphere.zrState[4];
    velocity[2] = Sphere.zrState[5];
    
    if (i == 0) {
        float u[3];
        unwrapPosition(u, firstSPS);
        DEBUG(("first SPS unit : %f %f %f", u[0], u[1], u[2]));
        gotoPosition(position, u, velocity);
        // placeSPSUnit(firstSPS);
    }
    /*else if (i == 1) {
        float u[3];
        unwrapPosition(u, secondSPS);
        DEBUG(("second SPS unit : %f %f %f", u[0], u[1], u[2]));
        gotoPosition(position, u, velocity);
        if (placeSPSUnit(secondSPS)) {
            evaluateNextItem();
        }
    } else if (i == 2) {
        if (game.hasItem(target.itemID) == 2) {
            evaluateNextItem();
        }
        
        float u[3];
        unwrapPosition(u, target);
        gotoPosition(position, u, velocity);
        placeSPSUnit(target);
    }*/
}

void evaluateNextItem () {
    int itemID = evaluateMVPItemId();
    float loc[3] = { 0, 0, 0 };
    game.getItemLoc(loc,itemID);
    
    target.X = loc[0];
    target.Y = loc[1];
    target.Z = loc[2];
    
    target.itemID = itemID;
}

int evaluateMVPItemId () {
    /* by point */
    float dist = game.getNumSPSHeld() != 0 ? 0 : 100;
    int index = 0;
    
    for (int i = 0; i < 6; i++) {
        if (game.hasItem(i) != 0)
            continue;
        
        float loc[3];
        game.getItemLoc(loc, i);
        float d = mathSquare(loc[0] - Sphere.zrState[0]) + 
                  mathSquare(loc[1] - Sphere.zrState[1]) +
                  mathSquare(loc[2] - Sphere.zrState[2]);
        if ((game.getNumSPSHeld() != 0 && d >= dist) || d <= dist) {
            dist = d;
            index = i;
        }
    }
    
    return index;
}

bool placeSPSUnit(position p) {
    float MIN_DIST = 0.2;
    if (fabsf(Sphere.zrState[0] - p.X) < MIN_DIST &&
        fabsf(Sphere.zrState[1] - p.Y) < MIN_DIST &&
        fabsf(Sphere.zrState[2] - p.Z) < MIN_DIST) {
            game.dropSPS();
            return true;
        }
        
    return false;
}
    
//add more parameters l8ter
void gotoPosition(float currentPos[3], float destPosition[3], float velocity[3]) {
    float MAX_SPEED = 0.05;
    float MIN_DIST = 0.5;
    float ACCEL_FACTOR = 0.1;
    
    //float speed = mathVecMagnitude(velocity,3);
    float directionVec[3];
    //mathVecSubtract(directionVec,destPosition,currentPos,3);
    vecSubtract(directionVec, destPosition, currentPos);
    DEBUG(("DEST : %f %f %f", destPosition[0], destPosition[1], destPosition[2]));
    DEBUG(("CURR : %f %f %f", currentPos[0], currentPos[1], currentPos[2]));
    DEBUG(("DIRR : %f %f %f", directionVec[0], directionVec[1], directionVec[2]));

    float accel[3];
    for (int i = 0; i< 3; i++) {
        if (fabsf(directionVec[i]) < MIN_DIST) {
             accel[i] = - 4.7 * (11.0/8.0)*powf(velocity[i],2)/(2*directionVec[i]);
            //accel[i] = -100;
            //from vf^2 = vi^2 - 2a deltax
        } 
        else if (fabsf(velocity[i]) < MAX_SPEED) {
          if (fabsf(directionVec[i]) > MIN_DIST) {
            mathVecNormalize(directionVec,3);
            accel[i] = directionVec[i] * ACCEL_FACTOR;
          }
        }
        else {
          accel[i] = 0;
        }
    }
    
    api.setForces(accel);
    DEBUG(("accel : %f %f %f", accel[0], accel[1], accel[2]));
}

void vecSubtract(float r[3], float a[3], float b[3]) {
    r[0] = a[0] - b[0];
    r[1] = a[1] - b[1];
    r[2] = a[2] - b[2];
}

//add more parameters for efficiency later
void gotoRotation(float destRot[3]) {
    
}

//depending on sps held / object held
float currentAccelerationFactor() {
    
    
}