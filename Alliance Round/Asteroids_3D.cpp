/*
* Strategy:
* Place SPSs, with the third at the cube not taken by the opposition, then go for yellow cubes, and really close pink cubes
*
* Testing:
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
    position zone;
    position zoneTarget;
    sphereState state;
    float zrState[12];
} sphere;

sphere Sphere;

position secondSPS;
position target;
position realTarget;
position thirdSPS;

void init() {
    Sphere.state = SPS_UNITS;
    api.getMyZRState(Sphere.zrState);

    int side = sign(Sphere.zrState[1]);

    secondSPS.pos[0] = side * -0.5;
    secondSPS.pos[1] = side * 0.5;
    secondSPS.pos[2] = 0;

    target.itemID = -1;

    DEBUG(("BEEP BOP HELLO"));
}

void loop() {
    api.getMyZRState(Sphere.zrState);
    int c = game.getNumSPSHeld();
    int numSPSRemaining= 3 - c;

    float position[3];
    position[0] = Sphere.zrState[0];
    position[1] = Sphere.zrState[1];
    position[2] = Sphere.zrState[2];

    float velocity[3];
    velocity[0] = Sphere.zrState[3];
    velocity[1] = Sphere.zrState[4];
    velocity[2] = Sphere.zrState[5];

    //DEBUG(("State: %d",Sphere.state));
    //DEBUG(("Target Item ID: %d",target.itemID));

    if (numSPSRemaining == 0) {
        // gotoPosition(position, firstSPS.pos, velocity);
        placeUnit(position, true, 0.01);
    } else if (numSPSRemaining == 1) {
        //DEBUG(("second SPS unit : %f %f %f", u[0], u[1], u[2]));
        gotoPosition(position, secondSPS.pos, velocity);
        placeUnit(secondSPS.pos, true, 0.015);
    } else if (numSPSRemaining == 2) {
        if (target.itemID == -1) {
            evaluateNextTarget(position);
            Sphere.state = GATHERING;
        }
        moveToPlaceOrGetItem(position, velocity);
        if (placeUnit(target.pos, true, 0.015)) {
            setZoneLocation();
        }
    } else {
        moveToPlaceOrGetItem(position, velocity);
    }
}

void setZoneLocation() {
    float zone[4];
    game.getZone(zone);
    Sphere.zone.pos[0] = zone[0];
    Sphere.zone.pos[1] = zone[1];
    Sphere.zone.pos[2] = zone[2];
}

void moveToPlaceOrGetItem (float pos[3], float vel[3]) {

    adjustTargetPosition(pos);
    if (Sphere.state == GATHERING) {
        rotateToTarget(pos,realTarget.pos);

        if (!isSphereWithinDockLoc(pos) || mathVecMagnitude(vel, 3) > 0.01 || game.hasItem(target.itemID) == 2) {
            gotoPosition(pos, target.pos, vel);
            //DEBUG(("Shifting position... to %f %f %f",target.pos[0],target.pos[1],target.pos[2]));
            return;
        }

        bool docked = game.dockItem(target.itemID);
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

        rotateToTarget(pos,Sphere.zone.pos);
        gotoPosition(pos, Sphere.zoneTarget.pos, vel);
        if (isWithinManhattanDist(cubePos,Sphere.zone.pos,0.02)){
            game.dropItem();
            Sphere.state = GATHERING;
            evaluateNextTarget(pos);
        }
    }

}


//TODO: Check if this is necessary, given the precision of gotoPosition
bool isSphereWithinDockLoc(float curPos[3]) {
    float loc[3];
    loc[0] = 0;
    loc[1] = 0;
    loc[2] = 0;

    game.getItemLoc(loc,target.itemID);

    float destVec[3];
    mathVecSubtract(destVec, loc, curPos, 3);
    float dist = mathVecMagnitude(destVec,3);

    //DEBUG(("Distance from center of cube: %f", dist));
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

void adjustTargetPosition(float pos[3]) {
    //TODO: the way this adjust target method works is a bit messy cause it modifies global vars
    //TODO: experiment with changing middle bound to upper bound
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

    game.getItemLoc(realTarget.pos,target.itemID);
    float dirVec[3];
    mathVecSubtract(dirVec, realTarget.pos, pos, 3);
    mathVecNormalize(dirVec, 3);
    dirVec[0] *= upperBound;
    dirVec[1] *= upperBound;
    dirVec[2] *= upperBound;
    mathVecSubtract(target.pos,realTarget.pos,dirVec,3);

    float dirVec2[3];
    mathVecSubtract(dirVec2, Sphere.zone.pos, pos, 3);
    mathVecNormalize(dirVec2, 3);
    dirVec2[0] *= upperBound;
    dirVec2[1] *= upperBound;
    dirVec2[2] *= upperBound;
    mathVecSubtract(Sphere.zoneTarget.pos,Sphere.zone.pos,dirVec2,3);

}

float itemDist(float pos[3],int itemID) {
    float itemLoc[3];
    float diff[3];
    game.getItemLoc(itemLoc,itemID);
    mathVecSubtract(diff, pos, itemLoc,3);
    float dist = mathVecMagnitude(diff,3);
    return dist;
}

//TODO: if not enough fuel to get to next cube after placing cube in zone, just block zone
void evaluateNextTarget (float pos[3]) {
    int itemID = 0;

    if(Sphere.state == SPS_UNITS) {

        //When placing third SPS, go for cube that isn't already taken
        if (game.hasItem(0) !=0 ) {
            itemID = 1;
        }
        else {
            itemID = 0;
        }

    }
    else {

        bool foundItem = false;
        float ITEM_DIST_THRESHOLD = 0.4;

        for (int i = 0; i < 6; i++) {
            if (game.hasItem(i) != 0 || game.itemInZone(i)) {
                continue;
            }
            float distFromZone = itemDist(Sphere.zone.pos,i);
            //DEBUG(("DIST FROM ZONE %f",distFromZone));
            if (distFromZone < ITEM_DIST_THRESHOLD) {
                foundItem = true;
                itemID = i;
                break;
            }
        }

        //DEBUG(("FOUND ITEM ID: %d",itemID));
        if (!foundItem) {
            for (int i = 0; i < 6; i++) {
                if (game.itemInZone(i)) {
                    continue;
                }

               itemID = i;
               break;
            }
        }
    }

    target.itemID = itemID;
    target.isItem = true;
    //DEBUG(("NEW TARGET, %d; %f %f %f", itemID, target.pos[0], target.pos[1], target.pos[2]));
    adjustTargetPosition(pos);
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

//TODO: specify exit velocities
//TODO: add parameter that specifies fuel / time trade off
void gotoPosition(float currentPos[3], float destPosition[3], float velocity[3]) {
    float MAX_SPEED =0.04;// 0.04; //max speed that should be obtained
    float ACCELERATION_FACTOR = 1; //what percentage of max accel should we accel at
    float SAT_MASS = 4.75 * currentMassFactor();//4.9;////4.85;//4.7;//4.8;//4.7;//4.8;//4.7 //normal mass in kg -- used for finding force to apply
    float SAT_MAX_ACCEL = 0.008 / currentMassFactor(); //how fast the sat can decelerate at normal mass
    float MIN_MIN_DIST = 0.01;          //to prevent random movement from effecting things
    float ZERO_VEL_DIST_THRESH = 0.005; //when to switch to zeroing velocity instead of accelerating

    float directionVec[3];
    mathVecSubtract(directionVec, destPosition, currentPos,3);

    //DEBUG(("DEST : X:%f Y:%f Z:%f", destPosition[0], destPosition[1], destPosition[2]));
    //DEBUG(("DIRR : %f %f %f", directionVec[0], directionVec[1], directionVec[2]));
    float MAX_1D_DIST = max(max(fabsf(directionVec[0]),fabsf(directionVec[1])),fabsf(directionVec[2]));
    //DEBUG(("MAX DIST DIM is %f",MAX_1D_DIST));

    float force[3];
    for (int i = 0; i < 3; i++) {

        //Factor that describes speed/accel in relation to other dimensions (i.e., keep vel / accel prop to dist)
        float PROPORTIONAL_FACTOR = fabsf(directionVec[i]/MAX_1D_DIST);
        float DIM_MAX_SPEED = MAX_SPEED * PROPORTIONAL_FACTOR;

        //Calculate both what we woud accel and decel the sattelite at (which we use depends on min_dist)
        float est_accel = sign(directionVec[i])*ACCELERATION_FACTOR*PROPORTIONAL_FACTOR*SAT_MAX_ACCEL;
        float est_decel = - sign(velocity[i]) * (mathSquare(velocity[i])/(2*fabsf(directionVec[i])));

        //Calculate the distance at which we need to start decelerating
        float powfResult = mathSquare(fabsf(velocity[i]) + fabsf(est_accel)); //v = v_current + a*(1 timestep ahead)
        float divisor = 2 * (SAT_MAX_ACCEL);
        float MIN_DIST = (powfResult/divisor)+(fabsf(velocity[i])+0.5*fabsf(est_accel));// dist + v_o*t + 0.5*a*t^2
        MIN_DIST = MIN_MIN_DIST > MIN_DIST ? MIN_MIN_DIST : MIN_DIST; //take min of min_dist and min_min_dist
        //MIN DIST SUMMARY: the position we need to start decelerating at, given what the velocity and position of the next timestep would be worst case (so that we start decel 1 timsetep early as opposed to late)

        if (fabsf(directionVec[i]) < ZERO_VEL_DIST_THRESH) {
            //if dist is low, try and cancel velocity
            force[i] = - velocity[i]* SAT_MASS;
            //DEBUG(("%d ZEROING VEL",i));
        } else if (fabsf(directionVec[i]) < MIN_DIST) {
            force[i] = est_decel * SAT_MASS;
            //DEBUG(("%i DECELING at %f, DIST %f, MIN_DIST %f",i,force[i]/SAT_MASS,directionVec[i],MIN_DIST));
        } else if (fabsf(velocity[i]) < DIM_MAX_SPEED || sign(velocity[i])!=sign(directionVec[i])) {
            force[i] = est_accel * SAT_MASS;
            //DEBUG(("%i ACCELING at %f, DIST %f, MIN_DIST %f",i,force[i]/SAT_MASS,directionVec[i],MIN_DIST));
        } else {
            force[i] = 0;
        }
    }

    api.setForces(force);
    // DEBUG(("FORCE : X:%f Y:%f Z:%f", force[0], force[1], force[2]));

}

void rotateToTarget(float currentPos[3],float targetPos[3]) {
    float destVec[3];
    mathVecSubtract(destVec, targetPos, currentPos, 3);
    mathVecNormalize(destVec, 3);
    gotoRotation(destVec);
}

void gotoRotation(float destRot[3]) {
    api.setAttitudeTarget(destRot);
}

float currentMassFactor() {
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
