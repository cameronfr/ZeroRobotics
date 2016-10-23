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
*/

/*
* from actual sphere: mass: 4kg, max accel: 0.17 m / s/s
* from tests: sphere: mass: ~4.6 - 4.8 kg, max accel: 0.008vm/s^2
* fuel use is related to accel, most efficient at max accel
* fuel use: ~0.13%  per sec at F=0.01, * n thrusters used
* fuel use: ~0.24%  per sec at F=0.02, * n thrusters used
* fuel use: ~0.375% per sec at F=0.03, * n thrusters used
* fuel use summary: ~n * 0.125%/0.01F = n * ( 12.5%) / (unit of F)
*/

typedef struct position {
    float pos[3];
    int itemID;
    bool isItem;
} position;

typedef enum state {
    SPS_UNITS,
    GATHERING,
    PLACING,
} sphereState;

typedef struct {
    // float zone[4];
    position zone;
    sphereState state;
    float zrState[12];
} sphere;

sphere Sphere;

// position firstSPS;
position secondSPS;
position target;

//Declare any variables shared between functions here
void init() {
    Sphere.state = SPS_UNITS;
    api.getMyZRState(Sphere.zrState);
    
    // firstSPS.pos[0] = 0.0;
    // firstSPS.pos[1] = sign(Sphere.zrState[1]) * 0.15;
    // firstSPS.pos[2] = 0;
    
    //TODO: TWEAK THIS
    secondSPS.pos[0] = sign(Sphere.zrState[1])* -0.45;
    secondSPS.pos[1] = 0.45;
    secondSPS.pos[2] = sign(Sphere.zrState[1])* - 0.2;
    target.itemID = -1;
    
    DEBUG(("BEEP BOP HELLO"));
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
    
    //First SPS Unit
    if (i == 0) {
        // gotoPosition(position, firstSPS.pos, velocity);
        placeUnit(position, true, 0.05);
    } else if (i == 1) {
        //DEBUG(("second SPS unit : %f %f %f", u[0], u[1], u[2]));
        gotoPosition(position, secondSPS.pos, velocity);
        placeUnit(secondSPS.pos, true, 0.05);
    } else if (i == 2) {
        if (game.hasItem(target.itemID) == 2 || target.itemID == -1) {
            evaluateNextTarget();
        }
        gotoPosition(position, target.pos, velocity);
        
        //Set the ZONE position
        if (placeUnit(target.pos, true, 0.05)) {
            float zone[4];
            game.getZone(zone);
            
            Sphere.zone.pos[0] = zone[0];
            Sphere.zone.pos[1] = zone[1];
            Sphere.zone.pos[2] = zone[2];
            Sphere.state = GATHERING;
        }
    } else {
        moveToPlaceItem(position, velocity);
    }
}

void moveToPlaceOrGetItem (float pos[3], float vel[3]) {
    if (Sphere.state == GATHERING) {
        if (game.hasItem(target.itemID) == 2) {
            evaluateNextTarget();
        }
        
        if (!isSphereWithinDockLoc(pos)) {
            gotoPosition(pos, target.pos, vel);
            //DEBUG(("Shifting position..."));
            return;
        }
        
        //TODO: THIS IS NEEDED BUT SHOULDNT BE -- FIX THIS
        else if (mathVecMagnitude(vel, 3) > 0.01) {
            //float zeroVel[3] = {0, 0, 0};
            float zeroVel[3];
            //zeros(zeroVel);
            zeroVel[0] = 0;
            zeroVel[1] = 0;
            zeroVel[2] = 0;
            
            api.setVelocityTarget(zeroVel);
            //DEBUG(("Slowing speed..."));
            return;
        }
        
        
        float curAttitude[3] = { Sphere.zrState[6], Sphere.zrState[7], Sphere.zrState[8] };
        float destVec[3];
        mathVecSubtract(destVec, u, pos, 3);
        mathVecNormalize(destVec, 3);
        if (!isRotationFinished(curAttitude, destVec)) {
            DEBUG(("Rotating..."));
            gotoRotation(destVec);
            return;
        }
        
        
        //todo: error checking to handle failed docks
        bool docked = game.dockItem();
        if (docked) {
            //DEBUG(("DOCKED WITH ITEM! :)"));
            Sphere.state = PLACING;
        } else {
            //DEBUG(("DOCK FAILED! Please DEBUG me"));
        }
    } 
    else if (Sphere.state==PLACING) {
        float cubePos[3];
        game.getItemLoc(cubePos, target.itemID);

        gotoPosition(cubePos, Sphere.zone.pos, vel);
        if (isWithinManhattanDist(cubePos,Sphere.zone.pos,0.017)){ 
            game.dropItem();
            Sphere.state = GATHERING;
            evaluateNextTarget();
        }
    }
}


//TODO: make moving so good that we don't need this
bool isSphereWithinDockLoc(float curPos[3]) {
    //float loc[3] = { 0, 0, 0 };
    float loc[3];
    //zeros(loc);
    loc[0] = 0;
    loc[1] = 0;
    loc[2] = 0;
    
    game.getItemLoc(loc,target.itemID);
    
    float destVec[3];
    mathVecSubtract(destVec, loc, curPos, 3);
    float dist = mathVecNormalize(destVec,3);
    
    DEBUG(("dist from center of cube: %f", dist));
    switch (target.itemID) {
        case 0:
        case 1:
            if (dist >= .151 && dist <= .173)
                return true;
            break;
        case 2:
        case 3:
            if (dist >= .138 && dist <= .160)
                return true;
            break;
        case 4:
        case 5:
            if (dist >= .124 && dist <= .146)
                return true;  
            break;
        default:
            break;
    }

    return false;
}

//TODO: adjust once rotation is implemented
void adjustTargetPosition() {
    float upperBound; /* a.k.a middle bound */
    switch (target.itemID) {
        case 0:
        case 1:
            upperBound = .162;
            break;
        case 2:
        case 3:
            upperBound = .149;
            break;
        case 4:
        case 5:
            upperBound = .135;
            break;
        default:
            upperBound = 0.0;
            break;
    }
    
    float attitude[3];
    for (int i = 0; i < 3; i++) {
        attitude[i] = Sphere.zrState[i + 6] * upperBound;
    }
    
    target.pos[0] = target.pos[0] - attitude[0];
    target.pos[1] = target.pos[1] - attitude[1];
    target.pos[2] = target.pos[2] - attitude[2];
    //DEBUG(("Permuted Loc: %f %f %f", target.X, target.Y, target.Z));
}

void evaluateNextTarget () {
    int itemID = 0;
    //TODO: IMPROVE THIS
    for (int i = 0; i < 6; i++) {
        if (game.hasItem(i) != 0 || game.itemInZone(i)) {
            continue;
        }
        
        itemID = i;
        break;
    }
    
    float loc[3];
    loc[0] = 0;
    loc[1] = 0;
    loc[2] = 0;
    
    game.getItemLoc(target.pos,itemID);
    target.itemID = itemID;
    target.isItem = true;
    
    adjustTargetPosition();
    //DEBUG(("Item %d; %f %f %f", itemID, target.X, target.Y, target.Z));
}

bool placeUnit(float pos[3], bool isSPS, float TOLERANCE) {
    //float u[3];
    //unwrapPosition(u,p);
    if (isWithinManhattanDist(Sphere.zrState,pos, TOLERANCE)) {
        if (isSPS)
            game.dropSPS();
        else
            game.dropItem();
        return true;
    }
    return false;
}

bool isWithinManhattanDist(float p1[3], float p2[3], float TOLERANCE) {
    if (fabsf(p2[0] - p1[0]) < TOLERANCE &&
        fabsf(p2[1] - p1[1]) < TOLERANCE &&
        fabsf(p2[2] - p1[2]) < TOLERANCE)
        return true;
    return false;
}

/*
bool isUnitWithinToleranceDistance(position p, float TOLERANCE) {
    // float TOLERANCE = 0.05;
    if (fabsf(Sphere.zrState[0] - p.X) < TOLERANCE &&
        fabsf(Sphere.zrState[1] - p.Y) < TOLERANCE &&
        fabsf(Sphere.zrState[2] - p.Z) < TOLERANCE)
        return true;
    return false;
}*/
    
//todo: specify exit velocities
//todo: current problems: why doesn't setting MAX_SPEED to max or ACCEL_FACTOR to max make it faster
//todo: add parameter that specifies fuel / time trade off
void gotoPosition(float currentPos[3], float destPosition[3], float velocity[3]) {
    float MAX_SPEED = 0.03;//4;         //max speed that should be obtained
    float SAT_MASS = 4.7;//4.8;//4.7           //mass in kg -- used for finding force to apply
    float SAT_MAX_ACCEL = 0.0065;//0.007;//0.0075//how fast the sat can decelerate
    float ACCEL_FACTOR = 0.18;//0.18;     //how fast to accelerate
    float MIN_DIST_VEL_PADDING = 1.2;   //make the min dist a little bit larger
    float MIN_MIN_DIST = 0.01;          //to prevent random movement from effecting things
    
    // for regular builtin position comparison testing//
    //api.setPositionTarget(destPosition);
    //return;
    
    // float directionVec[3];
    // mathVecSubtract(directionVec, destPosition, currentPos,3);
    // float spherePos[3];
    // spherePos[0] = Sphere.zrState[0];
    // spherePos[1] = Sphere.zrState[1];
    // spherePos[2] = Sphere.zrState[2];
    // float addVec[3];
    // mathVecAdd(addVec, directionVec, spherePos, 3);
    // api.setPositionTarget(addVec);
    // return;
    // DEBUG(("DEST : X:%f Y:%f Z:%f", destPosition[0], destPosition[1], destPosition[2]));
     //DEBUG(("DIRR : %f %f %f", directionVec[0], directionVec[1], directionVec[2]));
    
    float accel[3];
    for (int i = 0; i < 3; i++) {
        float powfResult = powf((velocity[i] * MIN_DIST_VEL_PADDING), 2);
        float divisor = 2 * SAT_MAX_ACCEL * (1.0 / currentAccelerationFactor());
        float MIN_DIST = MIN_MIN_DIST > (powfResult / divisor) ? MIN_MIN_DIST : (powfResult/divisor);
        
        if (fabsf(directionVec[i]) < MIN_DIST) {
            // DEBUG(("%d WITHIN MIN DIST",i));
            accel[i] = - SAT_MASS *sign(velocity[i])*currentAccelerationFactor()*(powf(velocity[i],2)/(2*fabsf(directionVec[i])));
        } else if (fabsf(velocity[i]) < MAX_SPEED || sign(velocity[i])!=sign(directionVec[i])) {
            if (sign(velocity[i])!=sign(directionVec[i])) {
                DEBUG(("%d OVERSHOT",i)); //note: it's hella expensive fuel-wise to overshoot
            }
            accel[i] = directionVec[i] * ACCEL_FACTOR;
        } else {
          accel[i] = 0;
        }
    }
    
    api.setForces(accel);
    // DEBUG(("FORCE : X:%f Y:%f Z:%f", accel[0], accel[1], accel[2]));
    
}

/*void vecSubtract(float r[3], float a[3], float b[3]) {
    r[0] = a[0] - b[0];
    r[1] = a[1] - b[1];
    r[2] = a[2] - b[2];
}*/
/*
bool isRotationFinished (float curAttitude[3], float destVec[3]) {
    //float ROTATION_RADIUS = 0.25;
    
    // both curAttitude & destVec are unit vectors
    if (fabsf(curAttitude[0] - destVec[0]) < 0.25 &&
        fabsf(curAttitude[1] - destVec[1]) < 0.25 &&
        fabsf(curAttitude[2] - destVec[2]) < 0.25)
        return true;
    
    return false;
}
*/
// //add more parameters for efficiency later
void gotoRotation(float destRot[3]) {
    api.setAttitudeTarget(destRot);
}

//depending on sps held / object held
float currentAccelerationFactor() {
    float factor = 1;
    switch(game.getNumSPSHeld()) {
        case 0:
            if (!target.isItem || game.hasItem(target.itemID)!=1)
                return 1.0;
                
            switch (target.itemID) {
                case 0:
                case 1:
                    factor *= (11.0/8.0);
                    break;
                case 2:
                case 3:
                    factor *= (5.0/4.0);
                    break;
                case 4:
                case 5:
                    factor *= (9.0/8.0);
                    break;
                default:
                    factor *= (1.0);
                    break;
            }
            
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
}

int sign(float n) {
    return n > 0 ? 1 : -1;
}

float max(float a, float b){
    return (a>b ? a : b);
}
//todo: make dynamic to size
/*void zeros(float u[3]) {
    for (int i = 0; i < 3;i++){
        u[i] = 0;
    }
}*/