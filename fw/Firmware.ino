#include <application.h>
#include "PDservo.h"
#include "Melodies.h"
#include "Version.h"

#define Success 1
#define ParametersValueError -1
#define ModeNotIdleError -2
#define NoneHomeBaseError -3
#define UnknownCommandError -4

//mode options:
#define Idle 0
#define DefaultRun 1
#define RewardRun 2
#define CueRun 3
#define EmptyFood 10
#define Testing 11
#define CheckPIR 12

#define MaxEmptyFoodDispenses 30
#define MaxRunDuration 3600000 		//1 hour in milliseconds
#define MaxCheckPIRDuration 20000 //20 sec in milliseconds

const int PIRPin   = A7;
const int HomeBaseJumperPin = 5;

int isHomeBase = 0;
int mode = Idle;

unsigned long startRun = 0; //In milliseconds from start up
long runDuration = 0; 			//In milliseconds
char unitTimeString [20]; 	//Use to change Unix time from int to string for publish event

int dispensesQueue = 0; 		//When emptyFood store the number of dispenses

void setup(void)
{
	//Initialize Particle functions
	Particle.function("Tester",Tester);
	Particle.function("AtomAction",AtomAction);
	Particle.function("Play",Play);
	/* Particle function return policy:
		1 - Success
		0 to 99 - function response
		-1 - Unexpected parameters value
		-2 - Mode isn't idle
		-3 - try to call HomeBase function to none HomeBase device
		-4 - unknown command
	*/

	//Initialize Particle Variables:
	Particle.variable("Compiled", buildDate, STRING);
	Particle.variable("Version", protocolVersion, STRING);
	Particle.variable("GitSHA", buildGitSha, STRING);

	//Define pins state
	pinMode(PIRPin  , INPUT);
	pinMode(HomeBaseJumperPin  , INPUT_PULLUP);

	//Check Jumper on HomeBaseJumperPin
	isHomeBase = (digitalRead(HomeBaseJumperPin) == 0); //True if jumper exists
	if(isHomeBase)
		setupServo();

	//Play start up melody:
	startMelody(0);
	while(isMelodyOn()) //Wait till melody end
		updateMelody();
}

void loop(void)
{
	//update melody section:
	if(isMelodyOn())
		updateMelody();
	else if(isBeepingOn())
		updateBeeping();

	//Check system mode:
	if(mode == DefaultRun || mode == RewardRun) //Default or Reward run
	{
		if(digitalRead(PIRPin))
		{
			sprintf(unitTimeString,"1:%d",Time.now());
			stopMelody();
			stopBeeping();


			if(isHomeBase && mode == RewardRun) //if it's reward fun feed once
				feed();
				Particle.publish("Run",unitTimeString,60,PRIVATE); //send event run finished. 1 - successfully, dog arrived in time
			mode = Idle;
		}
		else if(millis() - startRun > runDuration) //if time passed:
		{
			sprintf(unitTimeString,"0:%d",Time.now());
			Particle.publish("Run",unitTimeString,60,PRIVATE);  //send event run finished. 0 - Failed, dog didn't arrived in time
			stopMelody();
			stopBeeping();
			mode = Idle;
		}
	}
	else if(mode == CueRun) //Cue run
	{
		if(digitalRead(PIRPin))
		{
			startMelody(1);
			while(isMelodyOn()) //Wait till melody end
				 updateMelody();
			delay(500); //Wait another 0.5 second
			feed(); //Feed once
			sprintf(unitTimeString,"1:%d",Time.now());
			Particle.publish("Run",unitTimeString,60,PRIVATE); //send event run finished. 1 - successfully, dog arrived in time
			mode = Idle;
		}
		else if(millis() - startRun > runDuration)
		{
			sprintf(unitTimeString,"0:%d",Time.now());
			Particle.publish("Run",unitTimeString,60,PRIVATE);  //send event run finished. 0 - Failed, dog didn't arrived in time
			mode = Idle;
		}
	}
	else if(mode == CheckPIR) //Cue run
	{
		if(digitalRead(PIRPin))
		{
			startMelody(3);
			while(isMelodyOn()) //Wait till the melody end
				updateMelody();
			sprintf(unitTimeString,"1:%d",Time.now());
			Particle.publish("PIR",unitTimeString,60,PRIVATE); //send event run finished. 1 - successfully, dog arrived in time
			mode = Idle;
		}
		else if(millis() - startRun > runDuration)
		{
			sprintf(unitTimeString,"0:%d",Time.now());
			Particle.publish("PIR",unitTimeString,60,PRIVATE);  //send event run finished. 0 - Failed, dog didn't arrived in time
			mode = Idle;
		}
	}
	else if(mode == EmptyFood) //EmptyFood
	{
		sprintf(unitTimeString,"1:%d",Time.now());
		Particle.publish("Dispense",unitTimeString,60,PRIVATE);
		feed();
		dispensesQueue--;
		if(dispensesQueue <= 0)
			mode = Idle;
	}
	else if(mode == Testing) //Testing
	{
		delay(10);
		sprintf(unitTimeString,"1:%d",Time.now());
		Particle.publish("Test",unitTimeString,60,PRIVATE);
		mode = Idle;
	}
}

int Tester(String str)
{
	String cmd = getValue(str, ',' , 0);
	if(cmd == "PIR")
		return digitalRead(PIRPin);
	else if(cmd == "Is HomeBase")
		return isHomeBase;
	else if(cmd == "Get Mode")
		return mode;
	else if(cmd == "Connection")
	{
		if(mode != Idle)
			return ModeNotIdleError;

		mode = Testing;
		return Success;
	}
	else //unknown command
		return UnknownCommandError;
}

int AtomAction(String str)
{
	String cmd = getValue(str, ',' , 0);
	if(cmd == "Dispense")
	{
		if(!isHomeBase)
			return NoneHomeBaseError;
		else if(mode != Idle)
			return ModeNotIdleError;
		feed();
	}
	else if(cmd == "Play Melody")
	{
		if(mode != Idle)
			return ModeNotIdleError;

		startMelody(2);
	}
	else if(cmd == "Reset")
	{
		stopMelody();
		stopBeeping();
		mode = Idle;
	}
	else if(cmd == "Empty Food")
	{
		int counter = atoi(getValue(str, ',' , 1).c_str()); //get the number of dispenses needed
		if(!isHomeBase)
			return NoneHomeBaseError;
		else if(mode != Idle)
			return ModeNotIdleError;
		else if(counter<= 0 || counter > MaxEmptyFoodDispenses)
			return ParametersValueError;

		mode = EmptyFood;
		dispensesQueue = counter;
	}
	else if(cmd == "Check PIR")
	{
		runDuration = 1000 * atoi(getValue(str, ',' , 1).c_str()); //Get run duration in milliseconds
		if(runDuration < 1 || runDuration > MaxCheckPIRDuration) //Check for atoi problems, maximum time of 1 hour
			return ParametersValueError;
		else if(mode != Idle)
			return ModeNotIdleError;
		startMelody(4);
		while(isMelodyOn()) //Wait till melody end
			updateMelody();
		mode = CheckPIR;
		startRun = millis();
	}
	else //unknown command
		return UnknownCommandError;

	return Success;
}
int Play(String str) //str should be in "RunDuration[in seconds] , Default/Reward/Cue" form
{
	if(mode != Idle)
		return ModeNotIdleError;
	String cmd = getValue(str, ',' , 0);
	runDuration = 1000 * atoi(getValue(str, ',' , 1).c_str()); //Get run duration in milliseconds
	if(runDuration < 1 || runDuration > MaxRunDuration) //Check for atoi problems, maximum time of 1 hour
		return ParametersValueError;

	if(cmd == "Default")
	{
		mode = DefaultRun;

		startMelody(1, true);
		startRun = millis();
	}
	else if(cmd == "Reward")
	{
		if(!isHomeBase)
			return NoneHomeBaseError;

		mode = RewardRun;
		startMelody(1, true);
		startRun = millis();
	}
	else if(cmd == "Cue")
	{
		if(!isHomeBase)
			return NoneHomeBaseError;

		stopMelody();
		stopBeeping();
		mode = CueRun;

		feed();
		startRun = millis();
	}
	else
		return UnknownCommandError;

	return Time.now();
}


/* following function copied from:
http://stackoverflow.com/questions/9072320/split-string-into-string-array */
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
