//PD - PlayDog
#ifndef melodiesHeaderFile
#define melodiesHeaderFile


void updateMelody();
void startMelody(int melodyNumber, bool setBeepingAfterMelody = false);
void stopMelody(bool beeping = false);
int getStartPointer(int melodyNumber);
void startBeeping();
void updateBeeping();
void stopBeeping();
int isMelodyOn();
int isBeepingOn();


 #endif
