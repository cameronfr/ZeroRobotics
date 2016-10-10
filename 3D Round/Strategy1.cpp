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
* fuel use: ~0.13%  per sec at F=0.01, * n thrusters used
* fuel use: ~0.24%  per sec at F=0.02, * n thrusters used
* fuel use: ~0.375% per sec at F=0.03, * n thrusters used
* fuel use summary: ~n * 0.125%/0.01F = n * ( 12.5%) / (unit of F)
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
    
    firstSPS = {-0.6, 0.6, -0.6};
    secondSPS = {0.0, -0.0, -0.0};
}

void unwrapPosition (float pos[3], position p) {
    pos[0] = p.X;
    pos[1] = p.Y;
    pos[2] = p.Z;
}

void loop() {
    DEBUG(("-----------------------"));
    
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
        placeSPSUnit(firstSPS);
    }
    else if (i == 1) {
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
        //placeSPSUnit(target);
    }
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
    float TOLERANCE = 0.05;
    if (fabsf(Sphere.zrState[0] - p.X) < TOLERANCE &&
        fabsf(Sphere.zrState[1] - p.Y) < TOLERANCE &&
        fabsf(Sphere.zrState[2] - p.Z) < TOLERANCE) {
            game.dropSPS();
            return true;
        }
    return false;
}
    
//todo: specify exit velocities
//todo: parameterize all the tolerances / error things
void gotoPosition(float currentPos[3], float destPosition[3], float velocity[3]) {
    float MAX_SPEED = 0.04;//4;         //max speed that should be obtained
    float SAT_MASS = 4.7;               //mass in kg -- used for finding force to apply
    float SAT_MAX_ACCEL = 0.007;//0.0075//how fast the sat can decelerate
    float ACCEL_FACTOR = 0.18;//0.18;     //how fast to accelerate
    float MIN_DIST_VEL_PADDING = 1;   //make the min dist a little bit larger
    float MIN_MIN_DIST = 0.01;          //to prevent random movement from effecting things
    
    //api.setPositionTarget(destPosition);
    //return;
    
    float directionVec[3];
    vecSubtract(directionVec, destPosition, currentPos);     //mathVecSubtract(directionVec,destPosition,currentPos,3);
    DEBUG(("DEST : X:%f Y:%f Z:%f", destPosition[0], destPosition[1], destPosition[2]));
    DEBUG(("DIRR : %f %f %f", directionVec[0], directionVec[1], directionVec[2]));

    float accel[3];
    for (int i = 0; i< 3; i++) {
        float MIN_DIST = powf((velocity[i]*MIN_DIST_VEL_PADDING),2) / (2*SAT_MAX_ACCEL * (1.0/currentAccelerationFactor()));
        MIN_DIST = fmax(MIN_MIN_DIST,MIN_DIST);
        DEBUG(("%d HAS MIN DIST %f",i,MIN_DIST));
        //bool within = fabsf(directionVec[i]) < MIN_DIST;
        //DEBUG(("MDIST: %f, DIST: %f, ISLESS? %d",MIN_DIST,directionVec[i],within));
        if (fabsf(directionVec[i]) < MIN_DIST) {
            DEBUG(("%d WITHIN MIN DIST",i));
            accel[i] = - SAT_MASS *sign(velocity[i])*currentAccelerationFactor()*(powf(velocity[i],2)/(2*fabsf(directionVec[i])));

        } 
        else if (fabsf(velocity[i]) < MAX_SPEED || sign(velocity[i])!=sign(directionVec[i])) {
            if (sign(velocity[i])!=sign(directionVec[i])) {
                DEBUG(("%d OVERSHOT",i)); //note: it's hella expensive fuel-wise to overshoot
            }
            accel[i] = directionVec[i] * ACCEL_FACTOR;
        }
        else {
          accel[i] = 0;
        }
    }
    
    api.setForces(accel);
    DEBUG(("FORCE : X:%f Y:%f Z:%f", accel[0], accel[1], accel[2]));
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
    float factor = 1;
    switch(game.getNumSPSHeld()) {
        case 0:
            factor *= (1.0);
            break;
        case 1:
            factor *= (9.0/8.0);
            break;
        case 2:
            factor *= (5.0/4.0);
            break;
        case 3:
            factor *= (11.0/8.0);
            break;
    }
    return factor;
    //also take into account objects held
}

int sign(float n) {
    if (n < 0) {
        return -1;
    }
    else {
        return 1;
    }
    
}