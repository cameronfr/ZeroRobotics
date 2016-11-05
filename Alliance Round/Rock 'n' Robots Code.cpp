//==================================================================================
//Zero Robotics 2016 Competition
//Craig 2016 3D: SPACE-S
//Authors: Travis Duffy, Ben Glowacki, Sophie Werner, Frank Breu, Alexei Sapozhnikov
//==================================================================================

//Declare any variables 

float me[12];
float zoneInfo[4];
float location[3];
float triPoint[3];
float vectorBetween[3];

int pos;
int secondBlock;

bool enemyDocked;
bool firstItemDropped;
bool isDocked;
bool secondDock;
bool secondItemDocked;
bool secondItemDropped;
bool otherBlockFound;
bool triangleFinished;
//Initialization method
void init() {
    //Drop our first point
    game.dropSPS();
 
    //Find which side we are on:
    api.getMyZRState(me);
    if (me[1] < 0)
        triPoint[1] = -0.8f;
    else
        triPoint[1] = 0.8f;
    triPoint[0] = 0.0f;
    triPoint[2] = 0.0f;
    //Initialize Boolean and Int Values
    triangleFinished = false;
    isDocked = false;
    firstItemDropped = false;
    secondItemDocked = false;
    secondItemDropped = false;
    secondDock = false;
    otherBlockFound = false;
    enemyDocked = false;
    
    pos = 0;
    secondBlock = 0;
}
void loop() {
    updateLocations();  //Update location array for each object 
    
    if (!triangleFinished) {
        triangleFinished = triAndPickup(); //Make the triangle and pick up the first item
    } else if (!firstItemDropped) {
        firstItemDropped = dropItem(); //Drop the first item in our zone
    } else if (!secondItemDocked) {
        secondItemDocked = dockItem(findItem()); //Dock the second item
    } else if (!secondItemDropped) {
        secondItemDropped = dropItem(); //Drop the second item in our zone
    } else {
        DEBUG(("COMPLETED."));
    }
}

//Function dockItem()
//=======================
//INPUTS: int (Item ID)
//OUTPUTS: Boolean (True if item successfully docked)
//DESCRIPTION: Moves sphere to an item and docks it

bool dockItem(int item)
{
    //Get the location of the item from the item ID
    game.getItemLoc(location, item);
    
    //Move to the location of the item, and once we are there, attempt to dock the item.  
    //Return true when finished.
    if (moveTo(location, 0.01f, true) && (game.dockItem(item)))
          return true; 
    return false;
}

//Function dropItem()
//=======================
//INPUTS: NONE
//OUTPUTS: Boolean (True if item dropped)
//DESCRIPTION: Moves sphere to our zone and drops item

bool dropItem()
{
    //Once we get to our Zone, drop the item and return true
    if (moveTo(zoneInfo, .05f, true)) {
        game.dropItem();
        return true;
    }
    return false;
}



//function face
//================
//INPUTS: float* (Coordinates to face toward)
//OUTPUTS: none
//DESCRIPTION: Rotates and tilts sphere to look at a set of coordinates

void face(float* faceTarget)
{
    //Calculate the vector from the target coodinates and set the attitude 
    mathVecSubtract(vectorBetween, faceTarget, me, 3);
    mathVecNormalize(vectorBetween, 3);
    api.setAttitudeTarget(vectorBetween);
}

//function findItem
//=================
//INPUTS: none
//OUTPUTS: int (Item ID)
//DESCRIPTION: Rotates and tilts sphere to look at a set of coordinates

int findItem() {
    if (!triangleFinished) {


    //declaration
    api.getMyZRState(me);
    float distance[1];
    
    //initialization of the distance array
    game.getItemLoc(location, 0);
    distance[0] = sqrtf((power(location[0] - me[0]) + power(location[1] - me[1]) + power(location[2] - me[2])));
    game.getItemLoc(location, 1);
    distance[1] = sqrtf((power(location[0] - me[0]) + power(location[1] - me[1]) + power(location[2] - me[2])));
    //if enemy docked go for the opposite large block, else go for the nearest large block
    if (enemyDocked) {
        if (distance[0] < distance[1]) 
            return 1;
        else 
            return 0;
    } else {
        if (distance[0] < distance[1]) 
            return 0;
         else 
            return 1;
    }
    } 
    else if (firstItemDropped) {
        if (otherBlockFound) {
            if (secondBlock == 0) {
                return 1;
            } else {
                return 0;
            }
        } else {
            if (game.itemInZone(0)) {
                return 1;
                secondBlock = 0;
                otherBlockFound = true;
            } else {
                return 0;
                otherBlockFound = true;
                secondBlock = 1;
            }
        }
    } else {
        DEBUG(("No more blocks to find!"));
    }
}

float power(float num1) {
    return num1 * num1;
}

//function findSide
//================
//INPUTS: float* (y-Coordinate of the block)
//OUTPUTS: int (1 or -1 depending on which side to dock)
//DESCRIPTION: Finds out what side on the y-axis we start and dock on

int findSide(float* blockLocation)
{
    if (blockLocation[1] - me[1] > 0)
        return 1;
    return -1;
}

//function moveTo
//================
//INPUTS: float (coordinates to go to), float (sensitivity), bool(True if we are going to an item, false if not)
//OUTPUTS: bool (True if reached location)
//DESCRIPTION: Goes to a location and returns true when the location is reached.

bool moveTo(float objective[], float m, bool item)
{
    //If we are moving to an item, make sure to face it and move of to the side so we can dock
    if (item)
    {
      face(objective);
      objective[1] = objective[1] - (0.173f * findSide(objective));  
    }
    //Set the position target and return true if X, Y, and Z are within 'm' of the target
    api.setPositionTarget(objective);
     if ((me[0] >= objective[0] - m) && (me[0] <= objective[0] + m))
    {
        if ((me[1] >= (objective[1]) - m) && (me[1] <= (objective[1]) + m))
        {
            if ((me[2] >= objective[2] - m) && (me[2] <= objective[2] + m))
                return true;
        }
    }
    return false;
}

//function triAndPickup
//=====================
//INPUTS: none
//OUTPUTS: Boolean
//DESCRIPTION: Drops the final 2 SPS points and picks up the big block.  Returns true when third SPS point is placed

bool triAndPickup() {
    
    if (pos == 0) {
        //Move to a specific coordinate and drop a point
        if (moveTo(triPoint, 0.1f, false))
        {
            game.dropSPS();
            pos = 1;
        }
    }
    if (pos == 1) {
        
        if (game.hasItem(findItem()) == 2) {  //If the enemy has the closest block
            enemyDocked = true;               //Set a flag
            game.dropSPS();                   //Drop a point
            if (dockItem(findItem())) {       //Then find the other item
                game.dropSPS();               //Drop a point
                return true;
            }
        } else {
            if (dockItem(findItem())) {
                game.dropSPS();
                return true;
            }
        }
    }
    return false;
}

//function updateLocations()
//==========================
//INPUTS: none
//OUTPUTS: none
//DESCRIPTION: Update the locations of our sphere and our zone each round

void updateLocations()
{
      api.getMyZRState(me);  
      game.getZone(zoneInfo);
}

