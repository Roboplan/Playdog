//PD - PlayDog

#include <application.h>
#include "PDservo.h"

#define MinAngle 10
#define MaxAngle 145
#define ServoDelay 600
#define ServoSteps 30

const int ServoPin = A4;

Servo servo;

void setupServo()
{
	pinMode(ServoPin, OUTPUT);

	//Move servo to minimum:
	servo.attach(A4);
	servo.write(MinAngle);
	delay(ServoDelay); //wait till movement end
	//detach to prevent noises from the servo
	servo.detach();
	digitalWrite(ServoPin, LOW);
}

void servoStepMove(int startPos, int endPos, int duration)
{
    int newPos;
    for (int i=0; i<ServoSteps; i++)
    {
        newPos = startPos + (i*(endPos-startPos))/ServoSteps;
        servo.write(newPos);
        delay(duration/ServoSteps);
    }

    servo.write(endPos);
		delay(200);
}

void feed(int times) //Feed is the basic movement of the servo, when move all the way in one direction and back. By this movement it dispense some food
{
	servo.attach(ServoPin); //re attach to the servo
	for(int i=0;i<times;i++)
	{
		// turn servo to MaxAngle to dispense food
		servoStepMove(MinAngle, MaxAngle, ServoDelay);

		// turn servo back to MinAngle (Original Position)
		servoStepMove(MaxAngle, MinAngle, ServoDelay);
	}
	//detach to prevent noises from the servo
	servo.detach();
	digitalWrite(ServoPin, LOW);
}
