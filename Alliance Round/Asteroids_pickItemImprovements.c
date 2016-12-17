//New pickCube functions:

//1. Goes forward on the side, and then slides into the correct position
//Results: marginally faster (~2 points) than current strategy
void pickItem() {
    float dmax;
    float dmin;
    //position of target
    float targetPos[3];
    //vector from us to target
    float targetVec[3];
    //item attitude vec
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

    //has case for ID == 6 in special item strategy
    if (game.hasItem(ID) == 0) {

        for (int i = 0; i < 3; i++) {
            targetPos[i] = item[ID][i+6];
            targetPos[i] *= (dmin+dmax)/2;
            targetPos[i] += item[ID][i];
            itemAtt[i] = item[ID][i+6];
            orthagComponent[i] = item[ID][i+6];
            targetVec[i] = myState[i]-targetPos[i];

        }
        //this value if is positive if we need to go around the item
        //distance between position and plane of correct face of item
        float D = mathVecInner(itemAtt,targetPos,3);
        float pointPlaneDist = mathVecInner(itemAtt,myState,3)-D;
        DEBUG(("point plane dist %f",pointPlaneDist));

        if (pointPlaneDist < -0.2f) {
            float target[3];
            //projection of the distance vec (sattelite -> item) onto plane of correct face of item
            //http://maplecloud.maplesoft.com/application.jsp?appId=5641983852806144
            float projVecPlane[3];
            multVec(orthagComponent,mathVecInner(itemAtt,targetVec,3)/(mathVecMagnitude(itemAtt,3)));
            mathVecSubtract(projVecPlane,targetVec,orthagComponent,3);
            mathVecNormalize(projVecPlane,3);
            multVec(projVecPlane,dmax*1.5f);
            mathVecAdd(target,targetPos,projVecPlane,3);
            moveCapVelocity(target);
        }
        else {
            moveCapVelocity(targetPos);
        }
        multVec(itemAtt,-1);
        api.setAttitudeTarget(itemAtt);

        float cosalpha = mathVecInner(myAtt, vec[ID], 3)/(mathVecMagnitude(myAtt, 3)*mathVecMagnitude(vec[ID], 3));
        if (distance[ID] < dmax && mathVecMagnitude(myVel, 3) < 0.01f && cosalpha > 0.97f && game.isFacingCorrectItemSide(ID)) {
            game.dockItem(ID);
        }
    } else {
        reachZone();
    }
}

//2. move cap velocity with dynamic stopping distance
void moveCapVelocity(float *whereTo){
    float vel = mathVecMagnitude(myVel,3)+0.008;//what vel might be next timestep
    float minDist = (vel*vel)/(2*0.008);//when to start decel (i.e. using setPositionTarget), from v_f^2=v_i^2+2ad
    minDist += mathVecMagnitude(myVel,3) + 0.004; //accounting for what dist would be next timestep
    DEBUG(("min distance %f",minDist));
    
    if (dist(myState, whereTo) < minDist){
        DEBUG(("using PID"));
        api.setPositionTarget(whereTo);
    }
    else{
        float v[3];
        mathVecSubtract(v, whereTo, myState, 3);
        mathVecNormalize(v, 3);
        multVec(v, 0.05f);
        api.setVelocityTarget(v);
    }
}