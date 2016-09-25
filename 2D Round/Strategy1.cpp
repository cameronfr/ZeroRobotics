//Declare any variables shared between functions here
bool isDropped[3];
float spsUnits[3][3];
float spsDropMargin;

bool isZoned;
float zoneInfo[4];

/*struct STATE {
    bool SPS;
    bool SATTELITES;
};*/

//state currentState;

void init(){
	//currentState.SPS = true;
	//currentState.SATTELITES = false;
	
	isDropped[0] = false;
	isDropped[1] = false;
	isDropped[2] = false;
	 
	spsUnits[0][0] = -0.75;
	spsUnits[0][1] = 0.75;
	spsUnits[0][2] = 0;
	
	spsUnits[1][0] = 0.75;
	spsUnits[1][1] = 0.75;
	spsUnits[1][2] = 0;
	
	spsUnits[2][0] = 0;
	spsUnits[2][1] = -0.75;
	spsUnits[2][2] = 0;
	
	spsDropMargin = 0.1;
	isZoned = false;
}

void loop(){
	int next = nextDrop();
	DEBUG(("next %d", next));
	if (next != -1) {
	    placeSPSUnit(next, spsUnits[next]);
	    next = nextDrop();
	}
	
	if (!isZoned) {
	    game.getZone(zoneInfo);
	    isZoned = true;
	}
}

int nextDrop () {
    for (int i = 0; i < 3; i++) {
        if (!isDropped[i]) {
            return i;
        }
    }
    return -1;
}

void placeSPSUnit(int i, float pos[3]) {
    DEBUG(("drop pos: %f, %f, %f",pos[0],pos[1],pos[2]));
    api.setPositionTarget(pos);
    float state[12];
    api.getMyZRState(state);
    //DEBUG(("state: %f, %f, %f",state[0],state[1],state[2]));
    if (fabsf(state[0] - pos[0]) < spsDropMargin) {
        if (fabsf(state[1] - pos[1]) < spsDropMargin) {
            game.dropSPS();
            isDropped[i]=true;
        }
    }
    
}




