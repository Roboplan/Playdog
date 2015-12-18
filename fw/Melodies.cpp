//PD - PlayDog

#include <application.h>
#include "Melodies.h"

#define BeepingFirstTune 1
#define BeepingFirstPause 2
#define BeepingSecondTune 3
#define BeepingSecondPause 4


#define BeepingOff 0



#define BeepingOnDuration 1500
#define BeepTuneDuration 50
#define BeepPauseDuration 20

#define EndMelody -1
#define NoMelodyFound -1
#define MelodyOff -1

const int Speaker = D0;

const int Notes[] =  //C3 to B7 and zero(noTone)
{//C,  C#,  D,  D#,  E,  F,  F#, G,   G#,  A,  A#,  B
  0,
  131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, //0-12
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, //13 - 24
  523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, //25 - 36
  1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976, //37 - 48
  2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951}; //49 - 50
// C   C#    D    D#   E     F    F#   G   G#    A   A#    B

const int Melodies[] = { //form : Note[according to Notes array],duration[in millisecond],...,note,duration,255
25,100,20,100,20,100,EndMelody, // start up melody
25,200,20,200,25,200,EndMelody, // come melody
25,100,30,100,40,100,EndMelody, // detect melody - called when AtomAction Play Melody
27,50,26,50,25,50,EndMelody,    // PIR detection process - start
25,50,26,50,27,50,EndMelody     // PIR detection process - detect
};
const int MelodiesLength = 35;

int melodyState = MelodyOff;    //Current melody playing
int melodyStartPointer = 0;     //Start point of the melody in Melodies array
unsigned long lastNoteTime = 0; //store the time in millisecond
int melodyStep = 0;             //store melody note number

long beepingTimer = 0;			    //store the time in millisecond
int beepingMode = 0;            //Beeping state
/*
BeepingOff: 0 - not operate
BeepingOn: 1 - on, while beeping
BeepingOnWait: 2 - on, while waiting
*/
bool beepingAfterMelody = false;		//if not 0 - start beeping after melody



void updateMelody() //Melodies play well only when this function call at least 100 times in a second
{
	if(melodyState != MelodyOff) //if melody is on
	{
		if(millis() - lastNoteTime > Melodies[melodyStartPointer + melodyStep + 1] ) //if last note duration passed
		{
			melodyStep += 2; //continue to next note
			if(Melodies[melodyStartPointer + melodyStep]  == EndMelody) //if it's 255 -  Melody end
				stopMelody(beepingAfterMelody);
			else
			{
				lastNoteTime = millis();
				int currentTone = Melodies[melodyStartPointer + melodyStep] ; //get next note frequency
				if(currentTone != 0) //if it's not 0(Rest)
					tone(Speaker, Notes[currentTone], Melodies[melodyStartPointer + melodyStep + 1] );
				else
        {
					noTone(Speaker);
          digitalWrite(Speaker,HIGH);
        }
      }
		}
	}
}

void stopMelody(bool beeping)
{
	noTone(Speaker);
  digitalWrite(Speaker,HIGH);
	//Reset melody variables:
	melodyState = MelodyOff;
	melodyStartPointer = 0;
	melodyStep = 0;
	if(beeping)
		startBeeping();
}

void startMelody(int melodyNumber, bool setBeepingAfterMelody) //Play a new melody
{
	melodyStartPointer = getStartPointer(melodyNumber);
	if(melodyStartPointer == NoMelodyFound) //if starting point not found
	   return;
	stopBeeping();
	//Play first note:
	int currentTone = Melodies[melodyStartPointer] ;
	if(currentTone != 0)
		tone(Speaker, Notes[currentTone], Melodies[melodyStartPointer + 1]);
	else
  {
    noTone(Speaker);
    digitalWrite(Speaker,HIGH);
  }
	//Setting melody variables:
	melodyStep = 0;
	lastNoteTime = millis();
	melodyState = melodyNumber;

	beepingAfterMelody = setBeepingAfterMelody;
}

int getStartPointer(int melodyNumber)
{
	//Each melody have different size but they all end with 255 byte,
	//The function return the note after the melodyNumber times of 255 occurred
	int melodyCounter = 0;
	for(int i=0 ; i<MelodiesLength ; i++)
	{
		if(melodyNumber == melodyCounter)
			return i;
		if(Melodies[i] == EndMelody) //255 appear solely in the end of each melody
			melodyCounter++;
	}
	return NoMelodyFound;
}

int isMelodyOn()
{
	return (melodyState != MelodyOff);
}
int isBeepingOn()
{
	return beepingMode;
}

//Beeping section:
//The beeps are used to call the dog
void startBeeping()
{
	beepingMode = BeepingFirstTune;
	beepingTimer = millis();
}

void updateBeeping() //Beeping works only by calling this function at least 50 times a second
{
  if(beepingMode == BeepingFirstTune && millis() - beepingTimer > BeepingOnDuration) //Beep:
  {
  	beepingMode = BeepingFirstPause;
  	beepingTimer = millis();
  	tone(Speaker, Notes[13], BeepingOnDuration);
  }
  else if(beepingMode == BeepingFirstPause && millis() - beepingTimer > BeepTuneDuration) //Beep:
  {
    beepingMode = BeepingSecondTune;
    beepingTimer = millis();
    noTone(Speaker);//, Notes[20], BeepingOnDuration);
  }
  else if(beepingMode == BeepingSecondTune && millis() - beepingTimer > BeepPauseDuration) //Beep:
  {
  	beepingMode = BeepingSecondPause;
  	beepingTimer = millis();
  	tone(Speaker, Notes[20], BeepingOnDuration);
  }
  else if(beepingMode == BeepingSecondPause && millis() - beepingTimer > BeepTuneDuration) //Wait:
  {
  	beepingMode = BeepingFirstTune;
  	beepingTimer = millis();
    noTone(Speaker);
    digitalWrite(Speaker,HIGH);
  }
}

void stopBeeping()
{
	beepingMode = BeepingOff;
  noTone(Speaker);
  digitalWrite(Speaker,HIGH);
}
