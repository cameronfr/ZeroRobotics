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

//1. Modified 'rotate around item'
//modified so that attitude is always set to the right attitude
void pickItem() {
    float dmax;
    float dmin;
    float vecAtt[3];
    float a[3];
    float target[3];
    float A[3];
    float cosalpha = 0;
    float dirAtt[3]; //THIS LINE ADDED

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

            //new
            dirAtt[i] = -item[ID][i+6];
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
        api.setAttitudeTarget(dirAtt); //THIS LINE CHANGED
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
