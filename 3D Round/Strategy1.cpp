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
    position zoneTarget;
    sphereState state;
    float zrState[12];
} sphere;

sphere Sphere;

// position firstSPS;
position secondSPS;
position target;
position realTarget;

//Declare any variables shared between functions here
void init() {
    Sphere.state = SPS_UNITS;
    api.getMyZRState(Sphere.zrState);
    
    // firstSPS.pos[0] = 0.0;
    // firstSPS.pos[1] = sign(Sphere.zrState[1]) * 0.15;
    // firstSPS.pos[2] = 0;
    
    //TODO: TWEAK AND FIX THIS
    secondSPS.pos[0] = sign(Sphere.zrState[1])* -0.5;//-0.45;
    secondSPS.pos[1] = 0.45;
    secondSPS.pos[2] = 0;//sign(Sphere.zrState[1])* - 0.2;
    target.itemID = -1;
    
    DEBUG(("BEEP BOP HELLO"));
}

void loop() {
    //DEBUG(("-----------------------"));
    
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
    
    //DEBUG(("State: %d",Sphere.state));
    DEBUG(("Target Item ID: %d",target.itemID));
    
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
        
        // float curAttitude[3] = { Sphere.zrState[6], Sphere.zrState[7], Sphere.zrState[8] };
        // float destVec[3];
        // mathVecSubtract(destVec, realTarget.pos, position, 3);
        // mathVecNormalize(destVec, 3);
        // gotoRotation(destVec);
        rotateToTarget(position,realTarget.pos);

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
        moveToPlaceOrGetItem(position, velocity);
    }
}

void moveToPlaceOrGetItem (float pos[3], float vel[3]) {
    //float curAttitude[3] = { Sphere.zrState[6], Sphere.zrState[7], Sphere.zrState[8] };
    // float destVec[3];
    // mathVecSubtract(destVec, realTarget.pos, pos, 3);
    // mathVecNormalize(destVec, 3);
    // gotoRotation(destVec);
    
    //TODO: URGENT: RUNNING THIS CONSTANTLY GLITCHES THINGS OUT
    //on second thought maybe it should only run once
    adjustTargetPosition();
    if (Sphere.state == GATHERING) {
        rotateToTarget(pos,realTarget.pos);
        DEBUG(("TARGET %f %f %f",target.pos[0],target.pos[1],target.pos[2]));

        
        
        if (game.hasItem(target.itemID) == 2) {
            evaluateNextTarget();
        }
        
        if (!isSphereWithinDockLoc(pos)) {
            gotoPosition(pos, target.pos, vel);
            DEBUG(("Shifting position... to %f %f %f",target.pos[0],target.pos[1],target.pos[2]));
            return;
        }
        
        //TODO: THIS IS NEEDED BUT SHOULDNT BE -- FIX THIS
        //this might be interacting with the "adjustposition" function which is constantly called
        else if (mathVecMagnitude(vel, 3) > 0.01) {
            /*float zeroVel[3];
            zeroVel[0] = 0;
            zeroVel[1] = 0;
            zeroVel[2] = 0;
            
            api.setVelocityTarget(zeroVel);
            DEBUG(("Slowing speed..."));*/
            return;
        }
        
        //todo: error checking to handle failed docks
        bool docked = game.dockItem();
        if (docked) {
            DEBUG(("DOCKED WITH ITEM! :)"));
            Sphere.state = PLACING;
        } else {
            DEBUG(("DOCK FAILED! Please DEBUG me"));
        }
    } 
    else if (Sphere.state==PLACING) {
        float cubePos[3];
        game.getItemLoc(cubePos, target.itemID);
    
        //added:
        rotateToTarget(pos,Sphere.zone.pos);
        //changed from Sphere.zone.pos to Sphere.zoneTarget.pos
        gotoPosition(pos, Sphere.zoneTarget.pos, vel);
        //THIS WONT WORK WELL WITH ROTATION BECAUSE THE CUBE WILL CHANGE POSITION AS IT ROTATES
        //gotoPosition(cubePos, Sphere.zone.pos, vel);
        if (isWithinManhattanDist(cubePos,Sphere.zone.pos,0.017)){ 
            game.dropItem();
            Sphere.state = GATHERING;
            evaluateNextTarget();
        }
    }
}


//TODO: make moving so good that we don't need this
bool isSphereWithinDockLoc(float curPos[3]) {
    float loc[3];
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

void adjustTargetPosition() {
    //TODO: the way this adjust target method works is a bit messy cause it modifies global vars
    DEBUG(("ADJUSTING TARGET POS"));
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
    
    float pos[3];
    for (int i = 0; i < 3; i++) {
        pos[i] = Sphere.zrState[i];
    }
    
    //TODO: fix the problem which causes this position to rapidly change when the sphere passes over the center point of the cube
    //why is target.pos changing so much??
    // oh lol this is so fucked up because target.pos is changing itself based on itself each iteration... fix this
    
    float dirVec[3];
    mathVecSubtract(dirVec, realTarget.pos, pos, 3);
    mathVecNormalize(dirVec, 3);
    dirVec[0] *= upperBound;
    dirVec[1] *= upperBound;
    dirVec[2] *= upperBound;
    //realTarget.pos[0] = target.pos[0];
    //realTarget.pos[1] = target.pos[1];
    //realTarget.pos[2] = target.pos[2];
    //DEBUG(("NEW TARGET, %d; %f %f %f", 0, target.pos[0], target.pos[1], target.pos[2]));
    //4
    target.pos[0] = realTarget.pos[0] - dirVec[0];
    target.pos[1] = realTarget.pos[1] - dirVec[1];
    target.pos[2] = realTarget.pos[2] - dirVec[2];
    //5
    //DEBUG(("NEW TARGET, %d; %f %f %f", 0, target.pos[0], target.pos[1], target.pos[2]));

    
    //TODO: IMPORTANT: confirm that this works
    float dirVec2[3];
    mathVecSubtract(dirVec2, Sphere.zone.pos, pos, 3);
    mathVecNormalize(dirVec2, 3);
    dirVec2[0] *= upperBound;
    dirVec2[1] *= upperBound;
    dirVec2[2] *= upperBound;
    Sphere.zoneTarget.pos[0] = Sphere.zone.pos[0] - dirVec2[0];
    Sphere.zoneTarget.pos[1] = Sphere.zone.pos[1] - dirVec2[1];
    Sphere.zoneTarget.pos[2] = Sphere.zone.pos[2] - dirVec2[2];


    /*
    float attitude[3];
    for (int i = 0; i < 3; i++) {
        attitude[i] = Sphere.zrState[i + 6] * upperBound;
    }
    
    target.pos[0] = target.pos[0] - attitude[0];
    target.pos[1] = target.pos[1] - attitude[1];
    target.pos[2] = target.pos[2] - attitude[2];
    */
    //DEBUG(("Permuted Loc: %f %f %f", target.X, target.Y, target.Z));
}

void evaluateNextTarget () {
    //TODO TODAY (10/26)
    //FIX SECOND SPS PLACEMENT LOGIC BASED ON WHETHER WE ARE RED OR BLUE
    //GO FOR THE FARTHER YELLOW CUBE AFTER SECOND SPS
    //AFTER STATE is NOT SPS_UNITS, GO FOR CUBE THAT IS EITHER 1. WITHIN THRESHOLD (i.e. close), or 2. HIGHEST PT VAL
    
    int itemID = 0;
    //TODO: IMPROVE THIS -- WEIGHT DIST VS VALUE
    for (int i = 0; i < 6; i++) {
        if (game.hasItem(i) != 0 || game.itemInZone(i)) {
            continue;
        }
        
        
        itemID = i;
        break;
    }
    
    //game.getItemLoc(target.pos,itemID);
    game.getItemLoc(realTarget.pos,itemID);
    target.itemID = itemID;
    target.isItem = true;
    //DEBUG(("NEW TARGET, %d; %f %f %f", itemID, target.pos[0], target.pos[1], target.pos[2]));
    adjustTargetPosition();
}

bool placeUnit(float pos[3], bool isSPS, float TOLERANCE) {
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

//TODO: currently overshoots when it has to pass by the other sattelite
//TODO: EASY FIX: take the timestep into account -- assume th
//todo: specify exit velocities
//todo: current problems: why doesn't setting MAX_SPEED to max or ACCEL_FACTOR to max make it faster
//todo: add parameter that specifies fuel / time trade off
void gotoPosition(float currentPos[3], float destPosition[3], float velocity[3]) {
    float MAX_SPEED = 0.04;//4;         //max speed that should be obtained
    float SAT_MASS = 4.9;////4.85;//4.7;//4.8;//4.7;//4.8;//4.7           //mass in kg -- used for finding force to apply
    float SAT_MAX_ACCEL = 0.0065;//0.0065;//0.007;//0.0075//how fast the sat can decelerate
    float ACCEL_FACTOR = 0.18;//0.18;     //how fast to accelerate
    float MIN_DIST_VEL_PADDING = 1.2;//1.5;//1.4;//1.3;//1.2;   //make the min dist a little bit larger
    float MIN_MIN_DIST = 0.01;          //to prevent random movement from effecting things
    
    float directionVec[3];
    mathVecSubtract(directionVec, destPosition, currentPos,3);
    
    //TODO: parameterize and find good parameter, or don't use it at all
    //Switches to PID  when under certain distance
    if (mathVecMagnitude(directionVec,3) < 0.1) {
        float spherePos[3];
        spherePos[0] = Sphere.zrState[0];
        spherePos[1] = Sphere.zrState[1];
        spherePos[2] = Sphere.zrState[2];
        float addVec[3];
        mathVecAdd(addVec, directionVec, spherePos, 3);
        api.setPositionTarget(addVec);
        DEBUG(("Using PID position control..."));
        return;
        
    }
    //DEBUG(("DEST : X:%f Y:%f Z:%f", destPosition[0], destPosition[1], destPosition[2]));
    //DEBUG(("DIRR : %f %f %f", directionVec[0], directionVec[1], directionVec[2]));
    
    float accel[3];
    for (int i = 0; i < 3; i++) {
        float powfResult = mathSquare(velocity[i] * MIN_DIST_VEL_PADDING);
        float divisor = 2 * SAT_MAX_ACCEL * (1.0 / currentAccelerationFactor());
        float MIN_DIST = MIN_MIN_DIST > (powfResult / divisor) ? MIN_MIN_DIST : (powfResult/divisor);
        
        if (fabsf(directionVec[i]) < MIN_DIST) {
            accel[i] = - SAT_MASS *sign(velocity[i])*currentAccelerationFactor()*(mathSquare(velocity[i])/(2*fabsf(directionVec[i])));
            DEBUG(("%d WITHIN MIN DIST, DECEL AT %f",i,(accel[i]/SAT_MASS)));
        } else if (fabsf(velocity[i]) < MAX_SPEED || sign(velocity[i])!=sign(directionVec[i])) {
            /*if (sign(velocity[i])!=sign(directionVec[i])) {
                DEBUG(("%d OVERSHOT",i)); //note: it's hella expensive fuel-wise to overshoot
            }*/
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
void rotateToTarget(float currentPos[3],float targetPos[3]) {
    //float curAttitude[3] = { Sphere.zrState[6], Sphere.zrState[7], Sphere.zrState[8] };
    float destVec[3];
    mathVecSubtract(destVec, targetPos, currentPos, 3);
    mathVecNormalize(destVec, 3);
    gotoRotation(destVec);
}

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