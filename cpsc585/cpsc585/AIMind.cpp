#include "AIMind.h"


AIMind::AIMind(Racer* _racer, TypeOfRacer _racerType, int NumberOfRacers, std::string _racerName, std::string _colour)
{
	colour = _colour;
	racerName = _racerName;
	numberOfRacers = NumberOfRacers;
	numberOfLapsToWin = 3;
	timeToNextTeleport = 0;
	timeSinceTeleport = 0;
	teleportedRecently = false;
	racer = _racer;
	racerType = _racerType;
	newTime = 0;
	spawnTime = 0.0f;
	oldTime = time(NULL);
	lastPosition = racer->body->getPosition();
	currentWaypoint = 0;
	currentLap = 1;
	overallPosition = 0;
	placement = 1;
	checkPointTimer = new CheckpointTimer(racer);
	speedBoost = new Ability(SPEED);
	laser = new Ability(LASER);
	rocket = new Ability(ROCKET);
	landmine = new Ability(LANDMINE);

	knownNumberOfKills = 0;
	knownNumberOfDeaths = 0;
	knownNumberOfSuicides = 0;
	knownDamageDone = 0;
	knownDamageTaken = 0;


	rotationAngle = 0;
	finishedRace = false;
	playedNoAmmoSound = false;

	showAmmoTimer = 0.0f;

	playerHUD = NULL;
}

AIMind::~AIMind(void)
{
	if (checkPointTimer)
	{
		delete checkPointTimer;
		checkPointTimer = NULL;
	}

	if (speedBoost)
	{
		delete speedBoost;
		speedBoost = NULL;
	}

	if (laser)
	{
		delete laser;
		laser = NULL;
	}

	if (rocket)
	{
		delete rocket;
		rocket = NULL;
	}

	if (landmine)
	{
		delete landmine;
		landmine = NULL;
	}
}

void AIMind::update(HUD* hud, Intention intention, float seconds, Waypoint* waypoints[], Racer* racers[], AIMind* racerPlacement[], Waypoint* buildingWaypoint){
	// Once the race is completed, the player is turned into an AI at which point an end of game hud would display.
	if(currentLap == numberOfLapsToWin+1){ 
		finishedRace = true;
		//Possibly add a value here that means the racer has completed the race. Like raceCompleted = true;
		if(racerType == PLAYER){
			togglePlayerComputerAI(waypoints); 
		}
	}

	//checkPointTime = checkPointTimer->update(checkpoints, prevCheckpoints);

	updateWaypointsAndLap(seconds, waypoints, buildingWaypoint);

	/*
	if(checkPointTimer->downgradeAbility()){
		downgrade();
	}
	*/

	/* LIKELY TO EVENTUALLY REMOVE THIS COMPLETELY (THE UPGRADE SYSTEM)
	if(racer->kills > knownNumberOfKills){
		knownNumberOfKills += 1;
		upgrade();
	}
	*/

	if(racer->kills > knownNumberOfKills){
		knownNumberOfKills += 1;
		acquireAmmo();
	}
	else if(racer->kills < knownNumberOfKills){
		knownNumberOfKills = racer->kills;
	}
	knownNumberOfDeaths = racer->deaths;
	knownNumberOfSuicides = racer->suicides;
	knownDamageDone = racer->givenDamage;
	knownDamageTaken = racer->takenDamage;

	switch(racerType){
		case PLAYER:
			{
				playerHUD = hud;

				if (showAmmoTimer > 0.0f)
					showAmmoTimer -= seconds;
				else
					showAmmoTimer = 0.0f;

				if (showAmmoTimer == 0.0f)
				{
					hud->showAmmo = false;
				}


				// Update camera
				if (!intention.lbumpPressed)
				{
					if ((intention.cameraX != 0) || (intention.cameraY != 0))
					{
						hkReal angle;
						float height;

						angle = intention.cameraX * 0.05f;

						if (racer->config.inverse)
							height = intention.cameraY * -0.02f + racer->lookHeight;
						else
							height = intention.cameraY * 0.02f + racer->lookHeight;

						if (height > 0.5f)
							height = 0.5f;
						else if (height < -0.5f)
							height = -0.5f;

						racer->lookHeight = height;

						if (angle > M_PI)
							angle = (hkReal) M_PI;
						else if (angle < -M_PI)
							angle = (hkReal) -M_PI;


						hkQuaternion rotation;

						if (angle < 0.0f)
						{
							angle *= -1;
							rotation.setAxisAngle(hkVector4(0,-1,0), angle);
						}
						else
						{
							rotation.setAxisAngle(hkVector4(0,1,0), angle);
						}

						hkTransform transRot;
						transRot.setIdentity();
						transRot.setRotation(rotation);

						hkVector4 finalLookDir(0,0,1);
						finalLookDir.setTransformedPos(transRot, racer->lookDir);

						finalLookDir(1) = height;

						racer->lookDir.setXYZ(finalLookDir);
					}
				}

				// Update Heads Up Display
				hud->update(intention);

				hkVector4 vel = racer->body->getLinearVelocity();
				float velocity = vel.dot3(racer->drawable->getZhkVector());

				hud->setSpeed(velocity);
				hud->setHealth(racer->health);
				hud->setPosition(placement);
				hud->setLap(currentLap, numberOfLapsToWin);

				racer->computeRPM();

				racer->braking = false;

				if (intention.leftTrig && ((hkMath::abs(velocity) > 0.1f) ||
					 (hkMath::abs((racer->body->getAngularVelocity()).dot3(racer->body->getAngularVelocity())) > 0.1f)))
				{
					racer->brake(seconds);
				}


				if (hud->getSelectedAbility() == SPEED && !speedBoost->onCooldown())
				{
					if (intention.rightTrig)
					{
						if (speedBoost->getAmmoCount() > 0)
						{
							speedBoost->startCooldownTimer();
							speedBoost->decreaseAmmoCount();
							Sound::sound->playSoundEffect(SFX_BOOST, racer->emitter);
							hkVector4 impulse = racer->drawable->getZhkVector();
							impulse.mul(150.0f*racer->chassisMass*speedBoost->getBoostValue());
							racer->body->applyLinearImpulse(impulse);

							hud->speedAmmo--;
						}
						else if (!playedNoAmmoSound)
						{
							Sound::sound->playSoundEffect(SFX_NOAMMO, racer->emitter);
							playedNoAmmoSound = true;
						}
					}
					else
					{
						playedNoAmmoSound = false;
					}
				}

				if (hud->getSelectedAbility() == LASER && intention.rightTrig && !laser->onCooldown())
				{
					laser->startCooldownTimer();
					racer->fireLaser();
				}

				if (hud->getSelectedAbility() == ROCKET && !rocket->onCooldown())
				{
					if (intention.rightTrig)
					{
						if (rocket->getAmmoCount() > 0)
						{
							rocket->startCooldownTimer();
							rocket->decreaseAmmoCount();
							racer->fireRocket();

							hud->rocketAmmo--;
						}
						else if (!playedNoAmmoSound)
						{
							Sound::sound->playSoundEffect(SFX_NOAMMO, racer->emitter);
							playedNoAmmoSound = true;
						}
					}
					else
					{
						playedNoAmmoSound = false;
					}
				}

				if (hud->getSelectedAbility() == LANDMINE && !landmine->onCooldown())
				{
					if (intention.rightTrig)
					{
						if (landmine->getAmmoCount() > 0)
						{
							landmine->startCooldownTimer();
							landmine->decreaseAmmoCount();
							racer->dropMine();

							hud->landmineAmmo--;
						}
						else if (!playedNoAmmoSound)
						{
							Sound::sound->playSoundEffect(SFX_NOAMMO, racer->emitter);
							playedNoAmmoSound = true;
						}
					}
					else
					{
						playedNoAmmoSound = false;
					}
				}


				if (speedBoost->onCooldown())
				{
					char buf1[33];
					speedBoost->updateCooldown(seconds);
					std::string stringArray[] = { buf1 };//, buf2, buf3, buf4 };
					//renderer->setText(stringArray, 1);
				}

				if (laser->onCooldown())
				{
					laser->updateCooldown(seconds);
				}

				if (rocket->onCooldown())
				{
					rocket->updateCooldown(seconds);
				}

				if (landmine->onCooldown())
				{
					landmine->updateCooldown(seconds);
				}

				/************* STEERING CALCULATIONS *************/
				hkVector4 A = racer->drawable->getZhkVector();
				hkVector4 C = racer->body->getPosition();
				hkVector4 B;
				B.setXYZ(racer->lookDir);
				B(1) = 0.0f;

				float angle = acos(B.dot3(A));

				float sign = B.dot3(racer->drawable->getXhkVector());

				if (racer->currentAcceleration < 0.0f)
					sign *= -1.0f;

				if ((angle > 0.0f) && (sign > 0))
				{
					racer->steer(seconds, min(angle / 1.11f, 1.0f));
				}
				else if ((angle > 0.0f) && (sign < 0))
				{
					racer->steer(seconds, -min(angle / 1.11f, 1.0f));
				}
				else
				{
					racer->steer(seconds, 0.0f);
				}

				/****************************************************/

				racer->accelerate(seconds, intention.acceleration + speedBoost->getBoostValue());

				racer->applyForces(seconds);

				break;
			}
		case COMPUTER:
			{
				/*
					The computer travels at a base speed unless affected by particular modifiers.
					These modifiers can include things like speed boost, and waypoint types.
					Each waypoint can be a different type, if it is a standard waypoint, the vehicle will not
					change speed.  If it is a turn point or sharp point though, it will travel at slower speeds
					to make turns more successfully.
				*/
				bool rubberBanding = true; // Turn this off if multiplayer
				float cheatingSpeed = 0.0f;
				float baseSpeed = 0.0f;
				if(rubberBanding){
					int playerPlacement = 0;
					int indexOfPlayer = 0;
					for(int i = 0; i < numberOfRacers; i++){
						if(racerPlacement[i]->getTypeOfRacer() == PLAYER){
							playerPlacement = racerPlacement[i]->getPlacement();
							indexOfPlayer = i;
						}
					}
					if(placement < playerPlacement) // Slow the racers down the further ahead they are than the player
					{
						int distance = overallPosition - racerPlacement[indexOfPlayer]->getOverallPosition();
						if(distance > 8){
							baseSpeed = 0.0f;
						}
						else if(distance > 6){
							baseSpeed = 0.2f;
						}
						else if(distance > 4){
							baseSpeed = 0.4f;
						}
						else if(distance > 2){
							baseSpeed = 0.6f;
						}
						else{
							baseSpeed = 1.0f;
						}
						//if(waypoints[currentWaypoint]->getWaypointType() == WAY_POINT &&
						//	waypoints[currentWaypoint + 1]->getWaypointType() == WAY_POINT){
						//	cheatingSpeed = 0.1f;
						//}
					}
					else{ // Teleport the racers that are behind closer to the player if they get too far behind
						
						if(waypoints[currentWaypoint]->getWaypointType() == WAY_POINT &&
							waypoints[currentWaypoint + 1]->getWaypointType() == WAY_POINT){
							cheatingSpeed = 0.5f;
						}
						baseSpeed = 1.0f;
						int distance = racerPlacement[indexOfPlayer]->getOverallPosition() - overallPosition;
						srand((unsigned)time(0));
						int random_integer = (rand()+knownDamageDone)%100;
						int waypoint_offset = rand()%3;
						timeToNextTeleport += seconds;
						// If the racer is behind more than X waypoints of the racer, will teleport them closer
						// as long as they roll between 0 and 30 and so long as they haven't teleported in the last 5 seconds
						if(distance > 6 && random_integer <= 30 && timeToNextTeleport > 5) 
						{
							teleportedRecently = true;
							timeToNextTeleport = 0;
							int newCurrent = racerPlacement[indexOfPlayer]->getCurrentWaypoint()-(2+waypoint_offset);
							if(newCurrent >= 0){
								int previousWaypoint = currentWaypoint;
								currentWaypoint = newCurrent;
								if(previousWaypoint > currentWaypoint){ // Accounts for if the racer teleports past the lap marker
									currentLap += 1;
								}
							
								reset(waypoints);
							}
						}

					}
				}
				else{
					baseSpeed = 1.0f; // Constant speed that racers go at when in multiplayer games
				}
				hkVector4 vel = racer->body->getLinearVelocity();
				float velocity = vel.dot3(racer->drawable->getZhkVector());
				if(currentWaypoint+1 != 83){
					if(waypoints[currentWaypoint+1]->getWaypointType() == TURN_POINT && velocity > 60.0f){
						baseSpeed = 0.0f;
					}
					if(waypoints[currentWaypoint+1]->getWaypointType() == SHARP_POINT && velocity > 35.0f){
						baseSpeed = 0.0f;
					}
				}


				int previousWaypoint = 0;
				if(currentWaypoint - 1 == -1){
					previousWaypoint = 82;
				}
				else{
					previousWaypoint = currentWaypoint - 1;
				}
				if(!speedBoost->onCooldown() && 
					(waypoints[previousWaypoint]->getWaypointType() == TURN_POINT || waypoints[previousWaypoint]->getWaypointType() == SHARP_POINT)
					&& waypoints[currentWaypoint]->getWaypointType() == WAY_POINT
					&& speedBoost->getAmmoCount() > 0){
					speedBoost->startCooldownTimer();
					speedBoost->decreaseAmmoCount();
					Sound::sound->playSoundEffect(SFX_BOOST, racer->emitter);
					hkVector4 impulse = racer->drawable->getZhkVector();
					impulse.mul(150.0f*racer->chassisMass*speedBoost->getBoostValue());
					racer->body->applyLinearImpulse(impulse);
				}

				if(speedBoost->onCooldown()){
					speedBoost->updateCooldown(seconds);
				}

				if(!landmine->onCooldown() && landmine->getAmmoCount() > 0)
					{
						landmine->startCooldownTimer();
						landmine->decreaseAmmoCount();
						racer->dropMine();
					}

				if(landmine->onCooldown()){
					landmine->updateCooldown(seconds);
				}

				float extraSpeed = 0.0f;
				if(teleportedRecently){
					timeSinceTeleport += seconds;
					extraSpeed = 0.5f;
				}

				if(timeSinceTeleport > 5){ // Determines the duration of the extra acceleration boost the ai gets after teleporting
					timeSinceTeleport = 0;
					teleportedRecently = false;
				}
				

				racer->accelerate(seconds, baseSpeed + speedBoost->getBoostValue() + extraSpeed + cheatingSpeed);









				/************* STEERING CALCULATIONS *************/

				bool targetAssigned = false;
				bool avoidanceEngaged = false;
				Racer* target;
				for(int i = 0; i < 5; i++){ // Determines if any racers are within an acceptable range to go into attack mode
					if(racers[i]->getIndex() != racer->getIndex()){ // If it is a racer other than itself
						hkVector4 position = racers[i]->body->getPosition();
						float angle = calculateAngleToPosition(&position);
						hkVector4 A = racer->drawable->getZhkVector();
						hkVector4 B;
						B.setXYZ(racer->lookDir);
						B(1) = 0.0f;

						B.dot3(A);
						// Sign determines if it is pointing to the right or the left of the current waypoint
						float sign = B.dot3(racer->drawable->getXhkVector());
						hkSimdReal distance = (racers[i]->body->getPosition()).distanceTo(racer->body->getPosition());
						hkVector4 vel = racers[i]->body->getLinearVelocity();
						float velocity = vel.dot3(racers[i]->drawable->getZhkVector());
						if(distance.isLess(60) && angle < 0.34906 && velocity > 30){ // add a speed condition here (speed > 40)
							targetAssigned = true; // If there is a target, attack mode (targetAssigned) is enabled, and the target determined
							target = racers[i];
							break;
						}
						else if(distance.isLess(40) && angle < 0.34906 && velocity <= 30){
							avoidanceEngaged = true;
						}
					}
				}
				if(targetAssigned){ // Once targeted, trys to aim at the racer, and when aiming close enough, shoots the laser
					hkVector4 targetPos = target->body->getPosition();
					hkVector4 shooterPos = racer->body->getPosition();

					// Generate random numbers for offset of where the computer is aiming so they aren't guaranteed to hit
					srand((unsigned)time(0));
					int offset = rand()%6; // Increase modulo to decrease the likelihood of getting hit.
					srand((unsigned)time(0)+offset);
					int negative = rand()%20;
					if(negative%2 == 0){
						offset *= -1;
					}

					targetPos(0) += offset;
					targetPos(2) += offset;

					//------------------------------------

					shooterPos(1) += 2.0f;
					targetPos.sub(shooterPos);
					targetPos.normalize3();

					racer->lookDir.setXYZ(targetPos);

					if (!rocket->onCooldown() && rocket->getAmmoCount() > 0)
					{
						rocket->startCooldownTimer();
						rocket->decreaseAmmoCount();
						racer->fireRocket();
						targetAssigned = false;
					}
					else if(!laser->onCooldown())
					{
						laser->startCooldownTimer();
						racer->fireLaser();
						targetAssigned = false;
					}

					if(rocket->onCooldown()){
						rocket->updateCooldown(seconds);
					}

					if(laser->onCooldown()){
						laser->updateCooldown(seconds);
					}
					
					
				}
				else if(avoidanceEngaged){
					srand((unsigned) time(0)); // Randomizes whether the racer avoids to the left or right
					int sign = rand();
					if(sign%2 == 0){
						sign = -1;
					}
					else{
						sign = 1;
					}
					racer->steer(seconds, 1.0f*sign);
				}
				else{
					hkVector4 vel = racer->body->getLinearVelocity();
					float velocity = vel.dot3(racer->drawable->getZhkVector());
					
					// Using the indexer in place of currentWaypoint allows the ai to look one waypoint ahead for steering.
					// This is a bad idea when they are moving slow, but at higher speeds it increases the accuracy of their driving
					int indexer;
					if(currentWaypoint == 82){
						indexer = 0;
					}
					else{
						indexer = currentWaypoint+1;
					}
					
					int waypoint;
					if(velocity > 55){
						waypoint = indexer;
					}
					else{
						waypoint = currentWaypoint;
					}
					hkVector4 position = waypoints[waypoint]->wpPosition;

					float angle = calculateAngleToPosition(&position);

					hkVector4 A = racer->drawable->getZhkVector();
					hkVector4 B;
					B.setXYZ(racer->lookDir);
					B(1) = 0.0f;

					B.dot3(A);
					// Sign determines if it is pointing to the right or the left of the current waypoint
					float sign = B.dot3(racer->drawable->getXhkVector());

			

					// The computer only turns if it is pointing away from the current waypoint by more than 20
					// degrees in either direction.
					if ((angle > 0.0f) && (sign > 0))
					{
						racer->steer(seconds, min(angle / 1.11f, 1.0f));
					}
					else if ((angle > 0.0f) && (sign < 0))
					{
						racer->steer(seconds, -min(angle / 1.11f, 1.0f));
					}
					else
					{
						racer->steer(seconds, 0.0f);
					}
				}


				// This makes it so the AI's weapons aren't looking the right way
				/*
				hkVector4 racerPos = racer->body->getPosition();
				hkSimdReal camX = racerPos.getComponent(0);
				hkSimdReal camY = racerPos.getComponent(1);
				hkSimdReal camZ = racerPos.getComponent(2);
				camX.add(4);
				camY.add(4);
				camZ.sub(2);
				hkVector4 camPos = hkVector4(camX, camY, camZ);
				racerPos.sub(camPos);
				racer->lookDir = racerPos;*/
				
				
				/****************************************************/

				racer->applyForces(seconds);

				racer->computeRPM();
				
				break;
			}

	}
	/*
		If the computer is stuck, whether flipped, on its side,
		driving into a wall, or unable to move, if its position does
		not change for a particular amount of time, it will reset its location
		to its current waypoints location.
	*/
	hkVector4 currentPosition = racer->body->getPosition();
	int distanceTo = (int)currentPosition.distanceTo(lastPosition);
	hkVector4 upVec = racer->drawable->getYhkVector();
	hkVector4 actualUp = hkVector4(0,1,0);
	spawnTime += seconds;
	if(upVec.dot3(actualUp).isLess(0.1f) || (racer->body->getPosition()(1) < 5.0f) || (distanceTo < 1 && racerType == COMPUTER)){
		if(spawnTime > 2.0f){
			reset(waypoints);
			spawnTime = 0.0f;
		}
	}
	else{
		spawnTime = 0.0f;
		lastPosition = currentPosition;
	}
	//-----------------------------------------
	
	overallPosition = currentWaypoint + (currentLap-1)*83; // 83 represent the number of waypoints
	
}

void AIMind::reset(Waypoint* waypoints[])
{
	int nextWaypoint = currentWaypoint + 1;
	if(nextWaypoint == 83){
		nextWaypoint = 0;
	}
	D3DXVECTOR3 cwPosition = waypoints[currentWaypoint]->drawable->getPosition();
	D3DXVECTOR3 nextPosition = waypoints[nextWaypoint]->drawable->getPosition();
	hkVector4 wayptVec;
	wayptVec.set(nextPosition.x, nextPosition.y, nextPosition.z);

	wayptVec.sub(hkVector4(cwPosition.x, cwPosition.y, cwPosition.z));

	hkVector4 resetPosition;
	resetPosition.set(cwPosition.x, cwPosition.y, cwPosition.z);

			
	racer->reset(&resetPosition, 0);
	wayptVec(1) = 0.0f;
	wayptVec.normalize3();
 
	hkVector4 z = racer->drawable->getZhkVector();
	hkVector4 x = racer->drawable->getXhkVector();
 
	float angle = hkMath::acos(z.dot3(wayptVec)); // angle is between 0 and 183
			
	// if the vector is on the LEFT side of the car...
	if (x.dot3(wayptVec) < 0.0f)
		angle *= -1.0f;
 
	rotationAngle = angle;
 
	racer->reset(&resetPosition, angle);
	calculateAngleToPosition(&(hkVector4(nextPosition.x, nextPosition.y, nextPosition.z)));
}

float AIMind::calculateAngleToPosition(hkVector4* position)
{
	racer->lookDir = *position;
	racer->lookDir.sub3clobberW(racer->body->getPosition());
	racer->lookDir.normalize3();

	hkVector4 A = racer->drawable->getZhkVector();
	hkVector4 C = racer->body->getPosition();
	hkVector4 B;
	B.setXYZ(racer->lookDir);
	B(1) = 0.0f;

	float angle = acos(B.dot3(A));

	return angle;
}

// Returns the current time left on the checkpoint timer
int AIMind::getCheckpointTime()
{
	return checkPointTime;
}

// When an AI reaches its waypoint, it will update its goal to the next waypoint
void AIMind::updateWaypointsAndLap(float seconds, Waypoint* waypoints[], Waypoint* buildingWaypoint)
{
	int prevWaypoint;
	if(currentWaypoint - 1 == -1){
		prevWaypoint = 82;
	}
	else{
		prevWaypoint = currentWaypoint - 1;
	}
	D3DXVECTOR3 current = waypoints[currentWaypoint]->drawable->getPosition();
	hkVector4* currentPos = new hkVector4(current.x, current.y, current.z);
	D3DXVECTOR3 prev = waypoints[prevWaypoint]->drawable->getPosition();
	hkVector4* prevPos = new hkVector4(prev.x, prev.y, prev.z);

	hkSimdReal distanceOfRacer = waypoints[currentWaypoint]->wpPosition.distanceTo(getRacerPosition());

	if(waypoints[currentWaypoint]->passedWaypoint(currentPos, prevPos, &racer->body->getPosition())){
		if(waypoints[currentWaypoint]->getWaypointType() == LAP_POINT){
			currentLap += 1;
		}
	}
	if(waypoints[currentWaypoint]->passedWaypoint(currentPos, prevPos, &racer->body->getPosition())
		|| (distanceOfRacer.isLess(22) && waypoints[currentWaypoint]->getWaypointType() != LAP_POINT)){
		if(currentWaypoint == 82){
			currentWaypoint = 0;
		}
		else{
			currentWaypoint += 1;
		}
	}

	hkSimdReal distanceToBuilding = buildingWaypoint->wpPosition.distanceTo(getRacerPosition());
	if(distanceToBuilding.isLess(30)){
		currentWaypoint = 25;
	}
}

// Switches between whether the racer is being controlled by a player or computer
void AIMind::togglePlayerComputerAI(Waypoint* waypoints[])
{
	if(racerType == COMPUTER){
		racerType = PLAYER;
		D3DXVECTOR3 target = waypoints[currentWaypoint]->drawable->getPosition();
		hkVector4 targetPos = hkVector4(target.x, target.y, target.z);
		hkVector4 shooterPos = racer->body->getPosition();
		shooterPos(1) += 2.0f;
		targetPos.sub(shooterPos);
		targetPos.normalize3();

		racer->lookDir.setXYZ(targetPos);
	}
	else{
		racerType = COMPUTER;
	}
}

void AIMind::acquireAmmo()
{
	srand((unsigned)time(0));
	int random_integer = rand()%100;
	if(random_integer > 66){
		landmine->increaseAmmoCount();

		if (racerType == PLAYER)
		{
			// Show LANDMINE icon on screen
			playerHUD->showAmmo = true;
			playerHUD->ammoIconType = LANDMINE;
			showAmmoTimer = 2.0f;

			playerHUD->landmineAmmo++;
		}
	}
	else if(random_integer > 33){
		speedBoost->increaseAmmoCount();

		if (racerType == PLAYER)
		{
			// Show SPEED icon on screen
			playerHUD->showAmmo = true;
			playerHUD->ammoIconType = SPEED;
			showAmmoTimer = 2.0f;

			playerHUD->speedAmmo++;
		}
	}
	else if(random_integer >= 0){
		rocket->increaseAmmoCount();

		if (racerType == PLAYER)
		{
			// Show ROCKET icon on screen
			playerHUD->showAmmo = true;
			playerHUD->ammoIconType = ROCKET;
			showAmmoTimer = 2.0f;

			playerHUD->rocketAmmo++;
		}
	}

	if (racerType == PLAYER)
	{
		Sound::sound->playSoundEffect(SFX_PICKUP, racer->emitter);
	}
}

/*
	When a racer gets a kill, this function will determine 
	an ability to upgrade and change its associated paramaters.
*/
void AIMind::upgrade()
{
	int laserLevel = laser->getAbilityLevel();
	int speedLevel = speedBoost->getAbilityLevel();
	bool upgradeLaser = false;
	bool upgradeSpeed = false;

	if(laserLevel == speedLevel){
		srand((unsigned)time(0));
		int random_integer = rand()%100;
		if(random_integer > 50){
			upgradeLaser = true;
		}
		else if(random_integer > 0){
			upgradeSpeed = true;
		}
	}
	else{
		if(laserLevel < speedLevel){
			upgradeLaser = true;
		}
		else if(speedLevel < laserLevel){
			upgradeSpeed = true;
		}
	}

	if(upgradeLaser){
		if(laserLevel < 3){
			laser->update(laserLevel + 1);

			// The following method in racer was removed, since we're changing
			// the game's mechanics
			//racer->setDamageOutput(laser->getLaserDamage());
		}
	}
	else if(upgradeSpeed){
		if(speedLevel < 3){
			speedBoost->update(speedLevel + 1);
		}
	}
}

void AIMind::downgrade()
{
	int laserLevel = laser->getAbilityLevel();
	int speedLevel = speedBoost->getAbilityLevel();
	bool downgradeLaser = false;
	bool downgradeSpeed = false;

	if(laserLevel == speedLevel){
		srand((unsigned)time(0));
		int random_integer = rand()%100;
		if(random_integer > 50){
			downgradeLaser = true;
		}
		else if(random_integer > 0){
			downgradeSpeed = true;
		}
	}
	else{
		if(laserLevel > speedLevel){
			downgradeLaser = true;
		}
		else if(speedLevel > laserLevel){
			downgradeSpeed = true;
		}
	}

	if(downgradeLaser){
		if(laserLevel > 1){
			laser->update(laserLevel - 1);

			// The following method has been removed since we're
			// changing game mechanics
			//racer->setDamageOutput(laser->getLaserDamage());
		}
	}
	else if(downgradeSpeed){
		if(speedLevel > 1){
			speedBoost->update(speedLevel - 1);
		}
	}
}

// Returns the current lap that the racer is on
int AIMind::getCurrentLap()
{
	return currentLap;
}

// Returns the ID of the current waypoint the racer is trying to reach
int AIMind::getCurrentWaypoint()
{
	return currentWaypoint;
}

// Returns how much time is left on the cooldown of a racer
int AIMind::getSpeedCooldown()
{
	return speedBoost->getCooldownTime();
}

// A value based on how many laps and waypoints a racer has reached
int AIMind::getOverallPosition()
{
	return overallPosition;
}

// Sets the number representation of what place a racer is in (like 1st place, 2nd place, etc)
void AIMind::setPlacement(int place)
{
	if (racerType == PLAYER)
	{
		if ((placement >= 2) && (place == 1))
		{
			Sound::sound->playSoundEffect(SFX_TAKENLEAD, Sound::sound->playerEmitter);
		}
		else if ((placement == 1) && (place >= 2))
		{
			Sound::sound->playSoundEffect(SFX_LOSTLEAD, Sound::sound->playerEmitter);
		}
	}

	placement = place;
}

hkVector4 AIMind::getRacerPosition()
{
	return racer->body->getPosition(); 
}

// Returns the number representation of what place a racer is in (like 1st place, 2nd place, etc)
int AIMind::getPlacement()
{
	return placement;
}

int AIMind::getLaserLevel()
{
	return laser->getAbilityLevel();
}

int AIMind::getSpeedLevel()
{
	return speedBoost->getAbilityLevel();
}

int AIMind::getCurrentCheckpoint()
{
	return checkPointTimer->getCurrentCheckpoint();
}

float AIMind::getRotationAngle()
{
	return rotationAngle;
}

int AIMind::getRocketAmmo()
{
	return rocket->getAmmoCount();
}

int AIMind::getLandmineAmmo()
{
	return landmine->getAmmoCount();
}

int AIMind::getSpeedAmmo()
{
	return speedBoost->getAmmoCount();
}

TypeOfRacer AIMind::getTypeOfRacer()
{
	return racerType;
}

bool AIMind::isfinishedRace()
{
	return finishedRace;
}

std::string AIMind::getRacerName()
{
	return racerName;
}

int AIMind::getKills()
{
	return knownNumberOfKills;
}

int AIMind::getDeaths()
{
	return knownNumberOfDeaths;
}

int AIMind::getSuicides()
{
	return knownNumberOfSuicides;
}

int AIMind::getDamageDone()
{
	return knownDamageDone;
}

int AIMind::getDamageTaken()
{
	return knownDamageTaken;
}

std::string AIMind::getRacerColour()
{
	return colour;
}